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
        auto future{getParametersCommand.asyncExecuteFuture(1s, systemStatus)};
        if (!future) {
            std::clog << "StatusWatcher::" << device->getName()
                      << "::Could not request get status from the device." << std::endl;
        } else {
            future->wait();
            auto result{future->get()};
            const auto requestResult{result.first};
            const auto requestHasSucceed{requestResult == RequestResult::SUCCESS};
            const auto parameters{result.second};
            if (requestHasSucceed) {
                const auto status{DeviceStatus{parameters}};
                isConnected.store(true, std::memory_order_release);
                {
                    RealtimeStatus::ScopedAccess <farbot::ThreadType::realtime> scanGuard{*realtimeStatus};
                    *scanGuard = status;
                }
                std::unique_lock<LockType> guard{callbackLock, std::adopt_lock};
                for (auto &callback : callbacks) {
                    std::invoke(callback, status);
                }
            } else {
                std::clog << "StatusWatcher::" << device->getName() << "::Could not get the status "
                          << "from the device (" << asyncResultToString(requestResult) << ")." << std::endl;
                isConnected.store(false, std::memory_order_release);
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
