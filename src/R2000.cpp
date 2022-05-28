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

#include "R2000.hpp"
#include "Control/DeviceHandle.hpp"
#include <iostream>
#include <utility>

Device::DeviceAnswer::DeviceAnswer(Device::RequestResult result) : requestResult(result) {

}

Device::DeviceAnswer::DeviceAnswer(Device::RequestResult result, const Device::PropertyTree &tree) : requestResult(
        result), propertyTree(tree) {

}

Device::R2000::R2000(Device::DeviceConfiguration configuration) : configuration(std::move(configuration)) {
    ioServiceTaskFuture = std::async(std::launch::async, &R2000::ioServiceTask, this);
}

[[maybe_unused]] std::shared_ptr<Device::R2000>
Device::R2000::makeShared(const Device::DeviceConfiguration &configuration) {
    return enableMakeShared(configuration);
}

Device::DeviceAnswer Device::R2000::sendHttpCommand(const std::string &command,
                                                    const std::string &parameter,
                                                    std::string value) const noexcept(false) {
    Parameters::ParametersMap parameters{};
    if (!parameter.empty()) {
        parameters[parameter] = std::move(value);
    }
    return sendHttpCommand(command, parameters);
}

Device::DeviceAnswer Device::R2000::sendHttpCommand(const std::string &command,
                                                    const Parameters::ParametersMap &parameters) const
noexcept(false) {
    auto request{"/cmd/" + command};
    if (!parameters.empty()) {
        request += "?";
    }
    for (const auto &parameter: parameters) {
        request += parameter.first + "=" + parameter.second + "&";
    }
    if (request.back() == '&') {
        request.pop_back();
    }
    try {
        return HttpGet(request);
    }
    catch (const std::system_error &error) {
        std::clog << getName() << "sendHttpCommand::Http request error : " << error.what() << " ("
                  << error.code() << ")" << std::endl;
    }
    return {RequestResult::FAILED, {}};
}

bool Device::R2000::asyncSendHttpCommand(const std::string &command, CommandCallback callable,
                                         std::chrono::milliseconds timeout) noexcept(true) {
    return asyncSendHttpCommand(command, {}, std::move(callable), timeout);
}

bool Device::R2000::asyncSendHttpCommand(const std::string &command, const Parameters::ParametersMap &parameters,
                                         CommandCallback callable,
                                         std::chrono::milliseconds timeout) noexcept(true) {
    const auto request{makeRequestFromParameters(command, parameters)};
    return AsyncHttpGet(request, std::move(callable), timeout);
}

void Device::R2000::cancelPendingCommands() noexcept {
    asyncResolver.cancel();
    {
        boost::system::error_code error{};
        asyncSocket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
        if (error) {
            std::clog << configuration.name << "::Cancel pending commands::An error has occurred on socket shutdown ("
                      << error.message() << ")" << std::endl;
        }
    }
    {
        boost::system::error_code error{};
        asyncSocket.close(error);
        if (error) {
            std::clog << configuration.name << "::Cancel pending commands::An error has occurred on socket closure ("
                      << error.message() << ")" << std::endl;
        }
    }
}

