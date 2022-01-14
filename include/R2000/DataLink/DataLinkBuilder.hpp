//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#pragma once

#include "R2000/Control/DeviceHandle.hpp"
#include "R2000/Control/Parameters.hpp"
#include "R2000/R2000.hpp"
#include "TCPLink.hpp"
#include "UDPLink.hpp"
#include <R2000/Control/Commands.hpp>
#include <any>
#include <chrono>
#include <utility>

using namespace std::chrono_literals;

namespace Device
{
class builder_exception : public std::invalid_argument
{
public:
    explicit builder_exception(const std::string& what) : std::invalid_argument(what){};
    builder_exception(const builder_exception&) = default;
    builder_exception& operator=(const builder_exception&) = default;
    builder_exception(builder_exception&&) = default;
    builder_exception& operator=(builder_exception&&) = default;
    ~builder_exception() noexcept override = default;
};
namespace internals
{
template <typename KeyType> class Requirements
{
public:
    template <typename ValueType> void put(KeyType key, ValueType&& value)
    {
        requirements[key] = std::any(std::forward<ValueType>(value));
    }

    std::any get(KeyType key)
    {
        if (requirements.count(key))
            return requirements.at(key);
        return {};
    }

private:
    std::unordered_map<KeyType, std::any> requirements{};
};

template <class Context, class... Args> void rethrow(Context&& context, Args&&... args)
{
    std::ostringstream ss;
    ss << context;
    auto sep = " : ";
    using expand = int[];
    void(expand{0, ((ss << sep << args), sep = ", ", 0)...});
    try
    {
        std::rethrow_exception(std::current_exception());
    }
    catch (const builder_exception& e)
    {
        std::throw_with_nested(builder_exception(ss.str()));
    }
    catch (const std::invalid_argument& e)
    {
        std::throw_with_nested(std::invalid_argument(ss.str()));
    }
    catch (const std::logic_error& e)
    {
        std::throw_with_nested(std::logic_error(ss.str()));
    }
    catch (...)
    {
        std::throw_with_nested(std::runtime_error(ss.str()));
    }
}

void print_nested_exception(const std::exception& e, std::size_t depth = 0)
{
    std::clog << "exception: " << std::string(depth, ' ') << e.what() << std::endl;
    try
    {
        std::rethrow_if_nested(e);
    }
    catch (const std::exception& nested)
    {
        print_nested_exception(nested, depth + 1);
    }
}

template <typename T> auto castAnyOrThrow(const std::any& any, const std::string& throw_message) noexcept(false)
{
    if (!any.has_value())
        throw builder_exception(std::string(__func__) + " : Any is empty -> " + throw_message);
    return std::any_cast<T>(any);
}

auto findValueOrThrow(const ParametersMap& map, const std::string& key,
                      const std::string& throw_message) noexcept(false)
{
    if (!map.count(key))
        throw builder_exception(std::string(__func__) + " : Key not found -> " + throw_message);
    return map.at(key);
}

std::string find_value_or_default(const ParametersMap& map, const std::string& key,
                                  const std::string& defaultValue) noexcept(false)
{
    if (!map.count(key))
        return defaultValue;
    return map.at(key);
}

} // namespace internals

class DataLinkBuilder
{
private:
    using WatchdogConfiguration = std::pair<bool, std::chrono::milliseconds>;
    using AsyncBuildResult = std::pair<AsyncRequestResult, std::shared_ptr<DataLink>>;

public:
    /**
     *
     * @param builder
     */
    explicit DataLinkBuilder(const Parameters::ReadWriteParameters::TcpHandle& builder)
    {
        requirements.put("protocol", Device::PROTOCOL::TCP);
        requirements.put("handleBuilder", builder);
    }

    /**
     *
     * @param builder
     */
    explicit DataLinkBuilder(const Parameters::ReadWriteParameters::UdpHandle& builder)
    {
        requirements.put("protocol", Device::PROTOCOL::UDP);
        requirements.put("handleBuilder", builder);
    }

    /**
     *
     * @param device
     * @return
     */
    std::shared_ptr<DataLink> build(const std::shared_ptr<Device::R2000>& device) noexcept(false)
    {
        return syncBuildAndPotentiallyThrow(device);
    }

    std::future<AsyncBuildResult> build(const std::shared_ptr<Device::R2000>& device,
                                        std::chrono::milliseconds timeout) noexcept(true)
    {
        return asyncBuildAndPotentiallyThrow(device, timeout);
    }

private:
    /**
     *
     * @param map
     * @return
     */
    [[nodiscard]] static std::pair<bool, std::chrono::milliseconds> extractWatchdogParameters(const ParametersMap& map)
    {
        const auto watchdogEnabled{internals::find_value_or_default(map, PARAMETER_HANDLE_WATCHDOG,
                                                                    BOOL_PARAMETER_FALSE) == BOOL_PARAMETER_TRUE};
        const auto watchdogTimeout{
            watchdogEnabled ? std::stoi(internals::findValueOrThrow(map, PARAMETER_HANDLE_WATCHDOG_TIMEOUT, ""))
                            : 60000};
        return {watchdogEnabled, std::chrono::milliseconds(watchdogTimeout)};
    }

    /**
     *
     * @tparam T
     * @param requirements
     * @return
     */
    static std::pair<Parameters::ReadWriteParameters::TcpHandle, WatchdogConfiguration>
    extractTcpDataLinkParameters(internals::Requirements<std::string>& requirements) noexcept(false)
    {
        auto handleBuilder{internals::castAnyOrThrow<Parameters::ReadWriteParameters::TcpHandle>(
            requirements.get("handleBuilder"), "")};
        auto handleParameters{handleBuilder.build()};
        auto watchdog{extractWatchdogParameters(handleParameters)};
        return {std::move(handleBuilder), std::move(watchdog)};
    };

