//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#include "R2000/R2000.hpp"
#include "R2000/DeviceHandle.hpp"
#include <bitset>
#include <iostream>
#include <utility>

Device::R2000::R2000(Device::DeviceConfiguration configuration) : configuration(std::move(configuration)) {

    ioServiceTask = std::async(std::launch::async,
                               [this]() {
                                   for (; !interruptAsyncOpsFlag.load(std::memory_order_acquire);) {
                                       ioService.reset();
                                       ioService.run();
                                       std::unique_lock<std::mutex> interruptGuard{ioServiceOpsCvLock, std::adopt_lock};
                                       ioServiceOpsCv.wait(interruptGuard);
                                   }
                               });
}

[[maybe_unused]] std::shared_ptr<Device::R2000>
Device::R2000::makeShared(const Device::DeviceConfiguration &configuration) {
    return enableMakeShared(configuration);
}

std::pair<bool, Device::PropertyTree> Device::R2000::sendHttpCommand(const std::string &command,
                                                                     const std::string &parameter,
                                                                     std::string value) const noexcept(false) {
    ParametersMap parameters{};
    if (!parameter.empty())
        parameters[parameter] = std::move(value);
    return sendHttpCommand(command, parameters);
}

std::pair<bool, Device::PropertyTree> Device::R2000::sendHttpCommand(const std::string &command,
                                                                     const ParametersMap &parameters) const
noexcept(false) {
    auto request{"/cmd/" + command};
    if (!parameters.empty())
        request += "?";
    for (const auto &parameter : parameters)
        request += parameter.first + "=" + parameter.second + "&";
    if (request.back() == '&')
        request.pop_back();
    Device::PropertyTree propertyTree{};
    try {
        const auto result{HttpGet(request)};
        const auto responseCode{std::get<0>(result)};
        const auto content{std::get<2>(result)};
        std::stringstream stringStream{content};
        if (responseCode == 200) {
            boost::property_tree::json_parser::read_json(stringStream, propertyTree);
            return {verifyErrorCode(propertyTree), std::move(propertyTree)};
        } else {
            return {false, {}};
        }
    }
    catch (const std::system_error &error) {
        std::clog << __func__ << ": Http request error : " << error.what() << " (" << error.code() << ")" << std::endl;
    }
    return {false, {}};
}

bool Device::R2000::asyncSendHttpCommand(const std::string &command, AsyncCommandCallback callable,
                                         std::chrono::milliseconds timeout) noexcept(true) {
    return asyncSendHttpCommand(command, {}, std::move(callable), timeout);
}

bool Device::R2000::asyncSendHttpCommand(const std::string &command, const ParametersMap &parameters,
                                         AsyncCommandCallback callable,
                                         std::chrono::milliseconds timeout) noexcept(true) {
    const auto request{makeRequestFromParameters(command, parameters)};
    auto onHttpGetComplete{[callable = std::move(callable)](const HttpResult &httpResult) {
        Device::PropertyTree propertyTree{};
        const auto responseCode{std::get<0>(httpResult)};
        if (responseCode != 200) {
            if (responseCode == 408) {
                std::invoke(callable, AsyncResult(AsyncRequestResult::TIMEOUT, Device::PropertyTree{}));
            } else {
                std::invoke(callable, AsyncResult(AsyncRequestResult::FAILED, Device::PropertyTree{}));
            }
        } else {
            const auto content{std::get<2>(httpResult)};
            std::stringstream stringStream{content};
            boost::property_tree::json_parser::read_json(stringStream, propertyTree);
            const auto propertyTreeCode{verifyErrorCode(propertyTree)};
            AsyncRequestResult requestResult{propertyTreeCode ? AsyncRequestResult::SUCCESS
                                                              : AsyncRequestResult::FAILED};
            std::invoke(callable, AsyncResult(requestResult, propertyTree));
        }
    }};
    return AsyncHttpGet(request, onHttpGetComplete, timeout);
}

