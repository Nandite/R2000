//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#pragma once

#include "DeviceHandle.hpp"
#include "ExponentialBackoff.hpp"
#include "Worker.hpp"
#include <any>
#include <bitset>
#include <boost/asio.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/unordered_map.hpp>
#include <chrono>
#include <iostream>
#include <list>
#include <optional>
#include <utility>

#define BOOL_PARAMETER_TRUE "on"
#define BOOL_PARAMETER_FALSE "off"

#define ADD_RO_PARAMETER_BUILDER_METHOD(F_NAME, RETURN_TYPE, PARAMETER_NAME)                                           \
    RETURN_TYPE F_NAME()                                                                                               \
    {                                                                                                                  \
        parameters.push_back(PARAMETER_NAME);                                                                          \
        return *this;                                                                                                  \
    }

#define ADD_RW_PARAMETER_BUILDER_METHOD(F_NAME, RETURN_TYPE, ARGUMENT_TYPE, ARGUMENT_NAME, PARAMETER_NAME)             \
    RETURN_TYPE F_NAME(ARGUMENT_TYPE ARGUMENT_NAME)                                                                    \
    {                                                                                                                  \
        parameters[PARAMETER_NAME] = ARGUMENT_NAME;                                                                    \
        return *this;                                                                                                  \
    }

#define ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(F_NAME, RETURN_TYPE, ARGUMENT_TYPE, ARGUMENT_NAME, \
                                                       ARGUMENT_TRANSFORM, PARAMETER_NAME)                             \
    RETURN_TYPE F_NAME(ARGUMENT_TYPE ARGUMENT_NAME)                                                                    \
    {                                                                                                                  \
        parameters[PARAMETER_NAME] = ARGUMENT_TRANSFORM;                                                               \
        return *this;                                                                                                  \
    }

#define ADD_RW_PARAMETER_BUILDER_METHOD_ON(F_NAME, RETURN_TYPE, PARAMETER_NAME)                                        \
    RETURN_TYPE F_NAME()                                                                                               \
    {                                                                                                                  \
        parameters[PARAMETER_NAME] = BOOL_PARAMETER_TRUE;                                                              \
        return *this;                                                                                                  \
    }

#define ADD_RW_PARAMETER_BUILDER_METHOD_OFF(F_NAME, RETURN_TYPE, PARAMETER_NAME)                                       \
    RETURN_TYPE F_NAME()                                                                                               \
    {                                                                                                                  \
        parameters[PARAMETER_NAME] = BOOL_PARAMETER_FALSE;                                                             \
        return *this;                                                                                                  \
    }

#define ADD_BUILD_BUILDER_METHOD(F_NAME, RETURN_TYPE, RETURN_ARGUMENT_NAME)                                            \
    RETURN_TYPE F_NAME() const { return RETURN_ARGUMENT_NAME; }

#define COMMAND_GET_PROTOCOL_INFO "get_protocol_info"
#define COMMAND_RELEASE_HANDLE "release_handle"
#define COMMAND_START_SCAN_OUTPUT "start_scanoutput"
#define COMMAND_STOP_SCAN_OUTPUT "stop_scanoutput"
#define COMMAND_FEED_WATCHDOG "feed_watchdog"
#define COMMAND_REBOOT_DEVICE "reboot_device"
#define COMMAND_LIST_PARAMETERS "list_parameters"
#define COMMAND_GET_PARAMETER "get_parameter"
#define COMMAND_SET_PARAMETER "set_parameter"
#define COMMAND_RESET_PARAMETERS "reset_parameter"
#define COMMAND_FACTORY_RESET "factory_reset"
#define COMMAND_REQUEST_TCP_HANDLE "request_handle_tcp"
#define COMMAND_REQUEST_UDP_HANDLE "request_handle_udp"
#define COMMAND_SET_SCAN_OUTPUT_CONFIG "set_scanoutput_config"
#define COMMAND_GET_SCAN_OUTPUT_CONFIG "get_scanoutput_config"

#define PARAMETER_NAME_HANDLE "handle"
#define PARAMETER_NAME_LIST "list"

#define ERROR_CODE "error_code"
#define ERROR_TEXT "error_text"

#define PARAMETER_PROTOCOL_NAME "protocol_name"
#define PARAMETER_PROTOCOL_VERSION_MAJOR "version_major"
#define PARAMETER_PROTOCOL_VERSION_MINOR "version_minor"
#define PARAMETER_AVAILABLE_COMMANDS "commands"

