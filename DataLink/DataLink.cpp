//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file
#include "HttpController.hpp"
#include "DataLink/DataLink.hpp"
#include "DeviceHandle.hpp"

R2000::DataLink::DataLink(std::shared_ptr<HttpController> c, std::shared_ptr<DeviceHandle> h) :
        mController(std::move(c)), mHandle(std::move(h)) {
    if (mHandle->watchdogEnabled) {
        mWatchdogTask = spawnAsync([this]() {
            pthread_setname_np(pthread_self(), "R2000-Watchdog");
            for (; !mInterruptFlag.load(std::memory_order_acquire);) {
                try {
                    mIsConnected.store(mController->feedWatchdog(*mHandle), std::memory_order_release);
                } catch (...) {
                    mIsConnected.store(false, std::memory_order_release);
                }
                std::unique_lock<LockType> guard(mInterruptCvLock, std::adopt_lock_t{});
                mCv.wait_for(guard, mHandle->watchdogTimeout, [this]() -> bool {
                    return mInterruptFlag.load(std::memory_order_acquire);
                });
            }
        });
    }
}

R2000::DataLink::~DataLink() {
    if (!mInterruptFlag.load(std::memory_order_acquire)) {
        {
            std::lock_guard<LockType> guard(mInterruptCvLock);
            mInterruptFlag.store(true, std::memory_order_release);
            mCv.notify_one();
        }
        mWatchdogTask.wait();
    }
    mController->releaseHandle(*mHandle);
}

bool R2000::DataLink::isAlive() const {
    return mIsConnected.load(std::memory_order_acquire);
}
