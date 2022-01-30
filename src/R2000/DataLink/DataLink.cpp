//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file
#include "R2000/DataLink/DataLink.hpp"
#include "R2000/Control/DeviceHandle.hpp"
#include "R2000/R2000.hpp"
#include <R2000/Control/Commands.hpp>

Device::DataLink::DataLink(std::shared_ptr<R2000> iDevice, std::shared_ptr<DeviceHandle> iHandle,
                           std::chrono::milliseconds connectionTimeout)
        : device(std::move(iDevice)), deviceHandle(std::move(iHandle)) {
    if (!Device::Commands::StartScanCommand{*device}.asyncExecute(
            *deviceHandle, connectionTimeout,
            [&](const auto &result) -> void {
                if (result == AsyncRequestResult::SUCCESS) {
                    isConnected.store(true, std::memory_order_release);
                    if (deviceHandle->isWatchdogEnabled()) {
                        watchdogTaskFuture = std::async(std::launch::async, &DataLink::watchdogTask, this, 1s);
                    }
                } else {
                    std::clog << device->getName() << "::DataLink::Could not request the device to start the stream ("
                              << asyncResultToString(result) << ")" << std::endl;
                    isConnected.store(false, std::memory_order_release);
                }
            })) {
        isConnected.store(false, std::memory_order_release);
        std::clog << device->getName() << "::DataLink::Could not request the device to start the stream." << std::endl;
        return;
    }
}

Device::DataLink::~DataLink() {
    if (!interruptFlag.load(std::memory_order_acquire)) {
        {
            std::unique_lock<LockType> guard{interruptCvLock, std::adopt_lock};
            interruptFlag.store(true, std::memory_order_release);
            interruptCv.notify_one();
        }
        scanAvailableCv.notify_all();
        if (watchdogTaskFuture) {
            watchdogTaskFuture->wait();
        }

        auto stopScanFuture{Device::Commands::StopScanCommand{*device}.asyncExecute(*deviceHandle, 1s)};
        if (stopScanFuture) {
            stopScanFuture->wait();
            const auto result{stopScanFuture->get()};
            if (result != AsyncRequestResult::SUCCESS) {
                std::clog << device->getName() << "::DataLink::Could not stop the data stream ("
                          << asyncResultToString(result) << ")"
                          << std::endl;
            }
        }
        auto releaseHandleFuture{Device::Commands::ReleaseHandleCommand{*device}.asyncExecute(*deviceHandle, 1s)};
        if (releaseHandleFuture) {
            releaseHandleFuture->wait();
            const auto result{releaseHandleFuture->get()};
            if (result != AsyncRequestResult::SUCCESS) {
                std::clog << device->getName() << "::DataLink::Could not release the handle ("
                          << asyncResultToString(result) << ")"
                          << std::endl;
            }
        }
    }
}

bool Device::DataLink::isAlive() const noexcept {
    return isConnected.load(std::memory_order_acquire);
}

void Device::DataLink::watchdogTask(std::chrono::seconds commandTimeout) {
    const auto threadName{device->getName() + ".Watchdog"};
    pthread_setname_np(pthread_self(), threadName.c_str());
    Device::Commands::FeedWatchdogCommand feedWatchdogCommand{*device};
    const auto watchdogTimeout{deviceHandle->getWatchdogTimeout()};
    for (; !interruptFlag.load(std::memory_order_acquire);) {
        auto future{feedWatchdogCommand.asyncExecute(*deviceHandle, commandTimeout)};
        if (!future) {
            isConnected.store(false, std::memory_order_release);
        }
        future->wait();
        auto result{future->get()};
        const auto hasSucceed{result == AsyncRequestResult::SUCCESS};
        isConnected.store(hasSucceed, std::memory_order_release);
        std::unique_lock<LockType> guard{interruptCvLock, std::adopt_lock};
        interruptCv.wait_for(guard, watchdogTimeout,
                             [this]() -> bool { return interruptFlag.load(std::memory_order_acquire); });
    }
}
