//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#pragma once

#include "HttpController.hpp"

namespace R2000 {

    struct DeviceHandle {
    public:
        enum class PROTOCOL {
            TCP, UDP
        };
        enum class PACKET_TYPE {
            A, B, C
        };

    public:
        DeviceHandle(Handle_Id_Type value, const PACKET_TYPE packetType,
                     const int startAngle, const bool watchdogEnabled,
                     const std::chrono::milliseconds watchdogTimeout, const int port, boost::asio::ip::address hostname,
                     const PROTOCOL aProtocol)
                : value(std::move(value)),
                  packetType(packetType),
                  startAngle(startAngle),
                  watchdogEnabled(
                          watchdogEnabled),
                  watchdogTimeout(
                          watchdogTimeout),
                  port(port),
                  hostname(std::move(hostname)),
                  protocol(aProtocol){}

        virtual ~DeviceHandle() = default;
        [[maybe_unused]] const Handle_Id_Type value{};
        [[maybe_unused]] const PACKET_TYPE packetType{};
        [[maybe_unused]] const int startAngle{};
        [[maybe_unused]] const bool watchdogEnabled{};
        [[maybe_unused]] const std::chrono::milliseconds watchdogTimeout{};
        [[maybe_unused]] const int port{};
        [[maybe_unused]] const boost::asio::ip::address hostname{boost::asio::ip::address::from_string("0.0.0.0")};
        [[maybe_unused]] const PROTOCOL protocol{};
    };
    std::string packetTypeToString(DeviceHandle::PACKET_TYPE type);

}