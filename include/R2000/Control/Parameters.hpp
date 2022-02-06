//
// Created by chazz on 12/01/2022.
//
#pragma once

#include <boost/assign/list_of.hpp>
#include <boost/unordered_map.hpp>
#include <chrono>
#include <regex>

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

namespace Device::Parameters {
    using ParametersList = std::vector<std::string>;
    using ParametersMap = std::unordered_map<std::string, std::string>;
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

    struct ReadOnly {
        struct BasicInformation;
        struct Capabilities;
        struct Ethernet;
        struct Measuring;
        struct HmiDisplay;
        struct SystemStatus;
        struct LensContaminationMonitor;
    };
    struct ReadWrite {
        struct BasicInformation;
        struct Ethernet;
        struct Measuring;
        struct HmiDisplay;
        struct HandleUdp;
        struct HandleTcp;
        struct LensContaminationMonitor;
    };

    class ReadOnlyRequestBuilder {
    public:
        ADD_BUILD_BUILDER_METHOD(build, ParametersList, parameters);
    protected:
        ParametersList parameters{};
    };

    class ReadWriteRequestBuilder {
    public:
        ADD_BUILD_BUILDER_METHOD(build, ParametersMap, parameters);
    protected:
        ParametersMap parameters{};
    };

    class HandleParameters : public ReadWriteRequestBuilder {
    protected:
        const boost::unordered_map<PACKET_TYPE, std::string> scanPacketString =
                boost::assign::map_list_of(PACKET_TYPE::A, "A")(PACKET_TYPE::B, "B")(PACKET_TYPE::C, "C");
        const boost::unordered_map<CRC, std::string> crcToString =
                boost::assign::map_list_of(CRC::NONE, "none")(CRC::CRC32C, "crc32c");
    };

    namespace internals {

        template<typename Iterator>
        [[nodiscard]] inline std::string encoreSpaceInUrl(Iterator begin, Iterator end) {
            std::regex space("[[:space:]]");
            std::string output{};
            std::regex_replace(std::back_inserter(output), begin, end, space, "%20");
            return output;
        }

        template<typename Section>
        class ParametersRequestBuilderImpl {
        };

        template<>
        class ParametersRequestBuilderImpl<ReadWrite::BasicInformation> : public ReadWriteRequestBuilder {
        public:
            ADD_RW_PARAMETER_BUILDER_METHOD(withUserTag, ParametersRequestBuilderImpl &, const std::string&, userTag,
                                            PARAMETER_USER_TAG);

            ADD_RW_PARAMETER_BUILDER_METHOD(withUserNotes, ParametersRequestBuilderImpl &, const std::string&,
                                            userNotes,
                                            PARAMETER_USER_NOTES);
        };

        template<>
        class ParametersRequestBuilderImpl<ReadWrite::Ethernet> : public ReadWriteRequestBuilder {
        public:
            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withIpMode, ParametersRequestBuilderImpl &, IpMode, mode,
                                                           IpModeToString.at(mode), PARAMETER_IP_ADDRESS);

            ADD_RW_PARAMETER_BUILDER_METHOD(withIpAddress, ParametersRequestBuilderImpl &, const std::string&,
                                            ipAddress,
                                            PARAMETER_IP_ADDRESS);

            ADD_RW_PARAMETER_BUILDER_METHOD(withSubnetMask, ParametersRequestBuilderImpl &, const std::string&,
                                            subnetMask,
                                            PARAMETER_SUBNET_MASK);

            ADD_RW_PARAMETER_BUILDER_METHOD(withGateway, ParametersRequestBuilderImpl &, const std::string&, gateway,
                                            PARAMETER_GATEWAY);

        private:
            const boost::unordered_map<IpMode, std::string> IpModeToString =
                    boost::assign::map_list_of(IpMode::STATIC, "static")(IpMode::DHCP, "dhcp")(IpMode::AUTO_IP,
                                                                                               "autoip");
        };

