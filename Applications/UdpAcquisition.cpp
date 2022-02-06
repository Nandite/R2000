#include "Backtrace.hpp"
#include "R2000/Control/Parameters.hpp"
#include "R2000/DataLink/DataLink.hpp"
#include "R2000/DataLink/DataLinkBuilder.hpp"
#include "R2000/R2000.hpp"
#include "ScanToPointCloud.hpp"
#include <boost/asio.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <chrono>
#include <iostream>
#include <pcl/common/transforms.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <thread>
#include <valarray>

#define FREQUENCY 35
#define SAMPLES_PER_SCAN 7200
#define START_ANGLE (-1800000)
#define WATCHDOG_TIMEOUT 5000
#define PACKET_TYPE Device::Parameters::PACKET_TYPE::A

using namespace std::chrono_literals;

/**
 * Cast string to a type T.
 * @tparam T The type to cast the string to.
 * @param representation The string to cast.
 * @return An optional containing the casted type or nothing if the string cannot be casted into T.
 */
template<typename T>
std::optional<T> lexicalCast(const std::string &representation) {
    try {
        return {boost::lexical_cast<T>(representation)};
    }
    catch (const boost::bad_lexical_cast &) {
        return std::nullopt;
    }
}

/**
 * @param address The address to test.
 * @return True if the address has an ipv4 valid form, False otherwise.
 */
bool isValidIpv4(const std::string &address) {
    boost::system::error_code errorCode{};
    const auto ipv4Address{boost::asio::ip::address::from_string(address, errorCode)};
    return !errorCode && ipv4Address.is_v4();
}

/**
 * Split a string under the form address:port into a string containing the address and the port as an unsigned short.
 * @param input The string to split.
 * @return An optional of pair containing the address and the port.
 */
std::optional<std::pair<std::string, unsigned short>> splitAddressAndPort(const std::string &input) {
    std::stringstream stream{input};
    std::vector<std::string> fragments{};
    std::string fragment{};
    while (std::getline(stream, fragment, ':')) {
        fragments.push_back(fragment);
    }
    if (fragments.size() < 2) {
        return std::nullopt;
    }
    const auto &address{fragments[0]};
    const auto &port{fragments[1]};
    const auto portFromLexicalCast{lexicalCast<unsigned int>(port)};
    if (!isValidIpv4(address)) {
        std::clog << "You must specify a valid ipv4 target address (" << address << ")." << std::endl;
        return std::nullopt;
    }
    if (!portFromLexicalCast) {
        std::clog << "You must specify a valid portFromLexicalCast value (" << port << ")." << std::endl;
        return std::nullopt;
    }
    if (*portFromLexicalCast > 65535u) {
        std::clog << "Bad target port range (" << *portFromLexicalCast
                  << "). Supported port range is 0 - 65535" << std::endl;
        return std::nullopt;
    }
    return {{address, *portFromLexicalCast}};
}

bool interruptProgram{false};
void interrupt(int) {
    interruptProgram = true;
}

