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

#include <StatusWatcher.hpp>

Device::DeviceStatus::DeviceStatus(Parameters::ParametersMap parameters)
        : systemStatusMap(std::move(parameters)) {
}

Device::StatusFlagInterpreter::StatusFlagInterpreter(const uint32_t flags) : flags(flags) {}

void Device::StatusWatcher::statusWatcherTask() {
    const auto taskId{device->getName() + ".StatusWatcher"};
    pthread_setname_np(pthread_self(), taskId.c_str());
    Device::Commands::GetParametersCommand getParametersCommand{*device};
    const auto systemStatus{Parameters::ReadOnlyParameters::SystemStatus{}.requestLoadIndication()
                                    .requestSystemTimeRaw()
                                    .requestUpTime()
                                    .requestPowerCycles()
                                    .requestOperationTime()
                                    .requestOperationTimeScaled()
                                    .requestCurrentTemperature()
                                    .requestMinimalTemperature()
                                    .requestMaximalTemperature()
                                    .requestStatusFlags()};
    auto disconnectionHitCount{0u};
    for (; !interruptFlag.load(std::memory_order_acquire);) {
        auto future{getParametersCommand.asyncExecute(1s, systemStatus)};
        if (!future) {
            std::clog << device->getName() << "StatusWatcher:: Device is busy." << std::endl;
        } else {
            const auto & [requestResult, obtainedParameters]{future->get()};
            const auto disconnectionThreshold{disconnectionTriggerThreshold.load(std::memory_order_acquire)};
            std::scoped_lock scopedGuard{deviceConnectedCallbackLock, deviceDisconnectedCallbackLock};
            if (requestResult == RequestResult::SUCCESS) {
                disconnectionHitCount = 0;
                setStatusToOutputAndFireNewStatusEvent(std::make_shared<DeviceStatus>(obtainedParameters));
                const auto wasConnected{isConnected.exchange(true, std::memory_order_release)};
                if (!wasConnected) {
                    fireDeviceConnectionEvent();
                }
            } else if (++disconnectionHitCount >= disconnectionThreshold) {
                const auto wasConnected{isConnected.exchange(false, std::memory_order_release)};
                if (wasConnected) {
                    fireDeviceDisconnectionEvent();
                }
            }
        }
        std::unique_lock<LockType> interruptGuard{interruptCvLock};
        interruptCv.wait_for(interruptGuard, period,
                             [&]() { return interruptFlag.load(std::memory_order_acquire); });
    }
}

Device::StatusWatcher::~StatusWatcher() {
    if (!interruptFlag.load(std::memory_order_acquire)) {
        {
            std::lock_guard<LockType> guard{interruptCvLock};
            interruptFlag.store(true, std::memory_order_release);
            interruptCv.notify_one();
        }
        statusWatcherTaskFuture.wait();
    }
}