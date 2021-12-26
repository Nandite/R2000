//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#pragma once

#include "DataLink/TCPLink.hpp"
#include "DataLink/UDPLink.hpp"
#include "DeviceHandle.hpp"
#include "R2000.hpp"
#include <chrono>
#include <utility>

using namespace std::chrono_literals;

namespace Device {
    class builder_exception : public std::invalid_argument {
    public:
        explicit builder_exception(const std::string &what) : std::invalid_argument(what) {};
        builder_exception(const builder_exception &) = default;
        builder_exception &operator=(const builder_exception &) = default;
        builder_exception(builder_exception &&) = default;
        builder_exception &operator=(builder_exception &&) = default;
        ~builder_exception() noexcept override = default;
    };
    namespace internals {
        template<typename KeyType>
        class Requirements {
        public:
            template<typename ValueType>
            void put(KeyType key, ValueType &&value) {
                requirements[key] = std::any(std::forward<ValueType>(value));
            }

            std::any get(KeyType key) {
                if (requirements.count(key))
                    return requirements.at(key);
                return {};
            }

        private:
            std::unordered_map<KeyType, std::any> requirements{};
        };

        template<class Context, class... Args>
        void rethrow(Context &&context, Args &&... args) {
            std::ostringstream ss;
            ss << context;
            auto sep = " : ";
            using expand = int[];
            void(expand{0, ((ss << sep << args), sep = ", ", 0)...});
            try {
                std::rethrow_exception(std::current_exception());
            }
            catch (const builder_exception &e) {
                std::throw_with_nested(builder_exception(ss.str()));
            }
            catch (const std::invalid_argument &e) {
                std::throw_with_nested(std::invalid_argument(ss.str()));
            }
            catch (const std::logic_error &e) {
                std::throw_with_nested(std::logic_error(ss.str()));
            }
            catch (...) {
                std::throw_with_nested(std::runtime_error(ss.str()));
            }
        }

        void print_nested_exception(const std::exception &e, std::size_t depth = 0) {
            std::clog << "exception: " << std::string(depth, ' ') << e.what() << std::endl;
            try {
                std::rethrow_if_nested(e);
            }
            catch (const std::exception &nested) {
                print_nested_exception(nested, depth + 1);
            }
        }

        template<typename T>
        auto cast_any_or_throw(const std::any &any, const std::string &throw_message) noexcept(false) {
            if (!any.has_value())
                throw builder_exception(std::string(__func__) + " : Any is empty -> " + throw_message);
            return std::any_cast<T>(any);
        }

        auto find_value_or_throw(const ParametersMap &map,
                                 const std::string &key,
                                 const std::string &throw_message) noexcept(false) {
            if (!map.count(key))
                throw builder_exception(std::string(__func__) + " : Key not found -> " + throw_message);
            return map.at(key);
        }

        std::string find_value_or_default(const ParametersMap &map,
                                   const std::string &key, const std::string & defaultValue) noexcept(false) {
            if (!map.count(key))
                return defaultValue;
            return map.at(key);
        }

    } // namespace internals

    class DataLinkBuilder {
    public:

        explicit DataLinkBuilder(const Device::RWParameters::TcpHandle &builder) {
            requirements.put("protocol", Device::DeviceHandle::PROTOCOL::TCP);
            requirements.put("handleBuilder", builder);
        }

        explicit DataLinkBuilder(const Device::RWParameters::UdpHandle &builder) {
            requirements.put("protocol", Device::DeviceHandle::PROTOCOL::UDP);
            requirements.put("handleBuilder", builder);
        }

        std::shared_ptr<DataLink> build(const std::shared_ptr<Device::R2000> &device) noexcept(false) {
            try {
                return build_and_potentially_throw(device);
            }
            catch (std::exception &e) {
                internals::print_nested_exception(e);
                internals::rethrow(__func__, "Device builder has failed.");
            }
            return {nullptr};
        }

    private:
        std::shared_ptr<DataLink>
        build_and_potentially_throw(const std::shared_ptr<Device::R2000> &device) noexcept(false) {

            const auto protocol{internals::cast_any_or_throw<Device::DeviceHandle::PROTOCOL>(
                    requirements.get("protocol"), "Could not get the protocol parameter. You must specify it using the"
                                                  " methods usingUdp(...) or usingTcp(...)")};
            switch (protocol) {
                case DeviceHandle::PROTOCOL::TCP: {
                    const auto handleBuilder{internals::cast_any_or_throw<Device::RWParameters::TcpHandle>(
                            requirements.get("handleBuilder"),
                            "Could not get the handle builder parameter. You must specify it using the"
                            " method withHandleBuilder(...)")};
                    const auto handleParameters{handleBuilder.build()};
                    const auto watchdogEnabled{internals::find_value_or_default(handleParameters,
                                                                              PARAMETER_HANDLE_WATCHDOG,
                                                                                BOOL_PARAMETER_FALSE) == BOOL_PARAMETER_TRUE};
                    const auto watchdogTimeout{
                            watchdogEnabled ? std::stoi(internals::find_value_or_throw(handleParameters,
                                                                                       PARAMETER_HANDLE_WATCHDOG_TIMEOUT,
                                                                                       "")) : 60000};
                    const auto result{Device::Commands::RequestTcpHandleCommand{*device}.execute(handleParameters)};
                    if (!result)
                        return {nullptr};
                    const auto &deviceAnswer{result.value()};
                    const auto port{deviceAnswer.first};
                    const auto handle{deviceAnswer.second};
                    const auto deviceHandle{std::make_shared<DeviceHandle>(
                            handle, watchdogEnabled, std::chrono::milliseconds(watchdogTimeout),
                            port)};
                    return std::make_shared<TCPLink>(device, deviceHandle);
                }
                case DeviceHandle::PROTOCOL::UDP: {
                    const auto handleBuilder{internals::cast_any_or_throw<Device::RWParameters::UdpHandle>(
                            requirements.get("handleBuilder"),
                            "Could not get the handle builder parameter. You must specify it using the"
                            " method withHandleBuilder(...)")};
                    const auto handleParameters{handleBuilder.build()};
                    const auto watchdogEnabled{internals::find_value_or_default(handleParameters,
                                                                                PARAMETER_HANDLE_WATCHDOG,
                                                                                BOOL_PARAMETER_FALSE) == BOOL_PARAMETER_TRUE};
                    const auto watchdogTimeout{
                            watchdogEnabled ? std::stoi(internals::find_value_or_throw(handleParameters,
                                                                                       PARAMETER_HANDLE_WATCHDOG_TIMEOUT,
                                                                                       "")) : 60000};
                    const auto port{std::stoi(internals::find_value_or_throw(handleParameters,
                                                                             PARAMETER_HANDLE_PORT,
                                                                             "Could not get the port parameter."))};
                    const auto result{Device::Commands::RequestUdpHandleCommand{*device}.execute(handleParameters)};
                    if (!result)
                        return {nullptr};
                    const auto &deviceAnswer{result.value()};
                    const auto handle{deviceAnswer};
                    const auto deviceHandle{std::make_shared<DeviceHandle>(
                            handle, watchdogEnabled, std::chrono::milliseconds(watchdogTimeout),
                            port)};
                    return std::make_shared<UDPLink>(device, deviceHandle);
                }
            }
            return {nullptr};
        }

    private:
        internals::Requirements<std::string> requirements{};
    };

} // namespace Device