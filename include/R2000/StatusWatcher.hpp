#pragma once

#include "Control/Commands.hpp"
#include "R2000.hpp"
#include <unordered_map>
#include <any>
#include <bitset>

#define PARAMETER_STATUS_FLAGS "status_flags"
#define PARAMETER_STATUS_LOAD_INDICATION "load_indication"
#define PARAMETER_STATUS_SYSTEM_TIME_RAW "system_time_raw"
#define PARAMETER_STATUS_UP_TIME "up_time"
#define PARAMETER_STATUS_POWER_CYCLES "power_cycles"
#define PARAMETER_STATUS_OPERATION_TIME "operation_time"
#define PARAMETER_STATUS_OPERATION_TIME_SCALED "operation_time_scaled"
#define PARAMETER_STATUS_TEMPERATURE_CURRENT "temperature_current"
#define PARAMETER_STATUS_TEMPERATURE_MIN "temperature_min"
#define PARAMETER_STATUS_TEMPERATURE_MAX "temperature_max"

namespace Device {
    class StatusFlagInterpretation {
    public:
    private:
        explicit StatusFlagInterpretation(uint32_t flags);

    public:
        /**
         *
         * @return
         */
        [[nodiscard]] inline bool isInitializing() const { return flags[0]; }

        /**
         *
         * @return
         */
        [[nodiscard]] inline bool outputScanIsMuted() const { return flags[3]; }

        /**
         *
         * @return
         */
        [[nodiscard]] inline bool headHasUnstableRotation() const { return flags[4]; }

        /**
         *
         * @return
         */
        [[nodiscard]] inline bool deviceHasWarning() const { return flags[9]; }

        /**
         *
         * @return
         */
        [[nodiscard]] inline bool hasLensContaminationWarning() const { return flags[10]; }

        /**
         *
         * @return
         */
        [[nodiscard]] inline bool hasLowTemperatureWarning() const { return flags[11]; }

        /**
         *
         * @return
         */
        [[nodiscard]] inline bool hasHighTemperatureWarning() const { return flags[12]; }

        /**
         *
         * @return
         */
        [[nodiscard]] inline bool hasDeviceOverloadWarning() const { return flags[13]; }

        /**
         *
         * @return
         */
        [[nodiscard]] inline bool deviceHasError() const { return flags[17]; }

        /**
         *
         * @return
         */
        [[nodiscard]] inline bool hasLensContaminationError() const { return flags[18]; }

        /**
         *
         * @return
         */
        [[nodiscard]] inline bool hasLowTemperatureError() const { return flags[19]; }

        /**
         *
         * @return
         */
        [[nodiscard]] inline bool hasHighTemperatureError() const { return flags[20]; }

        /**
         *
         * @return
         */
        [[nodiscard]] inline bool hasDeviceOverloadError() const { return flags[21]; }

        /**
         *
         * @return
         */
        [[nodiscard]] inline bool hasUnrecoverableDefect() const { return flags[31]; }

    private:
        std::bitset<32> flags{};
        friend class DeviceStatus;
    };

    template<typename T>
    struct QuietAnyCastHelper;

    template<>
    struct QuietAnyCastHelper<int64_t> {
        /**
         *
         * @param any
         * @return
         */
        static inline unsigned int castQuietly(const std::any &any) {
            return any.has_value() ? std::stoi(std::any_cast<std::string>(any))
                                   : std::numeric_limits<int64_t>::quiet_NaN();
        }
    };

    template<>
    struct QuietAnyCastHelper<uint64_t> {
        /**
         *
         * @param any
         * @return
         */
        static inline unsigned int castQuietly(const std::any &any) {
            return any.has_value() ? std::stoul(std::any_cast<std::string>(any))
                                   : std::numeric_limits<uint64_t>::quiet_NaN();
        }
    };

    template<>
    struct QuietAnyCastHelper<uint32_t> {
        /**
         *
         * @param any
         * @return
         */
        static inline unsigned int castQuietly(const std::any &any) {
            return any.has_value() ? std::stoul(std::any_cast<std::string>(any))
                                   : std::numeric_limits<uint32_t>::quiet_NaN();
        }
    };

