#pragma once

#include "Control/Commands.hpp"
#include "R2000.hpp"
#include <any>
#include <bitset>
#include <chrono>
#include <optional>
#include <unordered_map>
#include "R2000/NotImplementedException.hpp"

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
    class StatusFlagInterpreter {
    private:
        /**
         * @param flags A 32-bits unsigned integer sequence of flags.
         */
        explicit StatusFlagInterpreter(uint32_t flags);

    public:
        /**
         * @return True if the device is initializing, False if running.
         */
        [[nodiscard]] inline bool isInitializing() const { return flags[0]; }

        /**
         * @return True if the output of scan is muted, False otherwise.
         */
        [[nodiscard]] inline bool outputScanIsMuted() const { return flags[3]; }

        /**
         * @return True if the rotation of the measuring head is unstable, False otherwise.
         */
        [[nodiscard]] inline bool headHasUnstableRotation() const { return flags[4]; }

        /**
         * @return True if any warning is active, False otherwise.
         */
        [[nodiscard]] inline bool deviceHasWarning() const { return flags[9]; }

        /**
         * @return True if the lens of the sensor is becoming dirty, False otherwise.
         */
        [[nodiscard]] inline bool hasLensContaminationWarning() const { return flags[10]; }

        /**
         * @return True if the ambient temperature is becoming to low, False otherwise.
         */
        [[nodiscard]] inline bool hasLowTemperatureWarning() const { return flags[11]; }

        /**
         * @return True if the ambient temperature is becoming to high, False otherwise.
         */
        [[nodiscard]] inline bool hasHighTemperatureWarning() const { return flags[12]; }

        /**
         * @return True if the device is becoming overloaded, False otherwise.
         */
        [[nodiscard]] inline bool hasDeviceOverloadWarning() const { return flags[13]; }

        /**
         * @return True if the device has an error, False otherwise.
         */
        [[nodiscard]] inline bool deviceHasError() const { return flags[17]; }

        /**
         * @return True if the lens of the sensor is dirty (actions need to be taken), False otherwise.
         */
        [[nodiscard]] inline bool hasLensContaminationError() const { return flags[18]; }

        /**
         * @return True if the temperature is too low (actions need to be taken), False otherwise.
         */
        [[nodiscard]] inline bool hasLowTemperatureError() const { return flags[19]; }

        /**
         * @return True if the temperature is too high (actions need to be taken), False otherwise.
         */
        [[nodiscard]] inline bool hasHighTemperatureError() const { return flags[20]; }

        /**
         * @return True if the device is overloaded (actions need to be taken), False otherwise.
         */
        [[nodiscard]] inline bool hasDeviceOverloadError() const { return flags[21]; }

        /**
         * @return True if the device has an unrecoverable defect (a reboot is needed or other action need to be
         * taken), False otherwise.
         */
        [[nodiscard]] inline bool hasUnrecoverableDefect() const { return flags[31]; }

    private:
        std::bitset<32> flags{};

        friend class DeviceStatus;
    };

    /**
     * Helper struct casting the content of an optional string to a numeric type T or a default fallback value.
     * @tparam T The return type T to cast the optional to.
     */
    template<typename T>
    struct QuietCastOptionalHelper {
        /**
         * @return Nothing, always throws NotImplementedException.
         */
        static inline T castQuietly(const std::optional<std::string> &) {
            throw Device::NotImplementedException({"QuietCastOptionalHelper<" + std::string(typeid(T).name()) +
                                                   ">::Not implemented."});
        };
    };

    template<>
    struct QuietCastOptionalHelper<int64_t> {
        /**
         * Convert the content of an optional string to a 64 bits long signed integer or return
         * nan if the optional is empty.
         * @param optional The optional string to convert the value for.
         * @return the converted int64 value or nan if the optional is empty.
         */
        static inline int64_t castQuietly(const std::optional<std::string> &optional) {
            return optional ? std::stoi(*optional) : std::numeric_limits<int64_t>::quiet_NaN();
        }
    };

    template<>
    struct QuietCastOptionalHelper<uint64_t> {
        /**
         * Convert the content of an optional string to a 64 bits long unsigned integer or return
         * nan if the optional is empty.
         * @param optional The optional string to convert the value for.
         * @return the converted uint64 value or nan if the optional is empty.
         */
        static inline uint64_t castQuietly(const std::optional<std::string> &optional) {
            return optional ? std::stoul(*optional) : std::numeric_limits<uint64_t>::quiet_NaN();
        }
    };

    template<>
    struct QuietCastOptionalHelper<uint32_t> {
        /**
         * Convert the content of an optional string to a 32 bits long unsigned integer or return
         * nan if the optional is empty.
         * @param optional The optional string to convert the value for.
         * @return the converted uint32 value or nan if the optional is empty.
         */
        static inline uint32_t castQuietly(const std::optional<std::string> &optional) {
            return optional ? std::stoul(*optional) : std::numeric_limits<uint32_t>::quiet_NaN();
        }
    };

    class DeviceStatus {
    private:
        /**
         * Get a node from a parameters map as an optional. If the key is not found, the optional is empty.
         * @param parameters The parameters map to search the key in.
         * @param key The key of the parameter to find.
         * @return An optional containing the parameter matching the key or empty if the key is not within the map.
         */
        static inline std::optional<std::string> getOptionalFromParameters(const Parameters::ParametersMap &parameters,
                                                                           const std::string &key) noexcept(false) {
            return !parameters.count(key) ? std::nullopt : std::make_optional<std::string>(parameters.at(key));
        }

    public:
        /**
         * Construct an empty device status. Values returned by the call of getter methods are NaNs.
         */
        DeviceStatus() = default;

        /**
         * Construct a device status from a parameters map containing the status values.
         * @param systemStatusMap The device status values.
         */
        explicit DeviceStatus(Parameters::ParametersMap systemStatusMap);

        /**
         * Get the status flag of the device.
         * @return a StatusFlagInterpreter allowing to query the state of the device.
         */
        [[nodiscard]] inline StatusFlagInterpreter getStatusFlags() const {
            const auto optional{getOptionalFromParameters(systemStatusMap, PARAMETER_STATUS_FLAGS)};
            return optional ? StatusFlagInterpreter(QuietCastOptionalHelper<uint32_t>::castQuietly(optional))
                            : StatusFlagInterpreter(0x00000000);
        }

        /**
         * @return Get the current CPU load of the device.
         */
        [[nodiscard]] inline uint64_t getCpuLoad() const {
            return QuietCastOptionalHelper<uint64_t>::castQuietly(
                    getOptionalFromParameters(systemStatusMap, PARAMETER_STATUS_LOAD_INDICATION));
        }

        /**
         * @return Get the internal raw system time of the device.
         */
        [[nodiscard]] inline uint64_t getRawSystemTime() const {
            return QuietCastOptionalHelper<uint64_t>::castQuietly(
                    getOptionalFromParameters(systemStatusMap, PARAMETER_STATUS_SYSTEM_TIME_RAW));
        }

        /**
         * @return Get the time since which the device has been turned on.
         */
        [[nodiscard]] inline uint64_t getUptime() const {
            return QuietCastOptionalHelper<uint64_t>::castQuietly(
                    getOptionalFromParameters(systemStatusMap, PARAMETER_STATUS_UP_TIME));
        }

        /**
         * @return Get the number of power cycle of the device.
         */
        [[nodiscard]] inline uint64_t getPowerCyclesCount() const {
            return QuietCastOptionalHelper<uint64_t>::castQuietly(
                    getOptionalFromParameters(systemStatusMap, PARAMETER_STATUS_POWER_CYCLES));
        }

        /**
         * @return Get the overall operating time of the device.
         */
        [[nodiscard]] inline uint64_t getOperationTime() const {
            return QuietCastOptionalHelper<uint64_t>::castQuietly(
                    getOptionalFromParameters(systemStatusMap, PARAMETER_STATUS_OPERATION_TIME));
        }

        /**
         * @return Get the overall operating time scaled by temperature of the device.
         */
        [[nodiscard]] inline uint64_t getScaledOperationTime() const {
            return QuietCastOptionalHelper<uint64_t>::castQuietly(
                    getOptionalFromParameters(systemStatusMap, PARAMETER_STATUS_OPERATION_TIME_SCALED));
        }

        /**
         * @return Get the current temperature of the device.
         */
        [[nodiscard]] inline uint64_t getCurrentTemperature() const {
            return QuietCastOptionalHelper<uint64_t>::castQuietly(
                    getOptionalFromParameters(systemStatusMap, PARAMETER_STATUS_TEMPERATURE_CURRENT));
        }

        /**
         * @return Get the minimal lifetime operating temperature of the device.
         */
        [[nodiscard]] inline uint64_t getMinimalTemperature() const {
            return QuietCastOptionalHelper<uint64_t>::castQuietly(
                    getOptionalFromParameters(systemStatusMap, PARAMETER_STATUS_TEMPERATURE_MIN));
        }

        /**
         * @return Get the maximal lifetime operating temperature of the device.
         */
        [[nodiscard]] inline uint64_t getMaximalTemperature() const {
            return QuietCastOptionalHelper<uint64_t>::castQuietly(
                    getOptionalFromParameters(systemStatusMap, PARAMETER_STATUS_TEMPERATURE_MAX));
        }

        /**
         * @return Get the timestamp of the status.
         */
        [[maybe_unused]] [[nodiscard]] inline const std::chrono::steady_clock::time_point &getTimestamp() const {
            return timestamp;
        }

    private:
        Parameters::ParametersMap systemStatusMap;
        std::chrono::steady_clock::time_point timestamp{std::chrono::steady_clock::now()};

        friend class StatusWatcher;
    };

    class StatusWatcher {
        using LockType = std::mutex;
        using Cv = std::condition_variable;
        using SharedStatus = std::shared_ptr<DeviceStatus>;
        using OnStatusAvailableCallback = std::function<void(SharedStatus)>;
        using OnDeviceConnected = std::function<void()>;
        using OnDeviceDisconnected = std::function<void()>;

    public:
        /**
         * @param sharedDevice The device to watch.
         * @param period The period of the status update request to the device.
         */
        StatusWatcher(std::shared_ptr<R2000> sharedDevice, std::chrono::seconds period);

        /**
         * @return True if the connection to the device is alive, False otherwise.
         */
        [[maybe_unused]] [[nodiscard]] inline bool isAlive() const {
            return isConnected.load(std::memory_order_acquire);
        }

        /**
         * @param callback a callback to execute when a new update status is available.
         */
        [[maybe_unused]] inline void addOnStatusAvailableCallback(OnStatusAvailableCallback callback) {
            std::unique_lock<LockType> guard{statusCallbackLock, std::adopt_lock};
            onStatusAvailableCallbacks.push_back(std::move(callback));
        }

        /**
         * @param callback a callback to execute when the device has connected. If the device is already connected,
         * the callback will be immediately executed.
         */
        [[maybe_unused]] inline void addOnDeviceConnectedCallback(OnDeviceConnected callback) {
            if (isConnected.load(std::memory_order_acquire)) {
                std::invoke(callback);
            }
            std::lock_guard<LockType> guard{deviceConnectedCallbackLock};
            onDeviceConnectedCallbacks.push_back(std::move(callback));
        }

        /**
         * @param callback a callback to execute when the device has disconnected.
         */
        [[maybe_unused]] inline void addOnDeviceDisconnectedCallback(OnDeviceDisconnected callback) {
            std::lock_guard<LockType> guard{deviceDisconnectedCallbackLock};
            onDeviceDisconnectedCallbacks.push_back(std::move(callback));
        }

        /**
         * Get the last status received from the device. This method is non-blocking, wait and lock free.
         * @return  the last status received.
         */
        [[maybe_unused]] [[nodiscard]] virtual inline SharedStatus getLastReceivedStatus() const {
            return std::atomic_load_explicit(&lastStatusReceived, std::memory_order_acquire);
        };

        /**
         * Stop the watcher task.
         */
        virtual ~StatusWatcher();

    private:
        /**
         * Main task that update periodically the device status.
         */
        void statusWatcherTask();

        /**
         * Set a new status to the output of this instance and call all the callbacks registered to
         * the new status event.
         * @param status The new status obtained from the device.
         */
        inline void setStatusToOutputAndFireNewStatusEvent(const SharedStatus &status) {
            std::atomic_store_explicit(&lastStatusReceived, status, std::memory_order_release);
            std::lock_guard<LockType> guard{statusCallbackLock};
            for (auto &callback: onStatusAvailableCallbacks) {
                std::invoke(callback, status);
            }
        }

        /**
         * Call all the connection callbacks currently registered.
         */
        inline void fireDeviceConnectionEvent() {
            for (auto &callback: onDeviceConnectedCallbacks) {
                std::invoke(callback);
            }
        }

        /**
         * Call all the disconnection callbacks currently registered.
         */
        inline void fireDeviceDisconnectionEvent() {
            for (auto &callback: onDeviceDisconnectedCallbacks) {
                std::invoke(callback);
            }
        }

    private:
        std::shared_ptr<R2000> device{nullptr};
        SharedStatus lastStatusReceived{std::make_shared<DeviceStatus>()};
        std::future<void> statusWatcherTaskFuture{};
        Cv interruptCv{};
        LockType interruptCvLock{};
        LockType statusCallbackLock{};
        LockType deviceConnectedCallbackLock{};
        LockType deviceDisconnectedCallbackLock{};
        std::atomic_bool interruptFlag{false};
        std::chrono::seconds period{};
        std::vector<OnStatusAvailableCallback> onStatusAvailableCallbacks{};
        std::vector<OnDeviceConnected> onDeviceConnectedCallbacks{};
        std::vector<OnDeviceDisconnected> onDeviceDisconnectedCallbacks{};
        std::atomic_bool isConnected{false};
    };
} // namespace Device
