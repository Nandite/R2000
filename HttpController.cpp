//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#include "HttpController.hpp"
#include "DeviceHandle.hpp"
#include <bitset>
#include <iostream>
#include <utility>

/**
 * Setup a new HTTP command interface.
 * @param configuration The device address and the chosen port.
 */
R2000::HttpController::HttpController(const R2000::HttpDeviceConfiguration& configuration)
    : mConfiguration(configuration)
{
}

/**
 *
 * @param configuration
 */
[[maybe_unused]] R2000::HttpController::HttpController(R2000::HttpDeviceConfiguration&& configuration)
    : mConfiguration(std::move(configuration))
{
}

[[maybe_unused]] std::shared_ptr<R2000::HttpController>
R2000::HttpController::Factory(const R2000::HttpDeviceConfiguration& configuration)
{
    return std::shared_ptr<R2000::HttpController>(new HttpController(configuration));
}

[[maybe_unused]] std::shared_ptr<R2000::HttpController>
R2000::HttpController::Factory(R2000::HttpDeviceConfiguration&& configuration)
{
    return std::shared_ptr<HttpController>(new HttpController(configuration));
}

/**
 * Use this method in order to send an HTTP Command with only one parameter and its corresponding value.
 * @param command Give a command among the available commands of the device.
 * @param parameters Give a parameter among the available parameters of the device.
 * @param values Give a value to the corresponding parameter
 * @return The method with the given command, parameter and its corresponding value if the parameter is not empty.
 */
std::pair<bool, R2000::PropertyTree> R2000::HttpController::sendHttpCommand(const std::string& command,
                                                                            const std::string& parameter,
                                                                            std::string value) noexcept(false)
{
    std::unordered_map<std::string, std::string> keyMap;
    if (!parameter.empty())
        keyMap[parameter] = std::move(value);
    return sendHttpCommand(command, keyMap);
}

/**
 * Use this method in order to send an HTTP Command with several parameters and their corresponding value.
 * @param command Give a command among the available commands of the device.
 * @param paramValues Give many parameters with their corresponding value.
 * @return The method with the given command, parameters and values if the HTTP Status code is equal to 200, an error
 * otherwise.
 */
std::pair<bool, R2000::PropertyTree> R2000::HttpController::sendHttpCommand(
    const std::string& command, const std::unordered_map<std::string, std::string>& parametersValues) noexcept(false)
{
    auto request = "/cmd/" + command + "?";
    for (auto& keyValue : parametersValues)
        request += keyValue.first + "=" + keyValue.second + "&";
    if (request.back() == '&')
        request.pop_back();
    const auto result = httpGet(request);
    const auto responseCode = std::get<0>(result);
    const auto content = std::get<2>(result);
    R2000::PropertyTree propertyTree;
    std::stringstream ss(content);
    try
    {
        boost::property_tree::json_parser::read_json(ss, propertyTree);
    }
    catch (...)
    {
        return {false, std::move(propertyTree)};
    }
    return {(responseCode == 200), std::move(propertyTree)};
}

/**
 * Send a HTTP GET
 * @param requestPath The last part of an URL with a slash leading.
 * @param header The response header returned as std::string.
 * @return The HTTP status code or 0 in case of an error.
 */
std::tuple<int, std::string, std::string> R2000::HttpController::httpGet(const std::string& requestPath) const
    noexcept(false)
{
    std::string header;
    std::string content;

    boost::asio::io_service io_service;
    boost::asio::ip::tcp::resolver resolver(io_service);
    boost::asio::ip::tcp::resolver::query query(mConfiguration.deviceAddress.to_string(),
                                                std::to_string(mConfiguration.httpServicePort));
    boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    boost::asio::ip::tcp::resolver::iterator end;
    // Create socket
    boost::asio::ip::tcp::socket socket(io_service);
    SocketGuard socketGuard(io_service);
    {
        boost::system::error_code error = boost::asio::error::host_not_found;
        while (error && endpoint_iterator != end)
        {
            socket.close();
            socket.connect(*endpoint_iterator++, error);
        }
        if (error)
            throw std::system_error(error);
    }

    // Prepare request
    boost::asio::streambuf request;
    std::ostream request_stream(&request);
    request_stream << "GET " << requestPath << " HTTP/1.0\r\n\r\n";

    boost::asio::write(socket, request);

    // Read the response status line. The response streambuf will automatically
    // grow to accommodate the entire line. The growth may be limited by passing
    // a maximum size to the streambuf constructor.
    boost::asio::streambuf response;
    boost::asio::read_until(socket, response, "\r\n");

    // Check that response is OK.
    std::istream response_stream(&response);
    std::string http_version;
    response_stream >> http_version;
    unsigned int status_code;
    response_stream >> status_code;
    std::string status_message;
    std::getline(response_stream, status_message);
    if (!response_stream || http_version.substr(0, 5) != "HTTP/")
    {
        std::clog << "Invalid response" << std::endl;
        return {0, "", ""};
    }

    // Read the response headers, which are terminated by a blank line.
    boost::asio::read_until(socket, response, "\r\n\r\n");

    // Process the response headers.
    std::string tmp;
    while (std::getline(response_stream, tmp) && tmp != "\r")
        header += tmp + "\n";

    // Write whatever content we already have to output.
    while (std::getline(response_stream, tmp))
        content += tmp;

    boost::system::error_code error = boost::asio::error::host_not_found;
    // Read until EOF, writing data to output as we go.
    while (boost::asio::read(socket, response, boost::asio::transfer_at_least(1), error))
    {
        response_stream.clear();
        while (std::getline(response_stream, tmp))
            content += tmp;
    }

    if (error != boost::asio::error::eof)
        throw boost::system::system_error(error);

    // Substitute CRs by a space
    for (auto& i : header)
        if (i == '\r')
            i = ' ';
    for (auto& i : content)
        if (i == '\r')
            i = ' ';

    return {status_code, header, content};
}

