#include "R2000/DataLinkBuilder.hpp"
#include "R2000/R2000.hpp"
#include "R2000/StatusWatcher.hpp"
#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <thread>
#include <pcl/visualization/pcl_visualizer.h>
#include "Backtrace.hpp"
#include "ScanConverter.hpp"
#include <pcl/common/transforms.h>

using namespace std::chrono_literals;
namespace basio = boost::asio;

int main() {

    using Point = pcl::PointXYZ;
    using PointCloudColorHandler = pcl::visualization::PointCloudColorHandlerCustom<Point>;
    const auto VIEWER_POINT_SIZE_ID = pcl::visualization::PCL_VISUALIZER_POINT_SIZE;
    const auto VIEWER_SCAN_CLOUD_ID = "scan_cloud";

    std::signal(SIGSEGV, Backtrace::printBacktraceAndExitHandler);
    std::signal(SIGABRT, Backtrace::printBacktraceAndExitHandler);
    std::signal(SIGILL, Backtrace::printBacktraceAndExitHandler);
    std::signal(SIGFPE, Backtrace::printBacktraceAndExitHandler);
    std::signal(SIGPIPE, Backtrace::printBacktraceAndExitHandler);
    std::signal(SIGTERM, Backtrace::printBacktraceAndExitHandler);

    const auto port(0);
    pcl::visualization::PCLVisualizer viewer("Scan viewer");
    viewer.setBackgroundColor(0.35, 0.35, 0.35, port);
    viewer.setSize(1280, 1024);

    const auto device{Device::R2000::makeShared({"R2000", "192.168.1.30"})};

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

    Device::StatusWatcher statusWatcher(device, 1s, [&device](const Device::DeviceStatus &status) {
        std::ofstream stream("/home/chazz/r2000.status", ios::trunc);
        if (!stream.is_open())
            return;
        const auto &statusFlags{status.getStatusFlags()};
        stream << "----------------------------------------------------------" << std::endl;
        stream << "R2000: " << device->getName() << "/" << device->getHostname() << std::endl;
        stream << "+ Cpu Load : " << status.getCpuLoad() << std::endl;
        stream << "+ Raw system time : " << status.getRawSystemTime() << std::endl;
        stream << "+ Up time : " << status.getUptime() << std::endl;
        stream << "+ Power cycles count : " << status.getPowerCyclesCount() << std::endl;
        stream << "+ Operation time : " << status.getOperationTime() << std::endl;
        stream << "+ Operation time (scaled) : " << status.getScaledOperationTime() << std::endl;
        stream << "+ Current temperature : " << status.getCurrentTemperature() << std::endl;
        stream << "+ Minimal temperature : " << status.getMinimalTemperature() << std::endl;
        stream << "+ Maximal temperature : " << status.getMaximalTemperature() << std::endl;
        stream << "----------------------------------------------------------" << std::endl;
        stream << "Device status flags" << std::endl;
        stream << "+ Device error : " << statusFlags.deviceHasError() << std::endl;
        stream << "+ Device warning : " << statusFlags.deviceHasWarning() << std::endl;
        stream << "+ Initialization : " << statusFlags.isInitializing() << std::endl;
        stream << "+ Output scan muted : " << statusFlags.outputScanIsMuted() << std::endl;
        stream << "+ Unstable head rotation : " << statusFlags.headHasUnstableRotation() << std::endl;
        stream << "+ Lens contamination (warning) : " << statusFlags.hasLensContaminationWarning() << std::endl;
        stream << "+ Lens contamination (Error) : " << statusFlags.hasLensContaminationError() << std::endl;
        stream << "+ Temperature is low (warning): " << statusFlags.hasLowTemperatureWarning() << std::endl;
        stream << "+ Temperature is low (error) : " << statusFlags.hasLowTemperatureError() << std::endl;
        stream << "+ Temperature is high (warning) : " << statusFlags.hasHighTemperatureWarning() << std::endl;
        stream << "+ Temperature is high (error) : " << statusFlags.hasHighTemperatureError() << std::endl;
        stream << "+ Device overload (warning) : " << statusFlags.hasDeviceOverloadWarning() << std::endl;
        stream << "+ Device overload (error) : " << statusFlags.hasDeviceOverloadError() << std::endl;
        stream << "+ Unrecoverable default : " << statusFlags.hasUnrecoverableDefect() << std::endl;
        stream << "----------------------------------------------------------" << std::endl;
    });

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
            std::cout << "Data link established with sensor at " << device->getHostname() << std::endl;
        } else {
            std::clog << "Could not establish data link with sensor at " << device->getHostname() << std::endl;
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
        std::clog << "Could not establish data link with sensor " << device->getName() << "at " << device->getHostname()
                  << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