Device::R2000::HttpResult Device::R2000::HttpGet(boost::asio::ip::tcp::socket &socket, const std::string &requestPath) {
    std::string header{};
    std::string content{};
    boost::asio::streambuf request{};
    std::ostream requestStream{&request};
    requestStream << "GET " << requestPath << " HTTP/1.0\r\n\r\n";
    {
        boost::system::error_code error{boost::asio::error::host_not_found};
        boost::asio::write(socket, request, error);
        if (error) {
            throw std::system_error(error);
        }
    }
    boost::asio::streambuf response{};
    {
        boost::system::error_code error{boost::asio::error::host_not_found};
        boost::asio::read_until(socket, response, "\r\n", error);
        if (error) {
            throw std::system_error(error);
        }
    }

    std::istream responseStream{&response};
    std::string httpVersion{};
    responseStream >> httpVersion;
    auto statusCode{0};
    responseStream >> statusCode;
    std::string status_message{};
    std::getline(responseStream, status_message);
    if (!responseStream || httpVersion.substr(0, 5) != "HTTP/") {
        std::clog << "Invalid response" << std::endl;
        return {0, "", ""};
    }

    {
        boost::system::error_code error{boost::asio::error::host_not_found};
        // Read the response headers, which are terminated by a blank line.
        boost::asio::read_until(socket, response, "\r\n\r\n", error);
        if (error) {
            throw std::system_error(error);
        }
    }

    std::string line{};
    while (std::getline(responseStream, line) && line != "\r") {
        header += line + "\n";
    }
    while (std::getline(responseStream, line)) {
        content += line;
    }

    boost::system::error_code error{boost::asio::error::host_not_found};
    for (; boost::asio::read(socket, response, boost::asio::transfer_at_least(1), error);) {
        responseStream.clear();
        while (std::getline(responseStream, line)) {
            content += line;
        }
    }
    if (error != boost::asio::error::eof) {
        throw std::system_error(error);
    }

    for (auto &character : header) {
        if (character == '\r') {
            character = ' ';
        }
    }
    for (auto &character : content) {
        if (character == '\r') {
            character = ' ';
        }
    }
    return {statusCode, header, content};
}

Device::R2000::HttpResult Device::R2000::HttpGet(const std::string &requestPath) const noexcept(false) {
    const auto deviceAddress{configuration.deviceAddress.to_string()};
    const auto port{std::to_string(configuration.httpServicePort)};
    boost::asio::ip::tcp::resolver resolver{ioService};
    boost::asio::ip::tcp::resolver::query query{deviceAddress, port};
    auto endpointIterator{resolver.resolve(query)};
    boost::asio::ip::tcp::resolver::iterator endpointEndIterator{};
    SocketGuard<boost::asio::ip::tcp::socket> socketGuard{ioService};
    auto &socket{socketGuard.getUnderlyingSocket()};
    {
        boost::system::error_code error{boost::asio::error::host_not_found};
        for (; error && endpointIterator != endpointEndIterator;) {
            boost::system::error_code placeholder{};
            socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, placeholder);
            socket.close(placeholder);
            socket.connect(*endpointIterator++, error);
        }
        if (error) {
            throw std::system_error(error);
        }
    }

    return HttpGet(socket, requestPath);
}

bool Device::R2000::AsyncHttpGet(const std::string &request, HttpGetCallback callable,
                                 std::chrono::milliseconds timeout) noexcept(true) {
    auto asyncWorkerJob{[this, request, timeout, callable = std::move(callable)]() {
        if (deviceEndpoints) {
            handleEndpointResolution(boost::system::error_code{}, request, callable, *deviceEndpoints, timeout);
        } else {
            const auto query{getDeviceQuery()};
            auto resolverHandler{[this, request, callable, timeout](const auto &error, auto queriedEndpoints) mutable {
                deviceEndpoints = std::move(queriedEndpoints);
                handleEndpointResolution(error, request, callable, *deviceEndpoints, timeout);
            }};
            asyncResolver.async_resolve(query, resolverHandler);
            scheduleResolverDeadline(timeout);
        }
        std::unique_lock<std::mutex> guard(ioServiceOpsCvLock, std::adopt_lock);
        ioServiceOpsCv.notify_one();
    }};
    return worker.pushJob(asyncWorkerJob);
}

void Device::R2000::handleEndpointResolution(const boost::system::error_code &error, const std::string &request,
                                             HttpGetCallback callable,
                                             boost::asio::ip::tcp::resolver::results_type &endpoints,
                                             std::chrono::milliseconds timeout) {
    if (!error) {
        boost::system::error_code placeholder{};
        const auto endpointsBegin{std::cbegin(endpoints)};
        const auto endpointsEnd{std::cend(endpoints)};
        asyncSocket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, placeholder);
        asyncSocket.close(placeholder);
        auto socketConnectionHandler{[this, request, callable = std::move(callable)](auto error, const auto &) {
            handleSocketConnection(error, request, callable);
        }};
        boost::asio::async_connect(asyncSocket, endpointsBegin, endpointsEnd, socketConnectionHandler);
        scheduleSocketConnectionDeadline(timeout);
    } else {
        if (error == boost::asio::error::operation_aborted) {
            std::clog << "R2000::" << configuration.name << "::Endpoints resolution timeout" << std::endl;
        } else {
            std::clog << "R2000::" << configuration.name << "::Network error while resolving endpoints ("
                      << error.message() << ")" << std::endl;
        }
        std::invoke(callable, std::make_tuple(0, std::string{}, std::string{}));
    }
}