Device::DeviceAnswer Device::R2000::HttpGet(boost::asio::ip::tcp::socket &socket, const std::string &requestPath) {
    std::string header{};
    std::string content{};
    boost::asio::streambuf request{};
    std::ostream requestStream{&request};
    requestStream << "GET " << requestPath << " HTTP/1.0\r\n\r\n";
    {
        boost::system::error_code error{boost::asio::error::host_not_found};
        boost::asio::write(socket, request, error);
        if (error) {
            throw std::system_error{error};
        }
    }
    boost::asio::streambuf response{};
    {
        boost::system::error_code error{boost::asio::error::host_not_found};
        boost::asio::read_until(socket, response, "\r\n", error);
        if (error) {
            throw std::system_error{error};
        }
    }

    std::istream responseStream{&response};
    std::string httpVersion{};
    responseStream >> httpVersion;
    auto statusCode{0};
    responseStream >> statusCode;
    std::string statusMessage{};
    std::getline(responseStream, statusMessage);
    if (!responseStream || httpVersion.substr(0, 5) != "HTTP/") {
        std::clog << "HttpGet::Invalid response" << std::endl;
        return DeviceAnswer{RequestResult::INVALID_DEVICE_RESPONSE};
    }

    {
        boost::system::error_code error{boost::asio::error::host_not_found};
        // Read the response headers, which are terminated by a blank line.
        boost::asio::read_until(socket, response, "\r\n\r\n", error);
        if (error) {
            throw std::system_error{error};
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
        throw std::system_error{error};
    }

    for (auto &character: content) {
        if (character == '\r') {
            character = ' ';
        }
    }

    const auto responseCode{requestResultFromCode(statusCode)};
    if (responseCode != RequestResult::SUCCESS) {
        return DeviceAnswer{responseCode};
    }
    Device::PropertyTree propertyTree{};
    std::stringstream stringStream{content};
    boost::property_tree::json_parser::read_json(stringStream, propertyTree);
    if (!verifyErrorCode(propertyTree)) {
        return DeviceAnswer{responseCode};
    }
    return {responseCode, propertyTree};
}

Device::DeviceAnswer Device::R2000::HttpGet(const std::string &requestPath) const noexcept(false) {
    boost::asio::ip::tcp::resolver resolver{ioService};
    const auto query{getDeviceQuery(configuration)};
    auto endpointIterator{resolver.resolve(query)};
    boost::asio::ip::tcp::resolver::iterator endpointEndIterator{};
    internals::SocketGuard<boost::asio::ip::tcp::socket> socketGuard{ioService};
    auto &socket{*socketGuard};
    {
        boost::system::error_code error{boost::asio::error::host_not_found};
        for (; error && endpointIterator != endpointEndIterator;) {
            boost::system::error_code placeholder{};
            socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, placeholder);
            socket.close(placeholder);
            socket.connect(*endpointIterator++, error);
        }
        if (error) {
            throw std::system_error{error};
        }
    }

    return HttpGet(socket, requestPath);
}

bool Device::R2000::AsyncHttpGet(const std::string &request, CommandCallback callable,
                                 std::chrono::milliseconds timeout) noexcept(true) {
    auto asyncWorkerJob{[&, request, timeout, fn = std::move(callable)]() {
        if (deviceEndpoints) {
            onEndpointResolutionAttemptCompleted({}, request, fn, *deviceEndpoints, timeout);
        } else {
            const auto query{getDeviceQuery(configuration)};
            auto resolverHandler{[&, request, fn, timeout](const auto &error, auto queriedEndpoints) {
                deviceEndpoints = std::move(queriedEndpoints);
                onEndpointResolutionAttemptCompleted(error, request, fn, *deviceEndpoints, timeout);
            }};
            asyncResolver.async_resolve(query, resolverHandler);
            scheduleNextEndpointResolutionDeadline(timeout);
        }
        std::unique_lock<std::mutex> guard{ioServiceLock, std::adopt_lock};
        ioServiceTaskCv.notify_one();
    }};

    return worker.pushJob(asyncWorkerJob);
}

void
Device::R2000::onEndpointResolutionAttemptCompleted(const boost::system::error_code &error, const std::string &request,
                                                    CommandCallback callable,
                                                    const boost::asio::ip::tcp::resolver::results_type &endpoints,
                                                    std::chrono::milliseconds timeout) {
    boost::system::error_code placeholder{};
    resolverDeadline.cancel(placeholder);
    if (!error) {
        const auto endpointsBegin{std::cbegin(endpoints)};
        const auto endpointsEnd{std::cend(endpoints)};
        asyncSocket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, placeholder);
        asyncSocket.close(placeholder);
        auto handleSocketConnectionAttempt{[&, request, fn = std::move(callable)](auto error, const auto &) {
            onSocketConnectionAttemptCompleted(error, request, fn);
        }};
        boost::asio::async_connect(asyncSocket, endpointsBegin, endpointsEnd, handleSocketConnectionAttempt);
        scheduleNextSocketConnectionDeadline(timeout);
    } else {
        if (error == boost::asio::error::operation_aborted) {
            std::clog << configuration.name << "::Endpoints resolution timeout" << std::endl;
            std::invoke(callable, DeviceAnswer{RequestResult::TIMEOUT});
        } else {
            std::clog << configuration.name << "::Network error while resolving endpoints ("
                      << error.message() << ")" << std::endl;
            std::invoke(callable, DeviceAnswer{RequestResult::FAILED});
        }
    }
}

