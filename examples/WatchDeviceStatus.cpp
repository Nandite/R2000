
#include "Control/Commands.hpp"
#include "R2000.hpp"
#include "StatusWatcher.hpp"
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

    boost::program_options::variables_map programOptions{};
    boost::program_options::options_description programOptionsDescriptions{
            "Execute a device watcher over the network. Program usage:", 1024, 512};
    programOptionsDescriptions.add_options()("help,h", "Print out how to use the program")
            ("address,a", boost::program_options::value<std::string>()->required(), "Address of the device")
            ("output,o", boost::program_options::value<std::string>()->default_value(""),
             "Output file path to save the status data in.");
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
    const auto outputPath{programOptions["output"].as<std::string>()};
    const auto device{Device::R2000::makeShared({"R2000", deviceAddress})};

    Device::Commands::GetProtocolVersionCommand getProtocolVersion{*device};
    auto future{getProtocolVersion.asyncExecute(3s)};
    future.wait();
    auto result{future.get()};
    if (result.first != Device::RequestResult::SUCCESS) {
        std::clog << "Could not get the device version (" << Device::asyncResultToString(result.first) << ")"
                  << std::endl;
    }
    auto deviceVersion{result.second};
    const auto period{1s};
    std::unique_ptr<Device::StatusWatcher> statusWatcher{nullptr};
    if (outputPath.empty()) {
        statusWatcher = std::make_unique<Device::StatusWatcher>(device, period);
        statusWatcher->addOnStatusAvailableCallback([&device, deviceVersion](auto status) {
            std::cout << formatDeviceStatus(*device, *status, deviceVersion) << std::endl;
        });
    } else {
        statusWatcher = std::make_unique<Device::StatusWatcher>(device, period);
        statusWatcher->addOnStatusAvailableCallback([&device, &outputPath, deviceVersion](auto status) {
            std::ofstream stream(outputPath, std::ios::trunc);
            if (!stream.is_open())
                return;
            stream << formatDeviceStatus(*device, *status, deviceVersion) << std::endl;
        });
    }
    while (!interruptProgram) {
        std::this_thread::sleep_for(1s);
    }
    std::cout << std::endl << "Stopping device watcher." << std::endl;
    return EXIT_SUCCESS;
}

