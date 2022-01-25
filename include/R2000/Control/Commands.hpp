//
// Created by chazz on 12/01/2022.
//

#pragma once

#include "DeviceHandle.hpp"
#include "R2000/R2000.hpp"

namespace Device::Commands
{
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
namespace internals
{

struct ParametersChaining
{
    /**
     *
     * @tparam T
     * @param parameters
     * @param builder
     * @return
     */
    template <typename T> static constexpr inline void chain(Parameters::ParametersList& parameters, T& builder)
    {
        const auto builderParameters{builder.build()};
        parameters.insert(std::end(parameters), std::cbegin(builderParameters), std::cend(builderParameters));
    }

    /**
     *
     * @tparam T
     * @tparam Rest
     * @param parameters
     * @param builder
     * @param args
     * @return
     */
    template <typename T, typename... Rest>
    static constexpr inline void chain(Parameters::ParametersList& parameters, T& builder, Rest&&... args)
    {
        const auto builderParameters{builder.build()};
        parameters.insert(std::end(parameters), std::cbegin(builderParameters), std::cend(builderParameters));
        chain<Rest...>(parameters, std::forward<Rest>(args)...);
    }

    /**
     *
     * @tparam T
     * @param parameters
     * @param builder
     * @return
     */
    template <typename T> static constexpr inline void chain(Parameters::ParametersMap& parameters, T& builder)
    {
        const auto builderParameters{builder.build()};
        parameters.insert(std::cbegin(builderParameters), std::cend(builderParameters));
    }

    /**
     *
     * @tparam T
     * @tparam Rest
     * @param parameters
     * @param builder
     * @param args
     * @return
     */
    template <typename T, typename... Rest>
    static constexpr inline void chain(Parameters::ParametersMap& parameters, T& builder, Rest&&... args)
    {
        const auto builderParameters{builder.build()};
        parameters.insert(std::cbegin(builderParameters), std::cend(builderParameters));
        chain<Rest...>(parameters, std::forward<Rest>(args)...);
    }
};

template <typename ReturnType> struct ReturnTypeDependantExecutionSelector
{
    /**
     *
     * @tparam Execute
     * @tparam T
     * @param execute
     * @param builder
     * @return
     */
    template <typename Execute, typename T> static constexpr inline ReturnType resolve(Execute&& execute, T&& builder)
    {
        return execute(std::forward<T>(builder));
    }

    /**
     *
     * @tparam Execute
     * @tparam T
     * @tparam Rest
     * @param execute
     * @param builder
     * @param args
     * @return
     */
    template <typename Execute, typename T, typename... Rest>
    static constexpr inline ReturnType resolve(Execute&& execute, T&& builder, Rest&&... args)
    {
        return execute(std::forward<T>(builder)) &&
               resolve<Execute, Rest...>(std::forward<Execute>(execute), std::forward<Rest>(args)...);
    }
};

template <> struct ReturnTypeDependantExecutionSelector<void>
{
    /**
     *
     * @tparam Execute
     * @tparam T
     * @param execute
     * @param builder
     * @return
     */
    template <typename Execute, typename T> static constexpr inline void resolve(Execute&& execute, T&& builder)
    {
        execute(std::forward<T>(builder));
    }

    /**
     *
     * @tparam Execute
     * @tparam T
     * @tparam Rest
     * @param execute
     * @param builder
     * @param args
     * @return
     */
    template <typename Execute, typename T, typename... Rest>
    static constexpr inline void resolve(Execute&& execute, T&& builder, Rest&&... args)
    {
        execute(std::forward<T>(builder));
        resolve<Execute, Rest...>(std::forward<Execute>(execute), std::forward<Rest>(args)...);
    }
};

struct RecursiveExecutorHelper
{
    /**
     *
     * @tparam Execute
     * @tparam T
     * @param execute
     * @param builder
     * @return
     */
    template <typename Execute, typename T>
    static constexpr inline auto resolve(Execute&& execute, T&& builder)
        -> std::decay_t<typename std::invoke_result_t<Execute, T>>
    {
        return ReturnTypeDependantExecutionSelector<std::decay_t<typename std::invoke_result_t<Execute, T>>>::
            template resolve<Execute, T>(std::forward<Execute>(execute), std::forward<T>(builder));
    }

