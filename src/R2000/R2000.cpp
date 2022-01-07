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
    ParametersMap parameters{};
    if (!parameter.empty())
        parameters[parameter] = std::move(value);
    return sendHttpCommand(command, parameters);
}

std::pair<bool, Device::PropertyTree>
Device::R2000::sendHttpCommand(const std::string &command, const ParametersMap &parameters) const noexcept(false) {
    auto request{"/cmd/" + command};
    if (!parameters.empty())
        request += "?";
    for (const auto &parameter : parameters)
        request += parameter.first + "=" + parameter.second + "&";
    if (request.back() == '&')
        request.pop_back();
    Device::PropertyTree propertyTree{};
    try {
        const auto result{httpGet(request)};
        const auto responseCode{std::get<0>(result)};
        const auto content{std::get<2>(result)};
        std::stringstream stringStream{content};
        boost::property_tree::json_parser::read_json(stringStream, propertyTree);
        return {(responseCode == 200) && verifyErrorCode(propertyTree), std::move(propertyTree)};
    }
    catch (const std::system_error &error) {
        std::clog << __func__ << ": Http request error : " << error.what() << " (" << error.code() << ")" << std::endl;
    }
    return {false, {}};
}

std::tuple<int, std::string, std::string> Device::R2000::httpGet(const std::string &requestPath) const
noexcept(false) {
    const auto deviceAddress{mConfiguration.deviceAddress.to_string()};
    const auto port{std::to_string(mConfiguration.httpServicePort)};
    std::string header{};
    std::string content{};
    boost::asio::io_service ioService{};
    boost::asio::ip::tcp::resolver resolver{ioService};
    boost::asio::ip::tcp::resolver::query query{deviceAddress, port};
    auto endpointIterator{resolver.resolve(query)};
    boost::asio::ip::tcp::resolver::iterator endpointEndIterator{};
    SocketGuard<boost::asio::ip::tcp::socket> socketGuard{ioService};
    auto &socket{socketGuard.getUnderlyingSocket()};
    {
        boost::system::error_code error{boost::asio::error::host_not_found};
        for (; error && endpointIterator != endpointEndIterator;) {
            socket.close();
            socket.connect(*endpointIterator++, error);
        }
        if (error) {
            throw std::system_error(error);
        }
    }

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
    for (; boost::asio::read(socket, response,
                             boost::asio::transfer_at_least(1),
                             error);) {
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

bool Device::R2000::verifyErrorCode(const Device::PropertyTree &tree) {
    const auto code{tree.get_optional<int>(ERROR_CODE)};
    const auto text{tree.get_optional<std::string>(ERROR_TEXT)};
    return (code && (*code) == 0 && text && (*text) == "success");
}

std::optional<Device::PFSDP> Device::R2000::getDeviceProtocolVersion() const {

    if (!deviceProtocolVersion) {
        Commands::GetProtocolInfoCommand getProtocolInfoCommand{*this};
        const auto result{getProtocolInfoCommand.execute()};
        if (result) {
            const auto parameters{(*result).first};
            const auto major{parameters.at(PARAMETER_PROTOCOL_VERSION_MAJOR)};
            const auto minor{parameters.at(PARAMETER_PROTOCOL_VERSION_MINOR)};
            deviceProtocolVersion = {protocolVersionFromString(major, minor)};
            return deviceProtocolVersion;
        }
    }
    return deviceProtocolVersion;
}
