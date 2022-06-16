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

#include "DeviceHandle.hpp"
#include "R2000.hpp"

namespace Device::Commands {
    struct GetProtocolVersion;
    struct GetProtocolInfo;
    struct ReleaseHandle;
    struct StartScanOutput;
    struct StopScanOutput;
    struct FeedWatchdog;
    struct GetParameters;
    struct FetchParameterList;
    struct SetParameters;
    struct FactoryResetParameters;
    struct FactoryResetDevice;
    struct RebootDevice;
    struct RequestUdpHandle;
    struct RequestTcpHandle;
    struct GetScanOutputConfig;
    struct SetScanOutputConfig;
    namespace internals {

        /**
         * Helper struct that chains the parameters of multiples ReadOnlyRequestBuilder or multiples
         * ReadWriteRequestBuilder into respectively a parameters list or a parameters map.
         */
        struct ParametersChaining {
            /**
             * Base case. Insert the parameters of a builder into a parameters list.
             * @tparam T Must be of base type ReadOnlyRequestBuilder.
             * @param parameters List containing the chained parameters.
             * @param builder The next builder of the sequence to chain.
             */
            template<typename T>
            static constexpr inline void list(Parameters::ParametersList &parameters, T &builder) {
                const auto builderParameters{builder.build()};
                parameters.insert(std::end(parameters), std::cbegin(builderParameters), std::cend(builderParameters));
            }

            /**
             * Unpack a sequence of builders and put the parameters name of all the builders into a parameters list.
             * @tparam T Must be of base type ReadOnlyRequestBuilder.
             * @tparam Rest Must be of base type ReadOnlyRequestBuilder.
             * @param parameters List containing the chained parameters.
             * @param builder The next builder of the sequence to chain.
             * @param args The rest of the builders to chain on next iterations.
             */
            template<typename T, typename... Rest>
            [[maybe_unused]] static constexpr inline void
            list(Parameters::ParametersList &parameters, T &builder, Rest &&... args) {
                const auto builderParameters{builder.build()};
                parameters.insert(std::end(parameters), std::cbegin(builderParameters), std::cend(builderParameters));
                list<Rest...>(parameters, std::forward<Rest>(args)...);
            }

            /**
             * Base case. Insert the parameters of a builder into a parameters map.
             * @tparam T Must be of base type ReadWriteRequestBuilder.
             * @param parameters Map containing the chained parameters and their values.
             * @param builder The next builder of the sequence to chain.
             */
            template<typename T>
            static constexpr inline void map(Parameters::ParametersMap &parameters, T &builder) {
                const auto builderParameters{builder.build()};
                parameters.insert(std::cbegin(builderParameters), std::cend(builderParameters));
            }

            /**
             * Unpack a sequence of builders and put the parameters name and values of all the
             * builders into a parameters map.
             * @tparam T Must be of base type ReadOnlyRequestBuilder.
             * @tparam Rest Must be of base type ReadOnlyRequestBuilder.
             * @param parameters Map containing the chained parameters.
             * @param builder The next builder of the sequence to chain.
             * @param args The rest of the builders to chain on next iterations.
             */
            template<typename T, typename... Rest>
            [[maybe_unused]] static constexpr inline void
            map(Parameters::ParametersMap &parameters, T &builder, Rest &&... args) {
                const auto builderParameters{builder.build()};
                parameters.insert(std::cbegin(builderParameters), std::cend(builderParameters));
                map<Rest...>(parameters, std::forward<Rest>(args)...);
            }
        };

        struct RecursiveExecutorHelper {
            /**
             *
             * @tparam Fn
             * @tparam Builder
             * @param fn
             * @param builder
             * @return
             */
            template<typename Fn,
                    typename Builder,
                    std::enable_if_t<std::is_convertible_v<std::invoke_result_t<Fn, Builder>, void>, int> = 0>
            static constexpr inline auto resolve(Fn &&fn, Builder &&builder) -> void {
                std::invoke(fn, std::forward<Builder>(builder));
            }

            /**
             *
             * @tparam Fn
             * @tparam Builder
             * @tparam Rest
             * @param fn
             * @param builder
             * @param args
             * @return
             */
            template<typename Fn,
                    typename Builder,
                    std::enable_if_t<std::is_convertible_v<std::invoke_result_t<Fn, Builder>, void>, int> = 0,
                    typename... Rest>
            static constexpr inline auto resolve(Fn &&fn, Builder &&builder, Rest &&... args) -> void {
                std::invoke(fn, std::forward<Builder>(builder));
                resolve<Fn, Rest...>(std::forward<Fn>(fn), std::forward<Rest>(args)...);
            }
        };

        /**
         * Base structure of a command.
         * @tparam Command Tag of the command for specialization.
         */
        template<typename Command>
        struct CommandExecutorImpl {
        };

        template<>
        struct CommandExecutorImpl<GetProtocolInfo> {
            using ResultType = std::tuple<Device::RequestResult, Parameters::ParametersMap, Parameters::ParametersList>;
            using AsyncResultType = ResultType;

            /**
             * @param device The device to the send the command to.
             */
            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            /**
             * Get the protocol information of the device. This method is blocking and may take time to
             * return in case of network latency or if the device cannot be joined.
             * @return An optional pair containing:
             * - a map of info of the protocol
             * - a list of commands available to execute with de device.
             */
            [[nodiscard]] [[maybe_unused]] ResultType execute() {
                const auto deviceAnswer{device.sendHttpCommand(COMMAND_GET_PROTOCOL_INFO)};
                if (!deviceAnswer) {
                    return {deviceAnswer.getRequestResult(), {}, {}};
                }
                const auto &[info, commands]{extractInfoFromPropertyTree(deviceAnswer.getPropertyTree())};
                return {deviceAnswer.getRequestResult(), info, commands};
            }

            /**
             * Asynchronously get the protocol information of the device.
             * @param timeout Maximum time allowed for the command to complete.
             * @return an optional future that can be used to obtain the result of the command.
             * if the optional is empty, the device is already busy with another command.
             */
            [[nodiscard]] [[maybe_unused]] std::optional<std::future<ResultType>>
            asyncExecute(std::chrono::milliseconds timeout) {
                auto promise{std::make_shared<std::promise<ResultType>>()};
                auto onComplete{[promise](const Device::DeviceAnswer &result) -> void {
                    if (!result) {
                        promise->set_value({result.getRequestResult(), {}, {}});
                        return;
                    }
                    const auto &[info, commands]{extractInfoFromPropertyTree(result.getPropertyTree())};
                    promise->set_value({result.getRequestResult(), info, commands});
                }};
                if (device.asyncSendHttpCommand(COMMAND_GET_PROTOCOL_INFO, onComplete, timeout)) {
                    return {promise->get_future()};
                }
                return std::nullopt;
            }

            /**
             * Asynchronously get the protocol information of the device.
             * @param timeout Maximum time allowed for the command to complete.
             * @param callable a handle called once the command has executed with the result.
             * @return True if the command is being handled, False if the device is busy.
             */
            [[maybe_unused]] bool asyncExecute(std::chrono::milliseconds timeout,
                                               std::function<void(const ResultType &)> callable) {
                return device.asyncSendHttpCommand(
                        COMMAND_GET_PROTOCOL_INFO,
                        [fn = std::move(callable)](const Device::DeviceAnswer &result) -> void {
                            if (!result) {
                                std::invoke(fn, ResultType{result.getRequestResult(), {}, {}});
                                return;
                            }
                            const auto &[info, commands]{extractInfoFromPropertyTree(result.getPropertyTree())};
                            std::invoke(fn, ResultType{result.getRequestResult(), info, commands});
                        },
                        timeout);
            }

