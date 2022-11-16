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

#pragma once

#include "Control/Commands.hpp"
#include "R2000.hpp"
#include <any>
#include <bitset>
#include <chrono>
#include <optional>
#include <unordered_map>
#include "NotImplementedException.hpp"

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
        [[maybe_unused]] [[nodiscard]] inline StatusFlagInterpreter getStatusFlags() const {
            const auto optional{getOptionalFromParameters(systemStatusMap, PARAMETER_STATUS_FLAGS)};
            return optional ? StatusFlagInterpreter(QuietCastOptionalHelper<uint32_t>::castQuietly(optional))
                            : StatusFlagInterpreter(0x00000000);
        }

        /**
         * @return Get the current CPU load of the device.
         */
        [[maybe_unused]] [[nodiscard]] inline uint64_t getCpuLoad(uint64_t otherwise = 0) const {
            try
            {
                return QuietCastOptionalHelper<uint64_t>::castQuietly(
                    getOptionalFromParameters(systemStatusMap, PARAMETER_STATUS_LOAD_INDICATION));
            }
            catch (const std::invalid_argument& e)
            {
                std::clog << "getCpuLoad:: cannot cast invalid argument from the device: " << e.what() << std::endl;
            }
            catch (const std::out_of_range& e)
            {
                std::clog << "getCpuLoad:: cannot cast out of range value from the device: " << e.what() << std::endl;
            }
            return otherwise;
        }

        /**
         * @return Get the internal raw system time of the device.
         */
        [[maybe_unused]] [[nodiscard]] inline uint64_t getRawSystemTime(uint64_t otherwise = 0) const {
            try
            {
                return QuietCastOptionalHelper<uint64_t>::castQuietly(
                    getOptionalFromParameters(systemStatusMap, PARAMETER_STATUS_SYSTEM_TIME_RAW));
            }
            catch (const std::invalid_argument& e)
            {
                std::clog << "getRawSystemTime:: cannot cast invalid argument from the device: " << e.what() << std::endl;
            }
            catch (const std::out_of_range& e)
            {
                std::clog << "getRawSystemTime:: cannot cast out of range value from the device: " << e.what() << std::endl;
            }
            return otherwise;
        }

        /**
         * @return Get the time since which the device has been turned on.
         */
        [[maybe_unused]] [[nodiscard]] inline uint64_t getUptime(uint64_t otherwise = 0) const {
            try
            {
                return QuietCastOptionalHelper<uint64_t>::castQuietly(
                    getOptionalFromParameters(systemStatusMap, PARAMETER_STATUS_UP_TIME));
            }
            catch (const std::invalid_argument& e)
            {
                std::clog << "getUptime:: cannot cast invalid argument from the device: " << e.what() << std::endl;
            }
            catch (const std::out_of_range& e)
            {
                std::clog << "getUptime:: cannot cast out of range value from the device: " << e.what() << std::endl;
            }
            return otherwise;
        }

        /**
         * @return Get the number of power cycle of the device.
         */
        [[maybe_unused]] [[nodiscard]] inline uint64_t getPowerCyclesCount(uint64_t otherwise = 0) const {
            try
            {
                return QuietCastOptionalHelper<uint64_t>::castQuietly(
                    getOptionalFromParameters(systemStatusMap, PARAMETER_STATUS_POWER_CYCLES));
            }
            catch (const std::invalid_argument& e)
            {
                std::clog << "getPowerCyclesCount:: cannot cast invalid argument from the device: " << e.what()
                          << std::endl;
            }
            catch (const std::out_of_range& e)
            {
                std::clog << "getPowerCyclesCount:: cannot cast out of range value from the device: " << e.what()
                          << std::endl;
            }
            return otherwise;
        }

        /**
         * @return Get the overall operating time of the device.
         */
        [[maybe_unused]] [[nodiscard]] inline uint64_t getOperationTime(uint64_t otherwise = 0) const {
            try
            {
                return QuietCastOptionalHelper<uint64_t>::castQuietly(
                    getOptionalFromParameters(systemStatusMap, PARAMETER_STATUS_OPERATION_TIME));
            }
            catch (const std::invalid_argument& e)
            {
                std::clog << "getOperationTime:: cannot cast invalid argument from the device: " << e.what() << std::endl;
            }
            catch (const std::out_of_range& e)
            {
                std::clog << "getOperationTime:: cannot cast out of range value from the device: " << e.what() << std::endl;
            }
            return otherwise;
        }

        /**
         * @return Get the overall operating time scaled by temperature of the device.
         */
        [[maybe_unused]] [[nodiscard]] inline uint64_t getScaledOperationTime(uint64_t otherwise = 0) const {
            try
            {
                return QuietCastOptionalHelper<uint64_t>::castQuietly(
                    getOptionalFromParameters(systemStatusMap, PARAMETER_STATUS_OPERATION_TIME_SCALED));
            }
            catch (const std::invalid_argument& e)
            {
                std::clog << "getScaledOperationTime:: cannot cast invalid argument from the device: " << e.what()
                          << std::endl;
            }
            catch (const std::out_of_range& e)
            {
                std::clog << "getScaledOperationTime:: cannot cast out of range value from the device: " << e.what()
                          << std::endl;
            }
            return otherwise;
        }

        /**
         * @return Get the current temperature of the device.
         */
        [[maybe_unused]] [[nodiscard]] inline uint64_t getCurrentTemperature(uint64_t otherwise = 0) const {
            try
            {
                return QuietCastOptionalHelper<uint64_t>::castQuietly(
                    getOptionalFromParameters(systemStatusMap, PARAMETER_STATUS_TEMPERATURE_CURRENT));
            }
            catch (const std::invalid_argument& e)
            {
                std::clog << "getCurrentTemperature:: cannot cast invalid argument from the device: " << e.what()
                          << std::endl;
            }
            catch (const std::out_of_range& e)
            {
                std::clog << "getCurrentTemperature:: cannot cast out of range value from the device: " << e.what()
                          << std::endl;
            }
            return otherwise;
        }

        /**
         * @return Get the minimal lifetime operating temperature of the device.
         */
        [[maybe_unused]] [[nodiscard]] inline uint64_t getMinimalTemperature(uint64_t otherwise = 0) const {
            try
            {
                return QuietCastOptionalHelper<uint64_t>::castQuietly(
                    getOptionalFromParameters(systemStatusMap, PARAMETER_STATUS_TEMPERATURE_MIN));
            }
            catch (const std::invalid_argument& e)
            {
                std::clog << "getMinimalTemperature:: cannot cast invalid argument from the device: " << e.what()
                          << std::endl;
            }
            catch (const std::out_of_range& e)
            {
                std::clog << "getMinimalTemperature:: cannot cast out of range value from the device: " << e.what()
                          << std::endl;
            }
            return otherwise;
        }

        /**
         * @return Get the maximal lifetime operating temperature of the device.
         */
        [[maybe_unused]] [[nodiscard]] inline uint64_t getMaximalTemperature(uint64_t otherwise = 0) const {
            try
            {
                return QuietCastOptionalHelper<uint64_t>::castQuietly(
                    getOptionalFromParameters(systemStatusMap, PARAMETER_STATUS_TEMPERATURE_MAX));
            }
            catch (const std::invalid_argument& e)
            {
                std::clog << "getMaximalTemperature:: cannot cast invalid argument from the device: " << e.what()
                          << std::endl;
            }
            catch (const std::out_of_range& e)
            {
                std::clog << "getMaximalTemperature:: cannot cast out of range value from the device: " << e.what()
                          << std::endl;
            }
            return otherwise;
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

    public:
        using SharedStatus = std::shared_ptr<DeviceStatus>;

    private:
        using LockType = std::mutex;
        using Cv = std::condition_variable;
        using OnStatusAvailableCallback = std::function<void(SharedStatus)>;
        using OnDeviceConnected = std::function<void()>;
        using OnDeviceDisconnected = std::function<void()>;

    public:
        /**
         * @param sharedDevice The device to watch.
         * @param period The period of the status update request to the device.
         */
        template <typename... Args>
        explicit StatusWatcher(std::chrono::seconds iPeriod, Args... args)
            : device(R2000::makeShared(std::forward<Args>(args)...)), period(iPeriod)
        {
            statusWatcherTaskFuture = std::async(std::launch::async, &StatusWatcher::statusWatcherTask, this);
        }

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
         * Set the number of consecutive request failures to reach before triggering a disconnection event.
         * The time T it will takes for the Watcher to trigger the disconnection event is proportional to its period
         * and the threshold: T >= threshold * period, given a number of consecutive failure equals to threshold.
         * @param threshold The number of request failure to reach before triggering a disconnection event.
         */
        [[maybe_unused]] inline void setDisconnectionTriggerThreshold(std::int64_t threshold){
            disconnectionTriggerThreshold.store(threshold, std::memory_order_release);
        }

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
        std::atomic_uint64_t disconnectionTriggerThreshold{3};
    };
} // namespace Device
