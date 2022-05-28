#include "R2000.hpp"
#include "StatusWatcher.hpp"
#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <thread>

using namespace std::chrono_literals;

/**
 * @param address The address to test.
 * @return True if the address has an ipv4 valid form, False otherwise.
 */
[[nodiscard]] bool isValidIpv4(const std::string &address) {
    boost::system::error_code errorCode{};
    const auto ipv4Address{boost::asio::ip::address::from_string(address, errorCode)};
    return !errorCode && ipv4Address.is_v4();
}

bool interruptProgram{false};

void interrupt(int) {
    interruptProgram = true;
}

void printUsage(std::ostream & stream)
{
    stream << "Execute a device watcher over the network. Program usage:" << std::endl
        << "./DeviceEventWatcher <ipv4>";
}

int main(int argc, char **argv) {

    std::signal(SIGTERM, interrupt);
    std::signal(SIGKILL, interrupt);
    std::signal(SIGINT, interrupt);

    if(argc < 2)
    {
        printUsage(std::cout);
        return EXIT_SUCCESS;
    }
    const std::string deviceAddress{argv[1]};
    if(!isValidIpv4(deviceAddress))
    {
        std::clog << "The provided address is not valid [" << deviceAddress << "]" << std::endl;
        printUsage(std::clog);
        return EXIT_FAILURE;
    }

    const auto device{Device::R2000::makeShared({"R2000", deviceAddress})};
    Device::StatusWatcher statusWatcher{device, 2s};
    statusWatcher.addOnDeviceConnectedCallback([&]() {
        std::cout << device->getName() << " has connected at [" << device->getHostname() << "]" << std::endl;
    });
    statusWatcher.addOnDeviceDisconnectedCallback([&]() {
        std::cout << device->getName() << " has disconnected at [" << device->getHostname() << "]" << std::endl;
    });
    while (!interruptProgram) {
        std::this_thread::sleep_for(1s);
    }
    std::cout << std::endl << "Stopping device event." << std::endl;
    return EXIT_SUCCESS;
}