    static std::tuple<Parameters::ReadWriteParameters::UdpHandle, WatchdogConfiguration, unsigned int>
    extractUdpDataLinkParameters(internals::Requirements<std::string>& requirements) noexcept(false)
    {
        auto handleBuilder{internals::castAnyOrThrow<Parameters::ReadWriteParameters::UdpHandle>(
            requirements.get("handleBuilder"), "")};
        auto handleParameters{handleBuilder.build()};
        auto watchdog{extractWatchdogParameters(handleParameters)};
        const auto port{std::stoi(internals::findValueOrThrow(handleParameters, PARAMETER_HANDLE_PORT,
                                                              "Could not find the port parameter."))};
        return {std::move(handleBuilder), std::move(watchdog), port};
    };

    static std::shared_ptr<DeviceHandle> makeTcpDeviceHandle(const Commands::RequestTcpHandleCommand::ResultType& from,
                                                             WatchdogConfiguration watchdog)
    {
        const auto& deviceAnswer{from.value()};
        const auto& handle{deviceAnswer.second};
        const auto port{deviceAnswer.first};
        return std::make_shared<DeviceHandle>(handle, watchdog.first, watchdog.second, port);
    }

    static std::shared_ptr<DeviceHandle> makeUdpDeviceHandle(const Commands::RequestUdpHandleCommand ::ResultType& from,
                                                             WatchdogConfiguration watchdog, unsigned int port)
    {
        const auto& deviceAnswer{from.value()};
        const auto& handle{deviceAnswer};
        return std::make_shared<DeviceHandle>(handle, watchdog.first, watchdog.second, port);
    }

    /**
     *
     * @param device
     * @return
     */
    std::shared_ptr<DataLink> syncBuildAndPotentiallyThrow(const std::shared_ptr<Device::R2000>& device) noexcept(false)
    {
        const auto protocol{internals::castAnyOrThrow<Device::PROTOCOL>(requirements.get("protocol"), "")};
        switch (protocol)
        {
        case PROTOCOL::TCP:
        {
            const auto params{extractTcpDataLinkParameters(requirements)};
            auto command{Device::Commands::RequestTcpHandleCommand{*device}};
            const auto result{command.execute(params.first)};
            if (!result)
                return {nullptr};
            return std::make_shared<TCPLink>(device, makeTcpDeviceHandle(*result, params.second));
        }
        case PROTOCOL::UDP:
        {
            const auto params{extractUdpDataLinkParameters(requirements)};
            const auto& builder{std::get<0>(params)};
            const auto& watchdog{std::get<1>(params)};
            const auto& port{std::get<2>(params)};
            auto command{Device::Commands::RequestUdpHandleCommand{*device}};
            const auto result{command.execute(builder)};
            if (!result)
                return {nullptr};
            return std::make_shared<UDPLink>(device, makeUdpDeviceHandle(*result, watchdog, port));
        }
        }
        return {nullptr};
    }

    std::future<AsyncBuildResult> asyncBuildAndPotentiallyThrow(const std::shared_ptr<Device::R2000>& device,
                                                                std::chrono::milliseconds timeout) noexcept(true)
    {
        auto promise{std::make_shared<std::promise<AsyncBuildResult>>()};
        const auto protocol{internals::castAnyOrThrow<Device::PROTOCOL>(requirements.get("protocol"), "")};
        switch (protocol)
        {
        case PROTOCOL::TCP:
        {
            const auto params{extractTcpDataLinkParameters(requirements)};
            const auto launched{Device::Commands::RequestTcpHandleCommand{*device}.execute(
                params.first, timeout,
                [promise, params, &device](const Commands::RequestTcpHandleCommand::AsyncResultType& result) -> void
                {
                    const auto error{result.first};
                    if (error != AsyncRequestResult::SUCCESS)
                    {
                        promise->set_value(AsyncBuildResult{error, nullptr});
                        return;
                    }
                    promise->set_value(AsyncBuildResult{
                        error, std::make_shared<TCPLink>(device, makeTcpDeviceHandle(result.second, params.second))});
                })};
            if (!launched)
                promise->set_value(AsyncBuildResult{AsyncRequestResult::FAILED, nullptr});
        }
        case PROTOCOL::UDP:
            const auto params{extractUdpDataLinkParameters(requirements)};
            const auto launched{Device::Commands::RequestUdpHandleCommand{*device}.execute(
                std::get<0>(params), timeout,
                [promise, params, &device](const Commands::RequestUdpHandleCommand::AsyncResultType& result)
                {
                    const auto error{result.first};
                    if (error != AsyncRequestResult::SUCCESS)
                    {
                        promise->set_value(AsyncBuildResult{error, nullptr});
                        return;
                    }
                    const auto& builder{std::get<0>(params)};
                    const auto& watchdog{std::get<1>(params)};
                    const auto& port{std::get<2>(params)};
                    promise->set_value(AsyncBuildResult{
                        error, std::make_shared<UDPLink>(device, makeUdpDeviceHandle(result.second, watchdog, port))});
                })};
            if (!launched)
                promise->set_value(AsyncBuildResult{AsyncRequestResult::FAILED, nullptr});
        }
        return promise->get_future();
    }

private:
    internals::Requirements<std::string> requirements{};
};

} // namespace Device