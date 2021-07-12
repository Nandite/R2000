#include "DataLinkBuilder.hpp"
#include "HttpController.hpp"
#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <jsoncpp/json/json.h>
#include <thread>

using namespace std::chrono_literals;
namespace basio = boost::asio;

int main()
{
    R2000::HttpDeviceConfiguration deviceConfiguration(basio::ip::address::from_string("192.168.1.30"));
    const auto controller = R2000::HttpController::Factory(deviceConfiguration);

    const auto result = controller->rebootDevice(60s);
    if (result == R2000::RebootStatus::FAILED || result == R2000::RebootStatus::TIMEOUT)
    {
        std::clog << "Failed to reboot the device" << std::endl;
        return EXIT_FAILURE;
    }
    if (!controller->displayText({"Gorkem+Keser", "IUT+de+Rennes"}))
    {
        std::clog << "Could not display the required text on the sensor" << std::endl;
    }
    std::this_thread::sleep_for(5s);

    try
    {
        const auto dataLink = R2000::DataLinkBuilder()
                                  .withProtocol(R2000::DeviceHandle::PROTOCOL::TCP)
                                  .withPacketType(R2000::DeviceHandle::PACKET_TYPE::C)
                                  .withStartAngle(-180)
                                  .withPort(43000)
                                  .withTargetHostname("192.168.1.40")
                                  .withWatchdogEnabled()
                                  .withWatchdogTimeout(200ms)
                                  .build(controller);
        std::this_thread::sleep_for(20s);
        std::cout << "Handle opened" << std::endl;
    }
    catch (...)
    {
        std::clog << "Error, could not request any handle" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
