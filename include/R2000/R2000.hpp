//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#pragma once

#include "ExponentialBackoff.hpp"
#include "DeviceHandle.hpp"
#include <any>
#include <bitset>
#include <boost/asio.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <chrono>
#include <iostream>
#include <list>
#include <optional>
#include <utility>

#include <boost/assign/list_of.hpp>
#include <boost/unordered_map.hpp>

#define BOOL_PARAMETER_TRUE "on"
#define BOOL_PARAMETER_FALSE "off"

#define ADD_RO_PARAMETER_BUILDER_METHOD(F_NAME, RETURN_TYPE, PARAMETER_NAME) \
RETURN_TYPE F_NAME () \
{                                                                     \
   parameters.push_back(PARAMETER_NAME); \
   return *this; \
}

#define ADD_RW_PARAMETER_BUILDER_METHOD(F_NAME, RETURN_TYPE, \
                        ARGUMENT_TYPE, \
                        ARGUMENT_NAME, \
                        PARAMETER_NAME) \
RETURN_TYPE F_NAME (ARGUMENT_TYPE ARGUMENT_NAME) \
{                                                                     \
   parameters[PARAMETER_NAME] = ARGUMENT_NAME; \
   return *this; \
}

#define ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(F_NAME, RETURN_TYPE, \
                        ARGUMENT_TYPE, \
                        ARGUMENT_NAME, \
                        ARGUMENT_TRANSFORM, \
                        PARAMETER_NAME) \
RETURN_TYPE F_NAME (ARGUMENT_TYPE ARGUMENT_NAME) \
{                                                                     \
   parameters[PARAMETER_NAME] = ARGUMENT_TRANSFORM; \
   return *this; \
}

#define ADD_RW_PARAMETER_BUILDER_METHOD_ON(F_NAME, RETURN_TYPE, \
                        PARAMETER_NAME) \
RETURN_TYPE F_NAME () \
{                                                                     \
   parameters[PARAMETER_NAME] = BOOL_PARAMETER_TRUE; \
   return *this; \
}

#define ADD_RW_PARAMETER_BUILDER_METHOD_OFF(F_NAME, RETURN_TYPE, \
                        PARAMETER_NAME) \
RETURN_TYPE F_NAME () \
{                                                                     \
   parameters[PARAMETER_NAME] = BOOL_PARAMETER_FALSE; \
   return *this; \
}