/**
 * Request the sensor to release a handle that has been previously given.
 * @param handle The handle to release.
 * @return True if the handle has been correctly released, false otherwise.
 * @throws if the Http request fails.
 */
[[maybe_unused]] bool R2000::HttpController::releaseHandle(const R2000::DeviceHandle& handle) noexcept(false)
{
    const auto result = sendHttpCommand("release_handle", "handle", handle.value);
    const auto status = result.first;
    const auto& propertyTree = result.second;
    return status && verifyErrorCode(propertyTree);
}

/**
 * Request the sensor to start the transmission of scan data for the data channel specified by the given handle.
 * @param handle The handle to release.
 * @return True if the handle has been correctly released, false otherwise.
 */
[[maybe_unused]] bool R2000::HttpController::startStream(const R2000::DeviceHandle& handle) noexcept(false)
{
    const auto result = sendHttpCommand("start_scanoutput", "handle", handle.value);
    const auto status = result.first;
    const auto& propertyTree = result.second;
    return status && verifyErrorCode(propertyTree);
}

/**
 * Request the sensor to stop the transmission of scan data for the data channel specified by the given handle.
 * @param handle The handle to release.
 * @return True if the handle has been correctly released, false otherwise.
 */
[[maybe_unused]] bool R2000::HttpController::stopStream(const R2000::DeviceHandle& handle) noexcept(false)
{
    const auto result = sendHttpCommand("stop_scanoutput", "handle", handle.value);
    const auto status = result.first;
    const auto& propertyTree = result.second;
    return status && verifyErrorCode(propertyTree);
}

/**
 * Request the sensor to feed the connection watchdog, each call of this command resets the watchdog timer.
 * @param handle The handle to release.
 * @return True if the handle has been correctly released, false otherwise.
 */
[[maybe_unused]] bool R2000::HttpController::feedWatchdog(const R2000::DeviceHandle& handle) noexcept(false)
{
    const auto result = sendHttpCommand("feed_watchdog", "handle", handle.value);
    const auto status = result.first;
    const auto& propertyTree = result.second;
    return status && verifyErrorCode(propertyTree);
}

std::optional<bool> R2000::HttpController::isRebooting() noexcept(true)
{
    try
    {
        const auto statusFg = getParameterValue("status_flags");
        if (statusFg.has_value())
        {
            const auto value = std::stoi(*statusFg);
            std::bitset<sizeof(int) * CHAR_BIT> bs(value);
            if (!bs[0])
            {
                return {false};
            }
            return {true};
        }
    }
    catch (boost::system::system_error&)
    {
        // No connection, probably rebooting
    }
    return std::nullopt;
}

/**
 * Request the sensor to reboot the device.
 * @return True if the command has been correctly received by the device, false otherwise.
 */
[[maybe_unused]] bool R2000::HttpController::rebootDevice() noexcept(false)
{
    const auto result = sendHttpCommand("reboot_device");
    const auto status = result.first;
    const auto& propertyTree = result.second;
    return status && verifyErrorCode(propertyTree);
}

/**
 * Request the sensor to get all the available parameters of the device.
 * @return The parameters list if the the command has been correctly received by the device and the parameter is not
 * empty, continues otherwise.
 */
[[maybe_unused]] R2000::ParametersList R2000::HttpController::fetchParametersList()
{
    std::vector<std::string> parameterList;

    const auto result = sendHttpCommand("list_parameters");
    const auto status = result.first;
    const auto& propertyTree = result.second;

    auto list = propertyTree.get_child_optional("parameters");
    if (!status && !verifyErrorCode(propertyTree))
        return {};
    for (const auto& name : *list)
    {
        auto parameterName = name.second.get<std::string>("");
        if (parameterName.empty())
            continue;
        parameterList.push_back(parameterName);
    }
    return parameterList;
}

/**
 * Request the sensor to get a parameter with its corresponding value.
 * @param name The parameter's name.
 * @return The parameter with its corresponding value if the command has been correctly received by the device, an error
 * otherwise.
 */
