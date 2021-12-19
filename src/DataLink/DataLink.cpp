//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file
#include "DataLink/DataLink.hpp"
#include "DeviceHandle.hpp"
#include "R2000.hpp"

Device::DataLink::DataLink(std::shared_ptr<R2000> controller, std::shared_ptr<DeviceHandle> handle)
        : mDevice(std::move(controller)), mHandle(std::move(handle)) {
    if (!Device::Commands::StartScanCommand{*mDevice}.execute(*mHandle)) {
        mIsConnected.store(false, std::memory_order_release);
        std::clog << __func__ << "Could not request the device to start the stream" << std::endl;
        return;
    }
    if (mHandle->watchdogEnabled) {
        mWatchdogTask = internals::spawnAsync(
                [this]() {
                    pthread_setname_np(pthread_self(), "Device-Watchdog");
                    Device::Commands::FeedWatchdogCommand feedWatchdogCommand{*mDevice};
                    for (; !mInterruptFlag.load(std::memory_order_acquire);) {
                        try {
                            mIsConnected.store(feedWatchdogCommand.execute(*mHandle), std::memory_order_release);
                        }
                        catch (...) {
                            mIsConnected.store(false, std::memory_order_release);
                        }
                        std::unique_lock<LockType> guard(mInterruptCvLock, std::adopt_lock_t{});
                        mCv.wait_for(guard, mHandle->watchdogTimeout / 2,
                                     [this]() -> bool { return mInterruptFlag.load(std::memory_order_acquire); });
                    }
                });
    }
}

Device::DataLink::~DataLink() {
    if (!mInterruptFlag.load(std::memory_order_acquire)) {
        {
            std::lock_guard<LockType> guard(mInterruptCvLock);
            mInterruptFlag.store(true, std::memory_order_release);
            mCv.notify_one();
        }
        if (mHandle->watchdogEnabled)
            mWatchdogTask.wait();
    }
    Device::Commands::StopScanCommand{*mDevice}.execute(*mHandle);
    Device::Commands::ReleaseHandleCommand{*mDevice}.execute(*mHandle);
}

bool Device::DataLink::isAlive() const {
    return mIsConnected.load(std::memory_order_acquire);
}