void Device::R2000::handleSocketConnection(const boost::system::error_code &error, const std::string &request,
                                           HttpGetCallback callable) {
    if (!error) {
        const auto result{HttpGet(asyncSocket, request)};
        std::invoke(callable, result);
    } else {
        if (error == boost::asio::error::operation_aborted) {
            std::clog << "R2000::" << configuration.name << "::Socket connection timout" << std::endl;
            std::invoke(callable, std::make_tuple(408, std::string{}, std::string{}));
        } else {
            std::clog << "R2000::" << configuration.name << "::Network error while connecting to the http socket ("
                      << error.message() << ")" << std::endl;
            std::invoke(callable, std::make_tuple(0, std::string{}, std::string{}));
        }
    }
}

bool Device::R2000::verifyErrorCode(const Device::PropertyTree &tree) {
    const auto code{tree.get_optional<int>(ERROR_CODE)};
    const auto text{tree.get_optional<std::string>(ERROR_TEXT)};
    return (code && (*code) == 0 && text && (*text) == "success");
}

std::optional<Device::PFSDP> Device::R2000::getDeviceProtocolVersion() {
    return std::nullopt;
//    if (!deviceProtocolVersion)
//    {
//        Commands::GetProtocolInfoCommand getProtocolInfoCommand{*this};
//        const auto result{getProtocolInfoCommand.execute()};
//        if (result)
//        {
//            const auto parameters{(*result).first};
//            const auto major{parameters.at(PARAMETER_PROTOCOL_VERSION_MAJOR)};
//            const auto minor{parameters.at(PARAMETER_PROTOCOL_VERSION_MINOR)};
//            deviceProtocolVersion = {protocolVersionFromString(major, minor)};
//            return deviceProtocolVersion;
//        }
//    }
//    return deviceProtocolVersion;
}

void Device::R2000::scheduleResolverDeadline(std::chrono::milliseconds timeout) {
    resolverDeadline.expires_from_now(boost::asio::chrono::milliseconds(
            std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count()));
    resolverDeadline.async_wait([this](const boost::system::error_code &error) {
        handleAsyncResolverDeadline(error);
    });
}

void Device::R2000::scheduleSocketConnectionDeadline(std::chrono::milliseconds timeout) {
    socketDeadline.expires_from_now(boost::asio::chrono::milliseconds(
            std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count()));
    socketDeadline.async_wait([this](const boost::system::error_code &error) { handleAsyncSocketDeadline(error); });
}

void Device::R2000::handleAsyncResolverDeadline(const boost::system::error_code &error) {
    if (error == boost::asio::error::operation_aborted) {
        std::clog << "R2000::" << configuration.name << "::Cancelling async resolver deadline" << std::endl;
    }
    const auto expiryPoint{resolverDeadline.expiry()};
    if (expiryPoint <= boost::asio::steady_timer::clock_type::now()) {
        asyncResolver.cancel();
    } else {
        resolverDeadline.async_wait(
                [this](const boost::system::error_code &error) { handleAsyncResolverDeadline(error); });
    }
}

void Device::R2000::handleAsyncSocketDeadline(const boost::system::error_code &error) {
    if (error == boost::asio::error::operation_aborted) {
        std::clog << "R2000::" << configuration.name << "::Cancelling async socket deadline" << std::endl;
    }
    const auto expiryPoint{socketDeadline.expiry()};
    if (expiryPoint <= boost::asio::steady_timer::clock_type::now()) {
        boost::system::error_code placeholder{};
        asyncSocket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, placeholder);
        asyncSocket.close(placeholder);
    } else {
        socketDeadline.async_wait(
                [this](const boost::system::error_code &error) { handleAsyncResolverDeadline(error); });
    }
}

Device::R2000::~R2000() {
    if (!interruptAsyncOpsFlag.load(std::memory_order_acquire)) {
        std::unique_lock<std::mutex> guard(ioServiceOpsCvLock, std::adopt_lock);
        interruptAsyncOpsFlag.store(true, std::memory_order_release);
        ioServiceOpsCv.notify_one();
    }
    boost::system::error_code placeholder;
    resolverDeadline.cancel(placeholder);
    socketDeadline.cancel(placeholder);
    asyncSocket.cancel(placeholder);
    asyncSocket.shutdown(boost::asio::ip::tcp::socket::shutdown_receive, placeholder);
    asyncSocket.close(placeholder);
    if (!ioService.stopped()) {
        ioService.stop();
        ioServiceTask.wait();
    }
}