    class DeviceStatus {
    private:
        /**
         *
         * @tparam T
         * @param parameters
         * @param key
         * @return
         */
        template<typename T>
        static inline std::any getAsAnyOrNullopt(const Parameters::ParametersMap &parameters,
                                                 const std::string &key) noexcept(false) {
            if (!parameters.count(key))
                return std::nullopt;
            return std::make_any<T>(parameters.at(key));
        }

        /**
         *
         * @param systemStatusMap
         */
        explicit DeviceStatus(Parameters::ParametersMap systemStatusMap);

    public:
        /**
         *
         * @return
         */
        [[nodiscard]] inline StatusFlagInterpretation getStatusFlags() const {
            const auto any{getAsAnyOrNullopt<std::string>(systemStatusMap, PARAMETER_STATUS_FLAGS)};
            return any.has_value() ?
                   StatusFlagInterpretation(QuietAnyCastHelper<uint32_t>::castQuietly(any))
                                   : StatusFlagInterpretation(0x00000000);
        }

        /**
         *
         * @return
         */
        [[nodiscard]] inline uint64_t getCpuLoad() const {
            return QuietAnyCastHelper<uint64_t>::castQuietly(
                    getAsAnyOrNullopt<std::string>(systemStatusMap, PARAMETER_STATUS_LOAD_INDICATION));
        }

        /**
         *
         * @return
         */
        [[nodiscard]] inline uint64_t getRawSystemTime() const {
            return QuietAnyCastHelper<uint64_t>::castQuietly(
                    getAsAnyOrNullopt<std::string>(systemStatusMap, PARAMETER_STATUS_SYSTEM_TIME_RAW));
        }

        /**
         *
         * @return
         */
        [[nodiscard]] inline uint64_t getUptime() const {
            return QuietAnyCastHelper<uint64_t>::castQuietly(
                    getAsAnyOrNullopt<std::string>(systemStatusMap, PARAMETER_STATUS_UP_TIME));
        }

        /**
         *
         * @return
         */
        [[nodiscard]] inline uint64_t getPowerCyclesCount() const {
            return QuietAnyCastHelper<uint64_t>::castQuietly(
                    getAsAnyOrNullopt<std::string>(systemStatusMap, PARAMETER_STATUS_POWER_CYCLES));
        }

        /**
         *
         * @return
         */
        [[nodiscard]] inline uint64_t getOperationTime() const {
            return QuietAnyCastHelper<uint64_t>::castQuietly(
                    getAsAnyOrNullopt<std::string>(systemStatusMap, PARAMETER_STATUS_OPERATION_TIME));
        }

        /**
         *
         * @return
         */
        [[nodiscard]] inline uint64_t getScaledOperationTime() const {
            return QuietAnyCastHelper<uint64_t>::castQuietly(
                    getAsAnyOrNullopt<std::string>(systemStatusMap, PARAMETER_STATUS_OPERATION_TIME_SCALED));
        }

        /**
         *
         * @return
         */
        [[nodiscard]] inline uint64_t getCurrentTemperature() const {
            return QuietAnyCastHelper<uint64_t>::castQuietly(
                    getAsAnyOrNullopt<std::string>(systemStatusMap, PARAMETER_STATUS_TEMPERATURE_CURRENT));
        }

        /**
         *
         * @return
         */
        [[nodiscard]] inline uint64_t getMinimalTemperature() const {
            return QuietAnyCastHelper<uint64_t>::castQuietly(
                    getAsAnyOrNullopt<std::string>(systemStatusMap, PARAMETER_STATUS_TEMPERATURE_MIN));
        }

        /**
         *
         * @return
         */
        [[nodiscard]] inline uint64_t getMaximalTemperature() const {
            return QuietAnyCastHelper<uint64_t>::castQuietly(
                    getAsAnyOrNullopt<std::string>(systemStatusMap, PARAMETER_STATUS_TEMPERATURE_MAX));
        }

