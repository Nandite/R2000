#include "Backtrace.hpp"
#include "R2000/Control/Parameters.hpp"
#include "R2000/DataLink/DataLink.hpp"
#include "R2000/DataLink/DataLinkBuilder.hpp"
#include "R2000/R2000.hpp"
#include "ScanToPointCloud.hpp"
#include <boost/algorithm/string.hpp>
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

using namespace std::chrono_literals;
namespace basio = boost::asio;

/**
 *
 * @tparam T
 * @param representation
 * @return
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
 *
 * @param address
 * @return
 */
bool isValidIpv4(const std::string &address) {
    boost::system::error_code errorCode{};
    const auto ipv4Address{boost::asio::ip::address::from_string(address, errorCode)};
    return !errorCode && ipv4Address.is_v4();
}

/**
 *
 * @param argument
 * @return
 */
std::optional<std::pair<std::string, unsigned short>> splitAddressAndPort(const std::string &argument) {
    std::stringstream stream{argument};
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
        std::clog << "Bad target portFromLexicalCast range (" << *portFromLexicalCast
                  << "). Support portFromLexicalCast range is 0 - 65535" << std::endl;
        return std::nullopt;
    }
    return {{address, *portFromLexicalCast}};
}

/**
 *
 * @param device
 * @param frequency
 * @param samplePerScan
 * @return
 */
bool configureDevice(Device::R2000 &device, const std::string &frequency, const std::string &samplePerScan) {

    Device::Commands::SetParametersCommand setParametersCommand{device};
    const auto hmiBuilder{Device::Parameters::ReadWriteParameters::HmiDisplay()
                                  .unlockHmiButton()
                                  .unlockHmiParameters()
                                  .withHmiLanguage(Device::Parameters::Language::ENGLISH)
                                  .withHmiDisplayMode(Device::Parameters::HMI_DISPLAY_MODE::APPLICATION_TEXT)
                                  .withHmiApplicationText1("Scan")
                                  .withHmiApplicationText2("Acquisition")};
    const auto measureBuilder{Device::Parameters::ReadWriteParameters::Measure()
                                      .withOperatingMode(Device::Parameters::OPERATING_MODE::MEASURE)
                                      .withScanFrequency(frequency)
                                      .withSamplesPerScan(samplePerScan)
                                      .withScanDirection(Device::Parameters::SCAN_DIRECTION::CCW)};

    auto future{setParametersCommand.asyncExecute(1s, hmiBuilder, measureBuilder)};
    if (!future)
        return false;
    future->wait();
    const auto requestResult{future->get()};
    const auto hasSucceed{requestResult == Device::RequestResult::SUCCESS};
    if (!hasSucceed) {
        std::clog << "Device is not reachable (" << Device::asyncResultToString(requestResult) << ") at "
                  << device.getHostname() << std::endl;
    }
    return hasSucceed;
}

/**
 *
 * @param arg
 * @return
 */
std::optional<Device::Parameters::PACKET_TYPE> packetTypeFromArgument(const std::string &arg) {
    const std::unordered_map<std::string, Device::Parameters::PACKET_TYPE> scanPacketFromString{
            {"A", Device::Parameters::PACKET_TYPE::A},
            {"B", Device::Parameters::PACKET_TYPE::B},
            {"C", Device::Parameters::PACKET_TYPE::C}};
    auto localCopyOfArg{arg};
    boost::to_upper(localCopyOfArg);
    if (scanPacketFromString.count(localCopyOfArg) > 0)
        return {scanPacketFromString.at(localCopyOfArg)};
    return std::nullopt;
}

template<typename Iterator>
void printMapRange(Iterator begin, Iterator end, ostream &stream) {
    auto count{0u};
    std::for_each(begin, end,
                  [&stream, &count](const auto &node) {
                      const auto entry{"(" + std::to_string(node.first) + " - " + std::to_string(node.second) + " Hz)"};
                      stream << std::left << std::setw(20) << entry;
                      count = (count + 1) % 4;
                      if (!count) {
                          stream << std::endl;
                      }
                  });
    stream << std::endl;
}

bool validateResolutionAndFrequency(const unsigned int scanResolution, const unsigned int frequency,
                                    const std::unordered_map<unsigned int, unsigned int> &map) {
    if (!map.count(scanResolution)) {
        return false;
    }
    const auto &maxFrequency{map.at(scanResolution)};
    if (frequency > maxFrequency) {
        return false;
    }
    return true;
}

