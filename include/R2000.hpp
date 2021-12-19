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


#define ADD_RO_PARAMETER_BUILDER(F_NAME, RETURN_TYPE, PARAMETER_NAME) \
RETURN_TYPE & F_NAME () \
{                                                                     \
   parameters.push_back(PARAMETER_NAME); \
   return *this; \
}


#define COMMAND_RELEASE_HANDLE "release_handle"
#define COMMAND_START_SCAN_OUTPUT "start_scanoutput"
#define COMMAND_STOP_SCAN_OUTPUT "stop_scanoutput"
#define COMMAND_FEED_WATCHDOG "feed_watchdog"
#define COMMAND_REBOOT_DEVICE "reboot_device"
#define COMMAND_LIST_PARAMETERS "list_parameters"
#define COMMAND_GET_PARAMETER "get_parameter"
#define COMMAND_SET_PARAMETER "set_parameter"
#define COMMAND_RESET_PARAMETERS "reset_parameters"
#define COMMAND_FACTORY_RESET "factory_reset"
#define COMMAND_REQUEST_TCP_HANDLE "request_handle_tcp"
#define COMMAND_REQUEST_UDP_HANDLE "request_handle_udp"

#define PARAMETER_NAME_HANDLE "handle"
#define PARAMETER_NAME_LIST "parametersList"

#define PARAMETER_VALUE_STATUS_FLAG "status_flags"
#define PARAMETER_VALUE_APPLICATION_TEXT "application_text"

#define ERROR_CODE "error_code"
#define ERROR_TEXT "error_text"

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
#define PARAMETER_NAME_HMI_DISPLAY_MODE "hmi_display_mode"
#define PARAMETER_NAME_HMI_STATIC_LOGO "hmi_static_logo"
#define PARAMETER_NAME_HMI_STATIC_TEXT_1 "hmi_static_text_1"
#define PARAMETER_NAME_HMI_STATIC_TEXT_2 "hmi_static_text_2"
#define PARAMETER_NAME_HMI_APPLICATION_BITMAP "hmi_application_bitmap"
#define PARAMETER_NAME_HMI_APPLICATION_TEXT_1 "hmi_application_text_1"
#define PARAMETER_NAME_HMI_APPLICATION_TEXT_2 "hmi_application_text_2"
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

using namespace std::chrono_literals;

namespace Device {
    class R2000;
    constexpr unsigned int STATUS_FLAG_BIT_SIZE = sizeof(int) * CHAR_BIT;

    class DataLink;

    struct DeviceConfiguration {
        explicit DeviceConfiguration(boost::asio::ip::address deviceAddress, unsigned short port = 80)
                : deviceAddress(std::move(deviceAddress)), httpServicePort(port) {
        }

        const boost::asio::ip::address deviceAddress{};
        const unsigned short httpServicePort{};
    };

    enum class RebootStatus {
        COMPLETED,
        TIMEOUT,
        FAILED
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

    namespace internals::Parameters {

        struct RoParametersTags {
            struct BasicInformationTag;
            struct CapabilitiesTag;
            struct EthernetTag;
            struct MeasuringTag;
            struct SystemStatusTag;
        };
        struct RwParametersTags {
            struct BasicInformationTag;
            struct EthernetTag;
            struct MeasuringTag;
            struct HmiDisplayTag;
        };

        template<typename Section>
        struct ParametersBuilderImpl {};

        template<>
        struct ParametersBuilderImpl<RwParametersTags::BasicInformationTag> {
            ParametersBuilderImpl &withUserTag(const std::string &userTag) {
                parameters[PARAMETER_USER_TAG] = userTag;
                return *this;
            }

            ParametersBuilderImpl &withUserNotes(const std::string &userNotes) {
                parameters[PARAMETER_USER_NOTES] = userNotes;
                return *this;
            }

            [[nodiscard]] ParametersMap build() const {
                return parameters;
            }

            struct AsList
            {
                AsList &withUserTag() {
                    parametersList.push_back(PARAMETER_USER_TAG);
                    return *this;
                }

                AsList &withUserNotes() {
                    parametersList.push_back(PARAMETER_USER_NOTES);
                    return *this;
                }

                [[nodiscard]] ParametersList build() const {
                    return parametersList;
                }
            private:
                ParametersList parametersList;
            };