#define ADD_BUILD_BUILDER_METHOD(F_NAME, RETURN_TYPE, RETURN_ARGUMENT_NAME) \
RETURN_TYPE F_NAME () const\
{                                                                     \
   return RETURN_ARGUMENT_NAME; \
}

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
                : name(std::move(name)), deviceAddress(boost::asio::ip::address::from_string(deviceAddress)),
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

    enum class PFSDP : unsigned int
    {
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
    inline PFSDP protocolVersionFromString(const std::string & major, const std::string & minor)
    {
        const auto numericMajor{std::stoul(major)};
        const auto numericMinor{std::stoul(minor)};
        const auto numericVersion{numericMajor * 100 + numericMinor};
        switch(numericVersion)
        {
            case underlyingType(PFSDP::V100) : return PFSDP::V100;
            case underlyingType(PFSDP::V101) : return PFSDP::V101;
            case underlyingType(PFSDP::V102) : return PFSDP::V102;
            case underlyingType(PFSDP::V103) : return PFSDP::V103;
            case underlyingType(PFSDP::V104) : return PFSDP::V104;
            default:
                if(numericVersion > underlyingType(PFSDP::V104))
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

        template<typename Section>
        struct ParametersBuilderImpl {
        };

        template<>
        struct ParametersBuilderImpl<RwParametersTags::BasicInformationTag> {

            ADD_RW_PARAMETER_BUILDER_METHOD(withUserTag,
                                            ParametersBuilderImpl &,
                                            const std::string &,
                                            userTag,
                                            PARAMETER_USER_TAG);

            ADD_RW_PARAMETER_BUILDER_METHOD(withUserNotes,
                                            ParametersBuilderImpl &,
                                            const std::string &,
                                            userNotes,
                                            PARAMETER_USER_NOTES);

            ADD_BUILD_BUILDER_METHOD(build, ParametersMap, parameters);

            struct AsList {
                ADD_RO_PARAMETER_BUILDER_METHOD(withUserTag, AsList &, PARAMETER_USER_TAG);

                ADD_RO_PARAMETER_BUILDER_METHOD(withUserNotes, AsList &, PARAMETER_USER_NOTES);

                ADD_BUILD_BUILDER_METHOD(build, ParametersList, parameters);
            private:
                ParametersList parameters;
            };

        private:
            ParametersMap parameters;
        };

        template<>
        struct ParametersBuilderImpl<RwParametersTags::EthernetTag> {

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withIpMode,
                                                           ParametersBuilderImpl &,
                                                           IpMode,
                                                           mode,
                                                           IpModeToString.at(mode),
                                                           PARAMETER_IP_ADDRESS);

            ADD_RW_PARAMETER_BUILDER_METHOD(withIpAddress,
                                            ParametersBuilderImpl &,
                                            const std::string &,
                                            ipAddress,
                                            PARAMETER_IP_ADDRESS);

            ADD_RW_PARAMETER_BUILDER_METHOD(withSubnetMask,
                                            ParametersBuilderImpl &,
                                            const std::string &,
                                            subnetMask,
                                            PARAMETER_SUBNET_MASK);

            ADD_RW_PARAMETER_BUILDER_METHOD(withGateway,
                                            ParametersBuilderImpl &,
                                            const std::string &,
                                            gateway,
                                            PARAMETER_GATEWAY);

            ADD_BUILD_BUILDER_METHOD(build, ParametersMap, parameters);

            struct AsList {
                ADD_RO_PARAMETER_BUILDER_METHOD(withIpMode, AsList &, PARAMETER_IP_MODE);

                ADD_RO_PARAMETER_BUILDER_METHOD(withIpAddress, AsList &, PARAMETER_IP_ADDRESS);

                ADD_RO_PARAMETER_BUILDER_METHOD(withSubnetMask, AsList &, PARAMETER_SUBNET_MASK);

                ADD_RO_PARAMETER_BUILDER_METHOD(withGateway, AsList &, PARAMETER_GATEWAY);

                ADD_BUILD_BUILDER_METHOD(build, ParametersList, parameters);
            private:
                ParametersList parameters;
            };

        private:
            ParametersMap parameters;
            const boost::unordered_map<IpMode, std::string> IpModeToString = boost::assign::map_list_of
                    (IpMode::STATIC, "static")
                    (IpMode::DHCP, "dhcp")
                    (IpMode::AUTO_IP, "autoip");
        };

        template<>
        struct ParametersBuilderImpl<RwParametersTags::MeasuringTag> {

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withOperatingMode,
                                                           ParametersBuilderImpl &,
                                                           OPERATING_MODE,
                                                           operatingMode,
                                                           operatingModeToString.at(operatingMode),
                                                           PARAMETER_OPERATING_MODE);

            ADD_RW_PARAMETER_BUILDER_METHOD(withScanFrequency,
                                            ParametersBuilderImpl &,
                                            const std::string &,
                                            scanFrequency,
                                            PARAMETER_SCAN_FREQUENCY);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withScanDirection,
                                                           ParametersBuilderImpl &,
                                                           SCAN_DIRECTION,
                                                           scanDirection,
                                                           scanDirectionToString.at(scanDirection),
                                                           PARAMETER_SCAN_DIRECTION);

            ADD_RW_PARAMETER_BUILDER_METHOD(withSamplesPerScan,
                                            ParametersBuilderImpl &,
                                            const std::string &,
                                            samplesPerScan,
                                            PARAMETER_SAMPLES_PER_SCAN);

            ADD_BUILD_BUILDER_METHOD(build, ParametersMap, parameters);

            struct AsList {
                ADD_RO_PARAMETER_BUILDER_METHOD(withOperatingMode, AsList &, PARAMETER_OPERATING_MODE);
                ADD_RO_PARAMETER_BUILDER_METHOD(withScanFrequency, AsList &, PARAMETER_SCAN_FREQUENCY);
                ADD_RO_PARAMETER_BUILDER_METHOD(withScanDirection, AsList &, PARAMETER_SCAN_DIRECTION);
                ADD_RO_PARAMETER_BUILDER_METHOD(withSamplesPerScan, AsList &, PARAMETER_SAMPLES_PER_SCAN);
                ADD_BUILD_BUILDER_METHOD(build, ParametersList, parameters);
            private:
                ParametersList parameters;
            };

        private:
            ParametersMap parameters;
            const boost::unordered_map<OPERATING_MODE, std::string> operatingModeToString = boost::assign::map_list_of
                    (OPERATING_MODE::MEASURE, "measure")
                    (OPERATING_MODE::EMITTER_OFF, "emitter_off");
            const boost::unordered_map<SCAN_DIRECTION, std::string> scanDirectionToString = boost::assign::map_list_of
                    (SCAN_DIRECTION::CW, "cw")
                    (SCAN_DIRECTION::CCW, "ccw");
        };

        template<>
        struct ParametersBuilderImpl<RwParametersTags::HmiDisplayTag> {

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withHmiDisplayMode,
                                                           ParametersBuilderImpl &,
                                                           HMI_DISPLAY_MODE,
                                                           displayMode,
                                                           HmiDisplayModeToString.at(displayMode),
                                                           PARAMETER_HMI_DISPLAY_MODE);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withHmiLanguage,
                                                           ParametersBuilderImpl &,
                                                           Language,
                                                           language,
                                                           LanguageToString.at(language),
                                                           PARAMETER_HMI_LANGUAGE);

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

            ADD_RW_PARAMETER_BUILDER_METHOD(withHmiStaticLogo,
                                            ParametersBuilderImpl &,
                                            const std::string &,
                                            base64Logo,
                                            PARAMETER_HMI_STATIC_LOGO);

            ADD_RW_PARAMETER_BUILDER_METHOD(withHmiStaticText1,
                                            ParametersBuilderImpl &,
                                            const std::string &,
                                            line,
                                            PARAMETER_HMI_STATIC_TEXT_1);

            ADD_RW_PARAMETER_BUILDER_METHOD(withHmiStaticText2,
                                            ParametersBuilderImpl &,
                                            const std::string &,
                                            line,
                                            PARAMETER_HMI_STATIC_TEXT_2);

            ADD_RW_PARAMETER_BUILDER_METHOD(withHmiApplicationBitmap,
                                            ParametersBuilderImpl &,
                                            const std::string &,
                                            base64Bitmap,
                                            PARAMETER_HMI_APPLICATION_BITMAP);

            ADD_RW_PARAMETER_BUILDER_METHOD(withHmiApplicationText1,
                                            ParametersBuilderImpl &,
                                            const std::string &,
                                            line,
                                            PARAMETER_HMI_APPLICATION_TEXT_1);

            ADD_RW_PARAMETER_BUILDER_METHOD(withHmiApplicationText2,
                                            ParametersBuilderImpl &,
                                            const std::string &,
                                            line,
                                            PARAMETER_HMI_APPLICATION_TEXT_2);

            ADD_BUILD_BUILDER_METHOD(build, ParametersMap, parameters);

            struct AsList {
                ADD_RO_PARAMETER_BUILDER_METHOD(withHmiDisplayMode, AsList &, PARAMETER_HMI_DISPLAY_MODE);

                ADD_RO_PARAMETER_BUILDER_METHOD(withHmiLanguage, AsList &, PARAMETER_HMI_LANGUAGE);

                ADD_RO_PARAMETER_BUILDER_METHOD(withLockHmiButton, AsList &, PARAMETER_HMI_BUTTON_LOCK);

                ADD_RO_PARAMETER_BUILDER_METHOD(withLockHmiParameters, AsList &, PARAMETER_HMI_PARAMETER_LOCK);

                ADD_RO_PARAMETER_BUILDER_METHOD(withLEDLocatorIndication, AsList &, PARAMETER_LOCATOR_INDICATION);

                ADD_RO_PARAMETER_BUILDER_METHOD(withHmiStaticLogo, AsList &, PARAMETER_HMI_STATIC_LOGO);

                ADD_RO_PARAMETER_BUILDER_METHOD(withHmiStaticText1, AsList &, PARAMETER_HMI_STATIC_TEXT_1);

                ADD_RO_PARAMETER_BUILDER_METHOD(withHmiStaticText2, AsList &, PARAMETER_HMI_STATIC_TEXT_2);

                ADD_RO_PARAMETER_BUILDER_METHOD(withHmiApplicationBitmap, AsList &, PARAMETER_HMI_APPLICATION_BITMAP);

                ADD_RO_PARAMETER_BUILDER_METHOD(withHmiApplicationText1, AsList &, PARAMETER_HMI_APPLICATION_TEXT_1);

                ADD_RO_PARAMETER_BUILDER_METHOD(withHmiApplicationText2, AsList &, PARAMETER_HMI_APPLICATION_TEXT_2);

                ADD_BUILD_BUILDER_METHOD(build, ParametersList, parameters);

            private:
                ParametersList parameters;
            };

        private:
            ParametersMap parameters;
            const boost::unordered_map<Language, std::string> LanguageToString = boost::assign::map_list_of
                    (Language::ENGLISH, "english")
                    (Language::GERMAN, "german");
            const boost::unordered_map<HMI_DISPLAY_MODE, std::string> HmiDisplayModeToString = boost::assign::map_list_of
                    (HMI_DISPLAY_MODE::OFF, "off")
                    (HMI_DISPLAY_MODE::STATIC_LOGO, "static_logo")
                    (HMI_DISPLAY_MODE::STATIC_TEXT, "static_text")
                    (HMI_DISPLAY_MODE::BAR_GRAPH_DISTANCE, "bargraph_distance")
                    (HMI_DISPLAY_MODE::BAR_GRAPH_ECHO, "bargraph_echo")
                    (HMI_DISPLAY_MODE::BAR_GRAPH_REFLECTOR, "bargraph_reflector")
                    (HMI_DISPLAY_MODE::APPLICATION_BITMAP, "application_bitmap")
                    (HMI_DISPLAY_MODE::APPLICATION_TEXT, "application_text");
        };

        struct HandleParameters {
            const boost::unordered_map<PACKET_TYPE, std::string> scanPacketString = boost::assign::map_list_of
                    (PACKET_TYPE::A, "A")
                    (PACKET_TYPE::B, "B")
                    (PACKET_TYPE::C, "C");
            const boost::unordered_map<CRC, std::string> crcToString = boost::assign::map_list_of
                    (CRC::NONE, "none")
                    (CRC::CRC32C, "crc32c");
        };

        template<>
        struct ParametersBuilderImpl<RwParametersTags::HandleTcpTag> : public HandleParameters {

            ParametersBuilderImpl() {
                (void) withoutWatchdog();
            }

            ADD_RW_PARAMETER_BUILDER_METHOD_ON(withWatchdog, ParametersBuilderImpl &, PARAMETER_HANDLE_WATCHDOG);

            ADD_RW_PARAMETER_BUILDER_METHOD_OFF(withoutWatchdog, ParametersBuilderImpl &, PARAMETER_HANDLE_WATCHDOG);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withWatchdogTimeout,
                                                           ParametersBuilderImpl &,
                                                           const unsigned int,
                                                           watchdogtimeout,
                                                           std::to_string(watchdogtimeout),
                                                           PARAMETER_HANDLE_WATCHDOG_TIMEOUT);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withPacketType,
                                                           ParametersBuilderImpl &,
                                                           PACKET_TYPE,
                                                           packetType,
                                                           scanPacketString.at(packetType),
                                                           PARAMETER_HANDLE_PACKET_TYPE);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withPacketCrc,
                                                           ParametersBuilderImpl &,
                                                           CRC,
                                                           crcType,
                                                           crcToString.at(crcType),
                                                           PARAMETER_HANDLE_PACKET_CRC);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withStartAngle,
                                                           ParametersBuilderImpl &,
                                                           const int,
                                                           startAngle,
                                                           std::to_string(startAngle),
                                                           PARAMETER_HANDLE_START_ANGLE);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withNumberOfPointsPerScan,
                                                           ParametersBuilderImpl &,
                                                           const unsigned int,
                                                           numberOfPointsPerScan,
                                                           std::to_string(numberOfPointsPerScan),
                                                           PARAMETER_HANDLE_MAX_NUM_POINTS_SCAN);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withNumberOfScanToSkip,
                                                           ParametersBuilderImpl &,
                                                           const unsigned int,
                                                           numberOfScanToSkip,
                                                           std::to_string(numberOfScanToSkip),
                                                           PARAMETER_HANDLE_SKIP_SCANS);

            ADD_BUILD_BUILDER_METHOD(build, ParametersMap, parameters);
        private:
            ParametersMap parameters;
        };

        template<>
        struct ParametersBuilderImpl<RwParametersTags::HandleUdpTag> : public HandleParameters {

            ParametersBuilderImpl() {
                (void) withoutWatchdog();
            }

            ADD_RW_PARAMETER_BUILDER_METHOD(withHostname,
                                            ParametersBuilderImpl &,
                                            const std::string &,
                                            hostname,
                                            PARAMETER_HANDLE_ADDRESS);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withPort,
                                                           ParametersBuilderImpl &,
                                                           const uint16_t,
                                                           port,
                                                           std::to_string(port),
                                                           PARAMETER_HANDLE_PORT);

            ADD_RW_PARAMETER_BUILDER_METHOD_ON(withWatchdog, ParametersBuilderImpl &, PARAMETER_HANDLE_WATCHDOG);

            ADD_RW_PARAMETER_BUILDER_METHOD_OFF(withoutWatchdog, ParametersBuilderImpl &, PARAMETER_HANDLE_WATCHDOG);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withWatchdogTimeout,
                                                           ParametersBuilderImpl &,
                                                           const unsigned int,
                                                           watchdogtimeout,
                                                           std::to_string(watchdogtimeout),
                                                           PARAMETER_HANDLE_WATCHDOG_TIMEOUT);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withPacketType,
                                                           ParametersBuilderImpl &,
                                                           PACKET_TYPE,
                                                           packetType,
                                                           scanPacketString.at(packetType),
                                                           PARAMETER_HANDLE_PACKET_TYPE);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withPacketCrc,
                                                           ParametersBuilderImpl &,
                                                           CRC,
                                                           crcType,
                                                           crcToString.at(crcType),
                                                           PARAMETER_HANDLE_PACKET_CRC);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withStartAngle,
                                                           ParametersBuilderImpl &,
                                                           const int,
                                                           startAngle,
                                                           std::to_string(startAngle),
                                                           PARAMETER_HANDLE_START_ANGLE);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withNumberOfPointsPerScan,
                                                           ParametersBuilderImpl &,
                                                           const unsigned int,
                                                           numberOfPointsPerScan,
                                                           std::to_string(numberOfPointsPerScan),
                                                           PARAMETER_HANDLE_MAX_NUM_POINTS_SCAN);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withNumberOfScanToSkip,
                                                           ParametersBuilderImpl &,
                                                           const unsigned int,
                                                           numberOfScanToSkip,
                                                           std::to_string(numberOfScanToSkip),
                                                           PARAMETER_HANDLE_SKIP_SCANS);

            ADD_BUILD_BUILDER_METHOD(build, ParametersMap, parameters);
        private:
            ParametersMap parameters;
        };

        template<>
        struct ParametersBuilderImpl<RwParametersTags::LensContaminationMonitor> {

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withSensitivity,
                                                           ParametersBuilderImpl &,
                                                           LCM_SENSITIVITY,
                                                           sensitivity,
                                                           LCMSensitivityToString.at(sensitivity),
                                                           PARAMETER_LCM_DETECTION_SENSITIVITY);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withDetectionPeriod,
                                                           ParametersBuilderImpl &,
                                                           std::chrono::milliseconds,
                                                           period,
                                                           std::to_string(period.count()),
                                                           PARAMETER_LCM_DETECTION_PERIOD);

            ADD_RW_PARAMETER_BUILDER_METHOD_ON(withLcmEnabled, ParametersBuilderImpl &, PARAMETER_LCM_SECTOR_ENABLE);

            ADD_RW_PARAMETER_BUILDER_METHOD_OFF(withLcmDisabled, ParametersBuilderImpl &, PARAMETER_LCM_SECTOR_ENABLE);

            ADD_BUILD_BUILDER_METHOD(build, ParametersMap, parameters);

            struct AsList {
                ADD_RO_PARAMETER_BUILDER_METHOD(withSensitivity, AsList &, PARAMETER_LCM_DETECTION_SENSITIVITY);

                ADD_RO_PARAMETER_BUILDER_METHOD(withDetectionPeriod, AsList &, PARAMETER_LCM_DETECTION_PERIOD);

                ADD_RO_PARAMETER_BUILDER_METHOD(withLcmState, AsList &, PARAMETER_LCM_SECTOR_ENABLE);

                ADD_BUILD_BUILDER_METHOD(build, ParametersList, parameters);
            private:
                ParametersList parameters;
            };

        private:
            ParametersMap parameters;
            const boost::unordered_map<LCM_SENSITIVITY, std::string> LCMSensitivityToString = boost::assign::map_list_of
                    (LCM_SENSITIVITY::DISABLED, "disabled")
                    (LCM_SENSITIVITY::LOW, "low")
                    (LCM_SENSITIVITY::MEDIUM, "medium")
                    (LCM_SENSITIVITY::HIGH, "high");
        };

        template<>
        struct ParametersBuilderImpl<RoParametersTags::BasicInformationTag> {
            ADD_RO_PARAMETER_BUILDER_METHOD(requestDeviceFamily, ParametersBuilderImpl &, PARAMETER_DEVICE_FAMILY);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestVendor, ParametersBuilderImpl &, PARAMETER_VENDOR);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestProduct, ParametersBuilderImpl &, PARAMETER_PRODUCT);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestPart, ParametersBuilderImpl &, PARAMETER_PART);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestSerial, ParametersBuilderImpl &, PARAMETER_SERIAL);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestFirmwareRevision, ParametersBuilderImpl &, PARAMETER_REVISION_FW);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestHardwareRevision, ParametersBuilderImpl &, PARAMETER_REVISION_HW);

            ADD_BUILD_BUILDER_METHOD(build, ParametersList, parameters);
        private:
            ParametersList parameters;
        };

        template<>
        struct ParametersBuilderImpl<RoParametersTags::CapabilitiesTag> {

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
            ADD_BUILD_BUILDER_METHOD(build, ParametersList, parameters);
        private:
            ParametersList parameters;
        };

        template<>
        struct ParametersBuilderImpl<RoParametersTags::EthernetTag> {
            ADD_RO_PARAMETER_BUILDER_METHOD(requestCurrentIpMode, ParametersBuilderImpl &, PARAMETER_IP_MODE_CURRENT);
            ADD_RO_PARAMETER_BUILDER_METHOD(requestCurrentIpAddress, ParametersBuilderImpl &,
                                            PARAMETER_IP_ADDRESS_CURRENT);
            ADD_RO_PARAMETER_BUILDER_METHOD(requestCurrentSubnetMask, ParametersBuilderImpl &,
                                            PARAMETER_SUBNET_MASK_CURRENT);
            ADD_RO_PARAMETER_BUILDER_METHOD(requestCurrentGateway, ParametersBuilderImpl &, PARAMETER_GATEWAY_CURRENT);
            ADD_RO_PARAMETER_BUILDER_METHOD(requestMacAddress, ParametersBuilderImpl &, PARAMETER_MAC_ADDRESS);
            ADD_BUILD_BUILDER_METHOD(build, ParametersList, parameters);
        private:
            ParametersList parameters;
        };

        template<>
        struct ParametersBuilderImpl<RoParametersTags::MeasuringTag> {
            ADD_RO_PARAMETER_BUILDER_METHOD(requestMeasuredFrequency, ParametersBuilderImpl &,
                                            PARAMETER_SCAN_FREQUENCY_MEASURED);
            ADD_BUILD_BUILDER_METHOD(build, ParametersList, parameters);
        private:
            ParametersList parameters;
        };

        template<>
        struct ParametersBuilderImpl<RoParametersTags::SystemStatusTag> {
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
            ADD_BUILD_BUILDER_METHOD(build, ParametersList, parameters);
        private:
            ParametersList parameters;
        };

        template<>
        struct ParametersBuilderImpl<RoParametersTags::LensContaminationMonitor> {
            ADD_RO_PARAMETER_BUILDER_METHOD(requestLcmSectorWarningFlag, ParametersBuilderImpl &,
                                            PARAMETER_LCM_SECTOR_WARN_FLAGS);
            ADD_RO_PARAMETER_BUILDER_METHOD(requestLcmSectorErrorFlag, ParametersBuilderImpl &,
                                            PARAMETER_LCM_SECTOR_ERROR_FLAGS);

            ADD_BUILD_BUILDER_METHOD(build, ParametersList, parameters);
        private:
            ParametersList parameters;
        };

        struct ReadOnly;
        struct ReadWrite;

        template<typename AccessType>
        struct ParametersBuilder;

        template<>
        struct ParametersBuilder<ReadOnly> {
            using BasicInformation = internals::Parameters::ParametersBuilderImpl<
                    internals::Parameters::RoParametersTags::BasicInformationTag>;
            using Capabilities = internals::Parameters::ParametersBuilderImpl<
                    internals::Parameters::RoParametersTags::CapabilitiesTag>;
            using Ethernet = internals::Parameters::ParametersBuilderImpl<
                    internals::Parameters::RoParametersTags::EthernetTag>;
            using Measure = internals::Parameters::ParametersBuilderImpl<
                    internals::Parameters::RoParametersTags::MeasuringTag>;
            using SystemStatus = internals::Parameters::ParametersBuilderImpl<
                    internals::Parameters::RoParametersTags::SystemStatusTag>;
        };

        template<>
        struct ParametersBuilder<ReadWrite> {
            using BasicInformation = internals::Parameters::ParametersBuilderImpl<
                    internals::Parameters::RwParametersTags::BasicInformationTag>;
            using Ethernet = internals::Parameters::ParametersBuilderImpl<
                    internals::Parameters::RwParametersTags::EthernetTag>;
            using Measure = internals::Parameters::ParametersBuilderImpl<
                    internals::Parameters::RwParametersTags::MeasuringTag>;
            using HmiDisplay = internals::Parameters::ParametersBuilderImpl<
                    internals::Parameters::RwParametersTags::HmiDisplayTag>;
            using UdpHandle = internals::Parameters::ParametersBuilderImpl<
                    internals::Parameters::RwParametersTags::HandleUdpTag>;
            using TcpHandle = internals::Parameters::ParametersBuilderImpl<
                    internals::Parameters::RwParametersTags::HandleTcpTag>;
        };

    }

    using ReadOnlyParameters = internals::Parameters::ParametersBuilder<internals::Parameters::ReadOnly>;
    using ReadWriteParameters = internals::Parameters::ParametersBuilder<internals::Parameters::ReadWrite>;

    class R2000 {
    private:
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
         * Use this method in order to send an HTTP Command with only one parameter and its corresponding value.
         * @param command Give a command among the available commands of the device.
         * @param parameters Give a parameter among the available parameters of the device.
         * @param values Give a value to the corresponding parameter
         * @return The method with the given command, parameter and its corresponding value if the parameter is not empty.
         */
        [[nodiscard]] std::pair<bool, Device::PropertyTree>
        sendHttpCommand(const std::string &command, const std::string &parameters = "",
                        std::string values = "") const noexcept(false);

        /**
         * Use this method in order to send an HTTP Command with several parameters and their corresponding value.
         * @param command Give a command among the available commands of the device.
         * @param paramValues Give many parameters with their corresponding value.
         * @return The method with the given command, parameters and values if the HTTP Status code is equal to 200, an
         * error otherwise.
         */
        [[nodiscard]] std::pair<bool, Device::PropertyTree>
        sendHttpCommand(const std::string &command, const ParametersMap &parameters) const noexcept(false);

        std::optional<PFSDP> getDeviceProtocolVersion() const;

        /**
         *
         * @return
         */
        [[nodiscard]] inline auto getHostname() const {
            return mConfiguration.deviceAddress;
        }

        /**
         *
         * @return
         */
        [[nodiscard]] inline auto getName() const {
            return mConfiguration.name;
        }

    private:
        /**
         * Send a HTTP GET request to the device.
         * @param requestPath The last part of an URL with a slash leading.
         * @param header The response header returned as std::string.
         * @return The HTTP systemStatusMap code or 0 in case of an error.
         */
        [[nodiscard]] std::tuple<int, std::string, std::string> httpGet(const std::string &requestPath) const
        noexcept(false);

        /**
         * Verify if the sensor does not generate an error.
         * @param tree Represent the property tree.
         * @return True if the error code is equal to 0 and the error text is equal to "success", false otherwise.
         */
        static bool verifyErrorCode(const PropertyTree &tree);

    private:
        DeviceConfiguration mConfiguration;
        mutable std::optional<PFSDP> deviceProtocolVersion{std::nullopt};
    };

    namespace internals::Parameters {

        struct CommandTags {
            struct GetProtocolInfo;
            struct ReleaseHandle;
            struct StartScanOutput;
            struct StopScanOutput;
            struct FeedWatchdog;
            struct GetParameters;
            struct FetchParameterList;
            struct SetParameters;
            struct FactoryResetParameters;
            struct FactoryResetDevice;
            struct RebootDevice;
            struct RequestUdpHandle;
            struct RequestTcpHandle;
            struct GetScanOutputConfig;
            struct SetScanOutputConfig;
        };

        struct ParametersChaining {
            template<typename T>
            static constexpr inline void chain(ParametersList &parameters, T &builder) {
                const auto builderParameters{builder.build()};
                parameters.insert(std::end(parameters), std::cbegin(builderParameters), std::cend(builderParameters));
            }

            template<typename T, typename... Rest>
            static constexpr inline void chain(ParametersList &parameters, T &builder, Rest &&... args) {
                const auto builderParameters{builder.build()};
                parameters.insert(std::end(parameters), std::cbegin(builderParameters), std::cend(builderParameters));
                chain<Rest...>(parameters, std::forward<Rest>(args)...);
            }
        };

        template<typename ReturnType>
        struct ReturnTypeDependantExecutionSelector {
            template<typename Execute, typename T>
            static constexpr inline ReturnType resolve(Execute &&execute, T &&builder) {
                return execute(std::forward<T>(builder));
            }

            template<typename Execute, typename T, typename... Rest>
            static constexpr inline ReturnType resolve(Execute &&execute, T &&builder, Rest &&... args) {
                return execute(std::forward<T>(builder)) &&
                       resolve<Execute, Rest...>(std::forward<Execute>(execute), std::forward<Rest>(args)...);
            }
        };

        template<>
        struct ReturnTypeDependantExecutionSelector<void> {
            template<typename Execute, typename T>
            static constexpr inline void resolve(Execute &&execute, T &&builder) {
                execute(std::forward<T>(builder));
            }

            template<typename Execute, typename T, typename... Rest>
            static constexpr inline void resolve(Execute &&execute, T &&builder, Rest &&... args) {
                execute(std::forward<T>(builder));
                resolve<Execute, Rest...>(std::forward<Execute>(execute), std::forward<Rest>(args)...);
            }
        };

        struct RecursiveExecutorHelper {

            template<typename Execute, typename T>
            static constexpr inline auto resolve(Execute &&execute, T &&builder) ->
            std::decay_t<typename std::result_of<Execute(T)>::type> {
                return ReturnTypeDependantExecutionSelector<std::decay_t<typename std::result_of<Execute(T)>::type>>
                ::template resolve<Execute, T>(std::forward<Execute>(execute), std::forward<T>(builder));
            }

            template<typename Execute, typename T, typename... Rest>
            static constexpr inline auto resolve(Execute &&execute, T &&builder, Rest &&... args) ->
            std::decay_t<typename std::result_of<Execute(T)>::type> {
                return ReturnTypeDependantExecutionSelector<std::decay_t<typename std::result_of<Execute(T)>::type>>
                ::template resolve<Execute,T, Rest...>(std::forward<Execute>(execute), std::forward<T>(builder),
                                                     std::forward<Rest>(args)...);
            }
        };

        template<typename Command>
        struct CommandExecutorImpl {
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::GetProtocolInfo> {
            [[maybe_unused]] explicit CommandExecutorImpl(const R2000 &device) : device(device) {}

            std::optional<std::pair<ParametersMap, ParametersList>> execute() {
                ParametersList availableCommands{};
                ParametersMap protocolInfo{};

                const auto result{device.sendHttpCommand(COMMAND_GET_PROTOCOL_INFO)};
                const auto status{result.first};
                const auto &propertyTree{result.second};

                const auto commands{propertyTree.get_child_optional(PARAMETER_AVAILABLE_COMMANDS)};
                const auto protocolName{propertyTree.get_optional<std::string>(PARAMETER_PROTOCOL_NAME)};
                const auto protocolVersionMajor{
                        propertyTree.get_optional<std::string>(PARAMETER_PROTOCOL_VERSION_MAJOR)};
                const auto protocolVersionMinor{
                        propertyTree.get_optional<std::string>(PARAMETER_PROTOCOL_VERSION_MINOR)};

                if (!status || !commands || !protocolName || !protocolVersionMajor || !protocolVersionMinor)
                    return std::nullopt;
                for (const auto &name : *commands) {
                    const auto parameterName{name.second.get<std::string>("")};
                    if (parameterName.empty())
                        continue;
                    availableCommands.emplace_back(parameterName);
                }
                protocolInfo.insert(std::make_pair(PARAMETER_PROTOCOL_NAME, *protocolName));
                protocolInfo.insert(std::make_pair(PARAMETER_PROTOCOL_VERSION_MAJOR, *protocolVersionMajor));
                protocolInfo.insert(std::make_pair(PARAMETER_PROTOCOL_VERSION_MINOR, *protocolVersionMinor));

                return std::make_optional(std::make_pair(protocolInfo, availableCommands));
            }

        private:
            const R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::ReleaseHandle> {
            [[maybe_unused]] explicit CommandExecutorImpl(const R2000 &device) : device(device) {}

            template<typename ... Args>
            inline bool execute(Args &&... args) {
                return RecursiveExecutorHelper::resolve([this](const auto &handle) {
                    return device.sendHttpCommand(COMMAND_RELEASE_HANDLE,
                                                  PARAMETER_NAME_HANDLE,
                                                  handle.value).first;
                }, std::forward<Args>(args)...);
            }

        private:
            const R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::StartScanOutput> {
            [[maybe_unused]] explicit CommandExecutorImpl(const R2000 &device) : device(device) {}

            template<typename ... Args>
            inline bool execute(Args &&... args) {
                return RecursiveExecutorHelper::resolve([this](const auto &handle) {
                    return device.sendHttpCommand(COMMAND_START_SCAN_OUTPUT,
                                                  PARAMETER_NAME_HANDLE,
                                                  handle.value).first;
                }, std::forward<Args>(args)...);
            }

        private:
            const R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::StopScanOutput> {
            [[maybe_unused]] explicit CommandExecutorImpl(const R2000 &device) : device(device) {}

            template<typename ... Args>
            inline bool execute(Args &&... args) {
                return RecursiveExecutorHelper::resolve([this](const auto &handle) {
                    return device.sendHttpCommand(COMMAND_STOP_SCAN_OUTPUT,
                                                  PARAMETER_NAME_HANDLE,
                                                  handle.value).first;
                }, std::forward<Args>(args)...);
            }

        private:
            const R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::FeedWatchdog> {
            [[maybe_unused]] explicit CommandExecutorImpl(const R2000 &device) : device(device) {}

            template<typename ... Args>
            inline bool execute(Args &&... args) {
                return RecursiveExecutorHelper::resolve([this](const auto &handle) {
                    return device.sendHttpCommand(COMMAND_FEED_WATCHDOG,
                                                  PARAMETER_NAME_HANDLE,
                                                  handle.value).first;
                }, std::forward<Args>(args)...);
            }

        private:
            const R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::GetParameters> {
            [[maybe_unused]] explicit CommandExecutorImpl(const R2000 &device) : device(device) {}

            template<typename ... Args>
            inline std::optional<ParametersMap> execute(Args &&... args) {
                ParametersMap parameters{};
                RecursiveExecutorHelper::resolve([this, &parameters](const auto &builder) {
                    const auto requestedParameters{builder.build()};
                    const auto retrievedParameters{getParametersValues(requestedParameters)};
                    parameters.template insert(std::cbegin(retrievedParameters), std::cend(retrievedParameters));
                }, std::forward<Args>(args)...);
                if (parameters.empty())
                    return std::nullopt;
                return parameters;
            }

        private:
            ParametersMap getParametersValues(const Device::ParametersList &list) {
                ParametersMap keysValues{};
                std::string namelist{};
                for (const auto &name : list)
                    namelist += (name + ";");
                namelist.pop_back();
                const auto result{device.sendHttpCommand(COMMAND_GET_PARAMETER, PARAMETER_NAME_LIST, namelist)};
                const auto status{result.first};
                const auto &propertyTree{result.second};
                if (!status)
                    return {};
                for (const auto &name : list) {
                    auto value{propertyTree.get_optional<std::string>(name)};
                    if (!value) {
                        keysValues.insert(std::make_pair(name, ""));
                        continue;
                    }
                    keysValues.insert(std::make_pair(name, *value));
                }
                return keysValues;
            }

        private:
            const R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::FetchParameterList> {
            [[maybe_unused]] explicit CommandExecutorImpl(const R2000 &device) : device(device) {}

            std::optional<ParametersList> execute() {
                ParametersList parameterList{};
                const auto result{device.sendHttpCommand(COMMAND_LIST_PARAMETERS)};
                const auto status{result.first};
                const auto &propertyTree{result.second};

                const auto list{propertyTree.get_child_optional("parameters")};
                if (!status || !list)
                    return std::nullopt;
                for (const auto &name : *list) {
                    const auto parameterName{name.second.get<std::string>("")};
                    if (parameterName.empty())
                        continue;
                    parameterList.emplace_back(parameterName);
                }
                return std::make_optional(parameterList);
            }

        private:
            const R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::SetParameters> {
            [[maybe_unused]] explicit CommandExecutorImpl(const R2000 &device) : device(device) {}

            template<typename ... Args>
            inline bool execute(Args &&... args) {
                return RecursiveExecutorHelper::template resolve([this](const auto &builder) {
                    const auto parameters{builder.build()};
                    return device.sendHttpCommand(COMMAND_SET_PARAMETER, parameters).first;
                }, std::forward<Args>(args)...);
            }

        private:
            const R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::FactoryResetParameters> {
            [[maybe_unused]] explicit CommandExecutorImpl(const R2000 &device) : device(device) {}

            template<typename ... Args>
            inline bool execute(Args &&... args) {
                ParametersList parameters{};
                ParametersChaining::template chain(parameters, std::forward<Args>(args)...);
                return resetParameters(parameters);
            }

        private:
            inline bool resetParameters(const Device::ParametersList &list) noexcept(false) {
                std::string parametersList{};
                for (const auto &names : list)
                    parametersList += (names + ";");
                parametersList.pop_back();
                return device.sendHttpCommand(COMMAND_RESET_PARAMETERS,
                                              PARAMETER_NAME_LIST,
                                              parametersList).first;
            }

        private:
            const R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::FactoryResetDevice> {
            [[maybe_unused]] explicit CommandExecutorImpl(const R2000 &device) : device(device) {}

            inline bool execute() {
                return device.sendHttpCommand(COMMAND_FACTORY_RESET).first;
            }

        private:
            const R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::RebootDevice> {
            [[maybe_unused]] explicit CommandExecutorImpl(const R2000 &device) : device(device) {}

            bool execute() {
                return device.sendHttpCommand(COMMAND_REBOOT_DEVICE).first;
            }

        private:
            const R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::RequestTcpHandle> {
            [[maybe_unused]] explicit CommandExecutorImpl(const R2000 &device) : device(device) {}

            std::optional<std::pair<int, Device::HandleType>> execute(const ParametersMap &parameters) {
                const auto result{device.sendHttpCommand(COMMAND_REQUEST_TCP_HANDLE, parameters)};
                const auto status{result.first};
                const auto &propertyTree{result.second};
                if (!status)
                    return std::nullopt;
                const auto port{propertyTree.get_optional<int>("port")};
                const auto handle{propertyTree.get_optional<HandleType>("handle")};
                if (!port || !handle)
                    return std::nullopt;
                return {std::make_pair(*port, *handle)};
            }

        private:
            const R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::RequestUdpHandle> {
            [[maybe_unused]] explicit CommandExecutorImpl(const R2000 &device) : device(device) {}

            std::optional<Device::HandleType> execute(const ParametersMap &parameters) {
                const auto result{device.sendHttpCommand(COMMAND_REQUEST_UDP_HANDLE, parameters)};
                const auto status{result.first};
                const auto &propertyTree{result.second};
                if (!status)
                    return std::nullopt;
                const auto handle{propertyTree.get_optional<std::string>("handle")};
                if (!handle)
                    return std::nullopt;
                return {*handle};
            }

        private:
            const R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::GetScanOutputConfig> {
            [[maybe_unused]] explicit CommandExecutorImpl(const R2000 &device) : device(device) {}

            template<typename ... Args>
            std::vector<std::optional<ParametersMap>> execute(Args &&...  args) {
                std::vector<std::optional<ParametersMap>> configurations;
                RecursiveExecutorHelper::resolve([this, &configurations](const auto &handle) {
                    configurations.template emplace_back(getScanOutputConfig(handle));
                }, std::forward<Args>(args)...);
                return configurations;
            }

        private:
            std::optional<ParametersMap> getScanOutputConfig(const DeviceHandle &handle) {
                ParametersMap scanOutputConfig{};
                const auto result{device.sendHttpCommand(COMMAND_GET_SCAN_OUTPUT_CONFIG, PARAMETER_NAME_HANDLE,
                                                         handle.value)};
                const auto status{result.first};
                const auto &propertyTree{result.second};
                if (!status)
                    return std::nullopt;
                for (const auto &name : propertyTree) {
                    const auto parameterName{name.second.get<std::string>("")};
                    auto value{propertyTree.get_optional<std::string>(parameterName)};
                    if (!value)
                        return std::nullopt;
                    scanOutputConfig.insert(std::make_pair(parameterName, *value));
                }
                return {scanOutputConfig};
            }

        private:
            const R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::SetScanOutputConfig> {
            [[maybe_unused]] explicit CommandExecutorImpl(const R2000 &device) : device(device) {}

            template<typename ... Args>
            inline bool execute(Args &&... args) {
                return RecursiveExecutorHelper::template resolve([this](const auto &builder) {
                    const auto parameters{builder.build()};
                    return device.sendHttpCommand(COMMAND_SET_SCAN_OUTPUT_CONFIG, parameters).first;
                }, std::forward<Args>(args)...);
            }

        private:
            const R2000 &device;
        };
    }

    struct Commands {
        using GetProtocolInfoCommand = internals::Parameters::CommandExecutorImpl<
                internals::Parameters::CommandTags::GetProtocolInfo>;
        using ReleaseHandleCommand = internals::Parameters::CommandExecutorImpl<
                internals::Parameters::CommandTags::ReleaseHandle>;
        using StartScanCommand = internals::Parameters::CommandExecutorImpl<
                internals::Parameters::CommandTags::StartScanOutput>;
        using StopScanCommand = internals::Parameters::CommandExecutorImpl<
                internals::Parameters::CommandTags::StopScanOutput>;
        using FeedWatchdogCommand = internals::Parameters::CommandExecutorImpl<
                internals::Parameters::CommandTags::FeedWatchdog>;
        using GetParametersCommand = internals::Parameters::CommandExecutorImpl<
                internals::Parameters::CommandTags::GetParameters>;
        using FetchParametersCommand = internals::Parameters::CommandExecutorImpl<
                internals::Parameters::CommandTags::FetchParameterList>;
        using SetParametersCommand = internals::Parameters::CommandExecutorImpl<
                internals::Parameters::CommandTags::SetParameters>;
        using FactoryResetParametersCommand = internals::Parameters::CommandExecutorImpl<
                internals::Parameters::CommandTags::FactoryResetParameters>;
        using FactoryResetDeviceCommand = internals::Parameters::CommandExecutorImpl<
                internals::Parameters::CommandTags::FactoryResetDevice>;
        using RebootDeviceCommand = internals::Parameters::CommandExecutorImpl<
                internals::Parameters::CommandTags::RebootDevice>;
        using RequestUdpHandleCommand = internals::Parameters::CommandExecutorImpl<
                internals::Parameters::CommandTags::RequestUdpHandle>;
        using RequestTcpHandleCommand = internals::Parameters::CommandExecutorImpl<
                internals::Parameters::CommandTags::RequestTcpHandle>;
        using GetScanOutputConfigCommand = internals::Parameters::CommandExecutorImpl<
                internals::Parameters::CommandTags::GetScanOutputConfig>;
        using SetScanOutputConfigCommand = internals::Parameters::CommandExecutorImpl<
                internals::Parameters::CommandTags::SetScanOutputConfig>;
    };

} // namespace Device