int main(int argc, char **argv) {

    using Point = pcl::PointXYZ;
    using PointCloudColorHandler = pcl::visualization::PointCloudColorHandlerCustom<Point>;
    const auto VIEWER_POINT_SIZE_ID{pcl::visualization::PCL_VISUALIZER_POINT_SIZE};
    const auto VIEWER_SCAN_CLOUD_ID{"scan_cloud"};

    std::signal(SIGSEGV, Backtrace::printBacktraceAndExitHandler);
    std::signal(SIGABRT, Backtrace::printBacktraceAndExitHandler);
    std::signal(SIGILL, Backtrace::printBacktraceAndExitHandler);
    std::signal(SIGFPE, Backtrace::printBacktraceAndExitHandler);
    std::signal(SIGPIPE, Backtrace::printBacktraceAndExitHandler);
    std::signal(SIGTERM, Backtrace::printBacktraceAndExitHandler);

    const std::unordered_map<unsigned int, unsigned int> UHD_RESOLUTION_FREQUENCY{
            {72,    50},
            {90,    50},
            {120,   50},
            {144,   50},
            {180,   50},
            {240,   50},
            {360,   50},
            {400,   50},
            {450,   50},
            {480,   50},
            {600,   50},
            {720,   50},
            {800,   50},
            {900,   50},
            {1200,  50},
            {1440,  50},
            {1680,  50},
            {1800,  50},
            {2100,  50},
            {2400,  50},
            {2520,  50},
            {2800,  50},
            {3150,  50},
            {3600,  50},
            {4200,  50},
            {5040,  50},
            {5600,  45},
            {6300,  40},
            {7200,  35},
            {8400,  30},
            {10080, 25},
            {12600, 20},
            {16800, 15},
            {25200, 10}};
    const std::unordered_map<unsigned int, unsigned int> HD_RESOLUTION_FREQUENCY{
            {72,   50},
            {90,   50},
            {120,  50},
            {144,  50},
            {180,  50},
            {240,  50},
            {360,  50},
            {400,  50},
            {450,  50},
            {480,  50},
            {600,  50},
            {720,  50},
            {800,  50},
            {900,  50},
            {1200, 50},
            {1440, 50},
            {1680, 50},
            {1800, 46},
            {2100, 40},
            {2400, 35},
            {2520, 33},
            {2800, 30},
            {3150, 26},
            {3600, 23},
            {4200, 20},
            {5040, 16},
            {5600, 15},
            {6300, 13},
            {7200, 11},
            {8400, 10}};
    const std::unordered_map<unsigned int, unsigned int> SD_RESOLUTION_FREQUENCY{
            {72,   30},
            {90,   30},
            {120,  30},
            {144,  30},
            {180,  30},
            {240,  30},
            {360,  30},
            {400,  30},
            {450,  30},
            {480,  30},
            {600,  30},
            {720,  30},
            {800,  30},
            {900,  30},
            {1200, 30},
            {1440, 30},
            {1680, 30},
            {1800, 30},
            {2100, 30},
            {2400, 30},
            {2520, 28},
            {2800, 25},
            {3150, 22},
            {3600, 20},
            {4200, 17},
            {5040, 14},
            {5600, 12},
            {6300, 11},
            {7200, 10}};

    boost::program_options::variables_map programOptions{};
    boost::program_options::options_description programOptionsDescriptions{
            "Perform a continuous scan data acquisition from a R2000 sensor and display it.", 1024, 512};
    programOptionsDescriptions
            .add_options()(
                    "help,h",
                    "Print out how to use the program")("table",
                                                        "Show the san resolution and frequency table for the supported "
                                                        "devices.")("udp",
                                                                    "Use the UDP transport layer protocol instead of the "
                                                                    "default TCP.")("address,a",
                                                                                    boost::program_options::value<std::string>()
                                                                                            ->default_value(""),
                                                                                    "Address of the device.")(
                    "target,t",
                    boost::program_options::
                    value<
                            std::
                            string>(),
                    "If using the "
                    "UDP protocol, "
                    "address and "
                    "port at which "
                    "the scan "
                    "datagrams must "
                    "be sent. The "
                    "address must be "
                    "under the "
                    "format "
                    "<address:port>"
                    ".")("samples_"
                         "per_scan,s",
                         boost::program_options::value<
                                 std::
                                 string>()
                                 ->default_value(
                                         "720"
                                         "0"),
                         "Number of "
                         "points per "
                         "scan.")("sc"
                                  "an"
                                  "_f"
                                  "re"
                                  "qu"
                                  "en"
                                  "cy"
                                  ","
                                  "f",
                                  boost::program_options::value<
                                          std::
                                          string>()
                                          ->default_value(
                                                  "35.0"),
                                  "Sc"
                                  "an"
                                  " f"
                                  "re"
                                  "qu"
                                  "en"
                                  "c"
                                  "y")("packet_type",
                                       boost::program_options::value<
                                               std::
                                               string>()
                                               ->default_value(
                                                       "A"),
                                       "Packet type (A, B or C).")("watchdog,w",
                                                                   boost::program_options::
                                                                   value<
                                                                           std::
                                                                           string>(),
                                                                   "Enable watchdog with timeout value.");
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

    const auto showScanResolutionAndFrequencyTable{programOptions.count("table") > 0};
    if (showScanResolutionAndFrequencyTable) {
        std::stringstream stream{};
        stream << "---------------------------------------------------------------" << std::endl;
        stream << "UHD - (Sample per scan - Maximum frequency)" << std::endl;
        printMapRange(std::begin(UHD_RESOLUTION_FREQUENCY), std::end(UHD_RESOLUTION_FREQUENCY), stream);
        stream << "---------------------------------------------------------------" << std::endl;
        stream << "HD - (Sample per scan - Maximum frequency)" << std::endl;
        printMapRange(std::begin(HD_RESOLUTION_FREQUENCY), std::end(HD_RESOLUTION_FREQUENCY), stream);
        stream << "---------------------------------------------------------------" << std::endl;
        stream << "SD - (Sample per scan - Maximum frequency)" << std::endl;
        printMapRange(std::begin(SD_RESOLUTION_FREQUENCY), std::end(SD_RESOLUTION_FREQUENCY), stream);
        stream << "---------------------------------------------------------------" << std::endl;
        std::cout << stream.str() << std::endl;
        return EXIT_SUCCESS;
    }
    const auto deviceAddressAsString{programOptions["address"].as<std::string>()};
    if (deviceAddressAsString.empty()) {
        std::clog << "You must specify the device address." << std::endl;
        return EXIT_FAILURE;
    }

    auto packetType{Device::Parameters::PACKET_TYPE::A};
    if (programOptions.count("packet_type") > 0) {
        const auto packetTypeArgument{programOptions["packet_type"].as<std::string>()};
        const auto parsedPacketType{packetTypeFromArgument(packetTypeArgument)};
        if (!parsedPacketType) {
            std::clog << "You must specify a valid packet type (" << packetTypeArgument
                      << "). Allowed type are A, B and C (not case sensitive)." << std::endl;
            return EXIT_FAILURE;
        }
        packetType = *parsedPacketType;
    }

    auto watchdog{std::optional<unsigned int>(std::nullopt)};
    if (programOptions.count("watchdog") > 0) {
        const auto watchdogArgument{programOptions["watchdog"].as<std::string>()};
        const auto watchdogValue{lexicalCast<unsigned int>(watchdogArgument)};
        if (!watchdogValue) {
            std::clog << "You must specify a valid watchdog timeout value (" << watchdogArgument << ")." << std::endl;
            return EXIT_FAILURE;
        }
        watchdog = std::optional(watchdogValue);
    }

    const auto useUdpTransportLayer{programOptions.count("udp") > 0};
    const auto samplePerScanAsString{programOptions["samples_per_scan"].as<std::string>()};
    const auto frequencyAsString{programOptions["scan_frequency"].as<std::string>()};

    if (!isValidIpv4(deviceAddressAsString)) {
        std::clog << "You must specify a valid ipv4 device address (" << deviceAddressAsString << ")." << std::endl;
        return EXIT_FAILURE;
    }
    const auto frequency{lexicalCast<float>(frequencyAsString)};
    if (!frequency) {
        std::clog << "You must specify a valid scan frequencyAsString value (" << frequencyAsString << ")."
                  << std::endl;
        return EXIT_FAILURE;
    }
    const auto samplePerScan{lexicalCast<unsigned int>(samplePerScanAsString)};
    if (!samplePerScan) {
        std::clog << "You must specify a valid sample per scan value (" << samplePerScanAsString << ")" << std::endl;
        return EXIT_FAILURE;
    }

    const auto validUHDParameters{
            validateResolutionAndFrequency(*samplePerScan, (unsigned int) (*frequency), UHD_RESOLUTION_FREQUENCY)};
    const auto validHDParameters{
            validateResolutionAndFrequency(*samplePerScan, (unsigned int) (*frequency), HD_RESOLUTION_FREQUENCY)};
    const auto validSDParameters{
            validateResolutionAndFrequency(*samplePerScan, (unsigned int) (*frequency), SD_RESOLUTION_FREQUENCY)};
    if (!validUHDParameters && !validHDParameters && !validSDParameters) {
        std::clog << "The provided scan resolution/frequency is not valid. Use the option --table to show the supported"
                     " scan resolution/frequency combination."
                  << std::endl;
        return EXIT_FAILURE;
    }

    const auto device{Device::R2000::makeShared({"R2000", deviceAddressAsString})};
    std::shared_ptr<Device::DataLink> dataLink{nullptr};
    if (useUdpTransportLayer) {
        if (!(programOptions.count("target") > 0)) {
            std::clog << "Using the UDP protocol, you must specify the scan datagram destination with the parameter "
                         "--target <address:port>"
                      << std::endl;
            return EXIT_FAILURE;
        }
        const auto target{splitAddressAndPort(programOptions["target"].as<std::string>())};
        if (!target) {
            std::clog << "Using the UDP protocol, you must specify the scan datagram destination with the parameter "
                         "and format --target <address(ipv4):port>"
                      << std::endl;
            return EXIT_FAILURE;
        }
        if (!configureDevice(*device, frequencyAsString, samplePerScanAsString)) {
            std::clog << "Could not set the parameters on the sensor." << std::endl;
            return EXIT_FAILURE;
        }
        std::cout << "Device configuration done. Setting up data link..." << std::endl;
        auto handleParameters{Device::Parameters::ReadWriteParameters::UdpHandle{}
                                      .withHostname(target->first)
                                      .withPort(target->second)
                                      .withPacketType(packetType)
                                      .withStartAngle(-1800000)};
        if (watchdog) {
            handleParameters.withWatchdog().withWatchdogTimeout(*watchdog);
        }
        auto future{Device::DataLinkBuilder(handleParameters).build(device, 3s)};
        future.wait();
        auto result{future.get()};
        const auto asyncRequestResult = result.first;
        if (asyncRequestResult != Device::RequestResult::SUCCESS) {
            std::clog << "Could not establish data link with sensor at " << device->getHostname() << " ("
                      << Device::asyncResultToString(asyncRequestResult) << ")" << std::endl;
            return EXIT_FAILURE;
        }
        dataLink = result.second;
    } else {
        if (!configureDevice(*device, frequencyAsString, samplePerScanAsString)) {
            std::clog << "Could not set the parameters on the sensor." << std::endl;
            return EXIT_FAILURE;
        }
        std::cout << "Device configuration done. Setting up data link..." << std::endl;
        auto handleParameters{
                Device::Parameters::ReadWriteParameters::TcpHandle{}.withPacketType(packetType).withStartAngle(
                        -1800000)};
        if (watchdog) {
            handleParameters.withWatchdog().withWatchdogTimeout(*watchdog);
        }
        auto future{Device::DataLinkBuilder(handleParameters).build(device, 3s)};
        future.wait();
        auto result{future.get()};
        const auto asyncRequestResult = result.first;
        if (asyncRequestResult != Device::RequestResult::SUCCESS) {
            std::clog << "Could not establish a data link with sensor at " << device->getHostname() << " ("
                      << Device::asyncResultToString(asyncRequestResult) << ")" << std::endl;
            return EXIT_FAILURE;
        }
        dataLink = result.second;
    }
    std::this_thread::sleep_for(1s);
    if (dataLink && dataLink->isAlive()) {
        std::cout << "Data link established with sensor at " << device->getHostname() << std::endl;
    } else {
        std::clog << "Could not establish a data link with sensor at " << device->getHostname() << std::endl;
    }

    const auto port{0};
    pcl::visualization::PCLVisualizer viewer{"Scan viewer"};
    viewer.setBackgroundColor(0.35, 0.35, 0.35, port);
    viewer.setSize(1280, 1024);
    viewer.addCoordinateSystem(150.0f, 0.0f, 0.0f, 0.0f, "Zero");
    PointCloud::ScanToPointCloud<Point> converter{*samplePerScan, -M_PI};

    while (!viewer.wasStopped()) {
        viewer.spinOnce();
        const auto &scan{dataLink->waitForNextScan(500ms)};
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