        private:
            ParametersMap parameters;
        };

        template<>
        struct ParametersBuilderImpl<RwParametersTags::EthernetTag> {
            ParametersBuilderImpl &withIpMode(IpMode mode) {
                parameters[PARAMETER_IP_MODE] = IpModeToString.at(mode);
                return *this;
            }

            ParametersBuilderImpl &withIpAddress(const std::string &ipAddress) {
                parameters[PARAMETER_IP_ADDRESS] = ipAddress;
                return *this;
            }

            ParametersBuilderImpl &withSubnetMask(const std::string &subnetMask) {
                parameters[PARAMETER_SUBNET_MASK] = subnetMask;
                return *this;
            }

            ParametersBuilderImpl &withGateway(const std::string &gateway) {
                parameters[PARAMETER_GATEWAY] = gateway;
                return *this;
            }

            struct AsList
            {
                AsList &withIpMode() {
                    parametersList.push_back(PARAMETER_IP_MODE);
                    return *this;
                }

                AsList &withIpAddress() {
                    parametersList.push_back(PARAMETER_IP_ADDRESS);
                    return *this;
                }

                AsList &withSubnetMask() {
                    parametersList.push_back(PARAMETER_SUBNET_MASK);
                    return *this;
                }

                AsList &withGateway() {
                    parametersList.push_back(PARAMETER_GATEWAY);
                    return *this;
                }

                [[nodiscard]] ParametersList build() const {
                    return parametersList;
                }
            private:
                ParametersList parametersList;
            };

            [[nodiscard]] ParametersMap build() const {
                return parameters;
            }

        private:
            ParametersMap parameters;
            const boost::unordered_map<IpMode, const char *> IpModeToString = boost::assign::map_list_of
                    (IpMode::STATIC, "static")
                    (IpMode::DHCP, "dhcp")
                    (IpMode::AUTO_IP, "autoip");
        };

        template<>
        struct ParametersBuilderImpl<RwParametersTags::MeasuringTag> {
            ParametersBuilderImpl &withOperatingMode(OPERATING_MODE operatingMode) {
                parameters[PARAMETER_OPERATING_MODE] = operatingModeToString.at(operatingMode);
                return *this;
            }

            ParametersBuilderImpl &withScanFrequency(const std::string &scanFrequency) {
                parameters[PARAMETER_SCAN_FREQUENCY] = scanFrequency;
                return *this;
            }

            ParametersBuilderImpl &withScanDirection(SCAN_DIRECTION scanDirection) {
                parameters[PARAMETER_SCAN_DIRECTION] = scanDirectionToString.at(scanDirection);
                return *this;
            }

            ParametersBuilderImpl &withSamplesPerScan(const std::string &samplesPerScan) {
                parameters[PARAMETER_SAMPLES_PER_SCAN] = samplesPerScan;
                return *this;
            }

            [[nodiscard]] ParametersMap build() const {
                return parameters;
            }

            struct AsList
            {
                AsList &withOperatingMode() {
                    parametersList.push_back(PARAMETER_OPERATING_MODE);
                    return *this;
                }

                AsList &withScanFrequency() {
                    parametersList.push_back(PARAMETER_SCAN_FREQUENCY);
                    return *this;
                }

                AsList &withScanDirection() {
                    parametersList.push_back(PARAMETER_SCAN_DIRECTION);
                    return *this;
                }

                AsList &withSamplesPerScan() {
                    parametersList.push_back(PARAMETER_SAMPLES_PER_SCAN);
                    return *this;
                }

                [[nodiscard]] ParametersList build() const {
                    return parametersList;
                }
            private:
                ParametersList parametersList;
            };

        private:
            ParametersMap parameters;
            const boost::unordered_map<OPERATING_MODE, const char *> operatingModeToString = boost::assign::map_list_of
                    (OPERATING_MODE::MEASURE, "measure")
                    (OPERATING_MODE::EMITTER_OFF, "emitter_off");
            const boost::unordered_map<SCAN_DIRECTION, const char *> scanDirectionToString = boost::assign::map_list_of
                    (SCAN_DIRECTION::CW, "cw")
                    (SCAN_DIRECTION::CCW, "ccw");
        };

        template<>
        struct ParametersBuilderImpl<RwParametersTags::HmiDisplayTag> {

