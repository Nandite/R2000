//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#include "R2000.hpp"
#include "DeviceHandle.hpp"
#include <bitset>
#include <iostream>
#include <utility>

Device::R2000::R2000(Device::DeviceConfiguration configuration)
        : mConfiguration(std::move(configuration)) {
}

[[maybe_unused]] std::shared_ptr<Device::R2000>
Device::R2000::makeShared(const Device::DeviceConfiguration &configuration) {
    return enableMakeShared(configuration);
}

std::pair<bool, Device::PropertyTree> Device::R2000::sendHttpCommand(const std::string &command,
                                                                     const std::string &parameter,
                                                                     std::string value) const noexcept(false) {
    std::unordered_map<std::string, std::string> parameters{};
    if (!parameter.empty())
        parameters[parameter] = std::move(value);
    return sendHttpCommand(command, parameters);
}

std::pair<bool, Device::PropertyTree>
Device::R2000::sendHttpCommand(const std::string &command, const ParametersMap &parameters) const noexcept(false) {
    auto request = "/cmd/" + command;
    if (!parameters.empty())
        request += "?";
    for (auto &parameter : parameters)
        request += parameter.first + "=" + parameter.second + "&";
    if (request.back() == '&')
        request.pop_back();
    const auto result{httpGet(request)};
    const auto responseCode{std::get<0>(result)};
    const auto content{std::get<2>(result)};
    Device::PropertyTree propertyTree{};
    std::stringstream stringStream{content};
    try {
        boost::property_tree::json_parser::read_json(stringStream, propertyTree);
    }
    catch (...) {
        return {false, std::move(propertyTree)};
    }
    return {(responseCode == 200) && verifyErrorCode(propertyTree), std::move(propertyTree)};
}

std::tuple<int, std::string, std::string> Device::R2000::httpGet(const std::string &requestPath) const
noexcept(false) {
    const auto deviceAddress{mConfiguration.deviceAddress.to_string()};
    const auto port{std::to_string(mConfiguration.httpServicePort)};
    std::string header{};
    std::string content{};
    boost::asio::io_service io_service{};
    boost::asio::ip::tcp::resolver resolver{io_service};
    boost::asio::ip::tcp::resolver::query query{deviceAddress, port};
    auto endpointIterator{resolver.resolve(query)};
    boost::asio::ip::tcp::resolver::iterator endpointEndIterator{};
    SocketGuard<boost::asio::ip::tcp::socket> socketGuard{io_service};
    auto &socket{socketGuard.getUnderlyingSocket()};
    {
        boost::system::error_code error{boost::asio::error::host_not_found};
        for (; error && endpointIterator != endpointEndIterator;) {
            socket.close();
            socket.connect(*endpointIterator++, error);
        }
        if (error)
            throw std::system_error(error);
    }

    boost::asio::streambuf request{};
    std::ostream requestStream{&request};
    requestStream << "GET " << requestPath << " HTTP/1.0\r\n\r\n";
    boost::asio::write(socket, request);
    boost::asio::streambuf response{};
    boost::asio::read_until(socket, response, "\r\n");

    std::istream responseStream{&response};
    std::string httpVersion{};
    responseStream >> httpVersion;
    unsigned int statusCode{};
    responseStream >> statusCode;
    std::string status_message{};
    std::getline(responseStream, status_message);
    if (!responseStream || httpVersion.substr(0, 5) != "HTTP/") {
        std::clog << "Invalid response" << std::endl;
        return {0, "", ""};
    }

    // Read the response headers, which are terminated by a blank line.
    boost::asio::read_until(socket, response, "\r\n\r\n");

    std::string line{};
    while (std::getline(responseStream, line) && line != "\r")
        header += line + "\n";
    while (std::getline(responseStream, line))
        content += line;

    boost::system::error_code error{boost::asio::error::host_not_found};
    for (; boost::asio::read(socket, response, boost::asio::transfer_at_least(1), error);) {
        responseStream.clear();
        while (std::getline(responseStream, line))
            content += line;
    }

    if (error != boost::asio::error::eof)
        throw boost::system::system_error(error);

    // Substitute CRs by a space
    for (auto &character : header)
        if (character == '\r')
            character = ' ';
    for (auto &character : content)
        if (character == '\r')
            character = ' ';

    return {statusCode, header, content};
}

bool Device::R2000::verifyErrorCode(const Device::PropertyTree &tree) {
    const auto code{tree.get_optional<int>(ERROR_CODE)};
    const auto text{tree.get_optional<std::string>(ERROR_TEXT)};
    return (code && (*code) == 0 && text && (*text) == "success");
}
