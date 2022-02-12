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
    for (; !interruptFlag.load(std::memory_order_acquire);) {
        auto future{getParametersCommand.asyncExecute(1s, systemStatus)};
        if (!future) {
            std::clog << "StatusWatcher::" << device->getName() << "::Device is busy." << std::endl;
        } else {
            future->wait();
            const auto result{future->get()};
            const auto requestResult{result.first};
            if (requestResult == RequestResult::SUCCESS) {
                setStatusToOutputAndFireNewStatusEvent(std::make_shared<DeviceStatus>(result.second));
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
        std::unique_lock<LockType> interruptGuard{interruptCvLock, std::adopt_lock};
        interruptCv.wait_for(interruptGuard, period,
                             [&]() { return interruptFlag.load(std::memory_order_acquire); });
    }
}

Device::StatusWatcher::~StatusWatcher() {
    if (!interruptFlag.load(std::memory_order_acquire)) {
        {
            std::unique_lock<LockType> guard{interruptCvLock, std::adopt_lock};
            interruptFlag.store(true, std::memory_order_release);
            interruptCv.notify_one();
        }
        statusWatcherTaskFuture.wait();
    }
}