            ParametersBuilderImpl &withHmiDisplayMode(HMI_DISPLAY_MODE displayMode) {
                parameters[PARAMETER_HMI_DISPLAY_MODE] = HmiDisplayModeToString.at(displayMode);
                return *this;
            }

            ParametersBuilderImpl &withHmiLanguage(Language language) {
                parameters[PARAMETER_HMI_LANGUAGE] = LanguageToString.at(language);
                return *this;
            }

            ParametersBuilderImpl &lockHmiButton() {
                parameters[PARAMETER_HMI_BUTTON_LOCK] = "on";
                return *this;
            }

            ParametersBuilderImpl &unlockHmiButton() {
                parameters[PARAMETER_HMI_BUTTON_LOCK] = "off";
                return *this;
            }

            ParametersBuilderImpl &lockHmiParameters() {
                parameters[PARAMETER_HMI_PARAMETER_LOCK] = "on";
                return *this;
            }

            ParametersBuilderImpl &unlockHmiParameters() {
                parameters[PARAMETER_HMI_PARAMETER_LOCK] = "off";
                return *this;
            }

            ParametersBuilderImpl &enableLEDLocatorIndication() {
                parameters[PARAMETER_LOCATOR_INDICATION] = "on";
                return *this;
            }

            ParametersBuilderImpl &disableLEDLocatorIndication() {
                parameters[PARAMETER_LOCATOR_INDICATION] = "off";
                return *this;
            }

            ParametersBuilderImpl &withHmiStaticLogo(const std::string &base64Logo) {
                withHmiDisplayMode(HMI_DISPLAY_MODE::STATIC_LOGO);
                parameters[PARAMETER_NAME_HMI_STATIC_LOGO] = base64Logo;
                return *this;
            }

            ParametersBuilderImpl &withHmiStaticText(const std::string &line1, const std::string &line2) {
                withHmiDisplayMode(HMI_DISPLAY_MODE::STATIC_TEXT);
                parameters[PARAMETER_NAME_HMI_STATIC_TEXT_1] = line1;
                parameters[PARAMETER_NAME_HMI_STATIC_TEXT_2] = line2;
                return *this;
            }

            ParametersBuilderImpl &withHmiApplicationBitmap(const std::string &base64Bitmap) {
                withHmiDisplayMode(HMI_DISPLAY_MODE::APPLICATION_BITMAP);
                parameters[PARAMETER_NAME_HMI_APPLICATION_BITMAP] = base64Bitmap;
                return *this;
            }

            ParametersBuilderImpl &withHmiApplicationText(const std::string &line1, const std::string &line2) {
                withHmiDisplayMode(HMI_DISPLAY_MODE::APPLICATION_TEXT);
                parameters[PARAMETER_NAME_HMI_APPLICATION_TEXT_1] = line1;
                parameters[PARAMETER_NAME_HMI_APPLICATION_TEXT_2] = line2;
                return *this;
            }

            [[nodiscard]] ParametersMap build() const {
                return parameters;
            }

            struct AsList
            {
                AsList &withHmiDisplayMode() {
                    parametersList.push_back(PARAMETER_HMI_DISPLAY_MODE);
                    return *this;
                }

                AsList &withHmiLanguage() {
                    parametersList.push_back(PARAMETER_HMI_LANGUAGE);
                    return *this;
                }

                AsList &withLockHmiButton() {
                    parametersList.push_back(PARAMETER_HMI_BUTTON_LOCK);
                    return *this;
                }

                AsList &withLockHmiParameters() {
                    parametersList.push_back(PARAMETER_HMI_PARAMETER_LOCK);
                    return *this;
                }

                AsList &withLEDLocatorIndication() {
                    parametersList.push_back(PARAMETER_LOCATOR_INDICATION);
                    return *this;
                }

                AsList &withHmiStaticLogo() {
                    parametersList.push_back(PARAMETER_NAME_HMI_STATIC_LOGO);
                    return *this;
                }

                AsList &withHmiStaticText() {
                    parametersList.push_back(PARAMETER_NAME_HMI_STATIC_TEXT_1);
                    parametersList.push_back(PARAMETER_NAME_HMI_STATIC_TEXT_2);
                    return *this;
                }

                AsList &withHmiApplicationBitmap() {
                    parametersList.push_back(PARAMETER_NAME_HMI_APPLICATION_BITMAP);
                    return *this;
                }