void
Device::R2000::onSocketConnectionAttemptCompleted(const boost::system::error_code &error, const std::string &request,
                                                  const CommandCallback &callable) {
    boost::system::error_code placeholder{};
    socketDeadline.cancel(placeholder);
    if (!error) {
        const auto result{HttpGet(asyncSocket, request)};
        std::invoke(callable, result);
    } else {
        if (error == boost::asio::error::operation_aborted) {
            std::invoke(callable, DeviceAnswer{RequestResult::TIMEOUT});
        } else {
            std::clog << configuration.name << "::Network error while connecting to the device ("
                      << error.message() << ")" << std::endl;
            std::invoke(callable, DeviceAnswer{RequestResult::FAILED});
        }
    }
}

bool Device::R2000::verifyErrorCode(const Device::PropertyTree &tree) {
    const auto code{tree.get_optional<int>(ERROR_CODE)};
    const auto text{tree.get_optional<std::string>(ERROR_TEXT)};
    return (code && (*code) == 0 && text && (*text) == "success");
}

void Device::R2000::scheduleNextEndpointResolutionDeadline(std::chrono::milliseconds timeout) {
    resolverDeadline.expires_from_now(
            boost::asio::chrono::milliseconds(std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count()));
    resolverDeadline.async_wait([&](const auto &) {
        onEndpointResolutionDeadlineReached();
    });
}

void Device::R2000::scheduleNextSocketConnectionDeadline(std::chrono::milliseconds timeout) {
    socketDeadline.expires_from_now(
            boost::asio::chrono::milliseconds(std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count()));
    socketDeadline.async_wait([&](const auto &) { onSocketConnectionDeadlineReached(); });
}

void Device::R2000::onEndpointResolutionDeadlineReached() {
    const auto expiryPoint{resolverDeadline.expiry()};
    if (expiryPoint <= boost::asio::steady_timer::clock_type::now()) {
        asyncResolver.cancel();
    }
}

void Device::R2000::onSocketConnectionDeadlineReached() {
    const auto expiryPoint{socketDeadline.expiry()};
    if (expiryPoint <= boost::asio::steady_timer::clock_type::now()) {
        boost::system::error_code placeholder{};
        asyncSocket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, placeholder);
        asyncSocket.close(placeholder);
    }
}

Device::R2000::~R2000() {
    boost::system::error_code placeholder{};
    resolverDeadline.cancel(placeholder);
    socketDeadline.cancel(placeholder);
    cancelPendingCommands();
    {
        std::unique_lock<std::mutex> guard{ioServiceLock, std::adopt_lock};
        interruptIoServiceTask.store(true, std::memory_order_release);
        ioService.stop();
        ioServiceTaskCv.notify_one();
    }
    ioServiceTaskFuture.wait();
}

std::string Device::R2000::makeRequestFromParameters(const std::string &command,
                                                     const Parameters::ParametersMap &parameters) {
    auto request{"/cmd/" + command};
    if (!parameters.empty())
        request += "?";
    for (const auto &parameter: parameters)
        request += parameter.first + "=" + parameter.second + "&";
    if (request.back() == '&')
        request.pop_back();
    return request;
}

boost::asio::ip::tcp::resolver::query Device::R2000::getDeviceQuery(const DeviceConfiguration &configuration) {
    const auto deviceAddress{configuration.deviceAddress.to_string()};
    const auto port{std::to_string(configuration.httpServicePort)};
    boost::asio::ip::tcp::resolver::query query{deviceAddress, port};
    return query;
}

void Device::R2000::ioServiceTask() {
    for (; !interruptIoServiceTask.load(std::memory_order_acquire);) {
        ioService.run();
        std::unique_lock<std::mutex> interruptGuard{ioServiceLock, std::adopt_lock};
        ioServiceTaskCv.wait(interruptGuard);
        ioService.reset();
    }
}

