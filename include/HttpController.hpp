//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#pragma once

#include "ExponentialBackoff.hpp"
#include <any>
#include <bitset>
#include <boost/asio.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <chrono>
#include <iostream>
#include <list>
#include <optional>
#include <utility>

using namespace std::chrono_literals;

namespace R2000
{
class HttpController;
using Handle_Id_Type = std::string;
struct DeviceHandle;
class DataLink;
struct HttpDeviceConfiguration
{
    explicit HttpDeviceConfiguration(boost::asio::ip::address deviceAddress, unsigned short port = 80)
        : deviceAddress(std::move(deviceAddress)), httpServicePort(port)
    {
    }

    boost::asio::ip::address deviceAddress;
    unsigned short httpServicePort;
};
enum class RebootStatus
{
    COMPLETED,
    TIMEOUT,
    FAILED
};

template <typename ExecutionContext> class SocketGuard
{
public:
    using Socket = boost::asio::ip::tcp::socket;
    explicit SocketGuard(ExecutionContext& context) : socket(context) {}
    ~SocketGuard() { socket.close(); }
    Socket& operator*() { return socket; }

private:
    Socket socket;
};

using ParametersList = std::vector<std::string>;
using PropertyTree = boost::property_tree::ptree;
class HttpController
{
private:
    explicit HttpController(const HttpDeviceConfiguration& configuration);

    [[maybe_unused]] explicit HttpController(HttpDeviceConfiguration&& configuration);

public:
    [[maybe_unused]] static std::shared_ptr<HttpController> Factory(const HttpDeviceConfiguration& configuration);
    [[maybe_unused]] static std::shared_ptr<HttpController> Factory(HttpDeviceConfiguration&& configuration);
    [[maybe_unused]] bool startStream(const DeviceHandle& handle) noexcept(false);
    [[maybe_unused]] bool stopStream(const DeviceHandle& handle) noexcept(false);
    [[maybe_unused]] bool feedWatchdog(const DeviceHandle& handle) noexcept(false);
    [[maybe_unused]] std::optional<bool> isRebooting() noexcept(true);
    [[maybe_unused]] bool rebootDevice() noexcept(false);
    template <typename T> [[maybe_unused]] R2000::RebootStatus rebootDevice(T timeout) noexcept(false)
    {
        {
            const auto status = isRebooting();
            if (status && !status)
            {
                if (!rebootDevice())
                {
                    return RebootStatus::FAILED;
                }
            }
        }
        const auto t = std::chrono::duration_cast<std::chrono::milliseconds>(timeout);
        const auto deadline = std::chrono::steady_clock::now() + t;
        const auto SleepAction = [deadline, t](const auto& duration)
        {
            const auto now = std::chrono::steady_clock::now();
            if (now + duration > deadline)
                std::this_thread::sleep_for(deadline - now);
            else
                std::this_thread::sleep_for(duration);
        };
        const auto IsRetryable = [](RebootStatus status)
        {
            if (status == RebootStatus::FAILED)
            {
                return true;
            }
            return false;
        };
        const auto Callable = [this, deadline]() -> RebootStatus
        {
            const auto status = isRebooting();
            if (status && *status)
            {
                return RebootStatus::COMPLETED;
            }
            const auto now = std::chrono::steady_clock::now();
            if (now > deadline)
                return RebootStatus::TIMEOUT;
            return RebootStatus::FAILED;
        };
        return Retry::ExponentialBackoff(std::numeric_limits<unsigned int>::max(), 100ms, 3000ms,
                                         SleepAction, IsRetryable, Callable);;
    }
    [[maybe_unused]] ParametersList fetchParametersList();
    [[maybe_unused]] std::optional<std::string> getParameterValue(const std::string& name);
    [[maybe_unused]] std::unordered_map<std::string, std::string> getParametersValues(const ParametersList& list);
    [[maybe_unused]] std::unordered_map<std::string, std::string> pullAllParametersValues();
    [[maybe_unused]] bool setParameter(const std::string& name, const std::string& value) noexcept(false);
    [[maybe_unused]] bool setParameters(const std::unordered_map<std::string, std::string>& parameters) noexcept(false);
    [[maybe_unused]] bool resetParameters(const ParametersList& list) noexcept(false);
    [[maybe_unused]] bool factoryResetParameters() noexcept(false);
    [[maybe_unused]] bool displayText(const std::array<std::string, 2>& lines) noexcept(false);

private:
    std::pair<bool, R2000::PropertyTree> sendHttpCommand(const std::string& command, const std::string& parameters = "",
                                                         std::string values = "") noexcept(false);

    std::pair<bool, R2000::PropertyTree>
    sendHttpCommand(const std::string& command,
                    const std::unordered_map<std::string, std::string>& paramValues) noexcept(false);

    [[nodiscard]] std::tuple<int, std::string, std::string> httpGet(const std::string& requestPath) const
        noexcept(false);

    [[maybe_unused]] bool releaseHandle(const DeviceHandle& handle) noexcept(false);

    static bool verifyErrorCode(const PropertyTree& tree);

private:
    HttpDeviceConfiguration mConfiguration;
    friend class DataLink;

    friend class DataLinkBuilder;
};
} // namespace R2000