#define PARAMETER_DEVICE_FAMILY "device_family"
#define PARAMETER_VENDOR "vendor"
#define PARAMETER_PRODUCT "product"
#define PARAMETER_PART "part"
#define PARAMETER_SERIAL "serial"
#define PARAMETER_REVISION_FW "revision_fw"
#define PARAMETER_REVISION_HW "revision_hw"
#define PARAMETER_FEATURE_FLAGS "feature_flags"
#define PARAMETER_EMITTER_TYPE "emitter_type"
#define PARAMETER_RADIAL_RANGE_MIN "radial_range_min"
#define PARAMETER_RADIAL_RANGE_MAX "radial_range_max"
#define PARAMETER_RADIAL_RESOLUTION "radial_resolution"
#define PARAMETER_ANGULAR_FOV "angular_fov"
#define PARAMETER_SCAN_FREQUENCY_MIN "scan_frequency_min"
#define PARAMETER_SCAN_FREQUENCY_MAX "scan_frequency_max"
#define PARAMETER_SAMPLING_RATE_MIN "sampling_rate_min"
#define PARAMETER_SAMPLING_RATE_MAX "angular_fov"
#define PARAMETER_ANGULAR_RESOLUTION "sampling_rate_max"
#define PARAMETER_MAX_CONNECTIONS "max_connections"
#define PARAMETER_IP_MODE "ip_mode"
#define PARAMETER_IP_ADDRESS "ip_address"
#define PARAMETER_SUBNET_MASK "subnet_mask"
#define PARAMETER_GATEWAY "gateway"
#define PARAMETER_IP_MODE_CURRENT "ip_mode_current"
#define PARAMETER_IP_ADDRESS_CURRENT "ip_address_current"
#define PARAMETER_SUBNET_MASK_CURRENT "subnet_mask_current"
#define PARAMETER_GATEWAY_CURRENT "gateway_current"
#define PARAMETER_MAC_ADDRESS "mac_address"
#define PARAMETER_OPERATING_MODE "operating_mode"
#define PARAMETER_SCAN_FREQUENCY "scan_frequency"
#define PARAMETER_SCAN_DIRECTION "scan_direction"
#define PARAMETER_SAMPLES_PER_SCAN "samples_per_scan"
#define PARAMETER_SCAN_FREQUENCY_MEASURED "scan_frequency_measured"
#define PARAMETER_DEVICE_FAMILY "device_family"
#define PARAMETER_HMI_DISPLAY_MODE "hmi_display_mode"
#define PARAMETER_HMI_LANGUAGE "hmi_language"
#define PARAMETER_HMI_BUTTON_LOCK "hmi_button_lock"
#define PARAMETER_HMI_PARAMETER_LOCK "hmi_parameter_lock"
#define PARAMETER_LOCATOR_INDICATION "locator_indication"
#define PARAMETER_HMI_STATIC_LOGO "hmi_static_logo"
#define PARAMETER_HMI_STATIC_TEXT_1 "hmi_static_text_1"
#define PARAMETER_HMI_STATIC_TEXT_2 "hmi_static_text_2"
#define PARAMETER_HMI_APPLICATION_BITMAP "hmi_application_bitmap"
#define PARAMETER_HMI_APPLICATION_TEXT_1 "hmi_application_text_1"
#define PARAMETER_HMI_APPLICATION_TEXT_2 "hmi_application_text_2"
#define PARAMETER_USER_TAG "user_tag"
#define PARAMETER_USER_NOTES "user_notes"
#define PARAMETER_LOCATOR_INDICATION "locator_indication"
#define PARAMETER_STATUS_FLAGS "status_flags"
#define PARAMETER_LOAD_INDICATION "load_indication"
#define PARAMETER_SYSTEM_TIME_RAW "system_time_raw"
#define PARAMETER_UP_TIME "up_time"
#define PARAMETER_POWER_CYCLES "power_cycles"
#define PARAMETER_OPERATION_TIME "operation_time"
#define PARAMETER_OPERATION_TIME_SCALED "operation_time_scaled"
#define PARAMETER_TEMPERATURE_CURRENT "temperature_current"
#define PARAMETER_TEMPERATURE_MIN "temperature_min"
#define PARAMETER_TEMPERATURE_MAX "temperature_max"
#define PARAMETER_LCM_DETECTION_SENSITIVITY "lcm_detection_sensitivity"
#define PARAMETER_LCM_DETECTION_PERIOD "lcm_detection_period"
#define PARAMETER_LCM_SECTOR_ENABLE "lcm_sector_enable"
#define PARAMETER_LCM_SECTOR_WARN_FLAGS "lcm_sector_warn_flags"
#define PARAMETER_LCM_SECTOR_ERROR_FLAGS "lcm_sector_error_flags"

#define PARAMETER_HANDLE_ADDRESS "address"
#define PARAMETER_HANDLE_PORT "port"
#define PARAMETER_HANDLE_WATCHDOG "watchdog"
#define PARAMETER_HANDLE_WATCHDOG_TIMEOUT "watchdogtimeout"
#define PARAMETER_HANDLE_PACKET_TYPE "packet_type"
#define PARAMETER_HANDLE_PACKET_CRC "packet_crc"
#define PARAMETER_HANDLE_START_ANGLE "start_angle"
#define PARAMETER_HANDLE_MAX_NUM_POINTS_SCAN "max_num_points_scan"
#define PARAMETER_HANDLE_SKIP_SCANS "skip_scans"

using namespace std::chrono_literals;

namespace Device {
    class R2000;

    class DataLink;

    struct DeviceConfiguration {
        DeviceConfiguration(std::string name, const std::string &deviceAddress, unsigned short port = 80)
                : name(std::move(name)),
                  deviceAddress(boost::asio::ip::address::from_string(deviceAddress)),
                  httpServicePort(port) {
        }

        std::string name;
        const boost::asio::ip::address deviceAddress{};
        const unsigned short httpServicePort{};
    };

    template<typename Socket>
    class SocketGuard {
    public:
        template<typename... Args>
        explicit SocketGuard(Args &&... args) : socket(std::forward<Args>(args)...) {}

        ~SocketGuard() { socket.close(); }

        inline Socket &operator*() { return socket; }

        inline Socket &getUnderlyingSocket() { return (*this).operator*(); }

    private:
        Socket socket;
    };

    using ParametersList = std::vector<std::string>;
    using ParametersMap = std::unordered_map<std::string, std::string>;
    using PropertyTree = boost::property_tree::ptree;

    enum IpMode {
        STATIC,
        DHCP,
        AUTO_IP
    };

    enum class Language {
        ENGLISH,
        GERMAN
    };
    enum class HMI_DISPLAY_MODE {
        OFF,
        STATIC_LOGO,
        STATIC_TEXT,
        BAR_GRAPH_DISTANCE,
        BAR_GRAPH_ECHO,
        BAR_GRAPH_REFLECTOR,
        APPLICATION_BITMAP,
        APPLICATION_TEXT
    };

    enum class OPERATING_MODE {
        MEASURE,
        EMITTER_OFF
    };

    enum class SCAN_DIRECTION {
        CW,
        CCW
    };

    enum class CRC {
        NONE,
        CRC32C
    };

    enum class PACKET_TYPE : uint16_t {
        A = 0x0041,
        B = 0x0042,
        C = 0x0043
    };

    enum class LCM_SENSITIVITY {
        DISABLED,
        LOW,
        MEDIUM,
        HIGH
    };

