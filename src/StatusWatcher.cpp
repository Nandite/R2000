//
// Created by chazz on 22/12/2021.
//

#include <R2000/StatusWatcher.hpp>

Device::DeviceStatus::DeviceStatus(Parameters::ParametersMap parameters)
        : systemStatusMap(std::move(parameters)) {
}

Device::StatusFlagInterpreter::StatusFlagInterpreter(const uint32_t flags) : flags(flags) {}

Device::StatusWatcher::StatusWatcher(std::shared_ptr<R2000> sharedDevice, std::chrono::seconds period)
        : device(std::move(sharedDevice)), period(period) {
    statusWatcherTaskFuture = std::async(std::launch::async, &StatusWatcher::statusWatcherTask, this);
}

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
    for (; !interruptFlag.load(std::memory_order_acquire);) {
        auto future{getParametersCommand.asyncExecute(1s, systemStatus)};
        if (!future) {
            std::clog << "StatusWatcher::" << device->getName() << "::Device is busy." << std::endl;
        } else {
            future->wait();
            const auto & [requestResult, obtainedParameters]{future->get()};
            std::scoped_lock scopedGuard{deviceConnectedCallbackLock, deviceDisconnectedCallbackLock};
            if (requestResult == RequestResult::SUCCESS) {
                setStatusToOutputAndFireNewStatusEvent(std::make_shared<DeviceStatus>(obtainedParameters));
                const auto wasConnected{isConnected.exchange(true, std::memory_order_release)};
                if (!wasConnected) {
                    fireDeviceConnectionEvent();
                }
            } else {
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