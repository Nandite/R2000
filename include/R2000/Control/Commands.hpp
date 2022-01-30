//
// Created by chazz on 12/01/2022.
//

#pragma once

#include "DeviceHandle.hpp"
#include "R2000/R2000.hpp"

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
                    std::enable_if_t<std::is_convertible_v<std::invoke_result_t<Fn, Builder>, bool>, int> = 0>
            static constexpr inline auto resolve(Fn &&fn, Builder &&builder)
            -> std::decay_t<typename std::invoke_result_t<Fn, Builder>> {
                return std::invoke(fn, std::forward<Builder>(builder));
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
                    std::enable_if_t<std::is_convertible_v<std::invoke_result_t<Fn, Builder>, bool>, int> = 0,
                    typename... Rest>
            static constexpr inline auto resolve(Fn &&fn, Builder &&builder, Rest &&... args)
            -> std::decay_t<typename std::invoke_result_t<Fn, Builder>> {
                return std::invoke(fn, std::forward<Builder>(builder)) &&
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
            using ResultType = std::optional<std::pair<Parameters::ParametersMap, Parameters::ParametersList>>;
            using AsyncResultType = std::pair<Device::AsyncRequestResult, ResultType>;

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
                const auto propertyTree{device.sendHttpCommand(COMMAND_GET_PROTOCOL_INFO)};
                if (!propertyTree) {
                    return std::nullopt;
                }
                return extractInfoFromPropertyTree(*propertyTree);
            }

            /**
             * Asynchronously get the protocol information of the device.
             * @param timeout Maximum time allowed for the command to complete.
             * @return an optional future that can be used to obtain the result of the command.
             * if the optional is empty, the device is already busy with another command.
             */
            [[nodiscard]] [[maybe_unused]] std::optional<std::future<AsyncResultType>>
            asyncExecute(std::chrono::milliseconds timeout) {
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::AsyncResult &result) -> void {
                    if (!result) {
                        promise->set_value({result.getRequestResult(), std::nullopt});
                        return;
                    }
                    const auto parameters{extractInfoFromPropertyTree(result.getPropertyTree())};
                    promise->set_value({result.getRequestResult(), parameters});
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
                                               std::function<void(const AsyncResultType &)> callable) {
                return device.asyncSendHttpCommand(
                        COMMAND_GET_PROTOCOL_INFO,
                        [c = std::move(callable)](const Device::AsyncResult &result) -> void {
                            if (!result) {
                                std::invoke(c, AsyncResultType{result.getRequestResult(), std::nullopt});
                                return;
                            }
                            const auto parameters{extractInfoFromPropertyTree(result.getPropertyTree())};
                            std::invoke(c, AsyncResultType{result.getRequestResult(), parameters});
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
            static ResultType extractInfoFromPropertyTree(const PropertyTree &propertyTree) {
                Parameters::ParametersList availableCommands{};
                Parameters::ParametersMap protocolInfo{};
                const auto commands{propertyTree.get_child_optional(PARAMETER_AVAILABLE_COMMANDS)};
                const auto protocolName{propertyTree.get_optional<std::string>(PARAMETER_PROTOCOL_NAME)};
                const auto protocolVersionMajor{
                        propertyTree.get_optional<std::string>(PARAMETER_PROTOCOL_VERSION_MAJOR)};
                const auto protocolVersionMinor{
                        propertyTree.get_optional<std::string>(PARAMETER_PROTOCOL_VERSION_MINOR)};

                if (!commands || !protocolName || !protocolVersionMajor || !protocolVersionMinor)
                    return std::nullopt;
                for (const auto &name : *commands) {
                    const auto parameterName{name.second.get<std::string>("")};
                    if (parameterName.empty())
                        continue;
                    availableCommands.emplace_back(parameterName);
                }
                protocolInfo.insert(std::make_pair(PARAMETER_PROTOCOL_NAME, *protocolName));
                protocolInfo.insert(std::make_pair(PARAMETER_PROTOCOL_VERSION_MAJOR, *protocolVersionMajor));
                protocolInfo.insert(std::make_pair(PARAMETER_PROTOCOL_VERSION_MINOR, *protocolVersionMinor));
                return std::make_optional(std::make_pair(protocolInfo, availableCommands));
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<GetProtocolVersion> {
            using ResultType = Parameters::PFSDP;
            using AsyncResultType = std::pair<Device::AsyncRequestResult, ResultType>;

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
                auto result{getProtocolInfo.execute()};
                if (!result)
                    return Parameters::PFSDP::UNKNOWN;
                const auto parameters{(*result).first};
                const auto major{parameters.at(PARAMETER_PROTOCOL_VERSION_MAJOR)};
                const auto minor{parameters.at(PARAMETER_PROTOCOL_VERSION_MINOR)};
                return protocolVersionFromString(major, minor);
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
                        return {Device::AsyncRequestResult::FAILED, Parameters::PFSDP::UNKNOWN};
                    }
                    future->wait();
                    auto result{future->get()};
                    if (result.first != Device::AsyncRequestResult::SUCCESS) {
                        return {result.first, Parameters::PFSDP::UNKNOWN};
                    }
                    const auto parameters{(result.second)->first};
                    const auto major{parameters.at(PARAMETER_PROTOCOL_VERSION_MAJOR)};
                    const auto minor{parameters.at(PARAMETER_PROTOCOL_VERSION_MINOR)};
                    return {result.first, protocolVersionFromString(major, minor)};
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
                    if (result.first != Device::AsyncRequestResult::SUCCESS) {
                        std::invoke(fn, AsyncResultType{result.first, Parameters::PFSDP::UNKNOWN});
                        return;
                    }
                    const auto parameters{(result.second)->first};
                    const auto major{parameters.at(PARAMETER_PROTOCOL_VERSION_MAJOR)};
                    const auto minor{parameters.at(PARAMETER_PROTOCOL_VERSION_MINOR)};
                    std::invoke(fn, AsyncResultType{result.first, protocolVersionFromString(major, minor)});
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
            using ResultType = bool;
            using AsyncResultType = AsyncRequestResult;

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
                return RecursiveExecutorHelper::resolve(
                        [this](const auto &handle) {
                            return device.sendHttpCommand(COMMAND_RELEASE_HANDLE, PARAMETER_NAME_HANDLE,
                                                          handle.value).has_value();
                        },
                        std::forward<Args>(args)...);
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
                auto onComplete{[promise](const Device::AsyncResult &result) -> void {
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
            [[maybe_unused]] bool asyncExecute(const Device::DeviceHandle &handle, std::chrono::milliseconds timeout,
                                               std::function<void(const AsyncResultType &)> callable) {
                return device.asyncSendHttpCommand(
                        COMMAND_RELEASE_HANDLE, {{PARAMETER_NAME_HANDLE, handle.getValue()}},
                        [c = std::move(callable)](const Device::AsyncResult &result) -> void {
                            if (!result) {
                                std::invoke(c, AsyncResultType{result.getRequestResult()});
                                return;
                            }
                            std::invoke(c, AsyncResultType{result.getRequestResult()});
                        },
                        timeout);
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<StartScanOutput> {
            using ResultType = bool;
            using AsyncResultType = Device::AsyncRequestResult;

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
                return RecursiveExecutorHelper::resolve(
                        [this](const auto &handle) {
                            return device.sendHttpCommand(COMMAND_START_SCAN_OUTPUT, PARAMETER_NAME_HANDLE,
                                                          handle.value).has_value();
                        },
                        std::forward<Args>(args)...);
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
                auto onComplete{[promise](const Device::AsyncResult &result) -> void {
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
            [[maybe_unused]] bool asyncExecute(const Device::DeviceHandle &handle, std::chrono::milliseconds timeout,
                                               std::function<void(const AsyncResultType &)> callable) {
                return device.asyncSendHttpCommand(
                        COMMAND_START_SCAN_OUTPUT, {{PARAMETER_NAME_HANDLE, handle.getValue()}},
                        [c = std::move(callable)](const Device::AsyncResult &result) -> void {
                            if (!result) {
                                std::invoke(c, AsyncResultType{result.getRequestResult()});
                                return;
                            }
                            std::invoke(c, AsyncResultType{result.getRequestResult()});
                        },
                        timeout);
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<StopScanOutput> {
            using ResultType = bool;
            using AsyncResultType = Device::AsyncRequestResult;

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
                return RecursiveExecutorHelper::resolve(
                        [this](const auto &handle) {
                            return device.sendHttpCommand(COMMAND_STOP_SCAN_OUTPUT, PARAMETER_NAME_HANDLE,
                                                          handle.value).has_value();
                        },
                        std::forward<Args>(args)...);
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
                auto onComplete{[promise](const Device::AsyncResult &result) -> void {
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
            [[maybe_unused]] bool asyncExecute(const Device::DeviceHandle &handle, std::chrono::milliseconds timeout,
                                               std::function<void(const AsyncResultType &)> callable) {
                return device.asyncSendHttpCommand(
                        COMMAND_STOP_SCAN_OUTPUT, {{PARAMETER_NAME_HANDLE, handle.getValue()}},
                        [c = std::move(callable)](const Device::AsyncResult &result) -> void {
                            if (!result) {
                                std::invoke(c, AsyncResultType{result.getRequestResult()});
                                return;
                            }
                            std::invoke(c, AsyncResultType{result.getRequestResult()});
                        },
                        timeout);
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<FeedWatchdog> {
            using ResultType = bool;
            using AsyncResultType = Device::AsyncRequestResult;

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
                return RecursiveExecutorHelper::resolve(
                        [this](const auto &handle) {
                            return device.sendHttpCommand(COMMAND_FEED_WATCHDOG, PARAMETER_NAME_HANDLE,
                                                          handle.value).has_value();
                        },
                        std::forward<Args>(args)...);
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
                auto onComplete{[promise](const Device::AsyncResult &result) -> void {
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
            [[maybe_unused]] bool asyncExecute(const Device::DeviceHandle &handle, std::chrono::milliseconds timeout,
                                               std::function<void(const AsyncResultType &)> callable) {
                return device.asyncSendHttpCommand(
                        COMMAND_FEED_WATCHDOG, {{PARAMETER_NAME_HANDLE, handle.getValue()}},
                        [fn = std::move(callable)](const Device::AsyncResult &result) -> void {
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
            using ResultType = std::optional<Parameters::ParametersMap>;
            using AsyncResultType = std::pair<Device::AsyncRequestResult, ResultType>;

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
                const auto parametersListAsString{chainAndConvertParametersToString(std::forward<Args>(args)...)};
                const auto propertyTree{
                        device.sendHttpCommand(COMMAND_GET_PARAMETER, PARAMETER_NAME_LIST,
                                               parametersListAsString.first)};
                if (!propertyTree) {
                    return std::nullopt;
                }
                return {extractParametersValues(parameters, *propertyTree)};
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
            asyncExecuteFuture(std::chrono::milliseconds timeout,
                               Args &&... args) {
                static_assert(
                        std::conjunction_v<std::is_base_of<Parameters::ReadOnlyRequestBuilder, typename std::decay_t<Args>>...>,
                        "All the provided handle must be of type ReadOnlyRequestBuilder");
                const auto chainedParameters{chainAndConvertParametersToString(std::forward<Args>(args)...)};
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{
                        [promise, parameters = std::move(chainedParameters.second)](
                                const Device::AsyncResult &result) -> void {
                            if (!result) {
                                promise->set_value({result.getRequestResult(), std::nullopt});
                                return;
                            }
                            const auto receivedParameters{
                                    extractParametersValues(parameters, result.getPropertyTree())};
                            promise->set_value({result.getRequestResult(), {receivedParameters}});
                        }};
                if (device.asyncSendHttpCommand(COMMAND_GET_PARAMETER, {{PARAMETER_NAME_LIST, chainedParameters.first}},
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
            [[maybe_unused]] bool asyncExecute(std::chrono::milliseconds timeout,
                                               std::function<void(const AsyncResultType &)> callable, Args &&... args) {
                static_assert(
                        std::conjunction_v<std::is_base_of<Parameters::ReadOnlyRequestBuilder, typename std::decay_t<Args>>...>,
                        "All the provided handle must be of type ReadOnlyRequestBuilder");
                const auto chainedParameters{chainAndConvertParametersToString(std::forward<Args>(args)...)};
                return device.asyncSendHttpCommand(
                        COMMAND_GET_PARAMETER, {{PARAMETER_NAME_LIST, chainedParameters}}, [fn = std::move(callable),
                                parameters = std::move(chainedParameters.second)](
                                const Device::AsyncResult &result) -> void {
                            if (!result) {
                                std::invoke(fn, AsyncResultType{result.getRequestResult(), std::nullopt});
                                return;
                            }
                            const auto receivedParameters{
                                    extractParametersValues(parameters, result.getPropertyTree())};
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
                for (const auto &name : list) {
                    auto value{propertyTree.get_optional<std::string>(name)};
                    if (!value) {
                        keysValues.insert(std::make_pair(name, ""));
                        continue;
                    }
                    keysValues.insert(std::make_pair(name, *value));
                }
                return {keysValues};
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
                for (const auto &names : parameters)
                    parametersAsString += (names + ";");
                parametersAsString.pop_back();
                return {parametersAsString, parameters};
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<SetParameters> {
            using ResultType = bool;
            using AsyncResultType = Device::AsyncRequestResult;

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
                return device.sendHttpCommand(COMMAND_SET_PARAMETER, parameters).has_value();
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
                auto onComplete{[promise](const Device::AsyncResult &result) -> void {
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
            asyncExecute(std::chrono::milliseconds timeout, std::function<void(const AsyncResultType &)> callable,
                         Args &&... args) {
                static_assert(
                        std::conjunction_v<std::is_base_of<Parameters::ReadWriteRequestBuilder, typename std::decay_t<Args>>...>,
                        "All the provided handle must be of type ReadWriteRequestBuilder");
                Parameters::ParametersMap parameters{};
                ParametersChaining::template map(parameters, std::forward<Args>(args)...);
                return device.asyncSendHttpCommand(
                        COMMAND_SET_PARAMETER, parameters,
                        [fn = std::move(callable)](const Device::AsyncResult &result) -> void {
                            std::invoke(fn, result.getRequestResult());
                        },
                        timeout);
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<FetchParameterList> {
            using ResultType = std::optional<Parameters::ParametersList>;
            using AsyncResultType = std::pair<Device::AsyncRequestResult, ResultType>;

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
                const auto propertyTree{device.sendHttpCommand(COMMAND_LIST_PARAMETERS)};
                if (!propertyTree) {
                    return std::nullopt;
                }
                return {extractParametersFromPropertyTree(*propertyTree)};
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
                auto onComplete{[promise](const Device::AsyncResult &result) -> void {
                    if (!result) {
                        promise->set_value({result.getRequestResult(), std::nullopt});
                        return;
                    }
                    const auto parameters{extractParametersFromPropertyTree(result.getPropertyTree())};
                    promise->set_value({result.getRequestResult(), {parameters}});
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
                        COMMAND_LIST_PARAMETERS, [fn = std::move(callable)](const Device::AsyncResult &result) -> void {
                            if (!result) {
                                std::invoke(fn, AsyncResultType{result.getRequestResult(), std::nullopt});
                                return;
                            }
                            const auto parameters{extractParametersFromPropertyTree(result.getPropertyTree())};
                            std::invoke(fn, AsyncResultType{result.getRequestResult(), {parameters}});
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
                for (const auto &name : *list) {
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
            using ResultType = bool;
            using AsyncResultType = Device::AsyncRequestResult;

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
                return device.sendHttpCommand(COMMAND_RESET_PARAMETERS, PARAMETER_NAME_LIST, params).has_value();
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
            asyncExecute(std::chrono::milliseconds timeout,
                         Args &&... args) {
                static_assert(
                        std::conjunction_v<std::is_base_of<Parameters::ReadWriteRequestBuilder, typename std::decay_t<Args>>...>,
                        "All the provided handle must be of type ReadWriteRequestBuilder");
                const auto parameters{chainAndConvertParametersToString(std::forward<Args>(args)...)};
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::AsyncResult &result) -> void {
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
            [[maybe_unused]] bool asyncExecute(std::chrono::milliseconds timeout,
                                               std::function<void(const AsyncResultType &)> callable, Args &&... args) {
                static_assert(
                        std::conjunction_v<std::is_base_of<Parameters::ReadWriteRequestBuilder, typename std::decay_t<Args>>...>,
                        "All the provided handle must be of type ReadWriteRequestBuilder");
                const auto parameters{chainAndConvertParametersToString(std::forward<Args>(args)...)};
                return device.asyncSendHttpCommand(
                        COMMAND_RESET_PARAMETERS, {{PARAMETER_NAME_LIST, parameters}},
                        [fn = std::move(callable)](const Device::AsyncResult &result) -> void {
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
                for (const auto &names : parameters) {
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
            using ResultType = bool;
            using AsyncResultType = Device::AsyncRequestResult;

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
                return device.sendHttpCommand(COMMAND_FACTORY_RESET).has_value();
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
                auto onComplete{[promise](const Device::AsyncResult &result) -> void {
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
                        [fn = std::move(callable)](const Device::AsyncResult &result) -> void {
                            std::invoke(fn, result.getRequestResult());
                        },
                        timeout);
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<RebootDevice> {
            using ResultType = bool;
            using AsyncResultType = Device::AsyncRequestResult;

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
                return device.sendHttpCommand(COMMAND_REBOOT_DEVICE).has_value();
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
                auto onComplete{[promise](const Device::AsyncResult &result) -> void {
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
                        [fn = std::move(callable)](const Device::AsyncResult &result) -> void {
                            std::invoke(fn, AsyncResultType{result.getRequestResult()});
                        },
                        timeout);
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<RequestTcpHandle> {
            using ResultType = std::optional<std::pair<int, Device::HandleType>>;
            using AsyncResultType = std::pair<Device::AsyncRequestResult, ResultType>;

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
                const auto propertyTree{device.sendHttpCommand(COMMAND_REQUEST_TCP_HANDLE, parameters)};
                if (!propertyTree) {
                    return std::nullopt;
                }
                return extractHandleInfoFromPropertyTree(*propertyTree);
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
                auto onComplete{[promise](const Device::AsyncResult &result) -> void {
                    if (!result) {
                        promise->set_value({result.getRequestResult(), std::nullopt});
                        return;
                    }
                    const auto handle{extractHandleInfoFromPropertyTree(result.getPropertyTree())};
                    promise->set_value({result.getRequestResult(), handle});
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
                return device.asyncSendHttpCommand(COMMAND_REQUEST_TCP_HANDLE, parameters, [fn = std::move(callable)](
                                                           const Device::AsyncResult &result) -> void {
                                                       if (!result) {
                                                           std::invoke(fn, AsyncResultType{result.getRequestResult(),
                                                                                           std::nullopt});
                                                           return;
                                                       }
                                                       const auto handle{extractHandleInfoFromPropertyTree(
                                                               result.getPropertyTree())};
                                                       std::invoke(fn,
                                                                   AsyncResultType{result.getRequestResult(), handle});
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
            static ResultType extractHandleInfoFromPropertyTree(const PropertyTree &propertyTree) {
                const auto port{propertyTree.get_optional<int>("port")};
                const auto handle{propertyTree.get_optional<HandleType>("handle")};
                if (!port || !handle)
                    return std::nullopt;
                return {std::make_pair(*port, *handle)};
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<RequestUdpHandle> {
            using ResultType = std::optional<Device::HandleType>;
            using AsyncResultType = std::pair<Device::AsyncRequestResult, ResultType>;

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
                const auto propertyTree{device.sendHttpCommand(COMMAND_REQUEST_UDP_HANDLE, parameters)};
                if (!propertyTree) {
                    return std::nullopt;
                }
                return extractHandleInfoFromPropertyTree(*propertyTree);
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
                auto onComplete{[promise](const Device::AsyncResult &result) -> void {
                    if (!result) {
                        promise->set_value({result.getRequestResult(), std::nullopt});
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
                        [fn = std::move(callable)](const Device::AsyncResult &result) -> void {
                            if (!result) {
                                std::invoke(fn, AsyncResultType{result.getRequestResult(), std::nullopt});
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
            static ResultType extractHandleInfoFromPropertyTree(const PropertyTree &propertyTree) {
                const auto handle{propertyTree.get_optional<std::string>("handle")};
                if (!handle)
                    return std::nullopt;
                return {*handle};
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<GetScanOutputConfig> {
            using ResultType = std::vector<std::optional<Parameters::ParametersMap>>;
            using AsyncResultType = std::pair<Device::AsyncRequestResult, std::optional<Parameters::ParametersMap>>;

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
                        [this, &configurations](const auto &handle) {
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
                auto onComplete{[promise](const Device::AsyncResult &result) -> void {
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
            [[maybe_unused]] bool asyncExecute(const Device::DeviceHandle &handle, std::chrono::milliseconds timeout,
                                               std::function<void(const AsyncResultType &)> callable) {
                return device.asyncSendHttpCommand(
                        COMMAND_GET_SCAN_OUTPUT_CONFIG, {{PARAMETER_NAME_HANDLE, handle.getValue()}},
                        [fn = std::move(callable)](const Device::AsyncResult &result) -> void {
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
                for (const auto &name : propertyTree) {
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
            using ResultType = bool;
            using AsyncResultType = Device::AsyncRequestResult;

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
                return RecursiveExecutorHelper::template resolve(
                        [this](const auto &builder) {
                            const auto parameters{builder.build()};
                            return device.sendHttpCommand(COMMAND_SET_SCAN_OUTPUT_CONFIG, parameters).has_value();
                        },
                        std::forward<Args>(args)...);
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
                auto onComplete{[promise](const Device::AsyncResult &result) -> void {
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
            asyncExecute(const Parameters::HandleParameters &builder, std::chrono::milliseconds timeout,
                         std::function<void(const AsyncResultType &)> callable) {
                const auto parameters{builder.build()};
                return device.asyncSendHttpCommand(
                        COMMAND_SET_SCAN_OUTPUT_CONFIG, parameters,
                        [fn = std::move(callable)](const Device::AsyncResult &result) -> void {
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