//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#pragma once

#include "HttpController.hpp"
#include "DeviceHandle.hpp"
#include "DataLink/TCPLink.hpp"
#include "DataLink/UDPLink.hpp"
#include <chrono>
#include <utility>

using namespace std::chrono_literals;

namespace R2000 {
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

        template<typename T>
        auto cast_any_or_default(std::any &&any, T defaultArg) noexcept(true) {
            if (!any.has_value())
                return defaultArg;
            const auto value = std::any_cast<T>(&any);
            return value != nullptr ? *value : defaultArg;
        }
    } // namespace internals

    class DataLinkBuilder {
    public:
        DataLinkBuilder &withProtocol(DeviceHandle::PROTOCOL protocol) {
            requirements.put("protocol", protocol);
            return *this;
        }

        DataLinkBuilder &withTargetHostname(const std::string &hostname) {
            requirements.put("hostname", hostname);
            return *this;
        }

        DataLinkBuilder &withPort(const unsigned short port) {
            requirements.put("port", port);
            return *this;
        }

        DataLinkBuilder &withStartAngle(const int startAngle) {
            requirements.put("start_angle", startAngle);
            return *this;
        }

        DataLinkBuilder &withPacketType(DeviceHandle::PACKET_TYPE packetType) {
            requirements.put("packet_type", packetType);
            return *this;
        }

        DataLinkBuilder &withWatchdogEnabled() {
            requirements.put("watchdog_enabled", true);
            return *this;
        }

        DataLinkBuilder &withWatchdogTimeout(std::chrono::milliseconds watchdogTimeout) {
            requirements.put("watchdog_timeout", watchdogTimeout);
            return *this;
        }

        std::unique_ptr<DataLink> build(const std::shared_ptr<R2000::HttpController> &controller) noexcept(false) {
            try {
                return build_and_potentially_throw(controller);
            }
            catch (std::exception &e) {
                internals::print_nested_exception(e);
                internals::rethrow(__func__, "Shockwave device builder has failed.");
            }
            return {nullptr};
        }

    private:


        std::unique_ptr<DataLink>
        build_and_potentially_throw(const std::shared_ptr<R2000::HttpController> &controller) noexcept(false) {
            const auto protocol = internals::cast_any_or_throw<R2000::DeviceHandle::PROTOCOL>(
                    requirements.get("protocol"),
                    "Could not get the protocol parameter. You must specify it using the"
                    " method withProtocol(...)"
            );
            const auto startAngle = internals::cast_any_or_throw<int>(requirements.get("start_angle"),
                                                                      "Could not get the start angle parameter. You must specify it using the"
                                                                      " method withStartAngle(...)");
            const auto packetType = internals::cast_any_or_throw<R2000::DeviceHandle::PACKET_TYPE>(
                    requirements.get("packet_type"),
                    "Could not get the packet type parameter. You must specify it using the method withPacketType(...)");

            switch (packetType) {
                case DeviceHandle::PACKET_TYPE::A:
                    throw builder_exception(
                            std::string(__func__) + " -> Packet type A is not yet supported by the data link.");
                case DeviceHandle::PACKET_TYPE::B:
                    throw builder_exception(
                            std::string(__func__) + " -> Packet type B is not yet supported by the data link.");
                default:
                    break;
            }

            const auto watchdogEnabled = internals::cast_any_or_default<bool>(
                    requirements.get("watchdog_enabled"), false);
            const auto watchdogTimeout = internals::cast_any_or_default<std::chrono::milliseconds>(
                    requirements.get("watchdog_timeout"), 100ms);

            std::unordered_map<std::string, std::string> parameters;
            parameters["packet_type"] = R2000::packetTypeToString(packetType);
            parameters["start_angle"] = std::to_string(startAngle);
            const auto hostname = internals::cast_any_or_throw<std::string>(requirements.get("hostname"),
                                                                            "Could not get the hostname. "
                                                                            "You must specify it using the method withTargetHostname(...)");
            switch (protocol) {

                case DeviceHandle::PROTOCOL::TCP: {
                    const auto result = controller->sendHttpCommand("request_handle_tcp", parameters);
                    const auto status = result.first;
                    const auto &propertyTree = result.second;
                    if (!status || !R2000::HttpController::verifyErrorCode(propertyTree))
                        return {nullptr};
                    const auto port = propertyTree.get_optional<int>("port");
                    const auto handle = propertyTree.get_optional<std::string>("handle");
                    if (!port || !handle)
                        return {nullptr};
                    const auto h = std::make_shared<DeviceHandle>(*handle, packetType, startAngle,
                                                                  watchdogEnabled,
                                                                  watchdogTimeout, *port,
                                                                  boost::asio::ip::address::from_string(hostname),
                                                                  DeviceHandle::PROTOCOL::TCP);
                    return std::make_unique<TCPLink>(controller, h);

                }
                case DeviceHandle::PROTOCOL::UDP: {
                    const auto port = internals::cast_any_or_throw<unsigned short>(requirements.get("port"),
                                                                                   "Could not get the UDP port parameter. You must specify it using the"
                                                                                   " method withPort(...)");
                    parameters["port"] = std::to_string(port);
                    parameters["address"] = hostname;
                    const auto result = controller->sendHttpCommand("request_handle_udp", parameters);
                    const auto status = result.first;
                    const auto &propertyTree = result.second;
                    if (!status || !R2000::HttpController::verifyErrorCode(propertyTree))
                        return {nullptr};
                    const auto handle = propertyTree.get_optional<std::string>("handle");
                    if (!handle)
                        return {nullptr};
                    const auto h = std::make_shared<DeviceHandle>(*handle, packetType, startAngle, watchdogEnabled,
                                                                  watchdogTimeout, port,
                                                                  boost::asio::ip::address::from_string(hostname),
                                                                  DeviceHandle::PROTOCOL::UDP);
                    return std::make_unique<UDPLink>(controller, h);
                }
            }
            return {nullptr};
        }

    private:
        internals::Requirements<std::string> requirements{};
    };

}