    /**
     *
     * @tparam Execute
     * @tparam T
     * @tparam Rest
     * @param execute
     * @param builder
     * @param args
     * @return
     */
    template <typename Execute, typename T, typename... Rest>
    static constexpr inline auto resolve(Execute&& execute, T&& builder, Rest&&... args)
        -> std::decay_t<typename std::invoke_result_t<Execute, T>>
    {
        return ReturnTypeDependantExecutionSelector<std::decay_t<typename std::invoke_result_t<Execute, T>>>::
            template resolve<Execute, T, Rest...>(std::forward<Execute>(execute), std::forward<T>(builder),
                                                  std::forward<Rest>(args)...);
    }
};

template <typename Command> struct CommandExecutorImpl
{
};

template <> struct CommandExecutorImpl<GetProtocolInfo>
{
    using ResultType = std::optional<std::pair<Parameters::ParametersMap, Parameters::ParametersList>>;
    using AsyncResultType = std::pair<Device::AsyncRequestResult, ResultType>;

    /**
     *
     * @param device
     */
    [[maybe_unused]] explicit CommandExecutorImpl(R2000& device) : device(device) {}

    /**
     *
     * @return
     */
    [[maybe_unused]] ResultType execute()
    {
        const auto result{device.sendHttpCommand(COMMAND_GET_PROTOCOL_INFO)};
        const auto status{result.first};
        const auto& propertyTree{result.second};
        if (!status)
            return std::nullopt;
        return extractInfoFromPropertyTree(propertyTree);
    }

    /**
     *
     * @param timeout
     * @return
     */
    [[maybe_unused]] std::optional<std::future<AsyncResultType>> asyncExecute(std::chrono::milliseconds timeout)
    {
        auto promise{std::make_shared<std::promise<AsyncResultType>>()};
        auto onComplete{[promise](const Device::AsyncResult& result) -> void
                        {
                            if (!result)
                            {
                                promise->set_value({result.getRequestResult(), std::nullopt});
                                return;
                            }
                            const auto parameters{extractInfoFromPropertyTree(result.getPropertyTree())};
                            promise->set_value({result.getRequestResult(), parameters});
                        }};
        if (device.asyncSendHttpCommand(COMMAND_GET_PROTOCOL_INFO, onComplete, timeout))
        {
            return {promise->get_future()};
        }
        return std::nullopt;
    }

    /**
     *
     * @param timeout
     * @param callable
     * @return
     */
    [[maybe_unused]] bool asyncExecute(std::chrono::milliseconds timeout,
                                       std::function<void(const AsyncResultType&)> callable)
    {
        return device.asyncSendHttpCommand(
            COMMAND_GET_PROTOCOL_INFO,
            [c = std::move(callable)](const Device::AsyncResult& result) -> void
            {
                if (!result)
                {
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
     *
     * @param propertyTree
     * @return
     */
    static ResultType extractInfoFromPropertyTree(const PropertyTree& propertyTree)
    {
        Parameters::ParametersList availableCommands{};
        Parameters::ParametersMap protocolInfo{};
        const auto commands{propertyTree.get_child_optional(PARAMETER_AVAILABLE_COMMANDS)};
        const auto protocolName{propertyTree.get_optional<std::string>(PARAMETER_PROTOCOL_NAME)};
        const auto protocolVersionMajor{propertyTree.get_optional<std::string>(PARAMETER_PROTOCOL_VERSION_MAJOR)};
        const auto protocolVersionMinor{propertyTree.get_optional<std::string>(PARAMETER_PROTOCOL_VERSION_MINOR)};

        if (!commands || !protocolName || !protocolVersionMajor || !protocolVersionMinor)
            return std::nullopt;
        for (const auto& name : *commands)
        {
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
    R2000& device;
};

template <> struct CommandExecutorImpl<GetProtocolVersion>
{
    using ResultType = Parameters::PFSDP;
    using AsyncResultType = std::pair<Device::AsyncRequestResult, ResultType>;

    /**
     * *
     * @param device
     */
    [[maybe_unused]] explicit CommandExecutorImpl(R2000& device) : device(device) {}

    /**
     *
     * @return
     */
    [[maybe_unused]] ResultType execute()
    {
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
     *
     * @param timeout
     * @return
     */
    [[maybe_unused]] std::future<AsyncResultType> asyncExecute(std::chrono::milliseconds timeout)
    {
        return std::async(std::launch::async,
                          [this, timeout]() -> AsyncResultType
                          {
                              CommandExecutorImpl<GetProtocolInfo> getProtocolInfo{device};
                              auto future{getProtocolInfo.asyncExecute(timeout)};
                              if (!future)
                              {
                                  return {Device::AsyncRequestResult::FAILED, Parameters::PFSDP::UNKNOWN};
                              }
                              future->wait();
                              auto result{future->get()};
                              if (result.first != Device::AsyncRequestResult::SUCCESS)
                              {
                                  return {result.first, Parameters::PFSDP::UNKNOWN};
                              }
                              const auto parameters{(result.second)->first};
                              const auto major{parameters.at(PARAMETER_PROTOCOL_VERSION_MAJOR)};
                              const auto minor{parameters.at(PARAMETER_PROTOCOL_VERSION_MINOR)};
                              return {result.first, protocolVersionFromString(major, minor)};
                          });
    }

    /**
     *
     * @param timeout
     * @param callable
     * @return
     */
    [[maybe_unused]] bool asyncExecute(std::chrono::milliseconds timeout,
                                       std::function<void(const AsyncResultType&)> callable)
    {
        CommandExecutorImpl<GetProtocolInfo> getProtocolInfo{device};
        return getProtocolInfo.asyncExecute(
            timeout,
            [c = std::move(callable)](const CommandExecutorImpl<GetProtocolInfo>::AsyncResultType& result) -> void
            {
                if (result.first != Device::AsyncRequestResult::SUCCESS)
                {
                    std::invoke(c, AsyncResultType{result.first, Parameters::PFSDP::UNKNOWN});
                    return;
                }
                const auto parameters{(result.second)->first};
                const auto major{parameters.at(PARAMETER_PROTOCOL_VERSION_MAJOR)};
                const auto minor{parameters.at(PARAMETER_PROTOCOL_VERSION_MINOR)};
                std::invoke(c, AsyncResultType{result.first, protocolVersionFromString(major, minor)});
            });
    }

private:
    /**
     *
     * @tparam E
     * @param enumerator
     * @return
     */
    template <typename E> static constexpr auto underlyingType(E enumerator)
    {
        return static_cast<std::underlying_type_t<E>>(enumerator);
    }

    /**
     *
     * @param major
     * @param minor
     * @return
     */
    static Device::Parameters::PFSDP protocolVersionFromString(const std::string& major, const std::string& minor)
    {
        const auto numericMajor{std::stoul(major)};
        const auto numericMinor{std::stoul(minor)};
        const auto numericVersion{numericMajor * 100 + numericMinor};
        switch (numericVersion)
        {
        case underlyingType(Parameters::PFSDP::V100):
        {
            return Parameters::PFSDP::V100;
        }
        case underlyingType(Parameters::PFSDP::V101):
        {
            return Parameters::PFSDP::V101;
        }
        case underlyingType(Parameters::PFSDP::V102):
        {
            return Parameters::PFSDP::V102;
        }
        case underlyingType(Parameters::PFSDP::V103):
        {
            return Parameters::PFSDP::V103;
        }
        case underlyingType(Parameters::PFSDP::V104):
        {
            return Parameters::PFSDP::V104;
        }
        default:
        {
            if (numericVersion > underlyingType(Parameters::PFSDP::V104))
                return Parameters::PFSDP::ABOVE_V104;
        }
        }
        return Parameters::PFSDP::UNKNOWN;
    }

private:
    R2000& device;
};

template <> struct CommandExecutorImpl<ReleaseHandle>
{
    using ResultType = bool;
    using AsyncResultType = AsyncRequestResult;

    /**
     *
     * @param device
     */
    [[maybe_unused]] explicit CommandExecutorImpl(R2000& device) : device(device) {}

    /**
     *
     * @tparam Args
     * @param args
     * @return
     */
    template <typename... Args> [[maybe_unused]] inline ResultType execute(Args&&... args)
    {
        static_assert(std::conjunction_v<std::is_base_of<Device::DeviceHandle, typename std::decay_t<Args>>...>,
                      "All the provided handle must be of type DeviceHandle");
        return RecursiveExecutorHelper::resolve(
            [this](const auto& handle)
            { return device.sendHttpCommand(COMMAND_RELEASE_HANDLE, PARAMETER_NAME_HANDLE, handle.value).first; },
            std::forward<Args>(args)...);
    }

    /**
     *
     * @param handle
     * @param timeout
     * @return
     */
    [[maybe_unused]] std::optional<std::future<AsyncResultType>> asyncExecute(const Device::DeviceHandle& handle,
                                                                              std::chrono::milliseconds timeout)
    {
        auto promise{std::make_shared<std::promise<AsyncResultType>>()};
        auto onComplete{[promise](const Device::AsyncResult& result) -> void
                        { promise->set_value(result.getRequestResult()); }};
        if (device.asyncSendHttpCommand(COMMAND_RELEASE_HANDLE, {{PARAMETER_NAME_HANDLE, handle.getValue()}},
                                        onComplete, timeout))
        {
            return {promise->get_future()};
        }
        return std::nullopt;
    }

    /**
     *
     * @param handle
     * @param timeout
     * @param callable
     * @return
     */
    [[maybe_unused]] bool asyncExecute(const Device::DeviceHandle& handle, std::chrono::milliseconds timeout,
                                       std::function<void(const AsyncResultType&)> callable)
    {
        return device.asyncSendHttpCommand(
            COMMAND_RELEASE_HANDLE, {{PARAMETER_NAME_HANDLE, handle.getValue()}},
            [c = std::move(callable)](const Device::AsyncResult& result) -> void
            {
                if (!result)
                {
                    std::invoke(c, AsyncResultType{result.getRequestResult()});
                    return;
                }
                std::invoke(c, AsyncResultType{result.getRequestResult()});
            },
            timeout);
    }

private:
    R2000& device;
};

template <> struct CommandExecutorImpl<StartScanOutput>
{
    using ResultType = bool;
    using AsyncResultType = Device::AsyncRequestResult;

    /**
     *
     * @param device
     */
    [[maybe_unused]] explicit CommandExecutorImpl(R2000& device) : device(device) {}

    /**
     *
     * @tparam Args
     * @param args
     * @return
     */
    template <typename... Args> [[maybe_unused]] inline ResultType execute(Args&&... args)
    {
        static_assert(std::conjunction_v<std::is_base_of<Device::DeviceHandle, typename std::decay_t<Args>>...>,
                      "All the provided handle must be of type DeviceHandle");
        return RecursiveExecutorHelper::resolve(
            [this](const auto& handle)
            { return device.sendHttpCommand(COMMAND_START_SCAN_OUTPUT, PARAMETER_NAME_HANDLE, handle.value).first; },
            std::forward<Args>(args)...);
    }

    /**
     *
     * @param handle
     * @param timeout
     * @return
     */
    [[maybe_unused]] std::optional<std::future<AsyncResultType>> asyncExecute(const Device::DeviceHandle& handle,
                                                                              std::chrono::milliseconds timeout)
    {
        auto promise{std::make_shared<std::promise<AsyncResultType>>()};
        auto onComplete{[promise](const Device::AsyncResult& result) -> void
                        { promise->set_value(result.getRequestResult()); }};
        if (device.asyncSendHttpCommand(COMMAND_START_SCAN_OUTPUT, {{PARAMETER_NAME_HANDLE, handle.getValue()}},
                                        onComplete, timeout))
        {
            return {promise->get_future()};
        }
        return std::nullopt;
    }

    /**
     *
     * @param handle
     * @param timeout
     * @param callable
     * @return
     */
    [[maybe_unused]] bool asyncExecute(const Device::DeviceHandle& handle, std::chrono::milliseconds timeout,
                                       std::function<void(const AsyncResultType&)> callable)
    {
        return device.asyncSendHttpCommand(
            COMMAND_START_SCAN_OUTPUT, {{PARAMETER_NAME_HANDLE, handle.getValue()}},
            [c = std::move(callable)](const Device::AsyncResult& result) -> void
            {
                if (!result)
                {
                    std::invoke(c, AsyncResultType{result.getRequestResult()});
                    return;
                }
                std::invoke(c, AsyncResultType{result.getRequestResult()});
            },
            timeout);
    }

private:
    R2000& device;
};

template <> struct CommandExecutorImpl<StopScanOutput>
{
    using ResultType = bool;
    using AsyncResultType = Device::AsyncRequestResult;

    /**
     *
     * @param device
     */
    [[maybe_unused]] explicit CommandExecutorImpl(R2000& device) : device(device) {}

    template <typename... Args> [[maybe_unused]] inline ResultType execute(Args&&... args)
    {
        static_assert(std::conjunction_v<std::is_base_of<Device::DeviceHandle, typename std::decay_t<Args>>...>,
                      "All the provided handle must be of type DeviceHandle");
        return RecursiveExecutorHelper::resolve(
            [this](const auto& handle)
            { return device.sendHttpCommand(COMMAND_STOP_SCAN_OUTPUT, PARAMETER_NAME_HANDLE, handle.value).first; },
            std::forward<Args>(args)...);
    }

    /**
     *
     * @param handle
     * @param timeout
     * @return
     */
    [[maybe_unused]] std::optional<std::future<AsyncResultType>> asyncExecute(const Device::DeviceHandle& handle,
                                                                              std::chrono::milliseconds timeout)
    {
        auto promise{std::make_shared<std::promise<AsyncResultType>>()};
        auto onComplete{[promise](const Device::AsyncResult& result) -> void
                        { promise->set_value(result.getRequestResult()); }};
        if (device.asyncSendHttpCommand(COMMAND_STOP_SCAN_OUTPUT, {{PARAMETER_NAME_HANDLE, handle.getValue()}},
                                        onComplete, timeout))
        {
            return {promise->get_future()};
        }
        return std::nullopt;
    }

    /**
     *
     * @param handle
     * @param timeout
     * @param callable
     * @return
     */
    [[maybe_unused]] bool asyncExecute(const Device::DeviceHandle& handle, std::chrono::milliseconds timeout,
                                       std::function<void(const AsyncResultType&)> callable)
    {
        return device.asyncSendHttpCommand(
            COMMAND_STOP_SCAN_OUTPUT, {{PARAMETER_NAME_HANDLE, handle.getValue()}},
            [c = std::move(callable)](const Device::AsyncResult& result) -> void
            {
                if (!result)
                {
                    std::invoke(c, AsyncResultType{result.getRequestResult()});
                    return;
                }
                std::invoke(c, AsyncResultType{result.getRequestResult()});
            },
            timeout);
    }

private:
    R2000& device;
};

template <> struct CommandExecutorImpl<FeedWatchdog>
{
    using ResultType = bool;
    using AsyncResultType = Device::AsyncRequestResult;

    /**
     *
     * @param device
     */
    [[maybe_unused]] explicit CommandExecutorImpl(R2000& device) : device(device) {}

    /**
     *
     * @tparam Args
     * @param args
     * @return
     */
    template <typename... Args> [[maybe_unused]] inline ResultType execute(Args&&... args)
    {
        static_assert(std::conjunction_v<std::is_base_of<Device::DeviceHandle, typename std::decay_t<Args>>...>,
                      "All the provided handle must be of type DeviceHandle");
        return RecursiveExecutorHelper::resolve(
            [this](const auto& handle)
            { return device.sendHttpCommand(COMMAND_FEED_WATCHDOG, PARAMETER_NAME_HANDLE, handle.value).first; },
            std::forward<Args>(args)...);
    }

    /**
     *
     * @param handle
     * @param timeout
     * @return
     */
    [[maybe_unused]] std::optional<std::future<AsyncResultType>> asyncExecute(const Device::DeviceHandle& handle,
                                                                              std::chrono::milliseconds timeout)
    {
        auto promise{std::make_shared<std::promise<AsyncResultType>>()};
        auto onComplete{[promise](const Device::AsyncResult& result) -> void
                        { promise->set_value(result.getRequestResult()); }};
        if (device.asyncSendHttpCommand(COMMAND_FEED_WATCHDOG, {{PARAMETER_NAME_HANDLE, handle.getValue()}}, onComplete,
                                        timeout))
        {
            return {promise->get_future()};
        }
        return std::nullopt;
    }

    /**
     *
     * @param handle
     * @param timeout
     * @param callable
     * @return
     */
    [[maybe_unused]] bool asyncExecute(const Device::DeviceHandle& handle, std::chrono::milliseconds timeout,
                                       std::function<void(const AsyncResultType&)> callable)
    {
        return device.asyncSendHttpCommand(
            COMMAND_FEED_WATCHDOG, {{PARAMETER_NAME_HANDLE, handle.getValue()}},
            [c = std::move(callable)](const Device::AsyncResult& result) -> void
            {
                if (!result)
                {
                    std::invoke(c, AsyncResultType{result.getRequestResult()});
                    return;
                }
                std::invoke(c, AsyncResultType{result.getRequestResult()});
            },
            timeout);
    }

private:
    R2000& device;
};

template <> struct CommandExecutorImpl<GetParameters>
{
    using ResultType = std::optional<Parameters::ParametersMap>;
    using AsyncResultType = std::pair<Device::AsyncRequestResult, ResultType>;

    /**
     *
     * @param device
     */
    [[maybe_unused]] explicit CommandExecutorImpl(R2000& device) : device(device) {}

    /**
     *
     * @tparam Args
     * @param args
     * @return
     */
    template <typename... Args> [[maybe_unused]] inline ResultType execute(Args&&... args)
    {
        static_assert(std::conjunction_v<std::is_same<Parameters::ReadOnlyRequestBuilder, Args>...>,
                      "All the provided handle must be of type ReadOnlyRequestBuilder");
        Parameters::ParametersList parameters{};
        ParametersChaining::template chain(parameters, std::forward<Args>(args)...);
        const auto parametersListAsString{chainAndConvertParametersToString(std::forward<Args>(args)...)};
        const auto result{
            device.sendHttpCommand(COMMAND_GET_PARAMETER, PARAMETER_NAME_LIST, parametersListAsString.first)};
        const auto status{result.first};
        if (!status)
            return std::nullopt;
        return extractParametersValues(parameters, result.second);
    }

    /**
     *
     * @tparam Args
     * @param timeout
     * @param args
     * @return
     */
    template <typename... Args>
    [[maybe_unused]] std::optional<std::future<AsyncResultType>> asyncExecuteFuture(std::chrono::milliseconds timeout,
                                                                                    Args&&... args)
    {
        static_assert(
            std::conjunction_v<std::is_base_of<Parameters::ReadOnlyRequestBuilder, typename std::decay_t<Args>>...>,
            "All the provided handle must be of type ReadOnlyRequestBuilder");
        const auto chainedParameters{chainAndConvertParametersToString(std::forward<Args>(args)...)};
        auto promise{std::make_shared<std::promise<AsyncResultType>>()};
        auto onComplete{
            [promise, parameters = std::move(chainedParameters.second)](const Device::AsyncResult& result) -> void
            {
                if (!result)
                {
                    promise->set_value({result.getRequestResult(), std::nullopt});
                    return;
                }
                const auto receivedParameters{extractParametersValues(parameters, result.getPropertyTree())};
                promise->set_value({result.getRequestResult(), receivedParameters});
            }};
        if (device.asyncSendHttpCommand(COMMAND_GET_PARAMETER, {{PARAMETER_NAME_LIST, chainedParameters.first}},
                                        onComplete, timeout))
        {
            return {promise->get_future()};
        }
        return std::nullopt;
    }

    /**
     *
     * @tparam Args
     * @param timeout
     * @param callable
     * @param args
     * @return
     */
    template <typename... Args>
    [[maybe_unused]] bool asyncExecute(std::chrono::milliseconds timeout,
                                       std::function<void(const AsyncResultType&)> callable, Args&&... args)
    {
        static_assert(
            std::conjunction_v<std::is_base_of<Parameters::ReadOnlyRequestBuilder, typename std::decay_t<Args>>...>,
            "All the provided handle must be of type ReadOnlyRequestBuilder");
        const auto chainedParameters{chainAndConvertParametersToString(std::forward<Args>(args)...)};
        return device.asyncSendHttpCommand(
            COMMAND_GET_PARAMETER, {{PARAMETER_NAME_LIST, chainedParameters}},
            [c = std::move(callable),
             parameters = std::move(chainedParameters.second)](const Device::AsyncResult& result) -> void
            {
                if (!result)
                {
                    std::invoke(c, AsyncResultType{result.getRequestResult(), std::nullopt});
                    return;
                }
                const auto receivedParameters{extractParametersValues(parameters, result.getPropertyTree())};
                std::invoke(c, AsyncResultType{result.getRequestResult(), receivedParameters});
            },
            timeout);
    }

private:
    /**
     *
     * @param list
     * @param propertyTree
     * @return
     */
    static ResultType extractParametersValues(const Parameters::ParametersList& list, const PropertyTree& propertyTree)
    {
        Parameters::ParametersMap keysValues{};
        for (const auto& name : list)
        {
            auto value{propertyTree.get_optional<std::string>(name)};
            if (!value)
            {
                keysValues.insert(std::make_pair(name, ""));
                continue;
            }
            keysValues.insert(std::make_pair(name, *value));
        }
        return {keysValues};
    }

private:
    /**
     *
     * @tparam Args
     * @param args
     * @return
     */
    template <typename... Args>
    std::pair<std::string, Parameters::ParametersList> chainAndConvertParametersToString(Args&&... args)
    {
        Parameters::ParametersList parameters{};
        ParametersChaining::template chain(parameters, std::forward<Args>(args)...);
        std::string parametersAsString{};
        for (const auto& names : parameters)
            parametersAsString += (names + ";");
        parametersAsString.pop_back();
        return {parametersAsString, parameters};
    }

private:
    R2000& device;
};

template <> struct CommandExecutorImpl<SetParameters>
{
    using ResultType = bool;
    using AsyncResultType = Device::AsyncRequestResult;

    /**
     *
     * @param device
     */
    [[maybe_unused]] explicit CommandExecutorImpl(R2000& device) : device(device) {}

    /**
     *
     * @tparam Args
     * @param args
     * @return
     */
    template <typename... Args> [[maybe_unused]] inline ResultType execute(Args&&... args)
    {
        static_assert(
            std::conjunction_v<std::is_base_of<Parameters::ReadWriteRequestBuilder, typename std::decay_t<Args>>...>,
            "All the provided handle must be of type ReadWriteRequestBuilder");
        Parameters::ParametersMap parameters{};
        ParametersChaining::template chain(parameters, std::forward<Args>(args)...);
        return device.sendHttpCommand(COMMAND_SET_PARAMETER, parameters).first;
    }

    /**
     *
     * @tparam Args
     * @param timeout
     * @param args
     * @return
     */
    template <typename... Args>
    [[maybe_unused]] std::optional<std::future<AsyncResultType>> asyncExecute(std::chrono::milliseconds timeout,
                                                                              Args&&... args)
    {
        static_assert(
            std::conjunction_v<std::is_base_of<Parameters::ReadWriteRequestBuilder, typename std::decay_t<Args>>...>,
            "All the provided handle must be of type ReadWriteRequestBuilder");
        Parameters::ParametersMap parameters{};
        ParametersChaining::template chain(parameters, std::forward<Args>(args)...);
        auto promise{std::make_shared<std::promise<AsyncResultType>>()};
        auto onComplete{[promise](const Device::AsyncResult& result) -> void
                        { promise->set_value(result.getRequestResult()); }};
        if (device.asyncSendHttpCommand(COMMAND_SET_PARAMETER, parameters, onComplete, timeout))
        {
            return {promise->get_future()};
        }
        return std::nullopt;
    }

    /**
     *
     * @tparam Args
     * @param timeout
     * @param callable
     * @param args
     * @return
     */
    template <typename... Args>
    [[maybe_unused]] bool asyncExecute(std::chrono::milliseconds timeout,
                                       std::function<void(const AsyncResultType&)> callable, Args&&... args)
    {
        static_assert(
            std::conjunction_v<std::is_base_of<Parameters::ReadWriteRequestBuilder, typename std::decay_t<Args>>...>,
            "All the provided handle must be of type ReadWriteRequestBuilder");
        Parameters::ParametersMap parameters{};
        ParametersChaining::template chain(parameters, std::forward<Args>(args)...);
        return device.asyncSendHttpCommand(
            COMMAND_SET_PARAMETER, parameters,
            [c = std::move(callable)](const Device::AsyncResult& result) -> void
            { std::invoke(c, result.getRequestResult()); },
            timeout);
    }

private:
    R2000& device;
};

template <> struct CommandExecutorImpl<FetchParameterList>
{
    using ResultType = std::optional<Parameters::ParametersList>;
    using AsyncResultType = std::pair<Device::AsyncRequestResult, ResultType>;

    /**
     *
     * @param device
     */
    [[maybe_unused]] explicit CommandExecutorImpl(R2000& device) : device(device) {}

    /**
     *
     * @return
     */
    [[maybe_unused]] ResultType execute()
    {
        const auto result{device.sendHttpCommand(COMMAND_LIST_PARAMETERS)};
        const auto status{result.first};
        const auto& propertyTree{result.second};
        if (!status)
            return std::nullopt;
        return extractParametersFromPropertyTree(propertyTree);
    }

    /**
     *
     * @param timeout
     * @return
     */
    [[maybe_unused]] std::optional<std::future<AsyncResultType>> asyncExecute(std::chrono::milliseconds timeout)
    {
        auto promise{std::make_shared<std::promise<AsyncResultType>>()};
        auto onComplete{[promise](const Device::AsyncResult& result) -> void
                        {
                            if (!result)
                            {
                                promise->set_value({result.getRequestResult(), std::nullopt});
                                return;
                            }
                            const auto parameters{extractParametersFromPropertyTree(result.getPropertyTree())};
                            promise->set_value({result.getRequestResult(), parameters});
                        }};
        if (device.asyncSendHttpCommand(COMMAND_LIST_PARAMETERS, onComplete, timeout))
        {
            return {promise->get_future()};
        }
        return std::nullopt;
    }

    /**
     *
     * @param timeout
     * @param callable
     * @return
     */
    [[maybe_unused]] bool asyncExecute(std::chrono::milliseconds timeout,
                                       std::function<void(const AsyncResultType&)> callable)
    {
        return device.asyncSendHttpCommand(
            COMMAND_LIST_PARAMETERS,
            [c = std::move(callable)](const Device::AsyncResult& result) -> void
            {
                if (!result)
                {
                    std::invoke(c, AsyncResultType{result.getRequestResult(), std::nullopt});
                    return;
                }
                const auto parameters{extractParametersFromPropertyTree(result.getPropertyTree())};
                std::invoke(c, AsyncResultType{result.getRequestResult(), parameters});
            },
            timeout);
    }

private:
    /**
     *
     * @param propertyTree
     * @return
     */
    static std::optional<Parameters::ParametersList> extractParametersFromPropertyTree(const PropertyTree& propertyTree)
    {
        Parameters::ParametersList parameterList{};
        const auto list{propertyTree.get_child_optional("parameters")};
        if (!list)
            return std::nullopt;
        for (const auto& name : *list)
        {
            const auto parameterName{name.second.get<std::string>("")};
            if (parameterName.empty())
                continue;
            parameterList.emplace_back(parameterName);
        }
        return std::make_optional(parameterList);
    }

private:
    R2000& device;
};

template <> struct CommandExecutorImpl<FactoryResetParameters>
{
    using ResultType = bool;
    using AsyncResultType = Device::AsyncRequestResult;

    /**
     *
     * @param device
     */
    [[maybe_unused]] explicit CommandExecutorImpl(R2000& device) : device(device) {}

    /**
     *
     * @tparam Args
     * @param args
     * @return
     */
    template <typename... Args> [[maybe_unused]] inline ResultType execute(Args&&... args)
    {
        static_assert(
            std::conjunction_v<std::is_base_of<Parameters::ReadWriteRequestBuilder, typename std::decay_t<Args>>...>,
            "All the provided handle must be of type ReadWriteRequestBuilder");
        const auto params{chainAndConvertParametersToString(std::forward<Args>(args)...)};
        return device.sendHttpCommand(COMMAND_RESET_PARAMETERS, PARAMETER_NAME_LIST, params).first;
    }

    /**
     *
     * @tparam Args
     * @param timeout
     * @param args
     * @return
     */
    template <typename... Args>
    [[maybe_unused]] std::optional<std::future<AsyncResultType>> asyncExecute(std::chrono::milliseconds timeout,
                                                                              Args&&... args)
    {
        static_assert(
            std::conjunction_v<std::is_base_of<Parameters::ReadWriteRequestBuilder, typename std::decay_t<Args>>...>,
            "All the provided handle must be of type ReadWriteRequestBuilder");
        const auto parameters{chainAndConvertParametersToString(std::forward<Args>(args)...)};
        auto promise{std::make_shared<std::promise<AsyncResultType>>()};
        auto onComplete{[promise](const Device::AsyncResult& result) -> void
                        { promise->set_value(result.getRequestResult()); }};
        if (device.asyncSendHttpCommand(COMMAND_RESET_PARAMETERS, {{PARAMETER_NAME_LIST, parameters}}, onComplete,
                                        timeout))
        {
            return {promise->get_future()};
        }
        return std::nullopt;
    }

    /**
     *
     * @tparam Args
     * @param timeout
     * @param callable
     * @param args
     * @return
     */
    template <typename... Args>
    [[maybe_unused]] bool asyncExecute(std::chrono::milliseconds timeout,
                                       std::function<void(const AsyncResultType&)> callable, Args&&... args)
    {
        static_assert(
            std::conjunction_v<std::is_base_of<Parameters::ReadWriteRequestBuilder, typename std::decay_t<Args>>...>,
            "All the provided handle must be of type ReadWriteRequestBuilder");
        const auto parameters{chainAndConvertParametersToString(std::forward<Args>(args)...)};
        return device.asyncSendHttpCommand(
            COMMAND_RESET_PARAMETERS, {{PARAMETER_NAME_LIST, parameters}},
            [c = std::move(callable)](const Device::AsyncResult& result) -> void
            { std::invoke(c, result.getRequestResult()); },
            timeout);
    }

private:
    /**
     *
     * @tparam Args
     * @param args
     * @return
     */
    template <typename... Args> std::string chainAndConvertParametersToString(Args&&... args)
    {
        Parameters::ParametersList parameters{};
        ParametersChaining::template chain(parameters, std::forward<Args>(args)...);
        std::string parametersList{};
        for (const auto& names : parameters)
            parametersList += (names + ";");
        parametersList.pop_back();
        return parametersList;
    }

private:
    R2000& device;
};

template <> struct CommandExecutorImpl<FactoryResetDevice>
{
    using ResultType = bool;
    using AsyncResultType = Device::AsyncRequestResult;

    /**
     *
     * @param device
     */
    [[maybe_unused]] explicit CommandExecutorImpl(R2000& device) : device(device) {}

    /**
     *
     * @return
     */
    [[maybe_unused]] inline ResultType execute() { return device.sendHttpCommand(COMMAND_FACTORY_RESET).first; }

    /**
     *
     * @param timeout
     * @return
     */
    [[maybe_unused]] std::optional<std::future<AsyncResultType>> asyncExecute(std::chrono::milliseconds timeout)
    {
        auto promise{std::make_shared<std::promise<AsyncResultType>>()};
        auto onComplete{[promise](const Device::AsyncResult& result) -> void
                        { promise->set_value(result.getRequestResult()); }};
        if (device.asyncSendHttpCommand(COMMAND_FACTORY_RESET, onComplete, timeout))
        {
            return {promise->get_future()};
        }
        return std::nullopt;
    }

    /**
     *
     * @param timeout
     * @param callable
     * @return
     */
    [[maybe_unused]] bool asyncExecute(std::chrono::milliseconds timeout,
                                       std::function<void(const AsyncResultType&)> callable)
    {
        return device.asyncSendHttpCommand(
            COMMAND_FACTORY_RESET,
            [c = std::move(callable)](const Device::AsyncResult& result) -> void
            { std::invoke(c, result.getRequestResult()); },
            timeout);
    }

private:
    R2000& device;
};

template <> struct CommandExecutorImpl<RebootDevice>
{
    using ResultType = bool;
    using AsyncResultType = Device::AsyncRequestResult;

    /**
     *
     * @param device
     */
    [[maybe_unused]] explicit CommandExecutorImpl(R2000& device) : device(device) {}

    /**
     *
     * @return
     */
    [[maybe_unused]] ResultType execute() { return device.sendHttpCommand(COMMAND_REBOOT_DEVICE).first; }

    /**
     *
     * @param timeout
     * @return
     */
    [[maybe_unused]] std::optional<std::future<AsyncResultType>> asyncExecute(std::chrono::milliseconds timeout)
    {
        auto promise{std::make_shared<std::promise<AsyncResultType>>()};
        auto onComplete{[promise](const Device::AsyncResult& result) -> void
                        { promise->set_value(result.getRequestResult()); }};
        if (device.asyncSendHttpCommand(COMMAND_REBOOT_DEVICE, onComplete, timeout))
        {
            return {promise->get_future()};
        }
        return std::nullopt;
    }

    /**
     *
     * @param timeout
     * @param callable
     * @return
     */
    [[maybe_unused]] bool asyncExecute(std::chrono::milliseconds timeout,
                                       std::function<void(const AsyncResultType&)> callable)
    {
        return device.asyncSendHttpCommand(
            COMMAND_REBOOT_DEVICE,
            [c = std::move(callable)](const Device::AsyncResult& result) -> void
            { std::invoke(c, AsyncResultType{result.getRequestResult()}); },
            timeout);
    }

private:
    R2000& device;
};

template <> struct CommandExecutorImpl<RequestTcpHandle>
{
    using ResultType = std::optional<std::pair<int, Device::HandleType>>;
    using AsyncResultType = std::pair<Device::AsyncRequestResult, ResultType>;

    /**
     *
     * @param device
     */
    [[maybe_unused]] explicit CommandExecutorImpl(R2000& device) : device(device) {}

    /**
     *
     * @param builder
     * @return
     */
    [[maybe_unused]] ResultType execute(const Parameters::ReadWriteParameters::TcpHandle& builder)
    {
        const auto parameters{builder.build()};
        const auto result{device.sendHttpCommand(COMMAND_REQUEST_TCP_HANDLE, parameters)};
        const auto status{result.first};
        const auto& propertyTree{result.second};
        if (!status)
            return std::nullopt;
        return extractHandleInfoFromPropertyTree(propertyTree);
    }

    /**
     *
     * @param builder
     * @param timeout
     * @return
     */
    [[maybe_unused]] std::optional<std::future<AsyncResultType>>
    asyncExecute(const Parameters::ReadWriteParameters::TcpHandle& builder, std::chrono::milliseconds timeout)
    {
        const auto parameters{builder.build()};
        auto promise{std::make_shared<std::promise<AsyncResultType>>()};
        auto onComplete{[promise](const Device::AsyncResult& result) -> void
                        {
                            if (!result)
                            {
                                promise->set_value({result.getRequestResult(), std::nullopt});
                                return;
                            }
                            const auto handle{extractHandleInfoFromPropertyTree(result.getPropertyTree())};
                            promise->set_value({result.getRequestResult(), handle});
                        }};
        if (device.asyncSendHttpCommand(COMMAND_REQUEST_TCP_HANDLE, parameters, onComplete, timeout))
        {
            return {promise->get_future()};
        }
        return std::nullopt;
    }

    /**
     *
     * @param builder
     * @param timeout
     * @param callable
     * @return
     */
    [[maybe_unused]] bool asyncExecute(const Parameters::ReadWriteParameters::TcpHandle& builder,
                                       std::chrono::milliseconds timeout,
                                       std::function<void(const AsyncResultType&)> callable)
    {
        const auto parameters{builder.build()};
        return device.asyncSendHttpCommand(
            COMMAND_REQUEST_TCP_HANDLE, parameters,
            [c = std::move(callable)](const Device::AsyncResult& result) -> void
            {
                if (!result)
                {
                    std::invoke(c, AsyncResultType{result.getRequestResult(), std::nullopt});
                    return;
                }
                const auto handle{extractHandleInfoFromPropertyTree(result.getPropertyTree())};
                std::invoke(c, AsyncResultType{result.getRequestResult(), handle});
            },
            timeout);
    }

private:
    /**
     *
     * @param propertyTree
     * @return
     */
    static ResultType extractHandleInfoFromPropertyTree(const PropertyTree& propertyTree)
    {
        const auto port{propertyTree.get_optional<int>("port")};
        const auto handle{propertyTree.get_optional<HandleType>("handle")};
        if (!port || !handle)
            return std::nullopt;
        return {std::make_pair(*port, *handle)};
    }

private:
    R2000& device;
};

template <> struct CommandExecutorImpl<RequestUdpHandle>
{
    using ResultType = std::optional<Device::HandleType>;
    using AsyncResultType = std::pair<Device::AsyncRequestResult, ResultType>;

    /**
     *
     * @param device
     */
    [[maybe_unused]] explicit CommandExecutorImpl(R2000& device) : device(device) {}

    /**
     *
     * @param builder
     * @return
     */
    [[maybe_unused]] ResultType execute(const Parameters::ReadWriteParameters::UdpHandle& builder)
    {
        const auto parameters{builder.build()};
        const auto result{device.sendHttpCommand(COMMAND_REQUEST_UDP_HANDLE, parameters)};
        const auto status{result.first};
        const auto& propertyTree{result.second};
        if (!status)
            return std::nullopt;
        return extractHandleInfoFromPropertyTree(propertyTree);
    }

    /**
     *
     * @param builder
     * @param timeout
     * @return
     */
    [[maybe_unused]] std::optional<std::future<AsyncResultType>>
    asyncExecute(const Parameters::ReadWriteParameters::UdpHandle& builder, std::chrono::milliseconds timeout)
    {
        const auto parameters{builder.build()};
        auto promise{std::make_shared<std::promise<AsyncResultType>>()};
        auto onComplete{[promise](const Device::AsyncResult& result) -> void
                        {
                            if (!result)
                            {
                                promise->set_value({result.getRequestResult(), std::nullopt});
                                return;
                            }
                            const auto handle{extractHandleInfoFromPropertyTree(result.getPropertyTree())};
                            promise->set_value({result.getRequestResult(), handle});
                        }};
        if (device.asyncSendHttpCommand(COMMAND_REQUEST_UDP_HANDLE, parameters, onComplete, timeout))
        {
            return {promise->get_future()};
        }
        return std::nullopt;
    }

    /**
     *
     * @param builder
     * @param timeout
     * @param callable
     * @return
     */
    [[maybe_unused]] bool asyncExecute(const Parameters::ReadWriteParameters::UdpHandle& builder,
                                       std::chrono::milliseconds timeout,
                                       std::function<void(const AsyncResultType&)> callable)
    {
        const auto parameters{builder.build()};
        return device.asyncSendHttpCommand(
            COMMAND_REQUEST_UDP_HANDLE, parameters,
            [c = std::move(callable)](const Device::AsyncResult& result) -> void
            {
                if (!result)
                {
                    std::invoke(c, AsyncResultType{result.getRequestResult(), std::nullopt});
                    return;
                }
                const auto handle{extractHandleInfoFromPropertyTree(result.getPropertyTree())};
                std::invoke(c, AsyncResultType{result.getRequestResult(), handle});
            },
            timeout);
    }

private:
    /**
     *
     * @param propertyTree
     * @return
     */
    static ResultType extractHandleInfoFromPropertyTree(const PropertyTree& propertyTree)
    {
        const auto handle{propertyTree.get_optional<std::string>("handle")};
        if (!handle)
            return std::nullopt;
        return {*handle};
    }

private:
    R2000& device;
};

template <> struct CommandExecutorImpl<GetScanOutputConfig>
{
    using ResultType = std::vector<std::optional<Parameters::ParametersMap>>;
    using AsyncResultType = std::pair<Device::AsyncRequestResult, std::optional<Parameters::ParametersMap>>;

    /**
     *
     * @param device
     */
    [[maybe_unused]] explicit CommandExecutorImpl(R2000& device) : device(device) {}

    /**
     *
     * @tparam Args
     * @param args
     * @return
     */
    template <typename... Args> [[maybe_unused]] ResultType execute(Args&&... args)
    {
        static_assert(std::conjunction_v<std::is_base_of<Device::DeviceHandle, typename std::decay_t<Args>>...>,
                      "All the provided handle must be of type DeviceHandle");
        std::vector<std::optional<Parameters::ParametersMap>> configurations;
        RecursiveExecutorHelper::resolve(
            [this, &configurations](const auto& handle)
            {
                const auto result{
                    device.sendHttpCommand(COMMAND_GET_SCAN_OUTPUT_CONFIG, PARAMETER_NAME_HANDLE, handle.value)};
                const auto status{result.first};
                if (!status)
                {
                    configurations.template emplace_back(std::nullopt);
                    return;
                }
                configurations.template emplace_back(getScanOutputConfig(result.second));
            },
            std::forward<Args>(args)...);
        return configurations;
    }

    /**
     *
     * @param handle
     * @param timeout
     * @return
     */
    [[maybe_unused]] std::optional<std::future<AsyncResultType>> asyncExecute(const Device::DeviceHandle& handle,
                                                                              std::chrono::milliseconds timeout)
    {
        auto promise{std::make_shared<std::promise<AsyncResultType>>()};
        auto onComplete{[promise](const Device::AsyncResult& result) -> void
                        {
                            if (!result)
                            {
                                promise->set_value({result.getRequestResult(), std::nullopt});
                                return;
                            }
                            const auto& propertyTree{result.getPropertyTree()};
                            promise->set_value({result.getRequestResult(), getScanOutputConfig(propertyTree)});
                        }};

        if (device.asyncSendHttpCommand(COMMAND_GET_SCAN_OUTPUT_CONFIG, {{PARAMETER_NAME_HANDLE, handle.getValue()}},
                                        onComplete, timeout))
        {
            return {promise->get_future()};
        }
        return std::nullopt;
    }

    /**
     *
     * @param handle
     * @param timeout
     * @param callable
     * @return
     */
    [[maybe_unused]] bool asyncExecute(const Device::DeviceHandle& handle, std::chrono::milliseconds timeout,
                                       std::function<void(const AsyncResultType&)> callable)
    {
        return device.asyncSendHttpCommand(
            COMMAND_GET_SCAN_OUTPUT_CONFIG, {{PARAMETER_NAME_HANDLE, handle.getValue()}},
            [c = std::move(callable)](const Device::AsyncResult& result) -> void
            {
                if (!result)
                {
                    std::invoke(c, AsyncResultType{result.getRequestResult(), std::nullopt});
                    return;
                }
                const auto& propertyTree{result.getPropertyTree()};
                std::invoke(c, AsyncResultType{result.getRequestResult(), getScanOutputConfig(propertyTree)});
            },
            timeout);
    }

private:
    /**
     *
     * @param propertyTree
     * @return
     */
    static std::optional<Parameters::ParametersMap> getScanOutputConfig(const Device::PropertyTree& propertyTree)
    {
        Parameters::ParametersMap scanOutputConfig{};
        for (const auto& name : propertyTree)
        {
            const auto parameterName{name.second.get<std::string>("")};
            auto value{propertyTree.get_optional<std::string>(parameterName)};
            if (!value)
                return std::nullopt;
            scanOutputConfig.insert(std::make_pair(parameterName, *value));
        }
        return {scanOutputConfig};
    }

private:
    R2000& device;
};

template <> struct CommandExecutorImpl<SetScanOutputConfig>
{
    using ResultType = bool;
    using AsyncResultType = Device::AsyncRequestResult;

    /**
     *
     * @param device
     */
    [[maybe_unused]] explicit CommandExecutorImpl(R2000& device) : device(device) {}

    /**
     *
     * @tparam Args
     * @param args
     * @return
     */
    template <typename... Args> [[maybe_unused]] inline ResultType execute(Args&&... args)
    {
        static_assert(std::conjunction_v<std::is_base_of<Parameters::HandleParameters, typename std::decay_t<Args>>...>,
                      "All the provided handle must be of type HandleParameters");
        return RecursiveExecutorHelper::template resolve(
            [this](const auto& builder)
            {
                const auto parameters{builder.build()};
                return device.sendHttpCommand(COMMAND_SET_SCAN_OUTPUT_CONFIG, parameters).first;
            },
            std::forward<Args>(args)...);
    }

    /**
     *
     * @param builder
     * @param timeout
     * @return
     */
    [[maybe_unused]] std::optional<std::future<AsyncResultType>>
    asyncExecute(const Parameters::HandleParameters& builder, std::chrono::milliseconds timeout)
    {
        const auto parameters{builder.build()};
        auto promise{std::make_shared<std::promise<AsyncResultType>>()};
        auto onComplete{[promise](const Device::AsyncResult& result) -> void
                        { promise->set_value(result.getRequestResult()); }};

        if (device.asyncSendHttpCommand(COMMAND_SET_SCAN_OUTPUT_CONFIG, parameters, onComplete, timeout))
        {
            return {promise->get_future()};
        }
        return std::nullopt;
    }

    /**
     *
     * @param builder
     * @param timeout
     * @param callable
     * @return
     */
    [[maybe_unused]] bool asyncExecute(const Parameters::HandleParameters& builder, std::chrono::milliseconds timeout,
                                       std::function<void(const AsyncResultType&)> callable)
    {
        const auto parameters{builder.build()};
        return device.asyncSendHttpCommand(
            COMMAND_SET_SCAN_OUTPUT_CONFIG, parameters,
            [c = std::move(callable)](const Device::AsyncResult& result) -> void
            { std::invoke(c, result.getRequestResult()); },
            timeout);
    }

private:
    R2000& device;
};

} // namespace internals
template <typename Tag> using Command [[maybe_unused]] = internals::CommandExecutorImpl<Tag>;
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