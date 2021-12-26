// MIT License
//
// Copyright (c) 2019 Fabian Renn
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// https://github.com/hogliux/farbot

#ifndef FARBOT_HPP
#define FARBOT_HPP

#include <atomic>
#include <memory>
#include <mutex>

#undef NDEBUG
#include <cassert>

namespace farbot
{

namespace internals
{

template <typename...> using void_t = void;

//==============================================================================
template <typename, bool> class NRMScopedAccessImpl;

template <typename T> class NonRealtimeMutatable
{
public:
    NonRealtimeMutatable() : storage(std::make_unique<T>()), pointer(storage.get()) {}

    explicit NonRealtimeMutatable(const T& obj) : storage(std::make_unique<T>(obj)), pointer(storage.get()) {}

    explicit NonRealtimeMutatable(T&& obj) : storage(std::make_unique<T>(std::move(obj))), pointer(storage.get()) {}

    ~NonRealtimeMutatable()
    {
        assert(pointer.load() != nullptr); // <- never delete this object while the realtime thread is holding the lock
        // Spin!
        while (pointer.load() == nullptr)
            ;
        auto acquired = nonRealtimeLock.try_lock();
        ((void)(acquired));
        assert(acquired); // <- you didn't call release on one of the non-realtime threads before deleting this object
        nonRealtimeLock.unlock();
    }

    template <typename... Args> static NonRealtimeMutatable create(Args&&... args)
    {
        return RealtimeObject(std::make_unique<T>(std::forward<Args>(args)...));
    }

    const T& realtimeAcquire() noexcept
    {
        assert(pointer.load() != nullptr); // <- You didn't balance your acquire and release calls!
        currentObj = pointer.exchange(nullptr);
        return *currentObj;
    }

    void realtimeRelease() noexcept
    {
        // You didn't balance your acquire and release calls
        assert(pointer.load() == nullptr);
        // replace back
        pointer.store(currentObj);
    }

    T& nonRealtimeAcquire()
    {
        nonRealtimeLock.lock();
        copy.reset(new T(*storage));
        return *copy.get();
    }

    void nonRealtimeRelease()
    {
        T* ptr;
        // block until realtime thread is done using the object
        do
        {
            ptr = storage.get();
        } while (!pointer.compare_exchange_weak(ptr, copy.get()));
        storage = std::move(copy);
        nonRealtimeLock.unlock();
    }

    template <typename... Args> void nonRealtimeReplace(Args&&... args)
    {
        nonRealtimeLock.lock();
        copy.reset(new T(std::forward<Args>(args)...));
        nonRealtimeRelease();
    }

    template <bool isRealtimeThread> class ScopedAccess : public NRMScopedAccessImpl<T, isRealtimeThread>
    {
    public:
        explicit ScopedAccess(NonRealtimeMutatable& parent) : NRMScopedAccessImpl<T, isRealtimeThread>(parent) {}

        ScopedAccess(const ScopedAccess&) = delete;

        ScopedAccess(ScopedAccess&&) = delete;

        ScopedAccess& operator=(const ScopedAccess&) = delete;

        ScopedAccess& operator=(ScopedAccess&&) = delete;
    };

private:
    friend class NRMScopedAccessImpl<T, true>;

    friend class NRMScopedAccessImpl<T, false>;

    explicit NonRealtimeMutatable(std::unique_ptr<T>&& u);

    std::unique_ptr<T> storage;
    std::atomic<T*>    pointer;

    std::mutex         nonRealtimeLock;
    std::unique_ptr<T> copy;

    // only accessed by realtime thread
    T* currentObj = nullptr;
};

template <typename T, bool> class NRMScopedAccessImpl
{
protected:
    NRMScopedAccessImpl(NonRealtimeMutatable<T>& parent) : p(parent), currentValue(&p.nonRealtimeAcquire()) {}

    ~NRMScopedAccessImpl() { p.nonRealtimeRelease(); }

public:
    T* get() noexcept { return currentValue; }

    const T* get() const noexcept { return currentValue; }

    T& operator*() noexcept { return *currentValue; }

    const T& operator*() const noexcept { return *currentValue; }

    T* operator->() noexcept { return currentValue; }

    const T* operator->() const noexcept { return currentValue; }

private:
    NonRealtimeMutatable<T>& p;
    T*                       currentValue;
};

template <typename T> class NRMScopedAccessImpl<T, true>
{
protected:
    explicit NRMScopedAccessImpl(NonRealtimeMutatable<T>& parent) noexcept
        : p(parent), currentValue(&p.realtimeAcquire())
    {
    }

    ~NRMScopedAccessImpl() noexcept { p.realtimeRelease(); }

public:
    const T* get() const noexcept { return currentValue; }

    const T& operator*() const noexcept { return *currentValue; }

    const T* operator->() const noexcept { return currentValue; }

private:
    NonRealtimeMutatable<T>& p;
    const T*                 currentValue;
};

//==============================================================================
//==============================================================================
template <typename, bool> class RMScopedAccessImpl;

template <typename T> class RealtimeMutatable
{
public:
    RealtimeMutatable() = default;

    explicit RealtimeMutatable(const T& obj) : data({obj, obj}), realtimeCopy(obj) {}

    ~RealtimeMutatable()
    {
        assert((control.load() & BUSY_BIT) ==
               0); // <- never delete this object while the realtime thread is still using it

        // Spin!
        while ((control.load() & BUSY_BIT) == 1)
            ;

        auto accquired = nonRealtimeLock.try_lock();

        ((void)(accquired));
        assert(accquired); // <- you didn't call release on one of the non-realtime threads before deleting this object

        nonRealtimeLock.unlock();
    }

    template <typename... Args> static RealtimeMutatable create(Args&&... args)
    {
        return RealtimeObject(false, std::forward<Args>(args)...);
    }

    T& realtimeAcquire() noexcept { return realtimeCopy; }

    void realtimeRelease() noexcept
    {
        auto idx = acquireIndex();
        data[idx] = realtimeCopy;
        releaseIndex(idx);
    }

    template <typename... Args> void realtimeReplace(Args&&... args)
    {
        T obj(std::forward<Args>(args)...);

        auto idx = acquireIndex();
        data[idx] = std::move(obj);
        releaseIndex(idx);
    }

    const T& nonRealtimeAcquire()
    {
        nonRealtimeLock.lock();
        auto current = control.load(std::memory_order_acquire);

        // there is new data so flip the indices around atomically ensuring we are not inside realtimeAssign
        if ((current & NEWDATA_BIT) != 0)
        {
            int newValue;

            do
            {
                // expect the realtime thread not to be inside the realtime-assign
                current &= ~BUSY_BIT;

                // realtime thread should flip index value and clear the newdata bit
                newValue = (current ^ INDEX_BIT) & INDEX_BIT;
            } while (!control.compare_exchange_weak(current, newValue, std::memory_order_acq_rel));

            current = newValue;
        }

        // flip the index bit as we always use the index that the realtime thread is currently NOT using
        auto nonRealtimeIndex = (current & INDEX_BIT) ^ 1;

        return data[nonRealtimeIndex];
    }

    void nonRealtimeRelease() { nonRealtimeLock.unlock(); }

    template <bool isRealtimeThread> class ScopedAccess : public RMScopedAccessImpl<T, isRealtimeThread>
    {
    public:
        explicit ScopedAccess(RealtimeMutatable& parent) : RMScopedAccessImpl<T, isRealtimeThread>(parent) {}

        ScopedAccess(const ScopedAccess&) = delete;

        ScopedAccess(ScopedAccess&&) = delete;

        ScopedAccess& operator=(const ScopedAccess&) = delete;

        ScopedAccess& operator=(ScopedAccess&&) = delete;
    };

private:
    friend class RMScopedAccessImpl<T, false>;

    friend class RMScopedAccessImpl<T, true>;

    template <typename... Args>
    explicit RealtimeMutatable(bool, Args&&... args)
        : data({T(std::forward(args)...), T(std::forward(args)...)}), realtimeCopy(std::forward(args)...)
    {
    }

    enum
    {
        INDEX_BIT = (1 << 0),
        BUSY_BIT = (1 << 1),
        NEWDATA_BIT = (1 << 2)
    };

    int acquireIndex() noexcept { return control.fetch_or(BUSY_BIT, std::memory_order_acquire) & INDEX_BIT; }

    void releaseIndex(int idx) noexcept { control.store((idx & INDEX_BIT) | NEWDATA_BIT, std::memory_order_release); }

    std::atomic<int> control = {0};

    std::array<T, 2> data;
    T                realtimeCopy;

    std::mutex nonRealtimeLock;
};

template <typename T, bool> class RMScopedAccessImpl
{
protected:
    explicit RMScopedAccessImpl(RealtimeMutatable<T>& parent) : p(parent), currentValue(&p.realtimeAcquire()) {}

    ~RMScopedAccessImpl() { p.realtimeRelease(); }

public:
    T* get() noexcept { return currentValue; }

    const T* get() const noexcept { return currentValue; }

    T& operator*() noexcept { return *currentValue; }

    const T& operator*() const noexcept { return *currentValue; }

    T* operator->() noexcept { return currentValue; }

    const T* operator->() const noexcept { return currentValue; }

private:
    RealtimeMutatable<T>& p;
    T*                    currentValue;
};

template <typename T> class RMScopedAccessImpl<T, false>
{
protected:
    explicit RMScopedAccessImpl(RealtimeMutatable<T>& parent) noexcept
        : p(parent), currentValue(&p.nonRealtimeAcquire())
    {
    }

    ~RMScopedAccessImpl() noexcept { p.nonRealtimeRelease(); }

public:
    const T* get() const noexcept { return currentValue; }

    const T& operator*() const noexcept { return *currentValue; }

    const T* operator->() const noexcept { return currentValue; }

private:
    RealtimeMutatable<T>& p;
    const T*              currentValue;
};
} // namespace internals

//==============================================================================
enum class RealtimeObjectOptions
{
    nonRealtimeMutatable,
    realtimeMutatable
};

enum class ThreadType
{
    realtime,
    nonRealtime
};

//==============================================================================
/** Useful class to synchronise access to an object from multiple threads with the additional feature that one
 * designated thread will never wait to get access to the object. */
template <typename T, RealtimeObjectOptions Options> class RealtimeObject
{
public:
    /** Creates a default constructed T */
    RealtimeObject() = default;

    /** Creates a copy of T */
    explicit RealtimeObject(const T& obj) : mImpl(obj) {}

    /** Moves T into this realtime wrapper */
    explicit RealtimeObject(T&& obj) : mImpl(std::move(obj)) {}

    ~RealtimeObject() = default;

    /** Create T by calling T's constructor which takes args */
    template <typename... Args> static RealtimeObject create(Args&&... args)
    {
        return Impl::create(std::forward<Args>(args)...);
    }

    //==============================================================================
    using RealtimeAcquireReturnType =
        std::conditional_t<Options == RealtimeObjectOptions::nonRealtimeMutatable, const T, T>;
    using NonRealtimeAcquireReturnType =
        std::conditional_t<Options == RealtimeObjectOptions::realtimeMutatable, const T, T>;

    //==============================================================================
    /** Returns a reference to T. Use this method on the real-time thread.
     *  It must be matched by realtimeRelease when you are finished using the
     *  object. Alternatively, use the ScopedAccess helper class below.
     *
     *  Only a single real-time thread can acquire this object at once!
     *
     *  This method is wait- and lock-free.
     */
    RealtimeAcquireReturnType& realtimeAcquire() noexcept { return mImpl.realtimeAcquire(); }

    /** +Releases the lock on T previously acquired by realtimeAcquire.
     *
     *  This method is wait- and lock-free.
     */
    void realtimeRelease() noexcept { mImpl.realtimeRelease(); }

    /** Replace the underlying value with a new instance of T by forwarding
     *  the method's arguments to T's constructor
     */
    template <RealtimeObjectOptions O = Options, typename... Args>
    std::enable_if_t<O == RealtimeObjectOptions::realtimeMutatable, internals::void_t<Args...>>
    realtimeReplace(Args&&... args) noexcept
    {
        mImpl.realtimeReplace(std::forward<Args>(args)...);
    }

    //==============================================================================
    /** Returns a reference to T. Use this method on the non real-time thread.
     *  It must be matched by nonRealtimeRelease when you are finished using
     *  the object. Alternatively, use the ScopedAccess helper class below.
     *
     *  Multiple non-realtime threads can acquire an object at the same time.
     *
     *  This method uses a lock should not be used on a realtime thread.
     */
    NonRealtimeAcquireReturnType& nonRealtimeAcquire() { return mImpl.nonRealtimeAcquire(); }

    /** Releases the lock on T previously acquired by nonRealtimeAcquire.
     *
     *  This method uses both a lock and a spin loop and should not be used
     *  on a realtime thread.
     */
    void nonRealtimeRelease() { mImpl.nonRealtimeRelease(); }

    /** Replace the underlying value with a new instance of T by forwarding
     *  the method's arguments to T's constructor
     */
    template <RealtimeObjectOptions O = Options, typename... Args>
    std::enable_if_t<O == RealtimeObjectOptions::nonRealtimeMutatable, internals::void_t<Args...>>
    nonRealtimeReplace(Args&&... args)
    {
        mImpl.nonRealtimeReplace(std::forward<Args>(args)...);
    }

    //==============================================================================
    /** Instead of calling acquire and release manually, you can also use this RAII
     *  version which calls acquire automatically on construction and release when
     *  destructed.
     */
    template <ThreadType threadType>
    class ScopedAccess
        : public std::conditional_t<Options == RealtimeObjectOptions::realtimeMutatable, internals::RealtimeMutatable<T>,
                                    internals::NonRealtimeMutatable<T>>::template ScopedAccess<threadType ==
                                                                                               ThreadType::realtime>
    {
    public:
        explicit ScopedAccess(RealtimeObject& parent)
            : Impl::template ScopedAccess<threadType == ThreadType::realtime>(parent.mImpl)
        {
        }

#if DOXYGEN
        // Various ways to get access to the underlying object.
        // Non-const method are only supported on the realtime
        // or non-realtime thread as indicated by the Options
        // template argument
        T*       get() noexcept;
        const T* get() const noexcept;
        T&       operator*() noexcept;
        const T& operator*() const noexcept;
        T*       operator->() noexcept;
        const T* operator->() const noexcept;
#endif

        //==============================================================================
        ScopedAccess(const ScopedAccess&) = delete;

        ScopedAccess(ScopedAccess&&) = delete;

        ScopedAccess& operator=(const ScopedAccess&) = delete;

        ScopedAccess& operator=(ScopedAccess&&) = delete;
    };

private:
    using Impl = std::conditional_t<Options == RealtimeObjectOptions::realtimeMutatable, internals::RealtimeMutatable<T>,
                                    internals::NonRealtimeMutatable<T>>;
    Impl mImpl;
};

} // namespace farbot

#endif // FARBOT_HPP
