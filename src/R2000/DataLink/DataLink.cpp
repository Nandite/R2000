//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file
#include "R2000/DataLink/DataLink.hpp"
#include "R2000/DeviceHandle.hpp"
#include "R2000/R2000.hpp"

Device::DataLink::DataLink(std::shared_ptr<R2000> iDevice, std::shared_ptr<DeviceHandle> iHandle)
        : device(std::move(iDevice)), deviceHandle(std::move(iHandle)) {
    if (!Device::Commands::StartScanCommand{*device}.execute(*deviceHandle)) {
        isConnected.store(false, std::memory_order_release);
        std::clog << __func__ << "Could not request the device to start the stream." << std::endl;
        return;
    }
    if (deviceHandle->watchdogEnabled) {
        watchdogTask = spawnAsync(
                [this]() {
                    pthread_setname_np(pthread_self(), "Device-Watchdog");
                    Device::Commands::FeedWatchdogCommand feedWatchdogCommand{*device};
                    for (; !interruptFlag.load(std::memory_order_acquire);) {
                        try {
                            isConnected.store(feedWatchdogCommand.execute(*deviceHandle), std::memory_order_release);
                        }
                        catch (...) {
                            isConnected.store(false, std::memory_order_release);
                        }
                        std::unique_lock<LockType> guard(interruptCvLock, std::adopt_lock);
                        interruptCv.wait_for(guard, deviceHandle->watchdogTimeout,
                                             [this]() -> bool {
                                                 return interruptFlag.load(std::memory_order_acquire);
                                             });
                    }
                });
    }
}

Device::DataLink::~DataLink() {
    if (!interruptFlag.load(std::memory_order_acquire)) {
        {
            std::unique_lock<LockType> guard(interruptCvLock, std::adopt_lock);
            interruptFlag.store(true, std::memory_order_release);
            interruptCv.notify_one();
        }
        scanAvailableCv.notify_all();
        if (deviceHandle->watchdogEnabled)
            watchdogTask.wait();

        if (!Device::Commands::StopScanCommand{*device}.execute(*deviceHandle)) {
            std::clog << __func__ << ": Could not stop the data stream." << std::endl;
        }
        if (!Device::Commands::ReleaseHandleCommand{*device}.execute(*deviceHandle)) {
            std::clog << __func__ << ": Could not release the handle." << std::endl;
        }
    }
}

bool Device::DataLink::isAlive() const {
    return isConnected.load(std::memory_order_acquire);
}