    private:
        Parameters::ParametersMap systemStatusMap;

        template<typename T> friend
        class StatusWatcher;
    };

    using OnStatusAvailableCallback = std::function<void(const DeviceStatus &)>;

    template<typename Duration>
    class StatusWatcher {
        using LockType = std::mutex;
        using Cv = std::condition_variable;
    public:
        /**
         *
         * @param sharedDevice
         * @param period
         * @param callback
         */
        StatusWatcher(std::shared_ptr<R2000> sharedDevice,
                      Duration period,
                      OnStatusAvailableCallback callback) : device(std::move(sharedDevice)),
                                                            period(period) {
            callbacks.push_back(std::move(callback));
            statusWatcherTaskFuture = std::async(std::launch::async, &StatusWatcher::statusWatcherTask, this);
        }

        /**
         *
         * @return
         */
        [[nodiscard]] inline bool isAlive() const {
            return isConnected.load(std::memory_order_acquire);
        }

        void statusWatcherTask() {
            const auto threadId{device->getName() + ".StatusWatcher"};
            pthread_setname_np(pthread_self(), threadId.c_str());
            Device::Commands::GetParametersCommand getParametersCommand{*device};
            for (; !interruptFlag.load(std::memory_order_acquire);) {
                auto future{getParametersCommand.asyncExecuteFuture(1s, systemStatusBuilder)};
                if (!future) {
                    std::clog << "StatusWatcher::" << device->getName()
                              << "::Could not request to get parameters from the device." << std::endl;
                } else {
                    future->wait();
                    auto result{future->get()};
                    const auto requestResult{result.first};
                    const auto hasSucceed{requestResult == AsyncRequestResult::SUCCESS};
                    const auto values{result.second};
                    if (hasSucceed && values) {
                        isConnected.store(true, std::memory_order_release);
                        const DeviceStatus status{*values};
                        std::unique_lock<LockType> guard{callbackLock, std::adopt_lock};
                        for (auto &callback : callbacks)
                            std::invoke(callback, status);
                    } else {
                        std::clog << "StatusWatcher::" << device->getName() << "::Could not get the status "
                                  << "from the device (" << asyncResultToString(requestResult) << ")." << std::endl;
                        isConnected.store(false, std::memory_order_release);
                    }
                }
                std::unique_lock<LockType> interruptGuard{interruptCvLock, std::adopt_lock};
                cv.wait_for(interruptGuard, period, [this]() -> bool {
                    return interruptFlag.load(std::memory_order_acquire);
                });
            }
        }

        /**
         *
         * @param callback
         */
        [[maybe_unused]] inline void addCallback(OnStatusAvailableCallback callback) {
            std::unique_lock<LockType> guard{callbackLock, std::adopt_lock};
            callbacks.push_back(std::move(callback));
        }

        /**
         *
         */
        virtual ~StatusWatcher() {
            if (!interruptFlag.load(std::memory_order_acquire)) {
                {
                    std::unique_lock<LockType> guard{interruptCvLock, std::adopt_lock};
                    interruptFlag.store(true, std::memory_order_release);
                    cv.notify_one();
                }
                statusWatcherTaskFuture.wait();
            }
        }

    private:
        std::shared_ptr<R2000> device{nullptr};
        std::future<void> statusWatcherTaskFuture{};
        Cv cv{};
        LockType interruptCvLock{};
        LockType callbackLock{};
        std::atomic_bool interruptFlag{false};
        Duration period{};
        std::vector<OnStatusAvailableCallback> callbacks{};
        std::atomic_bool isConnected{false};
        const Parameters::ReadOnlyParameters::SystemStatus systemStatusBuilder{
                Parameters::ReadOnlyParameters::SystemStatus{}
                        .requestStatusFlags()
                        .requestLoadIndication()
                        .requestSystemTimeRaw()
                        .requestUpTime()
                        .requestPowerCycles()
                        .requestOperationTime()
                        .requestOperationTimeScaled()
                        .requestCurrentTemperature()
                        .requestMinimalTemperature()
                        .requestMaximalTemperature()};
    };
}