                AsList &withHmiApplicationText() {
                    parametersList.push_back(PARAMETER_NAME_HMI_APPLICATION_TEXT_1);
                    parametersList.push_back(PARAMETER_NAME_HMI_APPLICATION_TEXT_2);
                    return *this;
                }

                [[nodiscard]] ParametersList build() const {
                    return parametersList;
                }
            private:
                ParametersList parametersList;
            };

        private:
            ParametersMap parameters;
            const boost::unordered_map<Language, const char *> LanguageToString = boost::assign::map_list_of
                    (Language::ENGLISH, "english")
                    (Language::GERMAN, "german");
            const boost::unordered_map<HMI_DISPLAY_MODE, const char *> HmiDisplayModeToString = boost::assign::map_list_of
                    (HMI_DISPLAY_MODE::OFF, "off")
                    (HMI_DISPLAY_MODE::STATIC_LOGO, "static_logo")
                    (HMI_DISPLAY_MODE::STATIC_TEXT, "static_text")
                    (HMI_DISPLAY_MODE::BAR_GRAPH_DISTANCE, "bargraph_distance")
                    (HMI_DISPLAY_MODE::BAR_GRAPH_ECHO, "bargraph_echo")
                    (HMI_DISPLAY_MODE::BAR_GRAPH_REFLECTOR, "bargraph_reflector")
                    (HMI_DISPLAY_MODE::APPLICATION_BITMAP, "application_bitmap")
                    (HMI_DISPLAY_MODE::APPLICATION_TEXT, "application_text");
        };

        template<>
        struct ParametersBuilderImpl<RoParametersTags::BasicInformationTag> {

            ADD_RO_PARAMETER_BUILDER(requestDeviceFamily, ParametersBuilderImpl, PARAMETER_DEVICE_FAMILY);
            ADD_RO_PARAMETER_BUILDER(requestVendor, ParametersBuilderImpl, PARAMETER_VENDOR);
            ADD_RO_PARAMETER_BUILDER(requestProduct, ParametersBuilderImpl, PARAMETER_PRODUCT);
            ADD_RO_PARAMETER_BUILDER(requestPart, ParametersBuilderImpl, PARAMETER_PART);
            ADD_RO_PARAMETER_BUILDER(requestSerial, ParametersBuilderImpl, PARAMETER_SERIAL);
            ADD_RO_PARAMETER_BUILDER(requestFirmwareRevision, ParametersBuilderImpl, PARAMETER_REVISION_FW);
            ADD_RO_PARAMETER_BUILDER(requestHardwareRevision, ParametersBuilderImpl, PARAMETER_REVISION_HW);

//            ParametersBuilderImpl &requestDeviceFamily() {
//                parameters.push_back(PARAMETER_DEVICE_FAMILY);
//                return *this;
//            }

//            ParametersBuilderImpl &requestVendor() {
//                parameters.push_back(PARAMETER_VENDOR);
//                return *this;
//            }

//            ParametersBuilderImpl &requestProduct() {
//                parameters.push_back(PARAMETER_PRODUCT);
//                return *this;
//            }

//            ParametersBuilderImpl &requestPart() {
//                parameters.push_back(PARAMETER_PART);
//                return *this;
//            }

//            ParametersBuilderImpl &requestSerial() {
//                parameters.push_back(PARAMETER_SERIAL);
//                return *this;
//            }

//            ParametersBuilderImpl &requestFirmwareRevision() {
//                parameters.push_back(PARAMETER_REVISION_FW);
//                return *this;
//            }

//            ParametersBuilderImpl &requestHardwareRevision() {
//                parameters.push_back(PARAMETER_REVISION_HW);
//                return *this;
//            }

            [[nodiscard]] ParametersList build() const {
                return parameters;
            }

        private:
            ParametersList parameters;
        };

        template<>
        struct ParametersBuilderImpl<RoParametersTags::CapabilitiesTag> {
            ParametersBuilderImpl &requestFeatureFlags() {
                parameters.push_back(PARAMETER_FEATURE_FLAGS);
                return *this;
            }

            ParametersBuilderImpl &requestEmitterType() {
                parameters.push_back(PARAMETER_EMITTER_TYPE);
                return *this;
            }

