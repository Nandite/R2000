//
// Created by chazz on 10/01/2022.
//
#include "R2000/R2000.hpp"
#include <boost/asio.hpp>
#include "R2000/Control/Commands.hpp"

using namespace std::chrono_literals;
namespace basio = boost::asio;

int main() {

    const auto device{Device::R2000::makeShared({"R2000", "192.168.2.30"})};
    Device::Commands::FetchParametersCommand fetchParametersCommand{*device};
    Device::Commands::RebootDeviceCommand rebootDeviceCommand{*device};
    Device::Commands::Command<Device::Commands::RebootDevice> reboot{*device};

    auto future{rebootDeviceCommand.asyncExecute(5s)};
    if (!future) {
        std::clog << "Could not issue the reboot command" << std::endl;
        return EXIT_FAILURE;
    }
    future->wait();
    auto result{future->get()};
    if (result != Device::RequestResult::SUCCESS) {
        std::clog << "Failed to reboot the device" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Device rebooted" << std::endl;
    return EXIT_SUCCESS;
}