        private:
            /**
             * Extract the protocol information from the answer property tree of the device.
             * @param propertyTree The answer from the device.
             * @return An optional pair containing:
             * - a map of info of the protocol
             * - a list of commands available to execute with de device.
             */
            static std::pair<Parameters::ParametersMap, Parameters::ParametersList>
            extractInfoFromPropertyTree(const PropertyTree &propertyTree) {
                Parameters::ParametersList availableCommands{};
                Parameters::ParametersMap protocolInfo{};
                const auto commands{propertyTree.get_child_optional(PARAMETER_AVAILABLE_COMMANDS)};
                const auto protocolName{propertyTree.get_optional<std::string>(PARAMETER_PROTOCOL_NAME)};
                const auto protocolVersionMajor{
                        propertyTree.get_optional<std::string>(PARAMETER_PROTOCOL_VERSION_MAJOR)};
                const auto protocolVersionMinor{
                        propertyTree.get_optional<std::string>(PARAMETER_PROTOCOL_VERSION_MINOR)};

                if (!commands || !protocolName || !protocolVersionMajor || !protocolVersionMinor)
                    return {{},
                            {}};
                for (const auto &name: *commands) {
                    const auto parameterName{name.second.get<std::string>("")};
                    if (parameterName.empty())
                        continue;
                    availableCommands.emplace_back(parameterName);
                }
                protocolInfo.insert(std::make_pair(PARAMETER_PROTOCOL_NAME, *protocolName));
                protocolInfo.insert(std::make_pair(PARAMETER_PROTOCOL_VERSION_MAJOR, *protocolVersionMajor));
                protocolInfo.insert(std::make_pair(PARAMETER_PROTOCOL_VERSION_MINOR, *protocolVersionMinor));
                return std::make_pair(protocolInfo, availableCommands);
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<GetProtocolVersion> {
            using ResultType = std::pair<Device::RequestResult, Parameters::PFSDP>;
            using AsyncResultType = ResultType;

            /**
             * @param device The device to the send the command to.
             */
            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            /**
             * Get the protocol version of the device. This method is blocking and may take time to
             * return in case of network latency or if the device cannot be joined.
             * @return PFSDP version of the device.
             */
            [[nodiscard]] [[maybe_unused]] ResultType execute() {
                CommandExecutorImpl<GetProtocolInfo> getProtocolInfo{device};
                const auto &[requestResult, parameters, ignore]{getProtocolInfo.execute()};
                if (requestResult == RequestResult::SUCCESS)
                    return {requestResult, Parameters::PFSDP::UNKNOWN};
                const auto major{parameters.at(PARAMETER_PROTOCOL_VERSION_MAJOR)};
                const auto minor{parameters.at(PARAMETER_PROTOCOL_VERSION_MINOR)};
                return {requestResult, protocolVersionFromString(major, minor)};
            }

            /**
             * Asynchronously get the protocol version of the device.
             * @param timeout Maximum time allowed for the command to complete.
             * @return an optional future that can be used to obtain the result of the command.
             * if the optional is empty, the device is already busy with another command.
             */
            [[nodiscard]] [[maybe_unused]] std::future<AsyncResultType>
            asyncExecute(std::chrono::milliseconds timeout) {
                return std::async(std::launch::async, [this, timeout]() -> AsyncResultType {
                    CommandExecutorImpl<GetProtocolInfo> getProtocolInfo{device};
                    auto future{getProtocolInfo.asyncExecute(timeout)};
                    if (!future) {
                        return {Device::RequestResult::FAILED, Parameters::PFSDP::UNKNOWN};
                    }
                    const auto &[requestResult, parameters, ignore]{future->get()};
                    if (requestResult != Device::RequestResult::SUCCESS) {
                        return {requestResult, Parameters::PFSDP::UNKNOWN};
                    }
                    const auto major{parameters.at(PARAMETER_PROTOCOL_VERSION_MAJOR)};
                    const auto minor{parameters.at(PARAMETER_PROTOCOL_VERSION_MINOR)};
                    return {requestResult, protocolVersionFromString(major, minor)};
                });
            }

            /**
             * Asynchronously get the protocol version of the device.
             * @param timeout Maximum time allowed for the command to complete.
             * @param callable a handle called once the command has executed with the result.
             * @return True if the command is being handled, False if the device is busy.
             */
            [[maybe_unused]] bool asyncExecute(std::chrono::milliseconds timeout,
                                               std::function<void(const AsyncResultType &)> callable) {
                CommandExecutorImpl<GetProtocolInfo> getProtocolInfo{device};
                return getProtocolInfo.asyncExecute(timeout, [fn = std::move(callable)](
                        const CommandExecutorImpl<GetProtocolInfo>::AsyncResultType &result) -> void {
                    const auto &[requestResult, parameters, ignore] = result;
                    if (requestResult != Device::RequestResult::SUCCESS) {
                        std::invoke(fn, AsyncResultType{requestResult, Parameters::PFSDP::UNKNOWN});
                        return;
                    }
                    const auto major{parameters.at(PARAMETER_PROTOCOL_VERSION_MAJOR)};
                    const auto minor{parameters.at(PARAMETER_PROTOCOL_VERSION_MINOR)};
                    std::invoke(fn, AsyncResultType{requestResult, protocolVersionFromString(major, minor)});
                });
            }

        private:
            /**
             * Get the underlying value of an enumeration.
             * @tparam E The type of the enumerator.
             * @param enumerator The instance of enumerator to get the value.
             * @return The underlying value of the enumeration.
             */
            template<typename E>
            static constexpr auto underlyingType(E enumerator) {
                return static_cast<std::underlying_type_t<E>>(enumerator);
            }

            /**
             * Convert the string minor and major to a PFSDP value.
             * @param major The major value of the protocol version.
             * @param minor The minor value of the protocol version.
             * @return The matching PFSDP or UNKNOWN if not supported.
             */
            static Device::Parameters::PFSDP
            protocolVersionFromString(const std::string &major, const std::string &minor) {
                const auto numericMajor{std::stoul(major)};
                const auto numericMinor{std::stoul(minor)};
                const auto numericVersion{numericMajor * 100 + numericMinor};
                switch (numericVersion) {
                    case underlyingType(Parameters::PFSDP::V100): {
                        return Parameters::PFSDP::V100;
                    }
                    case underlyingType(Parameters::PFSDP::V101): {
                        return Parameters::PFSDP::V101;
                    }
                    case underlyingType(Parameters::PFSDP::V102): {
                        return Parameters::PFSDP::V102;
                    }
                    case underlyingType(Parameters::PFSDP::V103): {
                        return Parameters::PFSDP::V103;
                    }
                    case underlyingType(Parameters::PFSDP::V104): {
                        return Parameters::PFSDP::V104;
                    }
                    default: {
                        if (numericVersion > underlyingType(Parameters::PFSDP::V104))
                            return Parameters::PFSDP::ABOVE_V104;
                    }
                }
                return Parameters::PFSDP::UNKNOWN;
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<ReleaseHandle> {
            using ResultType = std::vector<Device::RequestResult>;
            using AsyncResultType = Device::RequestResult;

            /**
             * @param device The device to the send the command to.
             */
            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            /**
             * Release a variadic number of handle given by the device. This method is blocking and may take time to
             * return in case of network latency or if the device cannot be joined.
             * @tparam Args Variadic types that must be of base type DeviceHandle.
             * @param args The handles to release.
             * @return True if all handles have been released, False otherwise.
             */
            template<typename... Args>
            [[nodiscard]] [[maybe_unused]] inline ResultType execute(Args &&... args) {
                static_assert(std::conjunction_v<std::is_base_of<Device::DeviceHandle, typename std::decay_t<Args>>...>,
                              "All the provided handle must be of type DeviceHandle");
                std::vector<Device::RequestResult> requestResults{};
                RecursiveExecutorHelper::resolve([&](const auto &handle) -> void {
                                                     auto deviceAnswer{device.sendHttpCommand(COMMAND_RELEASE_HANDLE, PARAMETER_NAME_HANDLE,
                                                                                              handle.value)};
                                                     requestResults.push_back(deviceAnswer.getRequestResult());
                                                 },
                                                 std::forward<Args>(args)...);
                return requestResults;
            }

            /**
             * Asynchronously release a handle.
             * @param handle The handle to release.
             * @param timeout Maximum time allowed for the command to complete.
             * @return an optional future that can be used to obtain the result of the command.
             * if the optional is empty, the device is already busy with another command.
             */
            [[nodiscard]] [[maybe_unused]] std::optional<std::future<AsyncResultType>>
            asyncExecute(const Device::DeviceHandle &handle, std::chrono::milliseconds timeout) {
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::DeviceAnswer &result) -> void {
                    promise->set_value(result.getRequestResult());
                }};
                if (device.asyncSendHttpCommand(COMMAND_RELEASE_HANDLE, {{PARAMETER_NAME_HANDLE, handle.getValue()}},
                                                onComplete, timeout)) {
                    return {promise->get_future()};
                }
                return std::nullopt;
            }

            /**
             * Asynchronously release a handle.
             * @param handle The handle to release.
             * @param timeout Maximum time allowed for the command to complete.
             * @param callable a callback called once the command has executed with the result.
             * @return True if the command is being handled, False if the device is busy.
             */
            [[maybe_unused]] bool asyncExecute(const Device::DeviceHandle &handle,
                                               std::function<void(const AsyncResultType &)> callable,
                                               std::chrono::milliseconds timeout) {
                return device.asyncSendHttpCommand(
                        COMMAND_RELEASE_HANDLE, {{PARAMETER_NAME_HANDLE, handle.getValue()}},
                        [fn = std::move(callable)](const Device::DeviceAnswer &result) -> void {
                            if (!result) {
                                std::invoke(fn, AsyncResultType{result.getRequestResult()});
                                return;
                            }
                            std::invoke(fn, AsyncResultType{result.getRequestResult()});
                        },
                        timeout);
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<StartScanOutput> {
            using ResultType = std::vector<Device::RequestResult>;
            using AsyncResultType = Device::RequestResult;

            /**
             * @param device The device to the send the command to.
             */
            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            /**
             * Start a scan output for a variadic number of handle given by the device. This method is blocking and
             * may take time to return in case of network latency or if the device cannot be joined.
             * @tparam Args Variadic types that must be of base type DeviceHandle.
             * @param args The handles to to start the scan for.
             * @return True if the scan output of all handles have started, False otherwise.
             */
            template<typename... Args>
            [[nodiscard]] [[maybe_unused]] inline ResultType execute(Args &&... args) {
                static_assert(std::conjunction_v<std::is_base_of<Device::DeviceHandle, typename std::decay_t<Args>>...>,
                              "All the provided handle must be of type DeviceHandle");
                std::vector<Device::RequestResult> requestResults{};
                RecursiveExecutorHelper::resolve(
                        [&](const auto &handle) -> void {
                            auto deviceAnswer{device.sendHttpCommand(COMMAND_START_SCAN_OUTPUT, PARAMETER_NAME_HANDLE,
                                                                     handle.value)};
                            requestResults.push_back(deviceAnswer.getRequestResult());
                        },
                        std::forward<Args>(args)...);
                return requestResults;
            }

            /**
             * Asynchronously start a handle scan output.
             * @param handle The handle to release.
             * @param timeout Maximum time allowed for the command to complete.
             * @return an optional future that can be used to obtain the result of the command.
             * if the optional is empty, the device is already busy with another command.
             */
            [[nodiscard]] [[maybe_unused]] std::optional<std::future<AsyncResultType>>
            asyncExecute(const Device::DeviceHandle &handle,
                         std::chrono::milliseconds timeout) {
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::DeviceAnswer &result) -> void {
                    promise->set_value(result.getRequestResult());
                }};
                if (device.asyncSendHttpCommand(COMMAND_START_SCAN_OUTPUT, {{PARAMETER_NAME_HANDLE, handle.getValue()}},
                                                onComplete, timeout)) {
                    return {promise->get_future()};
                }
                return std::nullopt;
            }

            /**
             * Asynchronously start a handle scan output.
             * @param handle The handle to release.
             * @param timeout Maximum time allowed for the command to complete.
             * @param callable a callback called once the command has executed with the result.
             * @return True if the command is being handled, False if the device is busy.
             */
            [[maybe_unused]] bool asyncExecute(const Device::DeviceHandle &handle,
                                               std::function<void(const AsyncResultType &)> callable,
                                               std::chrono::milliseconds timeout) {
                return device.asyncSendHttpCommand(
                        COMMAND_START_SCAN_OUTPUT, {{PARAMETER_NAME_HANDLE, handle.getValue()}},
                        [fn = std::move(callable)](const Device::DeviceAnswer &result) -> void {
                            if (!result) {
                                std::invoke(fn, AsyncResultType{result.getRequestResult()});
                                return;
                            }
                            std::invoke(fn, AsyncResultType{result.getRequestResult()});
                        },
                        timeout);
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<StopScanOutput> {
            using ResultType = std::vector<Device::RequestResult>;
            using AsyncResultType = Device::RequestResult;

            /**
             * @param device The device to the send the command to.
             */
            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            /**
             * Stop a scan output for a variadic number of handle given by the device. This method is blocking and
             * may take time to return in case of network latency or if the device cannot be joined.
             * @tparam Args Variadic types that must be of base type DeviceHandle.
             * @param args The handles to to start the scan for.
             * @return True if the scan output of all handles have stopped, False otherwise.
             */
            template<typename... Args>
            [[nodiscard]] [[maybe_unused]] inline ResultType execute(Args &&... args) {
                static_assert(std::conjunction_v<std::is_base_of<Device::DeviceHandle, typename std::decay_t<Args>>...>,
                              "All the provided handle must be of type DeviceHandle");
                std::vector<Device::RequestResult> requestResults{};
                RecursiveExecutorHelper::resolve(
                        [&](const auto &handle) -> void {
                            auto deviceAnswer{device.sendHttpCommand(COMMAND_STOP_SCAN_OUTPUT, PARAMETER_NAME_HANDLE,
                                                                     handle.value)};
                            requestResults.push_back(deviceAnswer.getRequestResult());
                        },
                        std::forward<Args>(args)...);
                return requestResults;
            }

            /**
             * Asynchronously stop a handle scan output.
             * @param handle The handle to release.
             * @param timeout Maximum time allowed for the command to complete.
             * @return an optional future that can be used to obtain the result of the command.
             * if the optional is empty, the device is already busy with another command.
             */
            [[nodiscard]] [[maybe_unused]] std::optional<std::future<AsyncResultType>>
            asyncExecute(const Device::DeviceHandle &handle,
                         std::chrono::milliseconds timeout) {
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::DeviceAnswer &result) -> void {
                    promise->set_value(result.getRequestResult());
                }};
                if (device.asyncSendHttpCommand(COMMAND_STOP_SCAN_OUTPUT, {{PARAMETER_NAME_HANDLE, handle.getValue()}},
                                                onComplete, timeout)) {
                    return {promise->get_future()};
                }
                return std::nullopt;
            }

            /**
             * Asynchronously stop a handle scan output.
             * @param handle The handle to release.
             * @param timeout Maximum time allowed for the command to complete.
             * @param callable a callback called once the command has executed with the result.
             * @return True if the command is being handled, False if the device is busy.
             */
            [[maybe_unused]] bool asyncExecute(const Device::DeviceHandle &handle,
                                               std::function<void(const AsyncResultType &)> callable,
                                               std::chrono::milliseconds timeout) {
                return device.asyncSendHttpCommand(
                        COMMAND_STOP_SCAN_OUTPUT, {{PARAMETER_NAME_HANDLE, handle.getValue()}},
                        [fn = std::move(callable)](const Device::DeviceAnswer &result) -> void {
                            if (!result) {
                                std::invoke(fn, AsyncResultType{result.getRequestResult()});
                                return;
                            }
                            std::invoke(fn, AsyncResultType{result.getRequestResult()});
                        },
                        timeout);
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<FeedWatchdog> {
            using ResultType = std::vector<Device::RequestResult>;
            using AsyncResultType = Device::RequestResult;

            /**
             * @param device The device to the send the command to.
             */
            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            /**
             * Feed the watchdog for a variadic number of handle given by the device. This method is blocking and
             * may take time to return in case of network latency or if the device cannot be joined.
             * @tparam Args Variadic types that must be of base type DeviceHandle.
             * @param args The handles to to start the scan for.
             * @return True if the watchdog of all handles have been fed, False otherwise.
             */
            template<typename... Args>
            [[nodiscard]] [[maybe_unused]] inline ResultType execute(Args &&... args) {
                static_assert(std::conjunction_v<std::is_base_of<Device::DeviceHandle, typename std::decay_t<Args>>...>,
                              "All the provided handle must be of type DeviceHandle");
                std::vector<Device::RequestResult> requestResults{};
                RecursiveExecutorHelper::resolve([&](const auto &handle) -> void {
                                                     auto deviceAnswer{device.sendHttpCommand(COMMAND_FEED_WATCHDOG, PARAMETER_NAME_HANDLE,
                                                                                              handle.value)};
                                                     requestResults.push_back(deviceAnswer.getRequestResult());
                                                 },
                                                 std::forward<Args>(args)...);
                return requestResults;
            }

            /**
             * Asynchronously fed a handle watchdog.
             * @param handle The handle to fed the watchdog to.
             * @param timeout Maximum time allowed for the command to complete.
             * @return an optional future that can be used to obtain the result of the command.
             * If the optional is empty, the device is already busy with another command.
             */
            [[nodiscard]] [[maybe_unused]] std::optional<std::future<AsyncResultType>>
            asyncExecute(const Device::DeviceHandle &handle,
                         std::chrono::milliseconds timeout) {
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::DeviceAnswer &result) -> void {
                    promise->set_value(result.getRequestResult());
                }};
                if (device.asyncSendHttpCommand(COMMAND_FEED_WATCHDOG, {{PARAMETER_NAME_HANDLE, handle.getValue()}},
                                                onComplete,
                                                timeout)) {
                    return {promise->get_future()};
                }
                return std::nullopt;
            }

            /**
             * Asynchronously fed a handle watchdog.
             * @param handle The handle to fed the watchdog to.
             * @param timeout Maximum time allowed for the command to complete.
             * @param callable a callback called once the command has executed with the result.
             * @return True if the command is being handled, False if the device is busy.
             */
            [[maybe_unused]] bool asyncExecute(const Device::DeviceHandle &handle,
                                               std::function<void(const AsyncResultType &)> callable,
                                               std::chrono::milliseconds timeout) {
                return device.asyncSendHttpCommand(
                        COMMAND_FEED_WATCHDOG, {{PARAMETER_NAME_HANDLE, handle.getValue()}},
                        [fn = std::move(callable)](const Device::DeviceAnswer &result) -> void {
                            if (!result) {
                                std::invoke(fn, AsyncResultType{result.getRequestResult()});
                                return;
                            }
                            std::invoke(fn, AsyncResultType{result.getRequestResult()});
                        },
                        timeout);
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<GetParameters> {
            using ResultType = std::pair<Device::RequestResult, Parameters::ParametersMap>;
            using AsyncResultType = ResultType;

            /**
             * @param device The device to the send the command to.
             */
            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            /**
             * Get the parameters values of a variadic number of builder. This method is blocking and
             * may take time to return in case of network latency or if the device cannot be joined.
             * @tparam Args Must be of base type ReadOnlyRequestBuilder.
             * @param args The builders to get the parameters names from.
             * @return An optional containing the map of requested parameters and their value.
             * If the optional is empty, the device is already busy with another command.
             */
            template<typename... Args>
            [[nodiscard]] [[maybe_unused]] inline ResultType execute(Args &&... args) {
                static_assert(std::conjunction_v<std::is_same<Parameters::ReadOnlyRequestBuilder, Args>...>,
                              "All the provided handle must be of type ReadOnlyRequestBuilder");
                Parameters::ParametersList parameters{};
                ParametersChaining::template list(parameters, std::forward<Args>(args)...);
                const auto &[chain, ignore]{chainAndConvertParametersToString(std::forward<Args>(args)...)};
                const auto deviceAnswer{
                        device.sendHttpCommand(COMMAND_GET_PARAMETER, PARAMETER_NAME_LIST, chain)};
                if (!deviceAnswer) {
                    return {deviceAnswer.getRequestResult(), {}};
                }
                return {deviceAnswer.getRequestResult(),
                        extractParametersValues(parameters, deviceAnswer.getPropertyTree())};
            }

            /**
             * Asynchronously get the parameters values of a variadic number of ReadOnlyRequestBuilder.
             * @tparam Args Must be of base type ReadOnlyRequestBuilder.
             * @param timeout Maximum time allowed for the command to complete.
             * @param args The builders to get the parameters names from.
             * @return an optional future that can be used to obtain the result of the command.
             * If the optional is empty, the device is already busy with another command.
             */
            template<typename... Args>
            [[nodiscard]] [[maybe_unused]] std::optional<std::future<AsyncResultType>>
            asyncExecute(std::chrono::milliseconds timeout, Args &&... args) {
                static_assert(
                        std::conjunction_v<std::is_base_of<Parameters::ReadOnlyRequestBuilder, typename std::decay_t<Args>>...>,
                        "All the provided handle must be of type ReadOnlyRequestBuilder");
                const auto &[chain, parameters]{chainAndConvertParametersToString(std::forward<Args>(args)...)};
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{
                        [promise, parameters = std::move(parameters)](
                                const Device::DeviceAnswer &result) -> void {
                            if (!result) {
                                promise->set_value({result.getRequestResult(), {}});
                                return;
                            }
                            const auto receivedParameters{
                                    extractParametersValues(parameters, result.getPropertyTree())};
                            promise->set_value({result.getRequestResult(), {receivedParameters}});
                        }};
                if (device.asyncSendHttpCommand(COMMAND_GET_PARAMETER, {{PARAMETER_NAME_LIST, chain}},
                                                onComplete, timeout)) {
                    return {promise->get_future()};
                }
                return std::nullopt;
            }

            /**
             * Asynchronously get the parameters values of a variadic number of ReadOnlyRequestBuilder.
             * @tparam Args Must be of base type ReadOnlyRequestBuilder.
             * @param timeout Maximum time allowed for the command to complete.
             * @param callable a callback called once the command has executed with the result.
             * @param args The builders to get the parameters names from.
             * @return True if the command is being handled, False if the device is busy.
             */
            template<typename... Args>
            [[maybe_unused]] bool asyncExecute(Parameters::ReadOnlyRequestBuilder parameters,
                                               std::function<void(const AsyncResultType &)> callable,
                                               std::chrono::milliseconds timeout) {
                auto [stringChainOfParameters, sequenceOfParameters]{chainAndConvertParametersToString(parameters)};
                return device.asyncSendHttpCommand(
                    COMMAND_GET_PARAMETER, {{PARAMETER_NAME_LIST, stringChainOfParameters}},
                    [fn = std::move(callable),
                     sequenceOfParameters = std::move(sequenceOfParameters)](const Device::DeviceAnswer& result) -> void
                    {
                        if (!result)
                        {
                            std::invoke(fn, AsyncResultType{result.getRequestResult(), {}});
                            return;
                        }
                        const auto receivedParameters{
                            extractParametersValues(sequenceOfParameters, result.getPropertyTree())};
                        std::invoke(fn, AsyncResultType{result.getRequestResult(), {receivedParameters}});
                    },
                    timeout);
            }

        private:
            /**
             * Extract the values of the parameters from the device answer property tree.
             * @param list The names of the parameters to extract.
             * @param propertyTree The device answer as a property tree.
             * @return A map of requested parameters and their values.
             */
            static Parameters::ParametersMap
            extractParametersValues(const Parameters::ParametersList &list, const PropertyTree &propertyTree) {
                Parameters::ParametersMap keysValues{};
                for (const auto &name: list) {
                    auto value{propertyTree.get_optional<std::string>(name)};
                    if (!value) {
                        keysValues.insert(std::make_pair(name, ""));
                        continue;
                    }
                    keysValues.insert(std::make_pair(name, *value));
                }
                return keysValues;
            }

        private:
            /**
             * Convert the contents of a variadic number of builder parameters name into a string list
             * that can be submitted to the device.
             * @tparam Args Must be of base type ReadOnlyRequestBuilder.
             * @param args The builders to get the parameters names from.
             * @return a pair containing:
             * - At first, the string chained parameters names.
             * - All the parameters name as a list.
             */
            template<typename... Args>
            std::pair<std::string, Parameters::ParametersList> chainAndConvertParametersToString(Args &&... args) {
                Parameters::ParametersList parameters{};
                ParametersChaining::template list(parameters, std::forward<Args>(args)...);
                std::string parametersAsString{};
                for (const auto &names: parameters)
                    parametersAsString += (names + ";");
                parametersAsString.pop_back();
                return {parametersAsString, parameters};
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<SetParameters> {
            using ResultType = Device::RequestResult;
            using AsyncResultType = ResultType;

            /**
             * @param device The device to the send the command to.
             */
            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            /**
             * Set the parameters values of a variadic number of ReadWriteRequestBuilder. This method is blocking and
             * may take time to return in case of network latency or if the device cannot be joined.
             * @tparam Args Must be of base type ReadWriteRequestBuilder.
             * @param args The builders to get the parameters values and names from.
             * @return True if the command is being handled, False if the device is busy.
             */
            template<typename... Args>
            [[nodiscard]] [[maybe_unused]] inline ResultType execute(Args &&... args) {
                static_assert(
                        std::conjunction_v<std::is_base_of<Parameters::ReadWriteRequestBuilder, typename std::decay_t<Args>>...>,
                        "All the provided handle must be of type ReadWriteRequestBuilder");
                Parameters::ParametersMap parameters{};
                ParametersChaining::template map(parameters, std::forward<Args>(args)...);
                const auto deviceAnswer{device.sendHttpCommand(COMMAND_SET_PARAMETER, parameters)};
                return deviceAnswer.getRequestResult();
            }

            /**
             * Asynchronously parameters values of a variadic number of ReadWriteRequestBuilder.
             * @tparam Args Must be of base type ReadWriteRequestBuilder.
             * @param timeout Maximum time allowed for the command to complete.
             * @param args The builders to get the parameters names from.
             * @return an optional future that can be used to obtain the result of the command.
             * If the optional is empty, the device is already busy with another command.
             */
            template<typename... Args>
            [[nodiscard]] [[maybe_unused]] std::optional<std::future<AsyncResultType>>
            asyncExecute(std::chrono::milliseconds timeout,
                         Args &&... args) {
                static_assert(
                        std::conjunction_v<std::is_base_of<Parameters::ReadWriteRequestBuilder, typename std::decay_t<Args>>...>,
                        "All the provided handle must be of type ReadWriteRequestBuilder");
                Parameters::ParametersMap parameters{};
                ParametersChaining::template map(parameters, std::forward<Args>(args)...);
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::DeviceAnswer &result) -> void {
                    promise->set_value(result.getRequestResult());
                }};
                if (device.asyncSendHttpCommand(COMMAND_SET_PARAMETER, parameters, onComplete, timeout)) {
                    return {promise->get_future()};
                }
                return std::nullopt;
            }

            /**
             * Asynchronously parameters values of a variadic number of ReadWriteRequestBuilder.
             * @tparam Args Must be of base type ReadWriteRequestBuilder.
             * @param timeout Maximum time allowed for the command to complete.
             * @param callable a callback called once the command has executed with the result.
             * @param args The builders to get the parameters names from.
             * @return True if the command is being handled, False if the device is busy.
             */
            template<typename... Args>
            [[maybe_unused]] bool
            asyncExecute(const Parameters::ReadWriteRequestBuilder& parameters,
                         std::function<void(const AsyncResultType &)> callable,
                         std::chrono::milliseconds timeout) {
                return device.asyncSendHttpCommand(
                        COMMAND_SET_PARAMETER, parameters.build(),
                        [fn = std::move(callable)](const Device::DeviceAnswer &result) -> void {
                            std::invoke(fn, result.getRequestResult());
                        },
                        timeout);
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<FetchParameterList> {
            using ResultType = std::pair<Device::RequestResult, Parameters::ParametersList>;
            using AsyncResultType = ResultType;

            /**
             * @param device The device to the send the command to.
             */
            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            /**
             * Fetch the list of parameters available on the device. This method is blocking and
             * may take time to return in case of network latency or if the device cannot be joined.
             * @return An optional containing the list of available parameters.
             * If the optional is empty, the device is already busy with another command.
             */
            [[nodiscard]] [[maybe_unused]] ResultType execute() {
                const auto deviceAnswer{device.sendHttpCommand(COMMAND_LIST_PARAMETERS)};
                if (!deviceAnswer) {
                    return {deviceAnswer.getRequestResult(), {}};
                }
                return {deviceAnswer.getRequestResult(),
                        extractParametersFromPropertyTree(deviceAnswer.getPropertyTree())};
            }

            /**
             * Asynchronously fetch the list of parameters available on the device.
             * @param timeout Maximum time allowed for the command to complete.
             * @return an optional future that can be used to obtain the result of the command.
             * If the optional is empty, the device is already busy with another command.
             */
            [[nodiscard]] [[maybe_unused]] std::optional<std::future<AsyncResultType>>
            asyncExecute(std::chrono::milliseconds timeout) {
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::DeviceAnswer &result) -> void {
                    if (!result) {
                        promise->set_value({result.getRequestResult(), {}});
                        return;
                    }
                    const auto parameters{extractParametersFromPropertyTree(result.getPropertyTree())};
                    promise->set_value({result.getRequestResult(), parameters});
                }};
                if (device.asyncSendHttpCommand(COMMAND_LIST_PARAMETERS, onComplete, timeout)) {
                    return {promise->get_future()};
                }
                return std::nullopt;
            }

            /**
             * Asynchronously fetch the list of parameters available on the device.
             * @param timeout Maximum time allowed for the command to complete.
             * @param callable a callback called once the command has executed with the result.
             * @param args The builders to get the parameters names from.
             * @return True if the command is being handled, False if the device is busy.
             */
            [[maybe_unused]] bool asyncExecute(std::chrono::milliseconds timeout,
                                               std::function<void(const AsyncResultType &)> callable) {
                return device.asyncSendHttpCommand(
                        COMMAND_LIST_PARAMETERS,
                        [fn = std::move(callable)](const Device::DeviceAnswer &result) -> void {
                            if (!result) {
                                std::invoke(fn, AsyncResultType{result.getRequestResult(), {}});
                                return;
                            }
                            const auto parameters{extractParametersFromPropertyTree(result.getPropertyTree())};
                            std::invoke(fn, AsyncResultType{result.getRequestResult(), parameters});
                        },
                        timeout);
            }

        private:
            /**
             * Extract the list of parameters from the device answer property tree.
             * @param propertyTree The device answer as a device tree.
             * @return The list of parameters available for the device.
             */
            static Parameters::ParametersList
            extractParametersFromPropertyTree(const PropertyTree &propertyTree) {
                Parameters::ParametersList parameterList{};
                const auto list{propertyTree.get_child_optional("parameters")};
                if (!list)
                    return parameterList;
                for (const auto &name: *list) {
                    const auto parameterName{name.second.get<std::string>("")};
                    if (parameterName.empty())
                        continue;
                    parameterList.emplace_back(parameterName);
                }
                return parameterList;
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<FactoryResetParameters> {
            using ResultType = Device::RequestResult;
            using AsyncResultType = ResultType;

            /**
             * @param device The device to the send the command to.
             */
            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            /**
             * Reset the parameters values of a variadic number of ReadWriteRequestBuilder to factory values. This
             * method is blocking and may take time to return in case of network latency or if the device cannot be joined.
             * @tparam Args Must be of base type ReadWriteRequestBuilder.
             * @param args The builders to reset the parameters for.
             * @return True if the command is being handled, False if the device is busy.
             */
            template<typename... Args>
            [[nodiscard]] [[maybe_unused]] inline ResultType execute(Args &&... args) {
                static_assert(
                        std::conjunction_v<std::is_base_of<Parameters::ReadWriteRequestBuilder, typename std::decay_t<Args>>...>,
                        "All the provided handle must be of type ReadWriteRequestBuilder");
                const auto params{chainAndConvertParametersToString(std::forward<Args>(args)...)};
                const auto deviceAnswer{device.sendHttpCommand(COMMAND_RESET_PARAMETERS, PARAMETER_NAME_LIST, params)};
                return deviceAnswer.getRequestResult();
            }

            /**
             * Asynchronously reset the parameters values of a variadic number of ReadWriteRequestBuilder
             * to factory values.
             * @tparam Args Must be of base type ReadWriteRequestBuilder.
             * @param timeout Maximum time allowed for the command to complete.
             * @param args The builders to reset the parameters for.
             * @return an optional future that can be used to obtain the result of the command.
             * If the optional is empty, the device is already busy with another command.
             */
            template<typename... Args>
            [[nodiscard]] [[maybe_unused]] std::optional<std::future<AsyncResultType>>
            asyncExecute(std::chrono::milliseconds timeout, Args &&... args) {
                static_assert(
                        std::conjunction_v<std::is_base_of<Parameters::ReadWriteRequestBuilder, typename std::decay_t<Args>>...>,
                        "All the provided handle must be of type ReadWriteRequestBuilder");
                const auto parameters{chainAndConvertParametersToString(std::forward<Args>(args)...)};
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::DeviceAnswer &result) -> void {
                    promise->set_value(result.getRequestResult());
                }};
                if (device.asyncSendHttpCommand(COMMAND_RESET_PARAMETERS, {{PARAMETER_NAME_LIST, parameters}},
                                                onComplete,
                                                timeout)) {
                    return {promise->get_future()};
                }
                return std::nullopt;
            }

            /**
             * Asynchronously reset the parameters values of a variadic number of ReadWriteRequestBuilder
             * to factory values.
             * @tparam Args Must be of base type ReadWriteRequestBuilder.
             * @param timeout Maximum time allowed for the command to complete.
             * @param callable a callback called once the command has executed with the result.
             * @param args The builders to reset the parameters for.
             * @return True if the command is being handled, False if the device is busy.
             */
            template<typename... Args>
            [[maybe_unused]] bool asyncExecute(Parameters::ReadWriteRequestBuilder parameters,
                                               std::function<void(const AsyncResultType &)> callable,
                                               std::chrono::milliseconds timeout) {
                const auto parametersAsString{chainAndConvertParametersToString(parameters)};
                return device.asyncSendHttpCommand(
                        COMMAND_RESET_PARAMETERS, {{PARAMETER_NAME_LIST, parametersAsString}},
                        [fn = std::move(callable)](const Device::DeviceAnswer &result) -> void {
                            std::invoke(fn, result.getRequestResult());
                        },
                        timeout);
            }

        private:
            /**
             * Convert the contents of a variadic number of builder parameters name into a string list
             * that can be submitted to the device.
             * @tparam Args Must be of base type ReadWriteRequestBuilder.
             * @param args The builders to get the parameters names from.
             * @return The string chained parameters names.
             */
            template<typename... Args>
            std::string chainAndConvertParametersToString(Args &&... args) {
                Parameters::ParametersList parameters{};
                ParametersChaining::template list(parameters, std::forward<Args>(args)...);
                std::string parametersList{};
                for (const auto &names: parameters) {
                    parametersList += (names + ";");
                }
                parametersList.pop_back();
                return parametersList;
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<FactoryResetDevice> {
            using ResultType = Device::RequestResult;
            using AsyncResultType = ResultType;

            /**
             * @param device The device to the send the command to.
             */
            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            /**
             * Reset the device to factory parameters. This method is blocking and
             * may take time to return in case of network latency or if the device cannot be joined.
             * @return True if device has been reset, False otherwise.
             */
            [[nodiscard]] [[maybe_unused]] inline ResultType execute() {
                const auto deviceAnswer{device.sendHttpCommand(COMMAND_FACTORY_RESET)};
                return deviceAnswer.getRequestResult();
            }

            /**
             * Asynchronously reset the device to factory parameters.
             * @param timeout Maximum time allowed for the command to complete.
             * @return an optional future that can be used to obtain the result of the command.
             * If the optional is empty, the device is already busy with another command.
             */
            [[nodiscard]] [[maybe_unused]] std::optional<std::future<AsyncResultType>>
            asyncExecute(std::chrono::milliseconds timeout) {
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::DeviceAnswer &result) -> void {
                    promise->set_value(result.getRequestResult());
                }};
                if (device.asyncSendHttpCommand(COMMAND_FACTORY_RESET, onComplete, timeout)) {
                    return {promise->get_future()};
                }
                return std::nullopt;
            }

            /**
             * Asynchronously reset the device to factory parameters.
             * @param timeout Maximum time allowed for the command to complete.
             * @param callable a callback called once the command has executed with the result.
             * @return True if the command is being handled, False if the device is busy.
             */
            [[maybe_unused]] bool asyncExecute(std::chrono::milliseconds timeout,
                                               std::function<void(const AsyncResultType &)> callable) {
                return device.asyncSendHttpCommand(
                        COMMAND_FACTORY_RESET,
                        [fn = std::move(callable)](const Device::DeviceAnswer &result) -> void {
                            std::invoke(fn, result.getRequestResult());
                        },
                        timeout);
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<RebootDevice> {
            using ResultType = Device::RequestResult;
            using AsyncResultType = ResultType;

            /**
             * @param device The device to the send the command to.
             */
            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            /**
             * Reboot the device. This method is blocking and may take time to return in case of network latency
             * or if the device cannot be joined.
             * @return True if the device has been rebooted, False otherwise.
             */
            [[nodiscard]] [[maybe_unused]] ResultType execute() {
                const auto deviceAnswer{device.sendHttpCommand(COMMAND_REBOOT_DEVICE)};
                return deviceAnswer.getRequestResult();
            }

            /**
             * Asynchronously reboot the device.
             * @param timeout Maximum time allowed for the command to complete.
             * @return an optional future that can be used to obtain the result of the command.
             * If the optional is empty, the device is already busy with another command.
             */
            [[nodiscard]] [[maybe_unused]] std::optional<std::future<AsyncResultType>>
            asyncExecute(std::chrono::milliseconds timeout) {
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::DeviceAnswer &result) -> void {
                    promise->set_value(result.getRequestResult());
                }};
                if (device.asyncSendHttpCommand(COMMAND_REBOOT_DEVICE, onComplete, timeout)) {
                    return {promise->get_future()};
                }
                return std::nullopt;
            }

            /**
             * Asynchronously reboot the device.
             * @param timeout Maximum time allowed for the command to complete.
             * @param callable a callback called once the command has executed with the result.
             * @return True if the command is being handled, False if the device is busy.
             */
            [[maybe_unused]] bool asyncExecute(std::chrono::milliseconds timeout,
                                               std::function<void(const AsyncResultType &)> callable) {
                return device.asyncSendHttpCommand(
                        COMMAND_REBOOT_DEVICE,
                        [fn = std::move(callable)](const Device::DeviceAnswer &result) -> void {
                            std::invoke(fn, AsyncResultType{result.getRequestResult()});
                        },
                        timeout);
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<RequestTcpHandle> {
            using ResultType = std::tuple<Device::RequestResult, int, Device::HandleType>;
            using AsyncResultType = ResultType;

            /**
             * @param device The device to the send the command to.
             */
            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            /**
             * Request a TCP handle from the device given a configuration. This method is blocking and
             * may take time to return in case of network latency or if the device cannot be joined.
             * @param builder Builder containing the configuration parameters of the handle to request.
             * @return An optional pair containing:
             * - the port of the device to connect to obtain scan data.
             * - the identification value of the handle to control the scan stream.
             * If the optional is empty, the device is already busy with another command.
             */
            [[nodiscard]] [[maybe_unused]] ResultType
            execute(const Parameters::ReadWriteParameters::TcpHandle &builder) {
                const auto parameters{builder.build()};
                const auto deviceAnswer{device.sendHttpCommand(COMMAND_REQUEST_TCP_HANDLE, parameters)};
                if (!deviceAnswer) {
                    return {deviceAnswer.getRequestResult(), {}, {}};
                }
                const auto[port, handle]{extractHandleInfoFromPropertyTree(deviceAnswer.getPropertyTree())};
                return {deviceAnswer.getRequestResult(), port, handle};
            }

            /**
             * Asynchronously request a TCP handle from the device given a configuration.
             * @param builder Builder containing the configuration parameters of the handle to request.
             * @param timeout Maximum time allowed for the command to complete.
             * @return an optional future that can be used to obtain the result of the command.
             * If the optional is empty, the device is already busy with another command.
             */
            [[nodiscard]] [[maybe_unused]] std::optional<std::future<AsyncResultType>>
            asyncExecute(const Parameters::ReadWriteParameters::TcpHandle &builder, std::chrono::milliseconds timeout) {
                const auto parameters{builder.build()};
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::DeviceAnswer &result) -> void {
                    if (!result) {
                        promise->set_value({result.getRequestResult(), {}, {}});
                        return;
                    }
                    const auto[port, handle]{extractHandleInfoFromPropertyTree(result.getPropertyTree())};
                    promise->set_value({result.getRequestResult(), port, handle});
                }};
                if (device.asyncSendHttpCommand(COMMAND_REQUEST_TCP_HANDLE, parameters, onComplete, timeout)) {
                    return {promise->get_future()};
                }
                return std::nullopt;
            }

            /**
             * Asynchronously request a TCP handle from the device given a configuration.
             * @param builder Builder containing the configuration parameters of the handle to request.
             * @param timeout Maximum time allowed for the command to complete.
             * @param callable a callback called once the command has executed with the result.
             * @return True if the command is being handled, False if the device is busy.
             */
            [[maybe_unused]] bool asyncExecute(const Parameters::ReadWriteParameters::TcpHandle &builder,
                                               std::chrono::milliseconds timeout,
                                               std::function<void(const AsyncResultType &)> callable) {
                const auto parameters{builder.build()};
                return device.asyncSendHttpCommand(COMMAND_REQUEST_TCP_HANDLE, parameters,
                                                   [fn = std::move(callable)](
                                                           const Device::DeviceAnswer &result) -> void {
                                                       if (!result) {
                                                           std::invoke(fn,
                                                                       AsyncResultType{result.getRequestResult(), {},
                                                                                       {}});
                                                           return;
                                                       }
                                                       const auto[port, handle]{extractHandleInfoFromPropertyTree(
                                                               result.getPropertyTree())};
                                                       std::invoke(fn, AsyncResultType{result.getRequestResult(), port,
                                                                                       handle});
                                                   },
                                                   timeout);
            }

        private:
            /**
             * Extract the handle information from the device answer.
             * @param propertyTree The device answer as a property tree.
             * @return a pair containing the port of the device at which to connect to receive scans
             * and the handle identification value.
             */
            static std::pair<int, Device::HandleType>
            extractHandleInfoFromPropertyTree(const PropertyTree &propertyTree) {
                const auto port{propertyTree.get<int>("port")};
                const auto handle{propertyTree.get<HandleType>("handle")};
                return std::make_pair(port, handle);
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<RequestUdpHandle> {
            using ResultType = std::pair<Device::RequestResult, HandleType>;
            using AsyncResultType = ResultType;

            /**
             * @param device The device to the send the command to.
             */
            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            /**
             * Request an UDP handle from the device given a configuration. This method is blocking and
             * may take time to return in case of network latency or if the device cannot be joined.
             * @param builder Builder containing the configuration parameters of the handle to request.
             * @return The identification value of the handle to control the scan stream. If the optional is
             * empty, the device is already busy with another command.
             */
            [[nodiscard]] [[maybe_unused]] ResultType
            execute(const Parameters::ReadWriteParameters::UdpHandle &builder) {
                const auto parameters{builder.build()};
                const auto deviceAnswer{device.sendHttpCommand(COMMAND_REQUEST_UDP_HANDLE, parameters)};
                if (!deviceAnswer) {
                    return {deviceAnswer.getRequestResult(), {}};
                }
                const auto handle{extractHandleInfoFromPropertyTree(deviceAnswer.getPropertyTree())};
                return {deviceAnswer.getRequestResult(), handle};
            }

            /**
             * Asynchronously request an UDP handle from the device given a configuration.
             * @param builder Builder containing the configuration parameters of the handle to request.
             * @param timeout Maximum time allowed for the command to complete.
             * @return an optional future that can be used to obtain the result of the command.
             * If the optional is empty, the device is already busy with another command.
             */
            [[nodiscard]] [[maybe_unused]] std::optional<std::future<AsyncResultType>>
            asyncExecute(const Parameters::ReadWriteParameters::UdpHandle &builder, std::chrono::milliseconds timeout) {
                const auto parameters{builder.build()};
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::DeviceAnswer &result) -> void {
                    if (!result) {
                        promise->set_value({result.getRequestResult(), {}});
                        return;
                    }
                    const auto handle{extractHandleInfoFromPropertyTree(result.getPropertyTree())};
                    promise->set_value({result.getRequestResult(), handle});
                }};
                if (device.asyncSendHttpCommand(COMMAND_REQUEST_UDP_HANDLE, parameters, onComplete, timeout)) {
                    return {promise->get_future()};
                }
                return std::nullopt;
            }

            /**
             * Asynchronously request an UDP handle from the device given a configuration.
             * @param builder Builder containing the configuration parameters of the handle to request.
             * @param timeout Maximum time allowed for the command to complete.
             * @param callable a callback called once the command has executed with the result.
             * @return True if the command is being handled, False if the device is busy.
             */
            [[maybe_unused]] bool asyncExecute(const Parameters::ReadWriteParameters::UdpHandle &builder,
                                               std::chrono::milliseconds timeout,
                                               std::function<void(const AsyncResultType &)> callable) {
                const auto parameters{builder.build()};
                return device.asyncSendHttpCommand(
                        COMMAND_REQUEST_UDP_HANDLE, parameters,
                        [fn = std::move(callable)](const Device::DeviceAnswer &result) -> void {
                            if (!result) {
                                std::invoke(fn, AsyncResultType{result.getRequestResult(), {}});
                                return;
                            }
                            const auto handle{extractHandleInfoFromPropertyTree(result.getPropertyTree())};
                            std::invoke(fn, AsyncResultType{result.getRequestResult(), handle});
                        },
                        timeout);
            }

        private:
            /**
             * Extract the handle information from the device answer.
             * @param propertyTree The device answer as a property tree.
             * @return The handle identification value.
             */
            static Device::HandleType extractHandleInfoFromPropertyTree(const PropertyTree &propertyTree) {
                return propertyTree.get<std::string>("handle");
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<GetScanOutputConfig> {
            using ResultType = std::vector<std::optional<Parameters::ParametersMap>>;
            using AsyncResultType = std::pair<Device::RequestResult, std::optional<Parameters::ParametersMap>>;

            /**
             * @param device The device to the send the command to.
             */
            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            /**
             * Get a variadic number of scan streams configuration identified by their handles. This method is blocking
             * and may take time to return in case of network latency or if the device cannot be joined.
             * @tparam Args Variadic types that must be of base type DeviceHandle.
             * @param args The handles of the streams to fetch the configurations of.
             * @return A vector of optionals containing the configurations.
             */
            template<typename... Args>
            [[nodiscard]] [[maybe_unused]] ResultType execute(Args &&... args) {
                static_assert(std::conjunction_v<std::is_base_of<Device::DeviceHandle, typename std::decay_t<Args>>...>,
                              "All the provided handle must be of type DeviceHandle");
                std::vector<std::optional<Parameters::ParametersMap>> configurations{};
                RecursiveExecutorHelper::resolve(
                        [this, &configurations](const auto &handle) -> void {
                            const auto propertyTree{
                                    device.sendHttpCommand(COMMAND_GET_SCAN_OUTPUT_CONFIG, PARAMETER_NAME_HANDLE,
                                                           handle.value)};
                            if (!propertyTree) {
                                configurations.template emplace_back(std::nullopt);
                                return;
                            }
                            configurations.template emplace_back({getScanOutputConfig(*propertyTree)});
                        },
                        std::forward<Args>(args)...);
                return configurations;
            }

            /**
             * Asynchronously release a handle.
             * @param handle The handle of the stream to fetch the configuration for.
             * @param timeout Maximum time allowed for the command to complete.
             * @return an optional future that can be used to obtain the result of the command.
             * if the optional is empty, the device is already busy with another command.
             */
            [[nodiscard]] [[maybe_unused]] std::optional<std::future<AsyncResultType>>
            asyncExecute(const Device::DeviceHandle &handle, std::chrono::milliseconds timeout) {
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::DeviceAnswer &result) -> void {
                    if (!result) {
                        promise->set_value({result.getRequestResult(), std::nullopt});
                        return;
                    }
                    const auto &propertyTree{result.getPropertyTree()};
                    promise->set_value({result.getRequestResult(), {getScanOutputConfig(propertyTree)}});
                }};

                if (device.asyncSendHttpCommand(COMMAND_GET_SCAN_OUTPUT_CONFIG,
                                                {{PARAMETER_NAME_HANDLE, handle.getValue()}},
                                                onComplete, timeout)) {
                    return {promise->get_future()};
                }
                return std::nullopt;
            }

            /**
             * Asynchronously release a handle.
             * @param handle The handle of the stream to fetch the configuration for.
             * @param timeout Maximum time allowed for the command to complete.
             * @param callable a callback called once the command has executed with the result.
             * @return True if the command is being handled, False if the device is busy.
             */
            [[maybe_unused]] bool asyncExecute(const Device::DeviceHandle &handle,
                                               std::function<void(const AsyncResultType &)> callable,
                                               std::chrono::milliseconds timeout) {
                return device.asyncSendHttpCommand(
                        COMMAND_GET_SCAN_OUTPUT_CONFIG, {{PARAMETER_NAME_HANDLE, handle.getValue()}},
                        [fn = std::move(callable)](const Device::DeviceAnswer &result) -> void {
                            const auto asyncResultValue{result.getRequestResult()};
                            if (!result) {
                                std::invoke(fn, AsyncResultType{asyncResultValue, std::nullopt});
                                return;
                            }
                            const auto &propertyTree{result.getPropertyTree()};
                            std::invoke(fn, AsyncResultType{asyncResultValue, {getScanOutputConfig(propertyTree)}});
                        },
                        timeout);
            }

        private:
            /**
             * Get the configuration of a scan stream from the device answer.
             * @param propertyTree The device answer as a property tree.
             * @return
             */
            static Parameters::ParametersMap
            getScanOutputConfig(const Device::PropertyTree &propertyTree) {
                Parameters::ParametersMap scanOutputConfig{};
                for (const auto &name: propertyTree) {
                    const auto parameterName{name.second.get<std::string>("")};
                    auto value{propertyTree.get_optional<std::string>(parameterName)};
                    if (!value) {
                        return scanOutputConfig;
                    }
                    scanOutputConfig.insert(std::make_pair(parameterName, *value));
                }
                return scanOutputConfig;
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<SetScanOutputConfig> {
            using ResultType = std::vector<Device::RequestResult>;
            using AsyncResultType = Device::RequestResult;

            /**
             * @param device The device to the send the command to.
             */
            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            /**
             * Set a variadic number of scan streams configuration identified by their handles. This method is blocking
             * and may take time to return in case of network latency or if the device cannot be joined.
             * @tparam Args Variadic types that must be of base type DeviceHandle.
             * @param args The handles of the streams to set the configurations of.
             * @return True if streams have been modified given the configurations, False otherwise.
             */
            template<typename... Args>
            [[nodiscard]] [[maybe_unused]] inline ResultType execute(Args &&... args) {
                static_assert(
                        std::conjunction_v<std::is_base_of<Parameters::HandleParameters, typename std::decay_t<Args>>...>,
                        "All the provided handle must be of type HandleParameters");
                std::vector<Device::RequestResult> requestResults{};
                RecursiveExecutorHelper::template resolve(
                        [&](const auto &builder) -> void {
                            const auto parameters{builder.build()};
                            auto result{device.sendHttpCommand(COMMAND_SET_SCAN_OUTPUT_CONFIG, parameters).first};
                            requestResults.push_back(result.first);
                        },
                        std::forward<Args>(args)...);
                return requestResults;
            }

            /**
             * Asynchronously set a scan stream configuration identified by its handle value.
             * @param builder Builder containing the configuration parameters of the stream to configure.
             * @param timeout Maximum time allowed for the command to complete.
             * @return an optional future that can be used to obtain the result of the command.
             * If the optional is empty, the device is already busy with another command.
             */
            [[nodiscard]] [[maybe_unused]] std::optional<std::future<AsyncResultType>>
            asyncExecute(const Parameters::HandleParameters &builder, std::chrono::milliseconds timeout) {
                const auto parameters{builder.build()};
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::DeviceAnswer &result) -> void {
                    promise->set_value(result.getRequestResult());
                }};

                if (device.asyncSendHttpCommand(COMMAND_SET_SCAN_OUTPUT_CONFIG, parameters, onComplete, timeout)) {
                    return {promise->get_future()};
                }
                return std::nullopt;
            }

            /**
             * Asynchronously set a scan stream configuration identified by its handle value.
             * @param builder Builder containing the configuration parameters of the stream to configure.
             * @param timeout Maximum time allowed for the command to complete.
             * @param callable a callback called once the command has executed with the result.
             * @return True if the command is being handled, False if the device is busy.
             */
            [[maybe_unused]] bool
            asyncExecute(const Parameters::HandleParameters &builder,
                         std::function<void(const AsyncResultType &)> callable,
                         std::chrono::milliseconds timeout) {
                const auto parameters{builder.build()};
                return device.asyncSendHttpCommand(
                        COMMAND_SET_SCAN_OUTPUT_CONFIG, parameters,
                        [fn = std::move(callable)](const Device::DeviceAnswer &result) -> void {
                            std::invoke(fn, result.getRequestResult());
                        },
                        timeout);
            }

        private:
            R2000 &device;
        };

    } // namespace internals
    template<typename Tag> using Command [[maybe_unused]] = internals::CommandExecutorImpl<Tag>;
    using GetProtocolVersionCommand [[maybe_unused]] = internals::CommandExecutorImpl<GetProtocolVersion>;
    using GetProtocolInfoCommand [[maybe_unused]] = internals::CommandExecutorImpl<GetProtocolInfo>;
    using ReleaseHandleCommand [[maybe_unused]] = internals::CommandExecutorImpl<ReleaseHandle>;
    using StartScanCommand [[maybe_unused]] = internals::CommandExecutorImpl<StartScanOutput>;
    using StopScanCommand [[maybe_unused]] = internals::CommandExecutorImpl<StopScanOutput>;
    using FeedWatchdogCommand [[maybe_unused]] = internals::CommandExecutorImpl<FeedWatchdog>;
    using GetParametersCommand [[maybe_unused]] = internals::CommandExecutorImpl<GetParameters>;
    using FetchParametersCommand [[maybe_unused]] = internals::CommandExecutorImpl<FetchParameterList>;
    using SetParametersCommand [[maybe_unused]] = internals::CommandExecutorImpl<SetParameters>;
    using FactoryResetParametersCommand [[maybe_unused]] = internals::CommandExecutorImpl<FactoryResetParameters>;
    using FactoryResetDeviceCommand [[maybe_unused]] = internals::CommandExecutorImpl<FactoryResetDevice>;
    using RebootDeviceCommand [[maybe_unused]] = internals::CommandExecutorImpl<RebootDevice>;
    using RequestUdpHandleCommand [[maybe_unused]] = internals::CommandExecutorImpl<RequestUdpHandle>;
    using RequestTcpHandleCommand [[maybe_unused]] = internals::CommandExecutorImpl<RequestTcpHandle>;
    using GetScanOutputConfigCommand [[maybe_unused]] = internals::CommandExecutorImpl<GetScanOutputConfig>;
    using SetScanOutputConfigCommand [[maybe_unused]] = internals::CommandExecutorImpl<SetScanOutputConfig>;
} // namespace Device::Commands