    enum class PFSDP : unsigned int {
        UNKNOWN = 0,
        V100 = 100,
        V101 = 101,
        V102 = 102,
        V103 = 103,
        V104 = 104,
        ABOVE_V104
    };

    template<typename E>
    constexpr auto underlyingType(E enumerator) {
        return static_cast<std::underlying_type_t<E>>(enumerator);
    }

    inline PFSDP protocolVersionFromString(const std::string &major, const std::string &minor) {
        const auto numericMajor{std::stoul(major)};
        const auto numericMinor{std::stoul(minor)};
        const auto numericVersion{numericMajor * 100 + numericMinor};
        switch (numericVersion) {
            case underlyingType(PFSDP::V100):
                return PFSDP::V100;
            case underlyingType(PFSDP::V101):
                return PFSDP::V101;
            case underlyingType(PFSDP::V102):
                return PFSDP::V102;
            case underlyingType(PFSDP::V103):
                return PFSDP::V103;
            case underlyingType(PFSDP::V104):
                return PFSDP::V104;
            default:
                if (numericVersion > underlyingType(PFSDP::V104))
                    return PFSDP::ABOVE_V104;
        }
        return PFSDP::UNKNOWN;
    }

    namespace internals::Parameters {

        struct RoParametersTags {
            struct BasicInformationTag;
            struct CapabilitiesTag;
            struct EthernetTag;
            struct MeasuringTag;
            struct HmiDisplayTag;
            struct SystemStatusTag;
            struct LensContaminationMonitor;
        };
        struct RwParametersTags {
            struct BasicInformationTag;
            struct EthernetTag;
            struct MeasuringTag;
            struct HmiDisplayTag;
            struct HandleUdpTag;
            struct HandleTcpTag;
            struct LensContaminationMonitor;
        };

        struct ReadOnlyBuilderBase {
            ADD_BUILD_BUILDER_METHOD(build, ParametersList, parameters);

        protected:
            ParametersList parameters;
        };

        struct WriteBuilderBase {
            ADD_BUILD_BUILDER_METHOD(build, ParametersMap, parameters);

        protected:
            ParametersMap parameters;
        };

        template<typename Section>
        struct ParametersBuilderImpl {
        };

        template<>
        struct ParametersBuilderImpl<RwParametersTags::BasicInformationTag> : public WriteBuilderBase {
            ADD_RW_PARAMETER_BUILDER_METHOD(withUserTag, ParametersBuilderImpl &, const std::string&, userTag,
                                            PARAMETER_USER_TAG);

            ADD_RW_PARAMETER_BUILDER_METHOD(withUserNotes, ParametersBuilderImpl &, const std::string&, userNotes,
                                            PARAMETER_USER_NOTES);
        };

        template<>
        struct ParametersBuilderImpl<RwParametersTags::EthernetTag> : public WriteBuilderBase {
            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withIpMode, ParametersBuilderImpl &, IpMode, mode,
                                                           IpModeToString.at(mode), PARAMETER_IP_ADDRESS);

            ADD_RW_PARAMETER_BUILDER_METHOD(withIpAddress, ParametersBuilderImpl &, const std::string&, ipAddress,
                                            PARAMETER_IP_ADDRESS);

            ADD_RW_PARAMETER_BUILDER_METHOD(withSubnetMask, ParametersBuilderImpl &, const std::string&, subnetMask,
                                            PARAMETER_SUBNET_MASK);

            ADD_RW_PARAMETER_BUILDER_METHOD(withGateway, ParametersBuilderImpl &, const std::string&, gateway,
                                            PARAMETER_GATEWAY);

        private:
            const boost::unordered_map<IpMode, std::string> IpModeToString =
                    boost::assign::map_list_of(IpMode::STATIC, "static")(IpMode::DHCP, "dhcp")(IpMode::AUTO_IP,
                                                                                               "autoip");
        };

        template<>
        struct ParametersBuilderImpl<RwParametersTags::MeasuringTag> : public WriteBuilderBase {
            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withOperatingMode, ParametersBuilderImpl &, OPERATING_MODE,
                                                           operatingMode, operatingModeToString.at(operatingMode),
                                                           PARAMETER_OPERATING_MODE);

