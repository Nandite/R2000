//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file
#include "Control/DeviceHandle.hpp"
#include "DataLink/TCPLink.hpp"
#include "DataLink/UDPLink.hpp"
#include "R2000.hpp"
#include <DataLink/DataLinkBuilder.hpp>

Device::DataLinkBuilder::DataLinkBuilder(const Parameters::ReadWriteParameters::TcpHandle &builder) {
    requirements.put("protocol", Device::PROTOCOL::TCP);
    requirements.put("handleBuilder", builder);
}

Device::DataLinkBuilder::DataLinkBuilder(const Parameters::ReadWriteParameters::UdpHandle &builder) {
    requirements.put("protocol", Device::PROTOCOL::UDP);
    requirements.put("handleBuilder", builder);
}

std::shared_ptr<Device::DataLink>
Device::DataLinkBuilder::build(const std::shared_ptr<Device::R2000> &device) noexcept(false) {
    return constructDataLink(device);
}

std::future<Device::DataLinkBuilder::AsyncBuildResult>
Device::DataLinkBuilder::build(const std::shared_ptr<Device::R2000> &device,
                               std::chrono::milliseconds timeout) noexcept(false) {
    return constructDataLink(device, timeout);
}

std::pair<bool, std::chrono::milliseconds>
Device::DataLinkBuilder::extractWatchdogParameters(const Device::Parameters::ParametersMap &map) {
    const auto watchdogEnabled{internals::findValueOrDefault(map, PARAMETER_HANDLE_WATCHDOG, BOOL_PARAMETER_FALSE) ==
                               BOOL_PARAMETER_TRUE};
    const auto watchdogTimeout{
            watchdogEnabled ? std::stoi(internals::findValueOrThrow(map, PARAMETER_HANDLE_WATCHDOG_TIMEOUT, ""))
                            : 60000};
    return {watchdogEnabled, std::chrono::milliseconds(watchdogTimeout)};
}

std::pair<Device::Parameters::ReadWriteParameters::TcpHandle, Device::DataLinkBuilder::WatchdogConfiguration>
Device::DataLinkBuilder::extractTcpDataLinkParameters(
        Device::internals::Requirements<std::string> &requirements) noexcept(false) {
    auto handleBuilder{
            internals::castAnyOrThrow<Parameters::ReadWriteParameters::TcpHandle>(requirements.get("handleBuilder"),
                                                                                  "")};
    auto handleParameters{handleBuilder.build()};
    auto watchdog{extractWatchdogParameters(handleParameters)};
    return {std::move(handleBuilder), std::move(watchdog)};
}

std::tuple<Device::Parameters::ReadWriteParameters::UdpHandle, Device::DataLinkBuilder::WatchdogConfiguration,
        boost::asio::ip::address, unsigned int>
Device::DataLinkBuilder::extractUdpDataLinkParameters(
        Device::internals::Requirements<std::string> &requirements) noexcept(false) {
    auto handleBuilder{
            internals::castAnyOrThrow<Parameters::ReadWriteParameters::UdpHandle>(requirements.get("handleBuilder"),
                                                                                  "")};
    auto handleParameters{handleBuilder.build()};
    auto watchdog{extractWatchdogParameters(handleParameters)};
    const auto port{std::stoi(
            internals::findValueOrThrow(handleParameters, PARAMETER_HANDLE_PORT,
                                        "Could not find the port parameter."))};
    const auto address{boost::asio::ip::address::from_string(internals::findValueOrThrow(
            handleParameters, PARAMETER_HANDLE_ADDRESS, "Could not find the address parameter."))};
    return {std::move(handleBuilder), std::move(watchdog), address, port};
}

std::shared_ptr<Device::DeviceHandle>
Device::DataLinkBuilder::makeTcpDeviceHandle(const Commands::RequestTcpHandleCommand::ResultType &commandResult,
                                             const boost::asio::ip::address &address,
                                             Device::DataLinkBuilder::WatchdogConfiguration watchdog) noexcept {
    const auto port{std::get<1>(commandResult)};
    const auto handle{std::get<2>(commandResult)};
    return std::make_shared<DeviceHandle>(handle, address, port, watchdog.first, watchdog.second);
}