int main(int argc, char **argv) {

    std::signal(SIGTERM, interrupt);
    std::signal(SIGKILL, interrupt);
    std::signal(SIGINT, interrupt);

    using Point = pcl::PointXYZ;
    using PointCloudColorHandler = pcl::visualization::PointCloudColorHandlerCustom<Point>;
    const auto VIEWER_POINT_SIZE_ID{pcl::visualization::PCL_VISUALIZER_POINT_SIZE};
    const auto VIEWER_SCAN_CLOUD_ID{"scan_cloud"};

    boost::program_options::variables_map programOptions{};
    boost::program_options::options_description programOptionsDescriptions{
            "Perform a continuous scan data acquisition from a R2000 sensor and display it.", 1024, 512};
    programOptionsDescriptions
            .add_options()("help,h", "Print out how to use the program")
                    ("address,a", boost::program_options::value<std::string>()->required(),
                     "Address of the device.")
                    ("target,t", boost::program_options::value<std::string>()->required(),
                     "Address and port at which the scan must be sent. The address must be under the format <address:port>.");
    try {
        boost::program_options::parsed_options parsedProgramOptions{
                boost::program_options::command_line_parser(argc, argv).options(programOptionsDescriptions).run()};
        boost::program_options::store(parsedProgramOptions, programOptions);
    }
    catch (const boost::program_options::unknown_option &error) {
        std::clog << "Unknown option (" << error.get_option_name() << ")." << std::endl;
        return EXIT_FAILURE;
    }
    catch (const boost::program_options::invalid_command_line_syntax &error) {
        std::clog << "The command syntax is invalid for the argument " << error.get_option_name() << " ("
                  << error.what() << ")" << std::endl;
        return EXIT_FAILURE;
    }
    const auto isHelpRequested{programOptions.count("help") > 0};
    if (isHelpRequested) {
        std::cout << programOptionsDescriptions << std::endl;
        return EXIT_SUCCESS;
    }
    try {
        boost::program_options::notify(programOptions);
    }
    catch (const boost::program_options::required_option &error) {
        std::clog << "Missing mandatory argument (" << error.get_option_name() << ")." << std::endl;
        std::clog << programOptionsDescriptions << std::endl;
        return EXIT_FAILURE;
    }

    const auto deviceAddressAsString{programOptions["address"].as<std::string>()};
    if (deviceAddressAsString.empty()) {
        std::clog << "You must specify the device address." << std::endl;
        return EXIT_FAILURE;
    }
    if (!isValidIpv4(deviceAddressAsString)) {
        std::clog << "You must specify a valid ipv4 device address (" << deviceAddressAsString << ")." << std::endl;
        return EXIT_FAILURE;
    }
    const auto target{splitAddressAndPort(programOptions["target"].as<std::string>())};
    if (!target) {
        std::clog << "You must specify the scan destination with the parameter and format --target <address(ipv4):port>"
                  << std::endl;
        return EXIT_FAILURE;
    }

    const auto device{Device::R2000::makeShared({"R2000", deviceAddressAsString})};

    Device::Commands::SetParametersCommand setParametersCommand{*device};
    const auto hmiBuilder{Device::Parameters::ReadWriteParameters::HmiDisplay()
                                  .unlockHmiButton()
                                  .unlockHmiParameters()
                                  .withHmiLanguage(Device::Parameters::Language::ENGLISH)
                                  .withHmiDisplayMode(Device::Parameters::HMI_DISPLAY_MODE::APPLICATION_TEXT)
                                  .withHmiApplicationText1("UDP Scan")
                                  .withHmiApplicationText2("Acquisition")};
    const auto measureBuilder{Device::Parameters::ReadWriteParameters::Measure()
                                      .withOperatingMode(Device::Parameters::OPERATING_MODE::MEASURE)
                                      .withScanFrequency(FREQUENCY)
                                      .withSamplesPerScan(SAMPLES_PER_SCAN)
                                      .withScanDirection(Device::Parameters::SCAN_DIRECTION::CCW)};

    auto configureFuture{setParametersCommand.asyncExecute(1s, hmiBuilder, measureBuilder)};
    if (!configureFuture) {
        std::clog << "Could not configure the device (Busy)." << std::endl;
        return EXIT_FAILURE;
    }
    configureFuture->wait();
    const auto configureResult{configureFuture->get()};
    if (configureResult != Device::RequestResult::SUCCESS) {
        std::clog << "Could not configure the sensor (" << Device::asyncResultToString(configureResult) << ")."
                  << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Device configured. Setting up data link..." << std::endl;
    auto handleParameters{Device::Parameters::ReadWriteParameters::UdpHandle{}
                                  .withHostname(target->first)
                                  .withPort(target->second)
                                  .withPacketType(PACKET_TYPE)
                                  .withStartAngle(START_ANGLE)
                                  .withWatchdog()
                                  .withWatchdogTimeout(WATCHDOG_TIMEOUT)};
    auto future{Device::DataLinkBuilder(handleParameters).build(device, 1s)};
    future.wait();
    auto buildResult{future.get()};
    const auto asyncRequestResult{buildResult.first};
    if (asyncRequestResult != Device::RequestResult::SUCCESS) {
        std::clog << "Could not establish a data link with sensor at " << device->getHostname() << " ("
                  << Device::asyncResultToString(asyncRequestResult) << ")." << std::endl;
        return EXIT_FAILURE;
    }
    std::shared_ptr<Device::DataLink> dataLink{buildResult.second};

    const auto port{0};
    pcl::visualization::PCLVisualizer viewer{"Scan viewer"};
    viewer.setBackgroundColor(0.35, 0.35, 0.35, port);
    viewer.setSize(1280, 1024);
    viewer.addCoordinateSystem(150.0f, 0.0f, 0.0f, 0.0f, "Zero");
    PointCloud::ScanToPointCloud<Point> converter{SAMPLES_PER_SCAN, -M_PI};

    while (!viewer.wasStopped() && !interruptProgram) {
        viewer.spinOnce();
        const auto &scan{dataLink->waitForNextScan(250ms)};
        if (!dataLink->isAlive()) {
            std::clog << "A disconnection of the sensor, or a network error has occurred." << std::endl;
            break;
        }
        if (!scan) {
            continue;
        }
        pcl::PointCloud<Point>::Ptr cloud{new pcl::PointCloud<Point>};
        converter.convert(*scan, *cloud);
        PointCloudColorHandler scannedCloudColor{cloud, 0, 240, 0};
        viewer.removePointCloud(VIEWER_SCAN_CLOUD_ID, port);
        viewer.addPointCloud(cloud, scannedCloudColor, VIEWER_SCAN_CLOUD_ID, port);
        viewer.setPointCloudRenderingProperties(VIEWER_POINT_SIZE_ID, 2, VIEWER_SCAN_CLOUD_ID, port);
    }

    viewer.close();
    return EXIT_SUCCESS;
}