            ParametersBuilderImpl &requestRadialMinRange() {
                parameters.push_back(PARAMETER_RADIAL_RANGE_MIN);
                return *this;
            }

            ParametersBuilderImpl &requestRadialMaxRange() {
                parameters.push_back(PARAMETER_RADIAL_RANGE_MAX);
                return *this;
            }

            ParametersBuilderImpl &requestRadialResolution() {
                parameters.push_back(PARAMETER_RADIAL_RESOLUTION);
                return *this;
            }

            ParametersBuilderImpl &requestAngularFov() {
                parameters.push_back(PARAMETER_ANGULAR_FOV);
                return *this;
            }

            ParametersBuilderImpl &requestAngularResolution() {
                parameters.push_back(PARAMETER_ANGULAR_RESOLUTION);
                return *this;
            }

            ParametersBuilderImpl &requestMinimalScanFrequency() {
                parameters.push_back(PARAMETER_SCAN_FREQUENCY_MIN);
                return *this;
            }

            ParametersBuilderImpl &requestMaximalScanFrequency() {
                parameters.push_back(PARAMETER_SCAN_FREQUENCY_MAX);
                return *this;
            }

            ParametersBuilderImpl &requestMinimalSamplingRate() {
                parameters.push_back(PARAMETER_SAMPLING_RATE_MIN);
                return *this;
            }

            ParametersBuilderImpl &requestMaximalSamplingRate() {
                parameters.push_back(PARAMETER_SAMPLING_RATE_MAX);
                return *this;
            }

            ParametersBuilderImpl &requestMaximalConnections() {
                parameters.push_back(PARAMETER_MAX_CONNECTIONS);
                return *this;
            }

            [[nodiscard]] ParametersList build() const {
                return parameters;
            }

        private:
            ParametersList parameters;
        };

        template<>
        struct ParametersBuilderImpl<RoParametersTags::EthernetTag> {
            ParametersBuilderImpl &requestCurrentIpMode() {
                parameters.push_back(PARAMETER_IP_MODE_CURRENT);
                return *this;
            }

            ParametersBuilderImpl &requestCurrentIpAddress() {
                parameters.push_back(PARAMETER_IP_ADDRESS_CURRENT);
                return *this;
            }

            ParametersBuilderImpl &requestCurrentSubnetMask() {
                parameters.push_back(PARAMETER_SUBNET_MASK_CURRENT);
                return *this;
            }

            ParametersBuilderImpl &requestCurrentGateway() {
                parameters.push_back(PARAMETER_GATEWAY_CURRENT);
                return *this;
            }

            ParametersBuilderImpl &requestMacAddress() {
                parameters.push_back(PARAMETER_MAC_ADDRESS);
                return *this;
            }

            [[nodiscard]] ParametersList build() const {
                return parameters;
            }

        private:
            ParametersList parameters;
        };

        template<>
        struct ParametersBuilderImpl<RoParametersTags::MeasuringTag> {
            ParametersBuilderImpl &requestMeasuredFrequency() {
                parameters.push_back(PARAMETER_SCAN_FREQUENCY_MEASURED);
                return *this;
            }

            [[nodiscard]] ParametersList build() const {
                return parameters;
            }

        private:
            ParametersList parameters;
        };

        template<>
        struct ParametersBuilderImpl<RoParametersTags::SystemStatusTag> {
            ParametersBuilderImpl &requestStatusFlags() {
                parameters.push_back(PARAMETER_STATUS_FLAGS);
                return *this;
            }

            ParametersBuilderImpl &requestLoadIndication() {
                parameters.push_back(PARAMETER_LOAD_INDICATION);
                return *this;
            }

            ParametersBuilderImpl &requestSystemTimeRaw() {
                parameters.push_back(PARAMETER_SYSTEM_TIME_RAW);
                return *this;
            }

            ParametersBuilderImpl &requestUpTime() {
                parameters.push_back(PARAMETER_UP_TIME);
                return *this;
            }

            ParametersBuilderImpl &requestPowerCycles() {
                parameters.push_back(PARAMETER_POWER_CYCLES);
                return *this;
            }

            ParametersBuilderImpl &requestOperationTime() {
                parameters.push_back(PARAMETER_OPERATION_TIME);
                return *this;
            }

            ParametersBuilderImpl &requestOperationTimeScaled() {
                parameters.push_back(PARAMETER_OPERATION_TIME_SCALED);
                return *this;
            }