std::shared_ptr<Device::DeviceHandle>
Device::DataLinkBuilder::makeUdpDeviceHandle(const Commands::RequestUdpHandleCommand::ResultType &commandResult,
                                             const boost::asio::ip::address &address, const unsigned int port,
                                             Device::DataLinkBuilder::WatchdogConfiguration watchdog) noexcept {
    return std::make_shared<DeviceHandle>(commandResult.second, address, port, watchdog.first, watchdog.second);
}

std::shared_ptr<Device::DataLink>
Device::DataLinkBuilder::constructDataLink(const std::shared_ptr<Device::R2000> &device) noexcept(false) {
    const auto protocol{internals::castAnyOrThrow<Device::PROTOCOL>(requirements.get("protocol"), "")};
    switch (protocol) {
        case PROTOCOL::TCP: {
            const auto &[builder, watchdog]{extractTcpDataLinkParameters(requirements)};
            auto command{Device::Commands::RequestTcpHandleCommand{*device}};
            const auto result{command.execute(builder)};
            const auto resultRequest{std::get<0>(result)};
            if (resultRequest != Device::RequestResult::SUCCESS)
                return {nullptr};
            const auto handle{makeTcpDeviceHandle(result, device->getHostname(), watchdog)};
            return std::make_shared<TCPLink>(device, handle);
        }
        case PROTOCOL::UDP: {
            const auto &[builder, watchdog, address, port]{extractUdpDataLinkParameters(requirements)};
            auto command{Device::Commands::RequestUdpHandleCommand{*device}};
            const auto result{command.execute(builder)};
            const auto resultRequest{std::get<0>(result)};
            if (resultRequest != Device::RequestResult::SUCCESS)
                return {nullptr};
            const auto handle{makeUdpDeviceHandle(result, address, port, watchdog)};
            return std::make_shared<UDPLink>(device, handle);
        }
    }
    return {nullptr};
}

std::future<Device::DataLinkBuilder::AsyncBuildResult>
Device::DataLinkBuilder::constructDataLink(const std::shared_ptr<Device::R2000> &device,
                                           std::chrono::milliseconds timeout) noexcept(true) {
    auto promise{std::make_shared<std::promise<AsyncBuildResult>>()};
    const auto protocol{internals::castAnyOrThrow<Device::PROTOCOL>(requirements.get("protocol"), "")};
    switch (protocol) {
        case PROTOCOL::TCP: {
            const auto &[handle, watchdog]{extractTcpDataLinkParameters(requirements)};
            const auto launched{Device::Commands::RequestTcpHandleCommand{*device}.asyncExecute(
                    handle, timeout,
                    [promise, w = watchdog, &device](const auto &result) -> void {
                        const auto resultRequest{std::get<0>(result)};
                        if (resultRequest != RequestResult::SUCCESS) {
                            promise->set_value(AsyncBuildResult{resultRequest, nullptr});
                            return;
                        }
                        const auto handleValue{makeTcpDeviceHandle(result, device->getHostname(), w)};
                        promise->set_value(AsyncBuildResult{
                                RequestResult::SUCCESS, std::make_shared<TCPLink>(device, handleValue)});
                    })};
            if (!launched)
                promise->set_value(AsyncBuildResult{RequestResult::FAILED, nullptr});
            break;
        }
        case PROTOCOL::UDP: {
            const auto &[builder, watchdog, address, port]{extractUdpDataLinkParameters(requirements)};
            const auto launched{Device::Commands::RequestUdpHandleCommand{*device}.asyncExecute(
                    builder, timeout,
                    [promise,  a = address, w = watchdog, p = port, &device](const auto &result) -> void {
                        const auto resultRequest{std::get<0>(result)};
                        if (resultRequest != RequestResult::SUCCESS) {
                            promise->set_value(AsyncBuildResult{resultRequest, nullptr});
                            return;
                        }
                        const auto handleValue{makeUdpDeviceHandle(result, a, p, w)};
                        promise->set_value(AsyncBuildResult{
                                RequestResult::SUCCESS,
                                std::make_shared<UDPLink>(device, handleValue)});
                    })};
            if (!launched)
                promise->set_value(AsyncBuildResult{RequestResult::FAILED, nullptr});
            break;
        }
    }
    return promise->get_future();
}