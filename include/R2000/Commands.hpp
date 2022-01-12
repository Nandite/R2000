//
// Created by chazz on 12/01/2022.
//

#pragma once

namespace Device {
    namespace internals::Parameters {

        struct CommandTags {
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
        };

        struct ParametersChaining {
            template<typename T>
            static constexpr inline void chain(ParametersList &parameters, T &builder) {
                const auto builderParameters{builder.build()};
                parameters.insert(std::end(parameters), std::cbegin(builderParameters), std::cend(builderParameters));
            }

            template<typename T, typename... Rest>
            static constexpr inline void chain(ParametersList &parameters, T &builder, Rest &&... args) {
                const auto builderParameters{builder.build()};
                parameters.insert(std::end(parameters), std::cbegin(builderParameters), std::cend(builderParameters));
                chain<Rest...>(parameters, std::forward<Rest>(args)...);
            }
        };

        template<typename ReturnType>
        struct ReturnTypeDependantExecutionSelector {
            template<typename Execute, typename T>
            static constexpr inline ReturnType resolve(Execute &&execute, T &&builder) {
                return execute(std::forward<T>(builder));
            }

            template<typename Execute, typename T, typename... Rest>
            static constexpr inline ReturnType resolve(Execute &&execute, T &&builder, Rest &&... args) {
                return execute(std::forward<T>(builder)) &&
                       resolve<Execute, Rest...>(std::forward<Execute>(execute), std::forward<Rest>(args)...);
            }
        };

        template<>
        struct ReturnTypeDependantExecutionSelector<void> {
            template<typename Execute, typename T>
            static constexpr inline void resolve(Execute &&execute, T &&builder) {
                execute(std::forward<T>(builder));
            }

            template<typename Execute, typename T, typename... Rest>
            static constexpr inline void resolve(Execute &&execute, T &&builder, Rest &&... args) {
                execute(std::forward<T>(builder));
                resolve<Execute, Rest...>(std::forward<Execute>(execute), std::forward<Rest>(args)...);
            }
        };

        struct RecursiveExecutorHelper {

            template<typename Execute, typename T>
            static constexpr inline auto resolve(Execute &&execute, T &&builder)
            -> std::decay_t<typename std::invoke_result_t<Execute, T>> {
                return ReturnTypeDependantExecutionSelector<std::decay_t<typename std::invoke_result_t<Execute, T>>>::
                template resolve<Execute, T>(std::forward<Execute>(execute), std::forward<T>(builder));
            }

            template<typename Execute, typename T, typename... Rest>
            static constexpr inline auto resolve(Execute &&execute, T &&builder, Rest &&... args)
            -> std::decay_t<typename std::invoke_result_t<Execute, T>> {
                return ReturnTypeDependantExecutionSelector<std::decay_t<typename std::invoke_result_t<Execute, T>>>::
                template resolve<Execute, T, Rest...>(std::forward<Execute>(execute), std::forward<T>(builder),
                                                      std::forward<Rest>(args)...);
            }
        };

