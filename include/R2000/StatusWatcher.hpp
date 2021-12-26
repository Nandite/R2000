#pragma once

#include "R2000.hpp"
#include <unordered_map>
#include <any>

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
        [[nodiscard]] inline bool isInitializing() const { return flags[0]; }
        [[nodiscard]] inline bool outputScanIsMuted() const { return flags[3]; }
        [[nodiscard]] inline bool headHasUnstableRotation() const { return flags[4]; }
        [[nodiscard]] inline bool deviceHasWarning() const { return flags[9]; }
        [[nodiscard]] inline bool hasLensContaminationWarning() const { return flags[10]; }
        [[nodiscard]] inline bool hasLowTemperatureWarning() const { return flags[11]; }
        [[nodiscard]] inline bool hasHighTemperatureWarning() const { return flags[12]; }
        [[nodiscard]] inline bool hasDeviceOverloadWarning() const { return flags[13]; }
        [[nodiscard]] inline bool deviceHasError() const { return flags[17]; }
        [[nodiscard]] inline bool hasLensContaminationError() const { return flags[18]; }
        [[nodiscard]] inline bool hasLowTemperatureError() const { return flags[19]; }
        [[nodiscard]] inline bool hasHighTemperatureError() const { return flags[20]; }
        [[nodiscard]] inline bool hasDeviceOverloadError() const { return flags[21]; }
        [[nodiscard]] inline bool hasUnrecoverableDefect() const { return flags[31]; }

    private:
        std::bitset<32> flags{};
        friend class DeviceStatus;
    };

    template<typename T>
    struct QuietAnyCastHelper;

    template<>
    struct QuietAnyCastHelper<int64_t> {
        static inline unsigned int castQuietly(const std::any &any) {
            return any.has_value() ? std::stoi(std::any_cast<std::string>(any))
                                   : std::numeric_limits<int64_t>::quiet_NaN();
        }
    };

    template<>
    struct QuietAnyCastHelper<uint64_t> {
        static inline unsigned int castQuietly(const std::any &any) {
            return any.has_value() ? std::stoul(std::any_cast<std::string>(any))
                                   : std::numeric_limits<uint64_t>::quiet_NaN();
        }
    };

    template<>
    struct QuietAnyCastHelper<uint32_t> {
        static inline unsigned int castQuietly(const std::any &any) {
            return any.has_value() ? std::stoul(std::any_cast<std::string>(any))
                                   : std::numeric_limits<uint32_t>::quiet_NaN();
        }
    };

    class DeviceStatus {
    private:
        template<typename T>
        static inline std::any getAsAnyOrNullopt(const Device::ParametersMap &parameters,
                                          const std::string &key) noexcept(false) {
            if (!parameters.count(key))
                return std::nullopt;
            return std::make_any<T>(parameters.at(key));
        }

        explicit DeviceStatus(Device::ParametersMap systemStatusMap);

    public:
        inline StatusFlagInterpretation getStatusFlags() const {
            const auto any{getAsAnyOrNullopt<std::string>(systemStatusMap, PARAMETER_STATUS_FLAGS)};
            return any.has_value() ?
                   StatusFlagInterpretation(QuietAnyCastHelper<uint32_t>::castQuietly(any))
                                   : StatusFlagInterpretation(0x00000000);
        }

        inline uint64_t getCpuLoad() const {
            return QuietAnyCastHelper<uint64_t>::castQuietly(
                    getAsAnyOrNullopt<std::string>(systemStatusMap, PARAMETER_STATUS_LOAD_INDICATION));
        }

        inline uint64_t getRawSystemTime() const {
            return QuietAnyCastHelper<uint64_t>::castQuietly(
                    getAsAnyOrNullopt<std::string>(systemStatusMap, PARAMETER_STATUS_SYSTEM_TIME_RAW));
        }

        inline uint64_t getUptime() const {
            return QuietAnyCastHelper<uint64_t>::castQuietly(
                    getAsAnyOrNullopt<std::string>(systemStatusMap, PARAMETER_STATUS_UP_TIME));
        }

        inline uint64_t getPowerCyclesCount() const {
            return QuietAnyCastHelper<uint64_t>::castQuietly(
                    getAsAnyOrNullopt<std::string>(systemStatusMap, PARAMETER_STATUS_POWER_CYCLES));
        }

        inline uint64_t getOperationTime() const {
            return QuietAnyCastHelper<uint64_t>::castQuietly(
                    getAsAnyOrNullopt<std::string>(systemStatusMap, PARAMETER_STATUS_OPERATION_TIME));
        }

        inline uint64_t getScaledOperationTime() const {
            return QuietAnyCastHelper<uint64_t>::castQuietly(
                    getAsAnyOrNullopt<std::string>(systemStatusMap, PARAMETER_STATUS_OPERATION_TIME_SCALED));
        }

        inline uint64_t getCurrentTemperature() const {
            return QuietAnyCastHelper<uint64_t>::castQuietly(
                    getAsAnyOrNullopt<std::string>(systemStatusMap, PARAMETER_STATUS_TEMPERATURE_CURRENT));
        }

        inline uint64_t getMinimalTemperature() const {
            return QuietAnyCastHelper<uint64_t>::castQuietly(
                    getAsAnyOrNullopt<std::string>(systemStatusMap, PARAMETER_STATUS_TEMPERATURE_MIN));
        }

        inline uint64_t getMaximalTemperature() const {
            return QuietAnyCastHelper<uint64_t>::castQuietly(
                    getAsAnyOrNullopt<std::string>(systemStatusMap, PARAMETER_STATUS_TEMPERATURE_MAX));
        }

    private:
        Device::ParametersMap systemStatusMap;
        template<typename T> friend
        class StatusWatcher;
    };

    using OnStatusAvailableCallback = std::function<void(const DeviceStatus &)>;

    template<typename Duration>
    class StatusWatcher {
        using LockType = std::mutex;
        using Cv = std::condition_variable;
    public:
        StatusWatcher(std::shared_ptr<R2000> sharedDevice,
                      Duration period,
                      OnStatusAvailableCallback callback) : device(std::move(sharedDevice)),
                                                            sleepPeriod(period) {
            callbacks.push_back(std::move(callback));
            statusWatcherTask = spawnAsync(
                    [this]() {
                        const auto threadId{device->getName() + ".StatusWatcher"};
                        pthread_setname_np(pthread_self(), threadId.c_str());
                        Device::Commands::GetParametersCommand commandExecutor{*device};
                        const auto systemStatusBuilder{Device::ROParameters::SystemStatus{}
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
                        for (; !interruptFlag.load(std::memory_order_acquire);) {
                            isConnected.store(true, std::memory_order_release);
                            try {
                                const auto parameters{commandExecutor.template execute(systemStatusBuilder)};
                                if (!parameters.empty()) {
                                    const DeviceStatus status{parameters};
                                    std::lock_guard<LockType> guard{callbackLock, std::adopt_lock};
                                    for (auto &callback : callbacks)
                                        callback(status);
                                }
                            } catch (const std::exception& error) {
                                std::clog << "Interrupting watcher due to error (" << error.what() << ")"
                                          << std::endl;
                                isConnected.store(false, std::memory_order_release);
                                break;
                            }
                            std::unique_lock<LockType> guard(interruptCvLock, std::adopt_lock_t{});
                            cv.wait_for(guard, sleepPeriod,
                                        [this]() -> bool { return interruptFlag.load(std::memory_order_acquire); });
                        }
                    });
        }

        [[nodiscard]] inline bool isAlive() const
        {
            return isConnected.load(std::memory_order_acquire);
        }

        inline void addCallback(OnStatusAvailableCallback callback) {
            std::lock_guard<LockType> guard{callbackLock, std::adopt_lock};
            callbacks.push_back(std::move(callback));
        }

        virtual ~StatusWatcher() {
            if (!interruptFlag.load(std::memory_order_acquire)) {
                {
                    std::lock_guard<LockType> guard(interruptCvLock);
                    interruptFlag.store(true, std::memory_order_release);
                    cv.notify_one();
                }
                statusWatcherTask.wait();
            }
        }

    private:
        template<typename F, typename... Ts>
        inline static std::future<typename std::result_of<F(Ts...)>::type> spawnAsync(F &&f, Ts &&... params) {
            return std::async(std::launch::async, std::forward<F>(f), std::forward<Ts>(params)...);
        }

    private:
        std::shared_ptr<R2000> device{nullptr};
        std::future<void> statusWatcherTask{};
        Cv cv{};
        LockType interruptCvLock{};
        LockType callbackLock{};
        std::atomic_bool interruptFlag{false};
        Duration sleepPeriod;
        std::vector<OnStatusAvailableCallback> callbacks{};
        std::atomic_bool isConnected{false};
    };
}
