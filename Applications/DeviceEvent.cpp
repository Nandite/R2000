
#include "R2000/Control/Commands.hpp"
#include "R2000/R2000.hpp"
#include "R2000/StatusWatcher.hpp"
#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <thread>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>

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

int main(int argc, char **argv) {

    std::signal(SIGTERM, interrupt);
    std::signal(SIGKILL, interrupt);
    std::signal(SIGINT, interrupt);

    boost::program_options::variables_map programOptions{};
    boost::program_options::options_description programOptionsDescriptions{
            "Execute a device watcher over the network. Program usage:", 1024, 512};
    programOptionsDescriptions.add_options()("help,h", "Print out how to use the program")
            ("address,a", boost::program_options::value<std::string>()->required(), "Address of the device");
    boost::program_options::parsed_options parsedProgramOptions{
            boost::program_options::command_line_parser(argc, argv).options(programOptionsDescriptions).run()};
    boost::program_options::store(parsedProgramOptions, programOptions);
    const auto isHelpRequested{programOptions.count("help") > 0};
    if (isHelpRequested) {
        std::cout << programOptionsDescriptions << std::endl;
        return EXIT_SUCCESS;
    }
    try {
        boost::program_options::notify(programOptions);
    } catch (const boost::program_options::required_option &error) {
        std::clog << "Missing mandatory argument (" << error.get_option_name() << ")." << std::endl;
        std::clog << programOptionsDescriptions << std::endl;
        return EXIT_FAILURE;
    }
    const auto deviceAddress{programOptions["address"].as<std::string>()};
    if (!isValidIpv4(deviceAddress)) {
        std::clog << "You must specify a valid ipv4 device address (" << deviceAddress << ")." << std::endl;
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

