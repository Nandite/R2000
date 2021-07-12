//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#pragma once
#include "Data.hpp"
#include <future>
#include <deque>
#include <condition_variable>
#include <mutex>
#include <atomic>

namespace R2000 {
    class HttpController;
    struct DeviceHandle;
    class DataLink {
        using LockType = std::mutex;
    protected:
        DataLink(std::shared_ptr<HttpController> c, std::shared_ptr<DeviceHandle> h);
    public:
        template <typename F, typename... Ts>
        inline static std::future<typename std::result_of<F(Ts...)>::type> spawnAsync(F&& f, Ts&&... params)
        {
            return std::async(std::launch::async, std::forward<F>(f), std::forward<Ts>(params)...);
        }
        [[nodiscard]] virtual bool isAlive() const;
        [[nodiscard]] virtual unsigned int available() const = 0;
        virtual R2000::Scan pop() = 0;
        virtual ~DataLink();
    private:
        std::shared_ptr<HttpController> mController{nullptr};
        std::future<void> mWatchdogTask{};
        std::condition_variable mCv{};
        LockType mInterruptCvLock{};
        std::atomic_bool mInterruptFlag{false};
    protected:
        std::shared_ptr<DeviceHandle> mHandle{nullptr};
        std::atomic_bool mIsConnected{false};

    };
}