        template<typename Command>
        struct CommandExecutorImpl {
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::GetProtocolInfo> {
            using ResultType = std::optional<std::pair<ParametersMap, ParametersList>>;
            using AsyncResultType = std::pair<R2000::AsyncRequestResult, ResultType>;

            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            ResultType execute() {
                const auto result{device.sendHttpCommand(COMMAND_GET_PROTOCOL_INFO)};
                const auto status{result.first};
                const auto &propertyTree{result.second};
                if (!status)
                    return std::nullopt;
                return extractInfoFromPropertyTree(propertyTree);
            }

            std::optional<std::future<AsyncResultType>> execute(std::chrono::milliseconds timeout) {
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::R2000::AsyncResult &result) -> void {
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

        private:
            static ResultType extractInfoFromPropertyTree(const PropertyTree &propertyTree) {
                ParametersList availableCommands{};
                ParametersMap protocolInfo{};
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
        struct CommandExecutorImpl<internals::Parameters::CommandTags::ReleaseHandle> {
            using ResultType = bool;
            using AsyncResultType = R2000::AsyncRequestResult;

            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            template<typename... Args>
            inline ResultType execute(Args &&... args) {
                static_assert(std::conjunction_v<std::is_same<Device::DeviceHandle, Args>...>,
                              "All the provided handle must be of type DeviceHandle");
                return RecursiveExecutorHelper::resolve(
                        [this](const auto &handle) {
                            return device.sendHttpCommand(COMMAND_RELEASE_HANDLE, PARAMETER_NAME_HANDLE,
                                                          handle.value).first;
                        },
                        std::forward<Args>(args)...);
            }

            std::optional<std::future<AsyncResultType>>
            execute(const Device::DeviceHandle &handle, std::chrono::milliseconds timeout) {
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::R2000::AsyncResult &result) -> void {
                    promise->set_value(result.getRequestResult());
                }};
                if (device.asyncSendHttpCommand(COMMAND_RELEASE_HANDLE,
                                                {{PARAMETER_NAME_HANDLE, handle.value}},
                                                onComplete, timeout)) {
                    return {promise->get_future()};
                }
                return std::nullopt;
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::StartScanOutput> {
            using ResultType = bool;
            using AsyncResultType = R2000::AsyncRequestResult;

            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            template<typename... Args>
            inline ResultType execute(Args &&... args) {
                static_assert(std::conjunction_v<std::is_same<Device::DeviceHandle, Args>...>,
                              "All the provided handle must be of type DeviceHandle");
                return RecursiveExecutorHelper::resolve(
                        [this](const auto &handle) {
                            return device.sendHttpCommand(COMMAND_START_SCAN_OUTPUT, PARAMETER_NAME_HANDLE,
                                                          handle.value).first;
                        },
                        std::forward<Args>(args)...);
            }

            std::optional<std::future<AsyncResultType>>
            execute(const Device::DeviceHandle &handle, std::chrono::milliseconds timeout) {
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::R2000::AsyncResult &result) -> void {
                    promise->set_value(result.getRequestResult());
                }};
                if (device.asyncSendHttpCommand(COMMAND_START_SCAN_OUTPUT,
                                                {{PARAMETER_NAME_HANDLE, handle.value}},
                                                onComplete, timeout)) {
                    return {promise->get_future()};
                }
                return std::nullopt;
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::StopScanOutput> {
            using ResultType = bool;
            using AsyncResultType = R2000::AsyncRequestResult;

            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            template<typename... Args>
            inline ResultType execute(Args &&... args) {
                static_assert(std::conjunction_v<std::is_same<Device::DeviceHandle, Args>...>,
                              "All the provided handle must be of type DeviceHandle");
                return RecursiveExecutorHelper::resolve(
                        [this](const auto &handle) {
                            return device.sendHttpCommand(COMMAND_STOP_SCAN_OUTPUT, PARAMETER_NAME_HANDLE,
                                                          handle.value).first;
                        },
                        std::forward<Args>(args)...);
            }

            std::optional<std::future<AsyncResultType>>
            execute(const Device::DeviceHandle &handle, std::chrono::milliseconds timeout) {
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::R2000::AsyncResult &result) -> void {
                    promise->set_value(result.getRequestResult());
                }};
                if (device.asyncSendHttpCommand(COMMAND_STOP_SCAN_OUTPUT,
                                                {{PARAMETER_NAME_HANDLE, handle.value}},
                                                onComplete, timeout)) {
                    return {promise->get_future()};
                }
                return std::nullopt;
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::FeedWatchdog> {
            using ResultType = bool;
            using AsyncResultType = R2000::AsyncRequestResult;

            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            template<typename... Args>
            inline ResultType execute(Args &&... args) {
                static_assert(std::conjunction_v<std::is_same<Device::DeviceHandle, Args>...>,
                              "All the provided handle must be of type DeviceHandle");
                return RecursiveExecutorHelper::resolve(
                        [this](const auto &handle) {
                            return device.sendHttpCommand(COMMAND_FEED_WATCHDOG, PARAMETER_NAME_HANDLE,
                                                          handle.value).first;
                        },
                        std::forward<Args>(args)...);
            }

            std::optional<std::future<AsyncResultType>>
            execute(const Device::DeviceHandle &handle, std::chrono::milliseconds timeout) {
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::R2000::AsyncResult &result) -> void {
                    promise->set_value(result.getRequestResult());
                }};
                if (device.asyncSendHttpCommand(COMMAND_FEED_WATCHDOG,
                                                {{PARAMETER_NAME_HANDLE, handle.value}},
                                                onComplete, timeout)) {
                    return {promise->get_future()};
                }
                return std::nullopt;
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::GetParameters> {
            using ResultType = std::optional<ParametersMap>;
            using AsyncResultType = std::pair<R2000::AsyncRequestResult, ResultType>;

            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            template<typename... Args>
            inline ResultType execute(Args &&... args) {
                static_assert(std::conjunction_v<std::is_same<ReadOnlyBuilderBase, Args>...>,
                              "All the provided handle must be of type ReadOnlyBuilderBase");
                ParametersList parameters{};
                ParametersChaining::template chain(parameters, std::forward<Args>(args)...);
                const auto parametersListAsString{chainAndConvertParametersToString(std::forward<Args>(args)...)};
                const auto result{
                        device.sendHttpCommand(COMMAND_GET_PARAMETER, PARAMETER_NAME_LIST,
                                               parametersListAsString.first)};
                const auto status{result.first};
                if (!status)
                    return std::nullopt;
                return extractParametersValues(parameters, result.second);
            }

            template<typename... Args>
            std::optional<std::future<AsyncResultType>>
            execute(std::chrono::milliseconds timeout, Args &&... args) {
                static_assert(std::conjunction_v<std::is_same<ReadOnlyBuilderBase, Args>...>,
                              "All the provided handle must be of type ReadOnlyBuilderBase");
                const auto chainedParameters{chainAndConvertParametersToString(std::forward<Args>(args)...)};
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise, parameters = std::move(chainedParameters.second)](
                        const Device::R2000::AsyncResult &result) -> void {
                    if (!result) {
                        promise->set_value({result.getRequestResult(), std::nullopt});
                        return;
                    }
                    const auto receivedParameters{extractParametersValues(parameters, result.getPropertyTree())};
                    promise->set_value({result.getRequestResult(), receivedParameters});
                }};
                if (device.asyncSendHttpCommand(COMMAND_GET_PARAMETER, {{PARAMETER_NAME_LIST, chainedParameters}},
                                                onComplete,
                                                timeout)) {
                    return {promise->get_future()};
                }
                return std::nullopt;
            }

        private:
            static ResultType extractParametersValues(const ParametersList &list, const PropertyTree &propertyTree) {
                ParametersMap keysValues{};
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
            template<typename... Args>
            std::pair<std::string, ParametersList> chainAndConvertParametersToString(Args &&... args) {
                ParametersList parameters{};
                ParametersChaining::template chain(parameters, std::forward<Args>(args)...);
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
        struct CommandExecutorImpl<internals::Parameters::CommandTags::SetParameters> {
            using ResultType = bool;
            using AsyncResultType = R2000::AsyncRequestResult;

            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            template<typename... Args>
            inline ResultType execute(Args &&... args) {
                static_assert(std::conjunction_v<std::is_same<WriteBuilderBase, Args>...>,
                              "All the provided handle must be of type WriteBuilderBase");
                ParametersMap parameters{};
                ParametersChaining::template chain(parameters, std::forward<Args>(args)...);
                return device.sendHttpCommand(COMMAND_SET_PARAMETER, parameters).first;
            }

            template<typename... Args>
            std::optional<std::future<AsyncResultType>>
            execute(std::chrono::milliseconds timeout, Args &&... args) {
                static_assert(std::conjunction_v<std::is_same<WriteBuilderBase, Args>...>,
                              "All the provided handle must be of type WriteBuilderBase");
                ParametersMap parameters{};
                ParametersChaining::template chain(parameters, std::forward<Args>(args)...);
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::R2000::AsyncResult &result) -> void {
                    promise->set_value(result.getRequestResult());
                }};
                if (device.asyncSendHttpCommand(COMMAND_SET_PARAMETER, parameters, onComplete, timeout)) {
                    return {promise->get_future()};
                }
                return std::nullopt;
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::FetchParameterList> {
            using ResultType = std::optional<ParametersList>;
            using AsyncResultType = std::pair<R2000::AsyncRequestResult, ResultType>;

            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            ResultType execute() {
                const auto result{device.sendHttpCommand(COMMAND_LIST_PARAMETERS)};
                const auto status{result.first};
                const auto &propertyTree{result.second};
                if (!status)
                    return std::nullopt;
                return extractParametersFromPropertyTree(propertyTree);
            }

            std::optional<std::future<AsyncResultType>> execute(std::chrono::milliseconds timeout) {
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::R2000::AsyncResult &result) -> void {
                    if (!result) {
                        promise->set_value({result.getRequestResult(), std::nullopt});
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

        private:
            static std::optional<ParametersList> extractParametersFromPropertyTree(const PropertyTree &propertyTree) {
                ParametersList parameterList{};
                const auto list{propertyTree.get_child_optional("parameters")};
                if (!list)
                    return std::nullopt;
                for (const auto &name : *list) {
                    const auto parameterName{name.second.get<std::string>("")};
                    if (parameterName.empty())
                        continue;
                    parameterList.emplace_back(parameterName);
                }
                return std::make_optional(parameterList);
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::FactoryResetParameters> {
            using ResultType = bool;
            using AsyncResultType = R2000::AsyncRequestResult;

            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            template<typename... Args>
            inline ResultType execute(Args &&... args) {
                static_assert(std::conjunction_v<std::is_same<WriteBuilderBase, Args>...>,
                              "All the provided handle must be of type WriteBuilderBase");
                const auto params{chainAndConvertParametersToString(std::forward<Args>(args)...)};
                return device.sendHttpCommand(COMMAND_RESET_PARAMETERS, PARAMETER_NAME_LIST, params).first;
            }

            template<typename... Args>
            std::optional<std::future<AsyncResultType>>
            execute(std::chrono::milliseconds timeout, Args &&... args) {
                static_assert(std::conjunction_v<std::is_same<WriteBuilderBase, Args>...>,
                              "All the provided handle must be of type WriteBuilderBase");
                const auto params{chainAndConvertParametersToString(std::forward<Args>(args)...)};
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::R2000::AsyncResult &result) -> void {
                    promise->set_value(result.getRequestResult());
                }};
                if (device.asyncSendHttpCommand(COMMAND_RESET_PARAMETERS, {{PARAMETER_NAME_LIST, params}}, onComplete,
                                                timeout)) {
                    return {promise->get_future()};
                }
                return std::nullopt;
            }

        private:
            template<typename... Args>
            std::string chainAndConvertParametersToString(Args &&... args) {
                ParametersList parameters{};
                ParametersChaining::template chain(parameters, std::forward<Args>(args)...);
                std::string parametersList{};
                for (const auto &names : parameters)
                    parametersList += (names + ";");
                parametersList.pop_back();
                return parametersList;
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::FactoryResetDevice> {
            using ResultType = bool;
            using AsyncResultType = R2000::AsyncRequestResult;

            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            inline ResultType execute() { return device.sendHttpCommand(COMMAND_FACTORY_RESET).first; }

            std::optional<std::future<AsyncResultType>> execute(std::chrono::milliseconds timeout) {
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::R2000::AsyncResult &result) -> void {
                    promise->set_value(result.getRequestResult());
                }};
                if (device.asyncSendHttpCommand(COMMAND_FACTORY_RESET, onComplete, timeout)) {
                    return {promise->get_future()};
                }
                return std::nullopt;
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::RebootDevice> {
            using ResultType = bool;
            using AsyncResultType = R2000::AsyncRequestResult;

            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            ResultType execute() { return device.sendHttpCommand(COMMAND_REBOOT_DEVICE).first; }

            std::optional<std::future<AsyncResultType>> execute(std::chrono::milliseconds timeout) {
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::R2000::AsyncResult &result) -> void {
                    promise->set_value(result.getRequestResult());
                }};
                if (device.asyncSendHttpCommand(COMMAND_REBOOT_DEVICE, onComplete, timeout)) {
                    return {promise->get_future()};
                }
                return std::nullopt;
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::RequestTcpHandle> {
            using ResultType = std::optional<std::pair<int, Device::HandleType>>;
            using AsyncResultType = std::pair<R2000::AsyncRequestResult, ResultType>;

            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            ResultType execute(const ReadWriteParameters::TcpHandle &builder) {
                const auto parameters{builder.build()};
                const auto result{device.sendHttpCommand(COMMAND_REQUEST_TCP_HANDLE, parameters)};
                const auto status{result.first};
                const auto &propertyTree{result.second};
                if (!status)
                    return std::nullopt;
                return extractHandleInfoFromPropertyTree(propertyTree);
            }

            std::optional<std::future<AsyncResultType>>
            execute(std::chrono::milliseconds timeout, const ReadWriteParameters::TcpHandle &builder) {
                const auto parameters{builder.build()};
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::R2000::AsyncResult &result) -> void {
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

        private:
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
        struct CommandExecutorImpl<internals::Parameters::CommandTags::RequestUdpHandle> {
            using ResultType = std::optional<Device::HandleType>;
            using AsyncResultType = std::pair<R2000::AsyncRequestResult, ResultType>;

            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            ResultType execute(const ReadWriteParameters::UdpHandle &builder) {
                const auto parameters{builder.build()};
                const auto result{device.sendHttpCommand(COMMAND_REQUEST_UDP_HANDLE, parameters)};
                const auto status{result.first};
                const auto &propertyTree{result.second};
                if (!status)
                    return std::nullopt;
                return extractHandleInfoFromPropertyTree(propertyTree);
            }

            std::optional<std::future<AsyncResultType>>
            execute(std::chrono::milliseconds timeout, const ReadWriteParameters::TcpHandle &builder) {
                const auto parameters{builder.build()};
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::R2000::AsyncResult &result) -> void {
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

        private:
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
        struct CommandExecutorImpl<internals::Parameters::CommandTags::GetScanOutputConfig> {
            using ResultType = std::vector<std::optional<ParametersMap>>;
            using AsyncResultType = std::pair<R2000::AsyncRequestResult, std::optional<ParametersMap>>;

            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            template<typename... Args>
            ResultType execute(Args &&... args) {
                static_assert(std::conjunction_v<std::is_same<Device::DeviceHandle, Args>...>,
                              "All the provided handle must be of type DeviceHandle");
                std::vector<std::optional<ParametersMap>> configurations;
                RecursiveExecutorHelper::resolve(
                        [this, &configurations](const auto &handle) {
                            const auto result{
                                    device.sendHttpCommand(COMMAND_GET_SCAN_OUTPUT_CONFIG, PARAMETER_NAME_HANDLE,
                                                           handle.value)};
                            const auto status{result.first};
                            if (!status) {
                                configurations.template emplace_back(std::nullopt);
                                return;
                            }
                            configurations.template emplace_back(getScanOutputConfig(result.second));
                        },
                        std::forward<Args>(args)...);
                return configurations;
            }

            std::optional<std::future<AsyncResultType>>
            execute(const Device::DeviceHandle &handle, std::chrono::milliseconds timeout) {
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::R2000::AsyncResult &result) -> void {
                    if (!result) {
                        promise->set_value({result.getRequestResult(), std::nullopt});
                        return;
                    }
                    const auto &propertyTree{result.getPropertyTree()};
                    promise->set_value({result.getRequestResult(), getScanOutputConfig(propertyTree)});
                }};

                if (device.asyncSendHttpCommand(COMMAND_GET_SCAN_OUTPUT_CONFIG,
                                                {{PARAMETER_NAME_HANDLE, handle.value}}, onComplete,
                                                timeout)) {
                    return {promise->get_future()};
                }
                return std::nullopt;
            }

        private:
            static std::optional<ParametersMap> getScanOutputConfig(const Device::PropertyTree &propertyTree) {
                ParametersMap scanOutputConfig{};
                for (const auto &name : propertyTree) {
                    const auto parameterName{name.second.get<std::string>("")};
                    auto value{propertyTree.get_optional<std::string>(parameterName)};
                    if (!value)
                        return std::nullopt;
                    scanOutputConfig.insert(std::make_pair(parameterName, *value));
                }
                return {scanOutputConfig};
            }

        private:
            R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::SetScanOutputConfig> {
            using ResultType = bool;
            using AsyncResultType = R2000::AsyncRequestResult;

            [[maybe_unused]] explicit CommandExecutorImpl(R2000 &device) : device(device) {}

            template<typename... Args>
            inline ResultType execute(Args &&... args) {
                static_assert(std::conjunction_v<std::is_same<HandleParameters, Args>...>,
                              "All the provided handle must be of type HandleParameters");
                return RecursiveExecutorHelper::template resolve(
                        [this](const auto &builder) {
                            const auto parameters{builder.build()};
                            return device.sendHttpCommand(COMMAND_SET_SCAN_OUTPUT_CONFIG, parameters).first;
                        },
                        std::forward<Args>(args)...);
            }

            std::optional<std::future<AsyncResultType>>
            execute(const HandleParameters &builder, std::chrono::milliseconds timeout) {
                const auto parameters{builder.build()};
                auto promise{std::make_shared<std::promise<AsyncResultType>>()};
                auto onComplete{[promise](const Device::R2000::AsyncResult &result) -> void {
                    promise->set_value(result.getRequestResult());
                }};

                if (device.asyncSendHttpCommand(COMMAND_SET_SCAN_OUTPUT_CONFIG, parameters, onComplete, timeout)) {
                    return {promise->get_future()};
                }
                return std::nullopt;
            }

        private:
            R2000 &device;
        };
    } // namespace internals::Parameters

    struct Commands {
        using GetProtocolInfoCommand =
        internals::Parameters::CommandExecutorImpl<internals::Parameters::CommandTags::GetProtocolInfo>;
        using ReleaseHandleCommand =
        internals::Parameters::CommandExecutorImpl<internals::Parameters::CommandTags::ReleaseHandle>;
        using StartScanCommand =
        internals::Parameters::CommandExecutorImpl<internals::Parameters::CommandTags::StartScanOutput>;
        using StopScanCommand =
        internals::Parameters::CommandExecutorImpl<internals::Parameters::CommandTags::StopScanOutput>;
        using FeedWatchdogCommand =
        internals::Parameters::CommandExecutorImpl<internals::Parameters::CommandTags::FeedWatchdog>;
        using GetParametersCommand =
        internals::Parameters::CommandExecutorImpl<internals::Parameters::CommandTags::GetParameters>;
        using FetchParametersCommand =
        internals::Parameters::CommandExecutorImpl<internals::Parameters::CommandTags::FetchParameterList>;
        using SetParametersCommand =
        internals::Parameters::CommandExecutorImpl<internals::Parameters::CommandTags::SetParameters>;
        using FactoryResetParametersCommand =
        internals::Parameters::CommandExecutorImpl<internals::Parameters::CommandTags::FactoryResetParameters>;
        using FactoryResetDeviceCommand =
        internals::Parameters::CommandExecutorImpl<internals::Parameters::CommandTags::FactoryResetDevice>;
        using RebootDeviceCommand =
        internals::Parameters::CommandExecutorImpl<internals::Parameters::CommandTags::RebootDevice>;
        using RequestUdpHandleCommand =
        internals::Parameters::CommandExecutorImpl<internals::Parameters::CommandTags::RequestUdpHandle>;
        using RequestTcpHandleCommand =
        internals::Parameters::CommandExecutorImpl<internals::Parameters::CommandTags::RequestTcpHandle>;
        using GetScanOutputConfigCommand =
        internals::Parameters::CommandExecutorImpl<internals::Parameters::CommandTags::GetScanOutputConfig>;
        using SetScanOutputConfigCommand =
        internals::Parameters::CommandExecutorImpl<internals::Parameters::CommandTags::SetScanOutputConfig>;
    };
}