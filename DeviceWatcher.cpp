
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
#include "Backtrace.hpp"

using namespace std::chrono_literals;
namespace basio = boost::asio;

/**
 *
 * @param device
 * @param status
 * @return
 */
std::string
formatDeviceStatus(const Device::R2000 &device, const Device::DeviceStatus &status, Device::Parameters::PFSDP version) {
    std::stringstream stream{};
    const auto &statusFlags{status.getStatusFlags()};
    constexpr auto columnWidth{55u};
    constexpr auto flagsColumnWidth{63u};
    stream << "--------------------------------------------------------------------" << std::endl;
    stream << device.getName() << "/" << device.getHostname() << std::endl;
    stream << std::left << std::setw(columnWidth) << "+ Cpu Load : " << status.getCpuLoad() << std::endl;
    stream << std::left << std::setw(columnWidth) << "+ Raw system time : " << status.getRawSystemTime() << std::endl;
    stream << std::left << std::setw(columnWidth) << "+ Up time : " << status.getUptime() << std::endl;
    stream << std::left << std::setw(columnWidth) << "+ Power cycles count : " << status.getPowerCyclesCount()
           << std::endl;
    stream << std::left << std::setw(columnWidth) << "+ Operation time : " << status.getOperationTime() << std::endl;
    stream << std::left << std::setw(columnWidth) << "+ Operation time (scaled) : " << status.getScaledOperationTime()
           << std::endl;
    stream << std::left << std::setw(columnWidth) << "+ Minimal temperature : " << status.getMinimalTemperature()
           << std::endl;
    stream << std::left << std::setw(columnWidth) << "+ Current temperature : " << status.getCurrentTemperature()
           << std::endl;
    stream << std::left << std::setw(columnWidth) << "+ Maximal temperature : " << status.getMaximalTemperature()
           << std::endl;
    stream << "--------------------------------------------------------------------" << std::endl;
    stream << "Device status flags" << std::boolalpha << std::endl;
    stream << std::left << std::setw(flagsColumnWidth) << "+ Device error :" << statusFlags.deviceHasError()
           << std::endl;
    stream << std::left << std::setw(flagsColumnWidth) << "+ Device warning :" << statusFlags.deviceHasWarning()
           << std::endl;
    stream << std::left << std::setw(flagsColumnWidth) << "+ Initialization :" << statusFlags.isInitializing()
           << std::endl;
    stream << std::left << std::setw(flagsColumnWidth) << "+ Output scan muted :" << statusFlags.outputScanIsMuted()
           << std::endl;
    stream << std::left << std::setw(flagsColumnWidth) << "+ Unstable head rotation :"
           << statusFlags.headHasUnstableRotation() << std::endl;
    if (version >= Device::Parameters::PFSDP::V103) {
        stream << std::left << std::setw(flagsColumnWidth) << "+ Lens contamination (warning) :"
               << statusFlags.hasLensContaminationWarning() << std::endl;
        stream << std::left << std::setw(flagsColumnWidth) << "+ Lens contamination (Error) :"
               << statusFlags.hasLensContaminationError() << std::endl;
    }
    stream << std::left << std::setw(flagsColumnWidth) << "+ Temperature is low (warning):"
           << statusFlags.hasLowTemperatureWarning() << std::endl;
    stream << std::left << std::setw(flagsColumnWidth) << "+ Temperature is low (error) :"
           << statusFlags.hasLowTemperatureError() << std::endl;
    stream << std::left << std::setw(flagsColumnWidth) << "+ Temperature is high (warning) :"
           << statusFlags.hasHighTemperatureWarning() << std::endl;
    stream << std::left << std::setw(flagsColumnWidth) << "+ Temperature is high (error) :"
           << statusFlags.hasHighTemperatureError() << std::endl;
    stream << std::left << std::setw(flagsColumnWidth) << "+ Device overload (warning) :"
           << statusFlags.hasDeviceOverloadWarning() << std::endl;
    stream << std::left << std::setw(flagsColumnWidth) << "+ Device overload (error) :"
           << statusFlags.hasDeviceOverloadError() << std::endl;
    stream << std::left << std::setw(flagsColumnWidth) << "+ Unrecoverable default :"
           << statusFlags.hasUnrecoverableDefect() << std::endl;
    stream << "--------------------------------------------------------------------" << std::noboolalpha << std::endl;
    return stream.str();
}

int main(int argc, char **argv) {
    std::signal(SIGSEGV, Backtrace::printBacktraceAndExitHandler);
    std::signal(SIGABRT, Backtrace::printBacktraceAndExitHandler);
    std::signal(SIGILL, Backtrace::printBacktraceAndExitHandler);
    std::signal(SIGFPE, Backtrace::printBacktraceAndExitHandler);
    std::signal(SIGPIPE, Backtrace::printBacktraceAndExitHandler);
    std::signal(SIGTERM, Backtrace::printBacktraceAndExitHandler);

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
    if (result.first != Device::AsyncRequestResult::SUCCESS) {
        std::clog << "Could not get the device version (" << Device::asyncResultToString(result.first) << ")"
                  << std::endl;
    }
    auto deviceVersion{result.second};
    const auto period{1s};
    std::unique_ptr<Device::StatusWatcher> statusWatcher{nullptr};
    if (outputPath.empty()) {
        auto onStatusReceived{[&device, deviceVersion](const Device::DeviceStatus &status) -> void {
            const auto statusAsString{formatDeviceStatus(*device, status, deviceVersion)};
            std::cout << statusAsString << std::endl;
        }};
        statusWatcher = std::make_unique<Device::StatusWatcher>(device, period);
        statusWatcher->addCallback(onStatusReceived);
    } else {
        auto onStatusReceived{[&device, &outputPath, deviceVersion](const Device::DeviceStatus &status) -> void {
            const auto statusAsString{formatDeviceStatus(*device, status, deviceVersion)};
            std::ofstream stream(outputPath, std::ios::trunc);
            if (!stream.is_open())
                return;
            stream << statusAsString << std::endl;
        }};
        statusWatcher = std::make_unique<Device::StatusWatcher>(device, period);
        statusWatcher->addCallback(onStatusReceived);
    }
    std::this_thread::sleep_for(2s);
    if (!statusWatcher->isAlive()) {
        std::clog << "Could not establish status link with sensor at " << device->getHostname() << std::endl;
    }
    std::this_thread::sleep_for(5h);
    return EXIT_SUCCESS;
}