            ADD_RW_PARAMETER_BUILDER_METHOD(withScanFrequency, ParametersBuilderImpl &, const std::string&,
                                            scanFrequency,
                                            PARAMETER_SCAN_FREQUENCY);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withScanDirection, ParametersBuilderImpl &, SCAN_DIRECTION,
                                                           scanDirection, scanDirectionToString.at(scanDirection),
                                                           PARAMETER_SCAN_DIRECTION);

            ADD_RW_PARAMETER_BUILDER_METHOD(withSamplesPerScan, ParametersBuilderImpl &, const std::string&,
                                            samplesPerScan,
                                            PARAMETER_SAMPLES_PER_SCAN);

        private:
            const boost::unordered_map<OPERATING_MODE, std::string> operatingModeToString =
                    boost::assign::map_list_of(OPERATING_MODE::MEASURE, "measure")(OPERATING_MODE::EMITTER_OFF,
                                                                                   "emitter_off");
            const boost::unordered_map<SCAN_DIRECTION, std::string> scanDirectionToString =
                    boost::assign::map_list_of(SCAN_DIRECTION::CW, "cw")(SCAN_DIRECTION::CCW, "ccw");
        };

        template<>
        struct ParametersBuilderImpl<RwParametersTags::HmiDisplayTag> : public WriteBuilderBase {

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withHmiDisplayMode, ParametersBuilderImpl &,
                                                           HMI_DISPLAY_MODE,
                                                           displayMode, HmiDisplayModeToString.at(displayMode),
                                                           PARAMETER_HMI_DISPLAY_MODE);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withHmiLanguage, ParametersBuilderImpl &, Language, language,
                                                           LanguageToString.at(language), PARAMETER_HMI_LANGUAGE);

            ADD_RW_PARAMETER_BUILDER_METHOD_ON(lockHmiButton, ParametersBuilderImpl &, PARAMETER_HMI_BUTTON_LOCK);

            ADD_RW_PARAMETER_BUILDER_METHOD_OFF(unlockHmiButton, ParametersBuilderImpl &, PARAMETER_HMI_BUTTON_LOCK);

            ADD_RW_PARAMETER_BUILDER_METHOD_ON(lockHmiParameters, ParametersBuilderImpl &,
                                               PARAMETER_HMI_PARAMETER_LOCK);

            ADD_RW_PARAMETER_BUILDER_METHOD_OFF(unlockHmiParameters, ParametersBuilderImpl &,
                                                PARAMETER_HMI_PARAMETER_LOCK);

            ADD_RW_PARAMETER_BUILDER_METHOD_ON(enableLEDLocatorIndication, ParametersBuilderImpl &,
                                               PARAMETER_LOCATOR_INDICATION);

            ADD_RW_PARAMETER_BUILDER_METHOD_OFF(disableLEDLocatorIndication, ParametersBuilderImpl &,
                                                PARAMETER_LOCATOR_INDICATION);

            ADD_RW_PARAMETER_BUILDER_METHOD(withHmiStaticLogo, ParametersBuilderImpl &, const std::string&, base64Logo,
                                            PARAMETER_HMI_STATIC_LOGO);

            ADD_RW_PARAMETER_BUILDER_METHOD(withHmiStaticText1, ParametersBuilderImpl &, const std::string&, line,
                                            PARAMETER_HMI_STATIC_TEXT_1);

            ADD_RW_PARAMETER_BUILDER_METHOD(withHmiStaticText2, ParametersBuilderImpl &, const std::string&, line,
                                            PARAMETER_HMI_STATIC_TEXT_2);

            ADD_RW_PARAMETER_BUILDER_METHOD(withHmiApplicationBitmap, ParametersBuilderImpl &, const std::string&,
                                            base64Bitmap,
                                            PARAMETER_HMI_APPLICATION_BITMAP);

            ADD_RW_PARAMETER_BUILDER_METHOD(withHmiApplicationText1, ParametersBuilderImpl &, const std::string&, line,
                                            PARAMETER_HMI_APPLICATION_TEXT_1);

            ADD_RW_PARAMETER_BUILDER_METHOD(withHmiApplicationText2, ParametersBuilderImpl &, const std::string&, line,
                                            PARAMETER_HMI_APPLICATION_TEXT_2);

        private:
            const boost::unordered_map<Language, std::string> LanguageToString =
                    boost::assign::map_list_of(Language::ENGLISH, "english")(Language::GERMAN, "german");
            const boost::unordered_map<HMI_DISPLAY_MODE, std::string> HmiDisplayModeToString = boost::assign::map_list_of(
                    HMI_DISPLAY_MODE::OFF, "off")(HMI_DISPLAY_MODE::STATIC_LOGO, "static_logo")(
                    HMI_DISPLAY_MODE::STATIC_TEXT,
                    "static_text")(HMI_DISPLAY_MODE::BAR_GRAPH_DISTANCE,
                                   "bargraph_distance")(HMI_DISPLAY_MODE::BAR_GRAPH_ECHO,
                                                        "bargraph_echo")(HMI_DISPLAY_MODE::BAR_GRAPH_REFLECTOR,
                                                                         "bargraph_reflector")(
                    HMI_DISPLAY_MODE::APPLICATION_BITMAP,
                    "application_"
                    "bitmap")(HMI_DISPLAY_MODE::
                              APPLICATION_TEXT,
                              "application_text");
        };

        struct HandleParameters : public WriteBuilderBase {
            const boost::unordered_map<PACKET_TYPE, std::string> scanPacketString =
                    boost::assign::map_list_of(PACKET_TYPE::A, "A")(PACKET_TYPE::B, "B")(PACKET_TYPE::C, "C");
            const boost::unordered_map<CRC, std::string> crcToString =
                    boost::assign::map_list_of(CRC::NONE, "none")(CRC::CRC32C, "crc32c");
        };

        template<>
        struct ParametersBuilderImpl<RwParametersTags::HandleTcpTag> : public HandleParameters {

            ParametersBuilderImpl() { (void) withoutWatchdog(); }

            ADD_RW_PARAMETER_BUILDER_METHOD_ON(withWatchdog, ParametersBuilderImpl &, PARAMETER_HANDLE_WATCHDOG);

            ADD_RW_PARAMETER_BUILDER_METHOD_OFF(withoutWatchdog, ParametersBuilderImpl &, PARAMETER_HANDLE_WATCHDOG);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withWatchdogTimeout, ParametersBuilderImpl &,
                                                           const unsigned int,
                                                           watchdogtimeout, std::to_string(watchdogtimeout),
                                                           PARAMETER_HANDLE_WATCHDOG_TIMEOUT);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withPacketType, ParametersBuilderImpl &, PACKET_TYPE,
                                                           packetType,
                                                           scanPacketString.at(packetType),
                                                           PARAMETER_HANDLE_PACKET_TYPE);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withPacketCrc, ParametersBuilderImpl &, CRC, crcType,
                                                           crcToString.at(crcType), PARAMETER_HANDLE_PACKET_CRC);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withStartAngle, ParametersBuilderImpl &, const int,
                                                           startAngle,
                                                           std::to_string(startAngle), PARAMETER_HANDLE_START_ANGLE);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withNumberOfPointsPerScan, ParametersBuilderImpl &,
                                                           const unsigned int, numberOfPointsPerScan,
                                                           std::to_string(numberOfPointsPerScan),
                                                           PARAMETER_HANDLE_MAX_NUM_POINTS_SCAN);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withNumberOfScanToSkip, ParametersBuilderImpl &,
                                                           const unsigned int,
                                                           numberOfScanToSkip, std::to_string(numberOfScanToSkip),
                                                           PARAMETER_HANDLE_SKIP_SCANS);
        };

        template<>
        struct ParametersBuilderImpl<RwParametersTags::HandleUdpTag> : public HandleParameters {

            ParametersBuilderImpl() { (void) withoutWatchdog(); }

            ADD_RW_PARAMETER_BUILDER_METHOD(withHostname, ParametersBuilderImpl &, const std::string&, hostname,
                                            PARAMETER_HANDLE_ADDRESS);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withPort, ParametersBuilderImpl &, const uint16_t, port,
                                                           std::to_string(port), PARAMETER_HANDLE_PORT);

            ADD_RW_PARAMETER_BUILDER_METHOD_ON(withWatchdog, ParametersBuilderImpl &, PARAMETER_HANDLE_WATCHDOG);

            ADD_RW_PARAMETER_BUILDER_METHOD_OFF(withoutWatchdog, ParametersBuilderImpl &, PARAMETER_HANDLE_WATCHDOG);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withWatchdogTimeout, ParametersBuilderImpl &,
                                                           const unsigned int,
                                                           watchdogtimeout, std::to_string(watchdogtimeout),
                                                           PARAMETER_HANDLE_WATCHDOG_TIMEOUT);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withPacketType, ParametersBuilderImpl &, PACKET_TYPE,
                                                           packetType,
                                                           scanPacketString.at(packetType),
                                                           PARAMETER_HANDLE_PACKET_TYPE);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withPacketCrc, ParametersBuilderImpl &, CRC, crcType,
                                                           crcToString.at(crcType), PARAMETER_HANDLE_PACKET_CRC);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withStartAngle, ParametersBuilderImpl &, const int,
                                                           startAngle,
                                                           std::to_string(startAngle), PARAMETER_HANDLE_START_ANGLE);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withNumberOfPointsPerScan, ParametersBuilderImpl &,
                                                           const unsigned int, numberOfPointsPerScan,
                                                           std::to_string(numberOfPointsPerScan),
                                                           PARAMETER_HANDLE_MAX_NUM_POINTS_SCAN);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withNumberOfScanToSkip, ParametersBuilderImpl &,
                                                           const unsigned int,
                                                           numberOfScanToSkip, std::to_string(numberOfScanToSkip),
                                                           PARAMETER_HANDLE_SKIP_SCANS);
        };

        template<>
        struct ParametersBuilderImpl<RwParametersTags::LensContaminationMonitor> : public WriteBuilderBase {

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withSensitivity, ParametersBuilderImpl &, LCM_SENSITIVITY,
                                                           sensitivity, LCMSensitivityToString.at(sensitivity),
                                                           PARAMETER_LCM_DETECTION_SENSITIVITY);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withDetectionPeriod, ParametersBuilderImpl &,
                                                           std::chrono::milliseconds, period,
                                                           std::to_string(period.count()),
                                                           PARAMETER_LCM_DETECTION_PERIOD);

            ADD_RW_PARAMETER_BUILDER_METHOD_ON(withLcmEnabled, ParametersBuilderImpl &, PARAMETER_LCM_SECTOR_ENABLE);

            ADD_RW_PARAMETER_BUILDER_METHOD_OFF(withLcmDisabled, ParametersBuilderImpl &, PARAMETER_LCM_SECTOR_ENABLE);

        private:
            const boost::unordered_map<LCM_SENSITIVITY, std::string> LCMSensitivityToString =
                    boost::assign::map_list_of(LCM_SENSITIVITY::DISABLED, "disabled")(LCM_SENSITIVITY::LOW, "low")(
                            LCM_SENSITIVITY::MEDIUM, "medium")(LCM_SENSITIVITY::HIGH, "high");
        };

        template<>
        struct ParametersBuilderImpl<RoParametersTags::BasicInformationTag> : public ReadOnlyBuilderBase {
            ADD_RO_PARAMETER_BUILDER_METHOD(requestDeviceFamily, ParametersBuilderImpl &, PARAMETER_DEVICE_FAMILY);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestVendor, ParametersBuilderImpl &, PARAMETER_VENDOR);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestProduct, ParametersBuilderImpl &, PARAMETER_PRODUCT);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestPart, ParametersBuilderImpl &, PARAMETER_PART);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestSerial, ParametersBuilderImpl &, PARAMETER_SERIAL);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestFirmwareRevision, ParametersBuilderImpl &, PARAMETER_REVISION_FW);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestHardwareRevision, ParametersBuilderImpl &, PARAMETER_REVISION_HW);

            ADD_RO_PARAMETER_BUILDER_METHOD(withUserTag, ParametersBuilderImpl &, PARAMETER_USER_TAG);

            ADD_RO_PARAMETER_BUILDER_METHOD(withUserNotes, ParametersBuilderImpl &, PARAMETER_USER_NOTES);
        };

        template<>
        struct ParametersBuilderImpl<RoParametersTags::CapabilitiesTag> : public ReadOnlyBuilderBase {
            ADD_RO_PARAMETER_BUILDER_METHOD(requestFeatureFlags, ParametersBuilderImpl &, PARAMETER_FEATURE_FLAGS);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestEmitterType, ParametersBuilderImpl &, PARAMETER_EMITTER_TYPE);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestRadialMinRange, ParametersBuilderImpl &, PARAMETER_RADIAL_RANGE_MIN);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestRadialMaxRange, ParametersBuilderImpl &, PARAMETER_RADIAL_RANGE_MAX);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestRadialResolution, ParametersBuilderImpl &,
                                            PARAMETER_RADIAL_RESOLUTION);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestAngularFov, ParametersBuilderImpl &, PARAMETER_ANGULAR_FOV);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestAngularResolution, ParametersBuilderImpl &,
                                            PARAMETER_ANGULAR_RESOLUTION);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestMinimalScanFrequency, ParametersBuilderImpl &,
                                            PARAMETER_SCAN_FREQUENCY_MIN);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestMaximalScanFrequency, ParametersBuilderImpl &,
                                            PARAMETER_SCAN_FREQUENCY_MAX);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestMinimalSamplingRate, ParametersBuilderImpl &,
                                            PARAMETER_SAMPLING_RATE_MIN);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestMaximalSamplingRate, ParametersBuilderImpl &,
                                            PARAMETER_SAMPLING_RATE_MAX);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestMaximalConnections, ParametersBuilderImpl &,
                                            PARAMETER_MAX_CONNECTIONS);
        };

        template<>
        struct ParametersBuilderImpl<RoParametersTags::EthernetTag> : public ReadOnlyBuilderBase {
            ADD_RO_PARAMETER_BUILDER_METHOD(requestCurrentIpMode, ParametersBuilderImpl &, PARAMETER_IP_MODE_CURRENT);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestCurrentIpAddress, ParametersBuilderImpl &,
                                            PARAMETER_IP_ADDRESS_CURRENT);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestCurrentSubnetMask, ParametersBuilderImpl &,
                                            PARAMETER_SUBNET_MASK_CURRENT);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestCurrentGateway, ParametersBuilderImpl &, PARAMETER_GATEWAY_CURRENT);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestMacAddress, ParametersBuilderImpl &, PARAMETER_MAC_ADDRESS);

            ADD_RO_PARAMETER_BUILDER_METHOD(withIpMode, ParametersBuilderImpl &, PARAMETER_IP_MODE);

            ADD_RO_PARAMETER_BUILDER_METHOD(withIpAddress, ParametersBuilderImpl &, PARAMETER_IP_ADDRESS);

            ADD_RO_PARAMETER_BUILDER_METHOD(withSubnetMask, ParametersBuilderImpl &, PARAMETER_SUBNET_MASK);

            ADD_RO_PARAMETER_BUILDER_METHOD(withGateway, ParametersBuilderImpl &, PARAMETER_GATEWAY);
        };

        template<>
        struct ParametersBuilderImpl<RoParametersTags::MeasuringTag> : public ReadOnlyBuilderBase {
            ADD_RO_PARAMETER_BUILDER_METHOD(requestMeasuredFrequency, ParametersBuilderImpl &,
                                            PARAMETER_SCAN_FREQUENCY_MEASURED);

            ADD_RO_PARAMETER_BUILDER_METHOD(withOperatingMode, ParametersBuilderImpl &, PARAMETER_OPERATING_MODE);

            ADD_RO_PARAMETER_BUILDER_METHOD(withScanFrequency, ParametersBuilderImpl &, PARAMETER_SCAN_FREQUENCY);

            ADD_RO_PARAMETER_BUILDER_METHOD(withScanDirection, ParametersBuilderImpl &, PARAMETER_SCAN_DIRECTION);

            ADD_RO_PARAMETER_BUILDER_METHOD(withSamplesPerScan, ParametersBuilderImpl &, PARAMETER_SAMPLES_PER_SCAN);
        };

        template<>
        struct ParametersBuilderImpl<RoParametersTags::HmiDisplayTag> : public ReadOnlyBuilderBase {
            ADD_RO_PARAMETER_BUILDER_METHOD(withHmiDisplayMode, ParametersBuilderImpl &, PARAMETER_HMI_DISPLAY_MODE);

            ADD_RO_PARAMETER_BUILDER_METHOD(withHmiLanguage, ParametersBuilderImpl &, PARAMETER_HMI_LANGUAGE);

            ADD_RO_PARAMETER_BUILDER_METHOD(withLockHmiButton, ParametersBuilderImpl &, PARAMETER_HMI_BUTTON_LOCK);

            ADD_RO_PARAMETER_BUILDER_METHOD(withLockHmiParameters, ParametersBuilderImpl &,
                                            PARAMETER_HMI_PARAMETER_LOCK);

            ADD_RO_PARAMETER_BUILDER_METHOD(withLEDLocatorIndication, ParametersBuilderImpl &,
                                            PARAMETER_LOCATOR_INDICATION);

            ADD_RO_PARAMETER_BUILDER_METHOD(withHmiStaticLogo, ParametersBuilderImpl &, PARAMETER_HMI_STATIC_LOGO);

            ADD_RO_PARAMETER_BUILDER_METHOD(withHmiStaticText1, ParametersBuilderImpl &, PARAMETER_HMI_STATIC_TEXT_1);

            ADD_RO_PARAMETER_BUILDER_METHOD(withHmiStaticText2, ParametersBuilderImpl &, PARAMETER_HMI_STATIC_TEXT_2);

            ADD_RO_PARAMETER_BUILDER_METHOD(withHmiApplicationBitmap, ParametersBuilderImpl &,
                                            PARAMETER_HMI_APPLICATION_BITMAP);

            ADD_RO_PARAMETER_BUILDER_METHOD(withHmiApplicationText1, ParametersBuilderImpl &,
                                            PARAMETER_HMI_APPLICATION_TEXT_1);

            ADD_RO_PARAMETER_BUILDER_METHOD(withHmiApplicationText2, ParametersBuilderImpl &,
                                            PARAMETER_HMI_APPLICATION_TEXT_2);
        };

        template<>
        struct ParametersBuilderImpl<RoParametersTags::SystemStatusTag> : public ReadOnlyBuilderBase {
            ADD_RO_PARAMETER_BUILDER_METHOD(requestStatusFlags, ParametersBuilderImpl &, PARAMETER_STATUS_FLAGS);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestLoadIndication, ParametersBuilderImpl &, PARAMETER_LOAD_INDICATION);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestSystemTimeRaw, ParametersBuilderImpl &, PARAMETER_SYSTEM_TIME_RAW);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestUpTime, ParametersBuilderImpl &, PARAMETER_UP_TIME);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestPowerCycles, ParametersBuilderImpl &, PARAMETER_POWER_CYCLES);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestOperationTime, ParametersBuilderImpl &, PARAMETER_OPERATION_TIME);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestOperationTimeScaled, ParametersBuilderImpl &,
                                            PARAMETER_OPERATION_TIME_SCALED);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestCurrentTemperature, ParametersBuilderImpl &,
                                            PARAMETER_TEMPERATURE_CURRENT);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestMinimalTemperature, ParametersBuilderImpl &,
                                            PARAMETER_TEMPERATURE_MIN);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestMaximalTemperature, ParametersBuilderImpl &,
                                            PARAMETER_TEMPERATURE_MAX);
        };

        template<>
        struct ParametersBuilderImpl<RoParametersTags::LensContaminationMonitor> : public ReadOnlyBuilderBase {
            ADD_RO_PARAMETER_BUILDER_METHOD(requestLcmSectorWarningFlag, ParametersBuilderImpl &,
                                            PARAMETER_LCM_SECTOR_WARN_FLAGS);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestLcmSectorErrorFlag, ParametersBuilderImpl &,
                                            PARAMETER_LCM_SECTOR_ERROR_FLAGS);

            ADD_RO_PARAMETER_BUILDER_METHOD(withSensitivity, ParametersBuilderImpl &,
                                            PARAMETER_LCM_DETECTION_SENSITIVITY);

            ADD_RO_PARAMETER_BUILDER_METHOD(withDetectionPeriod, ParametersBuilderImpl &,
                                            PARAMETER_LCM_DETECTION_PERIOD);

            ADD_RO_PARAMETER_BUILDER_METHOD(withLcmState, ParametersBuilderImpl &, PARAMETER_LCM_SECTOR_ENABLE);
        };

        struct ReadOnly;
        struct ReadWrite;

        template<typename AccessType>
        struct ParametersBuilder;

        template<>
        struct ParametersBuilder<ReadOnly> {
            using BasicInformation =
            internals::Parameters::ParametersBuilderImpl<internals::Parameters::RoParametersTags::BasicInformationTag>;
            using Capabilities =
            internals::Parameters::ParametersBuilderImpl<internals::Parameters::RoParametersTags::CapabilitiesTag>;
            using Ethernet = internals::Parameters::ParametersBuilderImpl<internals::Parameters::RoParametersTags::EthernetTag>;
            using Measure = internals::Parameters::ParametersBuilderImpl<internals::Parameters::RoParametersTags::MeasuringTag>;
            using SystemStatus =
            internals::Parameters::ParametersBuilderImpl<internals::Parameters::RoParametersTags::SystemStatusTag>;
        };

        template<>
        struct ParametersBuilder<ReadWrite> {
            using BasicInformation =
            internals::Parameters::ParametersBuilderImpl<internals::Parameters::RwParametersTags::BasicInformationTag>;
            using Ethernet = internals::Parameters::ParametersBuilderImpl<internals::Parameters::RwParametersTags::EthernetTag>;
            using Measure = internals::Parameters::ParametersBuilderImpl<internals::Parameters::RwParametersTags::MeasuringTag>;
            using HmiDisplay =
            internals::Parameters::ParametersBuilderImpl<internals::Parameters::RwParametersTags::HmiDisplayTag>;
            using UdpHandle =
            internals::Parameters::ParametersBuilderImpl<internals::Parameters::RwParametersTags::HandleUdpTag>;
            using TcpHandle =
            internals::Parameters::ParametersBuilderImpl<internals::Parameters::RwParametersTags::HandleTcpTag>;
        };

    } // namespace internals::Parameters

    using ReadOnlyParameters = internals::Parameters::ParametersBuilder<internals::Parameters::ReadOnly>;
    using ReadWriteParameters = internals::Parameters::ParametersBuilder<internals::Parameters::ReadWrite>;

    class R2000 {
    public:
        enum class AsyncRequestResult {
            SUCCESS,
            FAILED,
            TIMEOUT
        };

        struct AsyncResult {
        public:
            AsyncResult(AsyncRequestResult result, const PropertyTree &tree) : requestResult(result),
                                                                               propertyTree(tree) {}

        public:
            explicit operator bool() const { return requestResult == AsyncRequestResult::SUCCESS; }

            bool operator==(const AsyncRequestResult &rhs) const { return requestResult == rhs; }

            bool operator!=(const AsyncRequestResult &rhs) const { return !(rhs == (*this).requestResult); }

            [[nodiscard]] const PropertyTree &getPropertyTree() const { return propertyTree; }

            [[nodiscard]] AsyncRequestResult getRequestResult() const { return requestResult; }

        private:
            AsyncRequestResult requestResult{};
            PropertyTree propertyTree{};
        };

        using AsyncCommandCallback = std::function<void(const AsyncResult &)>;

    private:
        using HttpResult = std::tuple<int, std::string, std::string>;
        using HttpGetCallback = std::function<void(const HttpResult &)>;

        /**
         * Setup a new HTTP command interface.
         * @param configuration The device address and the chosen port.
         */
        explicit R2000(DeviceConfiguration configuration);

        template<typename... Arg>
        std::shared_ptr<R2000> static enableMakeShared(Arg &&... arg) {
            struct EnableMakeShared : public R2000 {
                explicit EnableMakeShared(Arg &&... arg) : R2000(std::forward<Arg>(arg)...) {}
            };
            return std::make_shared<EnableMakeShared>(std::forward<Arg>(arg)...);
        }

    public:
        [[maybe_unused]] static std::shared_ptr<R2000> makeShared(const DeviceConfiguration &configuration);

        /**
         * Send a HTTP Command with only one parameter and its corresponding value to the device.
         * @param command The command to send to the device.
         * @param parameter Parameters attached to the command to send.
         * @param value Values of the parameters.
         * @return A pair containing as first a flag indicating either or not the command has been successfully sent
         * and a reply received. The second of the pair is a property tree representing the content of the Http answer.
         */
        [[nodiscard]] std::pair<bool, Device::PropertyTree> sendHttpCommand(const std::string &command,
                                                                            const std::string &parameter = "",
                                                                            std::string value = "") const noexcept(false);

        /**
         * Send a HTTP Command with several parameters and their corresponding value to the device.
         * @param command The command to send to the device.
         * @param parameters Parameters and values map attached to the command to send.
         * @return A pair containing as first a flag indicating either or not the command has been successfully sent
         * and a reply received. The second of the pair is a property tree representing the content of the Http answer.
         */
        [[nodiscard]] std::pair<bool, Device::PropertyTree> sendHttpCommand(const std::string &command,
                                                                            const ParametersMap &parameters) const
        noexcept(false);

        /**
         *
         * @param command
         * @param callable
         * @param timeout
         * @return
         */
        bool asyncSendHttpCommand(const std::string &command, AsyncCommandCallback callable,
                                  std::chrono::milliseconds timeout) noexcept(true);

        /**
         * Asynchronously send a HTTP Command with several parameters and their corresponding value to the device.
         * @param command The command to send to the device.
         * @param parameters Parameters attached to the command to send.
         * @param callable A callable executed when the request has completed, failed or the timeout has been reached.
         * @param timeout The timeout of the request before being cancelled.
         * @return True if the request has been submitted, false if the request queue is full.
         */
        bool asyncSendHttpCommand(const std::string &command, const ParametersMap &parameters,
                                  AsyncCommandCallback callable,
                                  std::chrono::milliseconds timeout = 5000ms) noexcept(true);

        /**
         * Get the device protocol version.
         * @return The device protocol version or std::nullopt if the device cannot been reached.
         */
        std::optional<PFSDP> getDeviceProtocolVersion();

        /**
         * @return The device hostname.
         */
        [[nodiscard]] inline auto getHostname() const { return configuration.deviceAddress; }

        /**
         * @return The device name.
         */
        [[nodiscard]] inline auto getName() const { return configuration.name; }

        /**
         * Release the resources (mainly network)
         */
        virtual ~R2000();

    private:

        /**
         *
         * @param command
         * @param parameters
         * @return
         */
        static inline std::string makeRequestFromParameters(const std::string &command, const ParametersMap &parameters) {
            auto request{"/cmd/" + command};
            if (!parameters.empty())
                request += "?";
            for (const auto &parameter : parameters)
                request += parameter.first + "=" + parameter.second + "&";
            if (request.back() == '&')
                request.pop_back();
            return request;
        }

        [[nodiscard]] auto getDeviceQuery() const
        {
            const auto deviceAddress{configuration.deviceAddress.to_string()};
            const auto port{std::to_string(configuration.httpServicePort)};
            boost::asio::ip::tcp::resolver::query query{deviceAddress, port};
            return query;
        }

        /**
         * Send a HTTP GET request to the device. This methods blocks until the socket is connected.
         * @param requestPath The http request path to send.
         * @return A HttpResult object containing the status code, the header and the content of the answer.
         */
        [[nodiscard]] HttpResult HttpGet(const std::string &requestPath) const noexcept(false);

        /**
         * Send a HTTP GET to a remote endpoint through a given socket.
         * @param socket The socket to send the request through and read the answer from.
         * @param requestPath The http request path to send.
         * @return  A HttpResult object containing the status code, the header and the content of the answer.
         */
        [[nodiscard]] static HttpResult HttpGet(boost::asio::ip::tcp::socket &socket, const std::string &requestPath);

        /**
         * Asynchronously send a HTTP GET request to the device with a timeout.
         * @param request The http request to send.
         * @param callable A callable executed when the request has completed, failed or the timeout has been reached.
         * @param timeout The timeout of the request before being cancelled.
         * @return True if the request has been submitted, false if the request queue is full.
         */
        bool AsyncHttpGet(const std::string &request, HttpGetCallback callable,
                          std::chrono::milliseconds timeout) noexcept(true);

        /**
         * Handle the resolution of connection to endpoints and initiate a socket connection to send a http request.
         * @param error Error encountered during the resolution if any.
         * @param request The http request to send if the resolution has succeeded.
         * @param callable The http callback to execute once the http request is sent.
         * @param endpoints The endpoints to try to connect the socket to.
         * @param timeout The timeout of the socket connection before being cancelled.
         */
        void handleEndpointResolution(const boost::system::error_code &error, const std::string &request,
                                      HttpGetCallback callable,
                                      boost::asio::ip::tcp::resolver::results_type &endpoints,
                                      std::chrono::milliseconds timeout);

        /**
         * Handle the connection of the socket and send a http request.
         * @param error Error encountered during the socket connection if any.
         * @param request The http request to send if the resolution has succeeded.
         * @param callable The http callback to execute once the http request is sent.
         */
        void handleSocketConnection(const boost::system::error_code &error, const std::string &request,
                                    HttpGetCallback callable);

        /**
         * Verify if the sensor does not generate an error.
         * @param tree Represent the property tree.
         * @return True if the error code is equal to 0 and the error text is equal to "success", false otherwise.
         */
        static bool verifyErrorCode(const PropertyTree &tree);

        /**
         * Handle the asynchronous endpoints resolver deadline.
         * @param error Error encountered while waiting for the deadline of the resolver.
         */
        void handleAsyncResolverDeadline(const boost::system::error_code &error);

        /**
         * Handle the asynchronous socket connection deadline.
         * @param error Error encountered while waiting for the socket to connect.
         */
        void handleAsyncSocketDeadline(const boost::system::error_code &error);

        void scheduleResolverDeadline(std::chrono::milliseconds timeout);
        void scheduleSocketConnectionDeadline(std::chrono::milliseconds timeout);

    private:
        DeviceConfiguration configuration;
        std::future<void> ioServiceTask{};
        mutable boost::asio::io_service ioService{};
        boost::asio::steady_timer resolverDeadline{ioService};
        boost::asio::steady_timer socketDeadline{ioService};
        boost::asio::ip::tcp::resolver asyncResolver{ioService};
        boost::asio::ip::tcp::socket asyncSocket{ioService};
        std::optional<boost::asio::ip::tcp::resolver::results_type> deviceEndpoints{std::nullopt};
        std::condition_variable ioServiceOpsCv{};
        std::mutex ioServiceOpsCvLock{};
        std::atomic_bool interruptAsyncOpsFlag{false};
        Worker worker{10};
        mutable std::optional<PFSDP> deviceProtocolVersion{std::nullopt};
    };
} // namespace Device