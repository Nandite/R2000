// Copyright (c) 2022 Papa Libasse Sow.
// https://github.com/Nandite/R2000
// Distributed under the MIT Software License (X11 license).
//
// SPDX-License-Identifier: MIT
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of
// the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "DataLink/DataLink.hpp"
#include "Control/DeviceHandle.hpp"
#include "R2000.hpp"
#include <Control/Commands.hpp>

Device::DataLink::DataLink(std::shared_ptr<R2000> iDevice, std::shared_ptr<DeviceHandle> iHandle,
                           std::chrono::milliseconds connectionTimeout)
        : device(std::move(iDevice)), deviceHandle(std::move(iHandle)) {
    if (!Device::Commands::StartScanCommand{*device}.asyncExecute(
            *deviceHandle, connectionTimeout,
            [&](const auto &result) -> void {
                if (result == RequestResult::SUCCESS) {
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
            if (result != RequestResult::SUCCESS) {
                std::clog << device->getName() << "::DataLink::Could not stop the data stream ("
                          << asyncResultToString(result) << ")"
                          << std::endl;
            }
        }
        auto releaseHandleFuture{Device::Commands::ReleaseHandleCommand{*device}.asyncExecute(*deviceHandle, 1s)};
        if (releaseHandleFuture) {
            releaseHandleFuture->wait();
            const auto result{releaseHandleFuture->get()};
            if (result != RequestResult::SUCCESS) {
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
    const auto taskName{device->getName() + ".Watchdog"};
    pthread_setname_np(pthread_self(), taskName.c_str());
    Device::Commands::FeedWatchdogCommand feedWatchdogCommand{*device};
    const auto watchdogTimeout{deviceHandle->getWatchdogTimeout()};
    for (; !interruptFlag.load(std::memory_order_acquire);) {
        auto future{feedWatchdogCommand.asyncExecute(*deviceHandle, commandTimeout)};
        if (!future) {
            isConnected.store(false, std::memory_order_release);
        } else {
            future->wait();
            auto result{future->get()};
            if (result == RequestResult::SUCCESS) {
                isConnected.store(true, std::memory_order_release);
            } else {
                const auto wasConnected{isConnected.exchange(false, std::memory_order_release)};
                if (wasConnected) {
                    fireDataLinkConnectionLostEvent();
                }
            }
        }
        std::unique_lock<LockType> guard{interruptCvLock, std::adopt_lock};
        interruptCv.wait_for(guard, watchdogTimeout,
                             [&]() -> bool { return interruptFlag.load(std::memory_order_acquire); });
    }
}