            ParametersBuilderImpl &requestCurrentTemperature() {
                parameters.push_back(PARAMETER_TEMPERATURE_CURRENT);
                return *this;
            }

            ParametersBuilderImpl &requestMinimalTemperature() {
                parameters.push_back(PARAMETER_TEMPERATURE_MIN);
                return *this;
            }

            ParametersBuilderImpl &requestMaximalTemperature() {
                parameters.push_back(PARAMETER_TEMPERATURE_MAX);
                return *this;
            }

            [[nodiscard]] ParametersList build() const {
                return parameters;
            }

        private:
            ParametersList parameters;
        };

        struct RO;
        struct RW;

        template<typename AccessType>
        struct ParametersBuilder {
        };

        template<>
        struct ParametersBuilder<RO> {
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
        struct ParametersBuilder<RW> {
            using BasicInformation = internals::Parameters::ParametersBuilderImpl<
                    internals::Parameters::RwParametersTags::BasicInformationTag>;
            using Ethernet = internals::Parameters::ParametersBuilderImpl<
                    internals::Parameters::RwParametersTags::EthernetTag>;
            using Measure = internals::Parameters::ParametersBuilderImpl<
                    internals::Parameters::RwParametersTags::MeasuringTag>;
            using HmiDisplay = internals::Parameters::ParametersBuilderImpl<
                    internals::Parameters::RwParametersTags::HmiDisplayTag>;
        };

    }

    using ReadParametersBuilder = internals::Parameters::ParametersBuilder<internals::Parameters::RO>;
    using WriteParametersBuilder = internals::Parameters::ParametersBuilder<internals::Parameters::RW>;

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
         * Use this method in order to send an HTTP Command ParameterChaining only one parameter and its corresponding value.
         * @param command Give a command among the available commands of the device.
         * @param parameters Give a parameter among the available parameters of the device.
         * @param values Give a value to the corresponding parameter
         * @return The method ParameterChaining the given command, parameter and its corresponding value if the parameter is not empty.
         */
        [[nodiscard]] std::pair<bool, Device::PropertyTree>
        sendHttpCommand(const std::string &command, const std::string &parameters = "",
                        std::string values = "") const noexcept(false);