        template<>
        class ParametersRequestBuilderImpl<ReadWrite::Measuring> : public ReadWriteRequestBuilder {
        public:
            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withOperatingMode, ParametersRequestBuilderImpl &,
                                                           OPERATING_MODE,
                                                           operatingMode, operatingModeToString.at(operatingMode),
                                                           PARAMETER_OPERATING_MODE);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withScanFrequency, ParametersRequestBuilderImpl &,
                                                           const unsigned int,
                                                           scanFrequency, std::to_string(scanFrequency),
                                                           PARAMETER_SCAN_FREQUENCY);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withSamplesPerScan, ParametersRequestBuilderImpl &,
                                                           const unsigned int,
                                                           samplesPerScan, std::to_string(samplesPerScan),
                                                           PARAMETER_SAMPLES_PER_SCAN);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withScanDirection, ParametersRequestBuilderImpl &,
                                                           SCAN_DIRECTION,
                                                           scanDirection, scanDirectionToString.at(scanDirection),
                                                           PARAMETER_SCAN_DIRECTION);

        private:
            const boost::unordered_map<OPERATING_MODE, std::string> operatingModeToString =
                    boost::assign::map_list_of(OPERATING_MODE::MEASURE, "measure")(OPERATING_MODE::EMITTER_OFF,
                                                                                   "emitter_off");
            const boost::unordered_map<SCAN_DIRECTION, std::string> scanDirectionToString =
                    boost::assign::map_list_of(SCAN_DIRECTION::CW, "cw")(SCAN_DIRECTION::CCW, "ccw");
        };

        template<>
        class ParametersRequestBuilderImpl<ReadWrite::HmiDisplay> : public ReadWriteRequestBuilder {
        public:
            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withHmiDisplayMode, ParametersRequestBuilderImpl &,
                                                           HMI_DISPLAY_MODE,
                                                           displayMode, HmiDisplayModeToString.at(displayMode),
                                                           PARAMETER_HMI_DISPLAY_MODE);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withHmiLanguage, ParametersRequestBuilderImpl &, Language,
                                                           language,
                                                           LanguageToString.at(language), PARAMETER_HMI_LANGUAGE);

            ADD_RW_PARAMETER_BUILDER_METHOD_ON(lockHmiButton, ParametersRequestBuilderImpl &,
                                               PARAMETER_HMI_BUTTON_LOCK);

            ADD_RW_PARAMETER_BUILDER_METHOD_OFF(unlockHmiButton, ParametersRequestBuilderImpl &,
                                                PARAMETER_HMI_BUTTON_LOCK);

            ADD_RW_PARAMETER_BUILDER_METHOD_ON(lockHmiParameters, ParametersRequestBuilderImpl &,
                                               PARAMETER_HMI_PARAMETER_LOCK);

            ADD_RW_PARAMETER_BUILDER_METHOD_OFF(unlockHmiParameters, ParametersRequestBuilderImpl &,
                                                PARAMETER_HMI_PARAMETER_LOCK);

            ADD_RW_PARAMETER_BUILDER_METHOD_ON(enableLEDLocatorIndication, ParametersRequestBuilderImpl &,
                                               PARAMETER_LOCATOR_INDICATION);

            ADD_RW_PARAMETER_BUILDER_METHOD_OFF(disableLEDLocatorIndication, ParametersRequestBuilderImpl &,
                                                PARAMETER_LOCATOR_INDICATION);

            ADD_RW_PARAMETER_BUILDER_METHOD(withHmiStaticLogo, ParametersRequestBuilderImpl &, const std::string&,
                                            base64Logo,
                                            PARAMETER_HMI_STATIC_LOGO);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withHmiStaticText1, ParametersRequestBuilderImpl &,
                                                           const std::string&,
                                                           line, encoreSpaceInUrl(std::cbegin(line), std::cend(line)),
                                                           PARAMETER_HMI_STATIC_TEXT_1);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withHmiStaticText2, ParametersRequestBuilderImpl &,
                                                           const std::string&,
                                                           line, encoreSpaceInUrl(std::cbegin(line), std::cend(line)),
                                                           PARAMETER_HMI_STATIC_TEXT_2);

            ADD_RW_PARAMETER_BUILDER_METHOD(withHmiApplicationBitmap, ParametersRequestBuilderImpl &,
                                            const std::string&,
                                            base64Bitmap,
                                            PARAMETER_HMI_APPLICATION_BITMAP);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withHmiApplicationText1, ParametersRequestBuilderImpl &,
                                                           const std::string&,
                                                           line, encoreSpaceInUrl(std::cbegin(line), std::cend(line)),
                                                           PARAMETER_HMI_APPLICATION_TEXT_1);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withHmiApplicationText2, ParametersRequestBuilderImpl &,
                                                           const std::string&,
                                                           line, encoreSpaceInUrl(std::cbegin(line), std::cend(line)),
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

        template<>
        class ParametersRequestBuilderImpl<ReadWrite::HandleTcp> : public HandleParameters {
        public:
            ParametersRequestBuilderImpl() { (void) withoutWatchdog(); }

            ADD_RW_PARAMETER_BUILDER_METHOD_ON(withWatchdog, ParametersRequestBuilderImpl &, PARAMETER_HANDLE_WATCHDOG);

            ADD_RW_PARAMETER_BUILDER_METHOD_OFF(withoutWatchdog, ParametersRequestBuilderImpl &,
                                                PARAMETER_HANDLE_WATCHDOG);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withWatchdogTimeout, ParametersRequestBuilderImpl &,
                                                           const unsigned int,
                                                           watchdogtimeout, std::to_string(watchdogtimeout),
                                                           PARAMETER_HANDLE_WATCHDOG_TIMEOUT);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withPacketType, ParametersRequestBuilderImpl &, PACKET_TYPE,
                                                           packetType,
                                                           scanPacketString.at(packetType),
                                                           PARAMETER_HANDLE_PACKET_TYPE);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withPacketCrc, ParametersRequestBuilderImpl &, CRC, crcType,
                                                           crcToString.at(crcType), PARAMETER_HANDLE_PACKET_CRC);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withStartAngle, ParametersRequestBuilderImpl &, const int,
                                                           startAngle,
                                                           std::to_string(startAngle), PARAMETER_HANDLE_START_ANGLE);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withNumberOfPointsPerScan, ParametersRequestBuilderImpl &,
                                                           const unsigned int, numberOfPointsPerScan,
                                                           std::to_string(numberOfPointsPerScan),
                                                           PARAMETER_HANDLE_MAX_NUM_POINTS_SCAN);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withNumberOfScanToSkip, ParametersRequestBuilderImpl &,
                                                           const unsigned int,
                                                           numberOfScanToSkip, std::to_string(numberOfScanToSkip),
                                                           PARAMETER_HANDLE_SKIP_SCANS);
        };

        template<>
        class ParametersRequestBuilderImpl<ReadWrite::HandleUdp> : public HandleParameters {
        public:
            ParametersRequestBuilderImpl() { (void) withoutWatchdog(); }

            ADD_RW_PARAMETER_BUILDER_METHOD(withHostname, ParametersRequestBuilderImpl &, const std::string&, hostname,
                                            PARAMETER_HANDLE_ADDRESS);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withPort, ParametersRequestBuilderImpl &, const uint16_t,
                                                           port,
                                                           std::to_string(port), PARAMETER_HANDLE_PORT);

            ADD_RW_PARAMETER_BUILDER_METHOD_ON(withWatchdog, ParametersRequestBuilderImpl &, PARAMETER_HANDLE_WATCHDOG);

            ADD_RW_PARAMETER_BUILDER_METHOD_OFF(withoutWatchdog, ParametersRequestBuilderImpl &,
                                                PARAMETER_HANDLE_WATCHDOG);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withWatchdogTimeout, ParametersRequestBuilderImpl &,
                                                           const unsigned int,
                                                           watchdogtimeout, std::to_string(watchdogtimeout),
                                                           PARAMETER_HANDLE_WATCHDOG_TIMEOUT);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withPacketType, ParametersRequestBuilderImpl &, PACKET_TYPE,
                                                           packetType,
                                                           scanPacketString.at(packetType),
                                                           PARAMETER_HANDLE_PACKET_TYPE);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withPacketCrc, ParametersRequestBuilderImpl &, CRC, crcType,
                                                           crcToString.at(crcType), PARAMETER_HANDLE_PACKET_CRC);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withStartAngle, ParametersRequestBuilderImpl &, const int,
                                                           startAngle,
                                                           std::to_string(startAngle), PARAMETER_HANDLE_START_ANGLE);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withNumberOfPointsPerScan, ParametersRequestBuilderImpl &,
                                                           const unsigned int, numberOfPointsPerScan,
                                                           std::to_string(numberOfPointsPerScan),
                                                           PARAMETER_HANDLE_MAX_NUM_POINTS_SCAN);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withNumberOfScanToSkip, ParametersRequestBuilderImpl &,
                                                           const unsigned int,
                                                           numberOfScanToSkip, std::to_string(numberOfScanToSkip),
                                                           PARAMETER_HANDLE_SKIP_SCANS);
        };

        template<>
        class ParametersRequestBuilderImpl<ReadWrite::LensContaminationMonitor> : public ReadWriteRequestBuilder {
        public:
            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withSensitivity, ParametersRequestBuilderImpl &,
                                                           LCM_SENSITIVITY,
                                                           sensitivity, LCMSensitivityToString.at(sensitivity),
                                                           PARAMETER_LCM_DETECTION_SENSITIVITY);

            ADD_RW_PARAMETER_BUILDER_METHOD_WITH_TRANSFORM(withDetectionPeriod, ParametersRequestBuilderImpl &,
                                                           std::chrono::milliseconds, period,
                                                           std::to_string(period.count()),
                                                           PARAMETER_LCM_DETECTION_PERIOD);

            ADD_RW_PARAMETER_BUILDER_METHOD_ON(withLcmEnabled, ParametersRequestBuilderImpl &,
                                               PARAMETER_LCM_SECTOR_ENABLE);

            ADD_RW_PARAMETER_BUILDER_METHOD_OFF(withLcmDisabled, ParametersRequestBuilderImpl &,
                                                PARAMETER_LCM_SECTOR_ENABLE);

        private:
            const boost::unordered_map<LCM_SENSITIVITY, std::string> LCMSensitivityToString =
                    boost::assign::map_list_of(LCM_SENSITIVITY::DISABLED, "disabled")(LCM_SENSITIVITY::LOW, "low")(
                            LCM_SENSITIVITY::MEDIUM, "medium")(LCM_SENSITIVITY::HIGH, "high");
        };

        template<>
        class ParametersRequestBuilderImpl<ReadOnly::BasicInformation> : public ReadOnlyRequestBuilder {
        public:
            ADD_RO_PARAMETER_BUILDER_METHOD(requestDeviceFamily, ParametersRequestBuilderImpl &,
                                            PARAMETER_DEVICE_FAMILY);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestVendor, ParametersRequestBuilderImpl &, PARAMETER_VENDOR);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestProduct, ParametersRequestBuilderImpl &, PARAMETER_PRODUCT);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestPart, ParametersRequestBuilderImpl &, PARAMETER_PART);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestSerial, ParametersRequestBuilderImpl &, PARAMETER_SERIAL);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestFirmwareRevision, ParametersRequestBuilderImpl &,
                                            PARAMETER_REVISION_FW);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestHardwareRevision, ParametersRequestBuilderImpl &,
                                            PARAMETER_REVISION_HW);

            ADD_RO_PARAMETER_BUILDER_METHOD(withUserTag, ParametersRequestBuilderImpl &, PARAMETER_USER_TAG);

            ADD_RO_PARAMETER_BUILDER_METHOD(withUserNotes, ParametersRequestBuilderImpl &, PARAMETER_USER_NOTES);
        };

        template<>
        class ParametersRequestBuilderImpl<ReadOnly::Capabilities> : public ReadOnlyRequestBuilder {
        public:
            ADD_RO_PARAMETER_BUILDER_METHOD(requestFeatureFlags, ParametersRequestBuilderImpl &,
                                            PARAMETER_FEATURE_FLAGS);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestEmitterType, ParametersRequestBuilderImpl &, PARAMETER_EMITTER_TYPE);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestRadialMinRange, ParametersRequestBuilderImpl &,
                                            PARAMETER_RADIAL_RANGE_MIN);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestRadialMaxRange, ParametersRequestBuilderImpl &,
                                            PARAMETER_RADIAL_RANGE_MAX);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestRadialResolution, ParametersRequestBuilderImpl &,
                                            PARAMETER_RADIAL_RESOLUTION);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestAngularFov, ParametersRequestBuilderImpl &, PARAMETER_ANGULAR_FOV);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestAngularResolution, ParametersRequestBuilderImpl &,
                                            PARAMETER_ANGULAR_RESOLUTION);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestMinimalScanFrequency, ParametersRequestBuilderImpl &,
                                            PARAMETER_SCAN_FREQUENCY_MIN);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestMaximalScanFrequency, ParametersRequestBuilderImpl &,
                                            PARAMETER_SCAN_FREQUENCY_MAX);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestMinimalSamplingRate, ParametersRequestBuilderImpl &,
                                            PARAMETER_SAMPLING_RATE_MIN);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestMaximalSamplingRate, ParametersRequestBuilderImpl &,
                                            PARAMETER_SAMPLING_RATE_MAX);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestMaximalConnections, ParametersRequestBuilderImpl &,
                                            PARAMETER_MAX_CONNECTIONS);
        };

        template<>
        class ParametersRequestBuilderImpl<ReadOnly::Ethernet> : public ReadOnlyRequestBuilder {
        public:
            ADD_RO_PARAMETER_BUILDER_METHOD(requestCurrentIpMode, ParametersRequestBuilderImpl &,
                                            PARAMETER_IP_MODE_CURRENT);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestCurrentIpAddress, ParametersRequestBuilderImpl &,
                                            PARAMETER_IP_ADDRESS_CURRENT);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestCurrentSubnetMask, ParametersRequestBuilderImpl &,
                                            PARAMETER_SUBNET_MASK_CURRENT);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestCurrentGateway, ParametersRequestBuilderImpl &,
                                            PARAMETER_GATEWAY_CURRENT);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestMacAddress, ParametersRequestBuilderImpl &, PARAMETER_MAC_ADDRESS);

            ADD_RO_PARAMETER_BUILDER_METHOD(withIpMode, ParametersRequestBuilderImpl &, PARAMETER_IP_MODE);

            ADD_RO_PARAMETER_BUILDER_METHOD(withIpAddress, ParametersRequestBuilderImpl &, PARAMETER_IP_ADDRESS);

            ADD_RO_PARAMETER_BUILDER_METHOD(withSubnetMask, ParametersRequestBuilderImpl &, PARAMETER_SUBNET_MASK);

            ADD_RO_PARAMETER_BUILDER_METHOD(withGateway, ParametersRequestBuilderImpl &, PARAMETER_GATEWAY);
        };

        template<>
        class ParametersRequestBuilderImpl<ReadOnly::Measuring> : public ReadOnlyRequestBuilder {
        public:
            ADD_RO_PARAMETER_BUILDER_METHOD(requestMeasuredFrequency, ParametersRequestBuilderImpl &,
                                            PARAMETER_SCAN_FREQUENCY_MEASURED);

            ADD_RO_PARAMETER_BUILDER_METHOD(withOperatingMode, ParametersRequestBuilderImpl &,
                                            PARAMETER_OPERATING_MODE);

            ADD_RO_PARAMETER_BUILDER_METHOD(withScanFrequency, ParametersRequestBuilderImpl &,
                                            PARAMETER_SCAN_FREQUENCY);

            ADD_RO_PARAMETER_BUILDER_METHOD(withScanDirection, ParametersRequestBuilderImpl &,
                                            PARAMETER_SCAN_DIRECTION);

            ADD_RO_PARAMETER_BUILDER_METHOD(withSamplesPerScan, ParametersRequestBuilderImpl &,
                                            PARAMETER_SAMPLES_PER_SCAN);
        };

        template<>
        class ParametersRequestBuilderImpl<ReadOnly::HmiDisplay> : public ReadOnlyRequestBuilder {
        public:
            ADD_RO_PARAMETER_BUILDER_METHOD(withHmiDisplayMode, ParametersRequestBuilderImpl &,
                                            PARAMETER_HMI_DISPLAY_MODE);

            ADD_RO_PARAMETER_BUILDER_METHOD(withHmiLanguage, ParametersRequestBuilderImpl &, PARAMETER_HMI_LANGUAGE);

            ADD_RO_PARAMETER_BUILDER_METHOD(withLockHmiButton, ParametersRequestBuilderImpl &,
                                            PARAMETER_HMI_BUTTON_LOCK);

            ADD_RO_PARAMETER_BUILDER_METHOD(withLockHmiParameters, ParametersRequestBuilderImpl &,
                                            PARAMETER_HMI_PARAMETER_LOCK);

            ADD_RO_PARAMETER_BUILDER_METHOD(withLEDLocatorIndication, ParametersRequestBuilderImpl &,
                                            PARAMETER_LOCATOR_INDICATION);

            ADD_RO_PARAMETER_BUILDER_METHOD(withHmiStaticLogo, ParametersRequestBuilderImpl &,
                                            PARAMETER_HMI_STATIC_LOGO);

            ADD_RO_PARAMETER_BUILDER_METHOD(withHmiStaticText1, ParametersRequestBuilderImpl &,
                                            PARAMETER_HMI_STATIC_TEXT_1);

            ADD_RO_PARAMETER_BUILDER_METHOD(withHmiStaticText2, ParametersRequestBuilderImpl &,
                                            PARAMETER_HMI_STATIC_TEXT_2);

            ADD_RO_PARAMETER_BUILDER_METHOD(withHmiApplicationBitmap, ParametersRequestBuilderImpl &,
                                            PARAMETER_HMI_APPLICATION_BITMAP);

            ADD_RO_PARAMETER_BUILDER_METHOD(withHmiApplicationText1, ParametersRequestBuilderImpl &,
                                            PARAMETER_HMI_APPLICATION_TEXT_1);

            ADD_RO_PARAMETER_BUILDER_METHOD(withHmiApplicationText2, ParametersRequestBuilderImpl &,
                                            PARAMETER_HMI_APPLICATION_TEXT_2);
        };

        template<>
        class ParametersRequestBuilderImpl<ReadOnly::SystemStatus> : public ReadOnlyRequestBuilder {
        public:
            ADD_RO_PARAMETER_BUILDER_METHOD(requestStatusFlags, ParametersRequestBuilderImpl &, PARAMETER_STATUS_FLAGS);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestLoadIndication, ParametersRequestBuilderImpl &,
                                            PARAMETER_LOAD_INDICATION);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestSystemTimeRaw, ParametersRequestBuilderImpl &,
                                            PARAMETER_SYSTEM_TIME_RAW);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestUpTime, ParametersRequestBuilderImpl &, PARAMETER_UP_TIME);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestPowerCycles, ParametersRequestBuilderImpl &, PARAMETER_POWER_CYCLES);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestOperationTime, ParametersRequestBuilderImpl &,
                                            PARAMETER_OPERATION_TIME);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestOperationTimeScaled, ParametersRequestBuilderImpl &,
                                            PARAMETER_OPERATION_TIME_SCALED);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestCurrentTemperature, ParametersRequestBuilderImpl &,
                                            PARAMETER_TEMPERATURE_CURRENT);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestMinimalTemperature, ParametersRequestBuilderImpl &,
                                            PARAMETER_TEMPERATURE_MIN);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestMaximalTemperature, ParametersRequestBuilderImpl &,
                                            PARAMETER_TEMPERATURE_MAX);
        };

        template<>
        class ParametersRequestBuilderImpl<ReadOnly::LensContaminationMonitor> : public ReadOnlyRequestBuilder {
        public:
            ADD_RO_PARAMETER_BUILDER_METHOD(requestLcmSectorWarningFlag, ParametersRequestBuilderImpl &,
                                            PARAMETER_LCM_SECTOR_WARN_FLAGS);

            ADD_RO_PARAMETER_BUILDER_METHOD(requestLcmSectorErrorFlag, ParametersRequestBuilderImpl &,
                                            PARAMETER_LCM_SECTOR_ERROR_FLAGS);

            ADD_RO_PARAMETER_BUILDER_METHOD(withSensitivity, ParametersRequestBuilderImpl &,
                                            PARAMETER_LCM_DETECTION_SENSITIVITY);

            ADD_RO_PARAMETER_BUILDER_METHOD(withDetectionPeriod, ParametersRequestBuilderImpl &,
                                            PARAMETER_LCM_DETECTION_PERIOD);

            ADD_RO_PARAMETER_BUILDER_METHOD(withLcmState, ParametersRequestBuilderImpl &, PARAMETER_LCM_SECTOR_ENABLE);
        };

    } // namespace internals

    template<typename AccessType>
    class ParametersRequestBuilder;

    template<>
    class ParametersRequestBuilder<ReadOnly> {
    public:
        using BasicInformation [[maybe_unused]] = internals::ParametersRequestBuilderImpl<ReadOnly::BasicInformation>;
        using Capabilities [[maybe_unused]] = internals::ParametersRequestBuilderImpl<ReadOnly::Capabilities>;
        using Ethernet [[maybe_unused]] = internals::ParametersRequestBuilderImpl<ReadOnly::Ethernet>;
        using Measure [[maybe_unused]] = internals::ParametersRequestBuilderImpl<ReadOnly::Measuring>;
        using SystemStatus [[maybe_unused]] = internals::ParametersRequestBuilderImpl<ReadOnly::SystemStatus>;
    };

    template<>
    class ParametersRequestBuilder<ReadWrite> {
    public:
        using BasicInformation [[maybe_unused]] = internals::ParametersRequestBuilderImpl<ReadWrite::BasicInformation>;
        using Ethernet [[maybe_unused]] = internals::ParametersRequestBuilderImpl<ReadWrite::Ethernet>;
        using Measure [[maybe_unused]] = internals::ParametersRequestBuilderImpl<ReadWrite::Measuring>;
        using HmiDisplay [[maybe_unused]] = internals::ParametersRequestBuilderImpl<ReadWrite::HmiDisplay>;
        using UdpHandle [[maybe_unused]] = internals::ParametersRequestBuilderImpl<ReadWrite::HandleUdp>;
        using TcpHandle [[maybe_unused]] = internals::ParametersRequestBuilderImpl<ReadWrite::HandleTcp>;
    };

    template<typename AccessType> using Parameters [[maybe_unused]] = ParametersRequestBuilder<AccessType>;
    using ReadOnlyParameters [[maybe_unused]] = ParametersRequestBuilder<ReadOnly>;
    using ReadWriteParameters [[maybe_unused]] = ParametersRequestBuilder<ReadWrite>;
} // namespace Device::Parameters