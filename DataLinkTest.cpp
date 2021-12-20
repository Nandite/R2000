#include "DataLink/DataLink.hpp"
#include "DataLinkBuilder.hpp"
#include "R2000.hpp"
#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <thread>
#include <pcl/visualization/pcl_visualizer.h>
#include "Backtrace.hpp"
#include "ScanConverter.hpp"

using namespace std::chrono_literals;
namespace basio = boost::asio;

int main() {

    using Point = pcl::PointXYZ;
    using PointCloudColorHandler = pcl::visualization::PointCloudColorHandlerCustom<Point>;
    const auto VIEWER_POINT_SIZE_ID = pcl::visualization::PCL_VISUALIZER_POINT_SIZE;
    const auto VIEWER_SCAN_CLOUD_ID = "scan_cloud";

    std::signal(SIGSEGV, LocalizationToolkit::program::printBacktraceAndExitHandler);
    std::signal(SIGABRT, LocalizationToolkit::program::printBacktraceAndExitHandler);
    std::signal(SIGILL, LocalizationToolkit::program::printBacktraceAndExitHandler);
    std::signal(SIGFPE, LocalizationToolkit::program::printBacktraceAndExitHandler);
    std::signal(SIGPIPE, LocalizationToolkit::program::printBacktraceAndExitHandler);
    std::signal(SIGTERM, LocalizationToolkit::program::printBacktraceAndExitHandler);

    const auto port(0);
    pcl::visualization::PCLVisualizer viewer("Scan viewer");
    viewer.setBackgroundColor(0.35, 0.35, 0.35, port);
    viewer.setSize(1280, 1024);

    Device::DeviceConfiguration deviceConfiguration(basio::ip::address::from_string("192.168.1.30"));
    const auto device{Device::R2000::makeShared(deviceConfiguration)};

    std::this_thread::sleep_for(2s);
    Device::Commands::SetParametersCommand setParametersCommand{*device};
    const auto hmiParameters{Device::RWParameters::HmiDisplay()
                                            .unlockHmiButton()
                                            .unlockHmiParameters()
                                            .withHmiLanguage(Device::Language::ENGLISH)
                                            .withHmiDisplayMode(Device::HMI_DISPLAY_MODE::APPLICATION_TEXT)
                                            .withHmiApplicationText("Spook", ".../\\...")};
    const auto measureParameters{Device::RWParameters::Measure()
                                                .withOperatingMode(Device::OPERATING_MODE::MEASURE)
                                                .withScanFrequency(std::to_string(35.0f))
                                                .withSamplesPerScan(std::to_string(7200))
                                                .withScanDirection(Device::SCAN_DIRECTION::CCW)};
    if (!setParametersCommand.execute(hmiParameters, measureParameters)) {
        std::clog << "Could not set the parameters on the sensor." << std::endl;
        return EXIT_FAILURE;
    }

    try {
        const auto dataLink{Device::DataLinkBuilder(Device::RWParameters::TcpHandle{}
                                                      .withWatchdog()
                                                      .withWatchdogTimeout(20000)
                                                      .withPacketType(Device::PACKET_TYPE::A)
                                                      .withStartAngle(-1800000))
                                    .build(device)};

//        const auto dataLink{Device::DataLinkBuilder(Device::RWParameters::UdpHandle{}
//                                                            .withHostname("192.168.1.15")
//                                                            .withPort(43000)
//                                                            .withWatchdog()
//                                                            .withWatchdogTimeout(20000)
//                                                            .withPacketType(Device::PACKET_TYPE::A)
//                                                            .withStartAngle(-1800000))
//                                    .build(device)};
        if (dataLink && dataLink->isAlive()) {
            std::cout << "Data link established with sensor at " << deviceConfiguration.deviceAddress << std::endl;
        } else {
            std::clog << "Could not establish data link with sensor at " << deviceConfiguration.deviceAddress
                      << std::endl;
            return EXIT_FAILURE;
        }

        viewer.addCoordinateSystem(150.0f, 0.0f, 0.0f, 0.0f, "Zero");
        PointCloud::ScanToPointCloud<Point> converter(7200, -M_PI);
        while (!viewer.wasStopped() && dataLink->isAlive()) {
            pcl::PointCloud<Point>::Ptr cloud(new pcl::PointCloud<Point>);
            const auto &scan{dataLink->getLastScan()};
            converter.convert(scan, *cloud);
            PointCloudColorHandler scannedCloudColor(cloud, 0, 240, 0);
            viewer.removePointCloud(VIEWER_SCAN_CLOUD_ID, port);
            viewer.addPointCloud(cloud, scannedCloudColor, VIEWER_SCAN_CLOUD_ID, port);
            viewer.setPointCloudRenderingProperties(VIEWER_POINT_SIZE_ID, 2, VIEWER_SCAN_CLOUD_ID, port);
            viewer.spinOnce();
        }

    }
    catch (...) {
        std::clog << "Could not establish data link with sensor at " << deviceConfiguration.deviceAddress << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