        /**
         * Use this method in order to send an HTTP Command ParameterChaining several parameters and their corresponding value.
         * @param command Give a command among the available commands of the device.
         * @param paramValues Give many parameters ParameterChaining their corresponding value.
         * @return The method ParameterChaining the given command, parameters and values if the HTTP Status code is equal to 200, an
         * error otherwise.
         */
        [[nodiscard]] std::pair<bool, Device::PropertyTree>
        sendHttpCommand(const std::string &command, const ParametersMap &parameters) const noexcept(false);
    private:
        /**
         * Send a HTTP GET request to the device.
         * @param requestPath The last part of an URL ParameterChaining a slash leading.
         * @param header The response header returned as std::string.
         * @return The HTTP status code or 0 in case of an error.
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
    };

    namespace internals::Parameters {

        struct CommandTags
        {
            struct ReleaseHandle;
            struct StartScanOutput;
            struct StopScanOutput;
            struct FeedWatchdog;
            struct GetParameters;
            struct FetchParameterList;
            struct SetParameters;
            struct FactoryResetParameters;
            struct FactoryResetAll;
            struct RebootDevice;
            struct RequestUdpHandle;
            struct RequestTcpHandle;
        };

        template<typename Command>
        struct CommandExecutorImpl
        {};

        template<typename... Args>
        struct ParameterChaining {
            template<typename T>
            static constexpr inline void resolve(ParametersList &list, T &builder) {
                const auto parameters{builder.build()};
                list.insert(std::end(list), std::cbegin(parameters), std::cend(parameters));
            }

            template<typename T0, typename... Rest>
            static constexpr inline void resolve(ParametersList &list, T0 &builder, Args &&... args) {
                const auto parameters{builder.build()};
                list.insert(std::end(list), std::cbegin(parameters), std::cend(parameters));
                resolve<>(list, std::forward<Args>(args)...);
            }
        };

        struct RecursiveExecutorHelper {

            template<typename Func, typename T>
            static constexpr inline void resolve(Func && execute, T &&builder) {
                execute(std::forward<T>(builder));
            }

            template<typename Func,typename T, typename... Rest>
            static constexpr inline void resolve(Func && execute, T &&builder, Rest &&... args) {
                execute(std::forward<T>(builder));
                resolve<Func, Rest...>(std::forward<Func>(execute), std::forward<Rest>(args)...);
            }
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::ReleaseHandle>
        {
            explicit CommandExecutorImpl(const R2000 & device) : device(device){}
            template<typename ... Args>
            inline bool execute(Args &&... args) {
                auto result{true};
                RecursiveExecutorHelper::resolve([this, &result](const auto &handle) {
                    result &= device.sendHttpCommand(COMMAND_RELEASE_HANDLE,
                                                     PARAMETER_NAME_HANDLE,
                                                     handle.value).first;
                }, std::forward<Args>(args)...);
                return result;
            }
        private:
            const R2000 & device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::StartScanOutput>
        {
            explicit CommandExecutorImpl(const R2000 & device) : device(device){}
            template<typename ... Args>
            inline bool execute(Args &&... args) {
                auto result{true};
                RecursiveExecutorHelper::resolve([this, &result](const auto &handle) {
                    result &= device.sendHttpCommand(COMMAND_START_SCAN_OUTPUT,
                                                     PARAMETER_NAME_HANDLE,
                                                     handle.value).first;
                }, std::forward<Args>(args)...);
                return result;
            }
        private:
            const R2000 & device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::StopScanOutput>
        {
            explicit CommandExecutorImpl(const R2000 & device) : device(device){}
            template<typename ... Args>
            inline bool execute(Args &&... args) {
                auto result{true};
                RecursiveExecutorHelper::resolve([this, &result](const auto &handle) {
                    result &= device.sendHttpCommand(COMMAND_STOP_SCAN_OUTPUT,
                                                     PARAMETER_NAME_HANDLE,
                                                     handle.value).first;
                }, std::forward<Args>(args)...);
                return result;
            }
        private:
            const R2000 & device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::FeedWatchdog>
        {
            explicit CommandExecutorImpl(const R2000 & device) : device(device){}
            template<typename ... Args>
            inline bool execute(Args &&... args) {
                auto result{true};
                RecursiveExecutorHelper::resolve([this, &result](const auto &handle) {
                    result &= device.sendHttpCommand(COMMAND_FEED_WATCHDOG,
                                                     PARAMETER_NAME_HANDLE,
                                                     handle.value).first;
                }, std::forward<Args>(args)...);
                return result;
            }
        private:
            const R2000 & device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::GetParameters>
        {
            explicit CommandExecutorImpl(const R2000 & device) : device(device){}
            template<typename ... Args>
            inline ParametersMap execute(Args &&... args) {
                ParametersMap parameters{};
                RecursiveExecutorHelper::resolve([this, &parameters](const auto &builder) {
                    const auto requestedParameters{builder.build()};
                    const auto retrievedParameters{getParametersValues(requestedParameters)};
                    parameters.template insert(std::cbegin(retrievedParameters), std::cend(retrievedParameters));
                    }, std::forward<Args>(args)...);
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
            const R2000 & device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::FetchParameterList>
        {
            explicit CommandExecutorImpl(const R2000 & device) : device(device){}
            ParametersList execute()
            {
                ParametersList parameterList{};
                const auto result{device.sendHttpCommand(COMMAND_LIST_PARAMETERS)};
                const auto status{result.first};
                const auto &propertyTree{result.second};

                const auto list{propertyTree.get_child_optional("parameters")};
                if (!status || !list)
                    return {};
                for (const auto &name : *list) {
                    const auto parameterName{name.second.get<std::string>("")};
                    if (parameterName.empty())
                        continue;
                    parameterList.emplace_back(parameterName);
                }
                return parameterList;
            }
        private:
            const R2000 & device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::SetParameters> {
            explicit CommandExecutorImpl(const R2000 &device) : device(device) {}
            template<typename ... Args>
            inline bool execute(Args &&... args) {
                auto result{true};
                RecursiveExecutorHelper::template resolve([this, &result](const auto &builder) {
                    const auto parameters{builder.build()};
                    result &= device.sendHttpCommand(COMMAND_SET_PARAMETER, parameters).first;
                }, std::forward<Args>(args)...);
                return result;
            }
        private:
            const R2000 &device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::FactoryResetParameters>
        {
            explicit CommandExecutorImpl(const R2000 & device) : device(device){}

            template<typename ... Args>
            inline bool execute(Args&&... args) {
                ParametersList parameters{};
                ParameterChaining<Args...>::resolve(parameters, std::forward<Args>(args)...);
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
            const R2000 & device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::FactoryResetAll>
        {
            explicit CommandExecutorImpl(const R2000 & device) : device(device){}
            inline bool execute() {
                return device.sendHttpCommand(COMMAND_FACTORY_RESET).first;
            }
        private:
            const R2000 & device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::RebootDevice>
        {
            explicit CommandExecutorImpl(const R2000 & device) : device(device){}
            bool execute()
            {
                return device.sendHttpCommand(COMMAND_REBOOT_DEVICE).first;
            }
            template<typename T>
            RebootStatus execute( T timeout) noexcept(false)
            {
                {
                    const auto status{isRebooting()};
                    if (status && !status.value()) {
                        if (!execute()) {
                            return RebootStatus::FAILED;
                        }
                    }
                }
                const auto timeoutDuration{std::chrono::duration_cast<std::chrono::milliseconds>(timeout)};
                const auto deadline{std::chrono::steady_clock::now() + timeoutDuration};
                const auto SleepAction{[deadline, timeoutDuration](const auto &duration) {
                    const auto now{std::chrono::steady_clock::now()};
                    if (now + duration > deadline)
                        std::this_thread::sleep_for(deadline - now);
                    else
                        std::this_thread::sleep_for(duration);
                }};
                const auto IsRetryable{[](RebootStatus status) {
                    if (status == RebootStatus::FAILED) {
                        return true;
                    }
                    return false;
                }};
                const auto Callable{[this, deadline]() -> RebootStatus {
                    const auto status{isRebooting()};
                    if (status && status.value()) {
                        return RebootStatus::COMPLETED;
                    }
                    const auto now{std::chrono::steady_clock::now()};
                    if (now > deadline)
                        return RebootStatus::TIMEOUT;
                    return RebootStatus::FAILED;
                }};
                return Retry::ExponentialBackoff(std::numeric_limits<unsigned int>::max(), 100ms, 3000ms, SleepAction,
                                                 IsRetryable, Callable);
            }
        private:
            std::optional<bool> isRebooting() noexcept(true) {
                try {
                    CommandExecutorImpl<internals::Parameters::CommandTags::GetParameters> executor{device};
                    const auto result{executor.execute(ReadParametersBuilder::SystemStatus().requestStatusFlags())};
                    if (!result.empty()) {
                        const auto statusFlags{result.at(PARAMETER_STATUS_FLAGS)};
                        const auto value{std::stoi(statusFlags)};
                        std::bitset<STATUS_FLAG_BIT_SIZE> bitsetRepresentation(value);
                        return {bitsetRepresentation[0]};
                    }
                }
                catch (boost::system::system_error &) {
                    // No connection, probably rebooting
                }
                return std::nullopt;
            }

        private:
            const R2000 & device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::RequestTcpHandle>
        {
            explicit CommandExecutorImpl(const R2000 & device) : device(device){}
            std::optional<std::pair<int, Device::HandleType>> execute(const ParametersMap & parameters)
            {
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
            const R2000 & device;
        };

        template<>
        struct CommandExecutorImpl<internals::Parameters::CommandTags::RequestUdpHandle>
        {
            explicit CommandExecutorImpl(const R2000 & device) : device(device){}
            std::optional<Device::HandleType> execute(const ParametersMap & parameters)
            {
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
            const R2000 & device;
        };
    }

    struct Commands {
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
        using FactoryResetAllCommand = internals::Parameters::CommandExecutorImpl<
                internals::Parameters::CommandTags::FactoryResetAll>;
        using RebootDeviceCommand = internals::Parameters::CommandExecutorImpl<
                internals::Parameters::CommandTags::RebootDevice>;
        using RequestUdpHandleCommand = internals::Parameters::CommandExecutorImpl<
                internals::Parameters::CommandTags::RequestUdpHandle>;
        using RequestTcpHandleCommand = internals::Parameters::CommandExecutorImpl<
                internals::Parameters::CommandTags::RequestTcpHandle>;
    };
} // namespace Device