[[maybe_unused]] std::optional<std::string> R2000::HttpController::getParameterValue(const std::string& name)
{
    const auto result = sendHttpCommand("get_parameter", "list", name);
    const auto status = result.first;
    const auto& propertyTree = result.second;
    const auto value = propertyTree.get_optional<std::string>(name);
    if (!status || !verifyErrorCode(propertyTree) || !value)
        return std::nullopt;
    return {*value};
}

/**
 * Request the sensor to get many parameters with their corresponding value.
 * @param list The parameters list.
 * @return The parameters with their corresponding value if the command has been correctly received by the device, skips
 * otherwise the empty parameter.
 */
[[maybe_unused]] std::unordered_map<std::string, std::string>
R2000::HttpController::getParametersValues(const R2000::ParametersList& list)
{
    std::unordered_map<std::string, std::string> keysValues;
    std::string namelist;
    for (const auto& name : list)
        namelist += (name + ";");
    namelist.pop_back();
    const auto result = sendHttpCommand("get_parameter", "list", namelist);
    const auto status = result.first;
    const auto& propertyTree = result.second;
    if (!status && !verifyErrorCode(propertyTree))
        return {};
    for (const auto& name : list)
    {
        auto value = propertyTree.get_optional<std::string>(name);
        if (!value)
        {
            keysValues.insert(std::make_pair(name, ""));
            continue;
        }
        keysValues.insert(std::make_pair(name, *value));
    }
    return keysValues;
}

/**
 * Request the sensor to get all the available parameters with their corresponding value.
 * @return The parameters with their corresponding value if the the command has been correctly received by the device,
 * skips otherwise the empty parameters.
 */
[[maybe_unused]] std::unordered_map<std::string, std::string> R2000::HttpController::pullAllParametersValues()
{
    const auto list = fetchParametersList();
    if (list.empty())
        return {};
    return getParametersValues(list);
}

/**
 * Request the sensor to set plenty parameters with the chosen values.
 * @param parameters Give the parameters among the available parameters of the device.
 * @return True if the command has been correctly received by the device, false otherwise.
 */
[[maybe_unused]] bool
R2000::HttpController::setParameters(const std::unordered_map<std::string, std::string>& parameters) noexcept(false)
{
    const auto result = sendHttpCommand("set_parameter", parameters);
    const auto status = result.first;
    const auto& propertyTree = result.second;
    return status && verifyErrorCode(propertyTree);
}

/**
 * Request the sensor to set only one parameter with given value.
 * @param name Give the parameter among the available parameters of the device.
 * @param value Give the value to the corresponding parameter.
 * @return True if the command has been correctly received by the device, false otherwise.
 */
[[maybe_unused]] bool R2000::HttpController::setParameter(const std::string& name,
                                                          const std::string& value) noexcept(false)
{
    const auto result = sendHttpCommand("set_parameter", name, value);
    const auto status = result.first;
    const auto& propertyTree = result.second;
    return status && verifyErrorCode(propertyTree);
}

/**
 * Request the sensor to reset the given parameters to factory default.
 * @param list The chosen parameters list.
 * @return True if the command has been correctly received by the device, false otherwise.
 */
[[maybe_unused]] bool R2000::HttpController::resetParameters(const R2000::ParametersList& list) noexcept(false)
{
    std::string parametersList;
    for (const auto& names : list)
        parametersList += (names + ";");
    parametersList.pop_back();
    const auto result = sendHttpCommand("reset_parameters", "list", parametersList);
    const auto status = result.first;
    const auto& propertyTree = result.second;
    return status && verifyErrorCode(propertyTree);
}

/**
 * Request the sensor to reset all the available parameters of the device to factory default.
 * @return True if the command has been correctly received by the device, false otherwise.
 */
[[maybe_unused]] bool R2000::HttpController::factoryResetParameters() noexcept(false)
{
    const auto result = sendHttpCommand("factory_reset");
    const auto status = result.first;
    const auto& propertyTree = result.second;
    return status && verifyErrorCode(propertyTree);
}
bool R2000::HttpController::displayText(const std::array<std::string, 2>& lines) noexcept(false)
{
    if (setParameter("hmi_display_mode", "application_text"))
    {
        std::unordered_map<std::string, std::string> params;
        params["hmi_application_text_1"] = lines[0];
        params["hmi_application_text_2"] = lines[1];
        return setParameters(params);
    }
    return false;
}
/**
 * Verify if the sensor does not generate an error.
 * @param tree Represent the property tree.
 * @return True if the error code is equal to 0 and the error text is equal to "success", false otherwise.
 */
bool R2000::HttpController::verifyErrorCode(const R2000::PropertyTree& tree)
{
    boost::optional<int> code = tree.get_optional<int>("error_code");
    boost::optional<std::string> text = tree.get_optional<std::string>("error_text");
    return (code && (*code) == 0 && text && (*text) == "success");
}
