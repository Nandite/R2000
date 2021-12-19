//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#pragma once

#include "Data.hpp"
#include <boost/asio.hpp>

namespace Device {
    using HandleType = std::string;
    struct DeviceHandle {
    public:
        enum class PROTOCOL {
            TCP,
            UDP
        };

    public:
        DeviceHandle(HandleType value, const Data::PACKET_TYPE packetType, const int startAngle,
                     const bool watchdogEnabled,
                     const std::chrono::milliseconds watchdogTimeout, const unsigned short port,
                     boost::asio::ip::address hostname,
                     const PROTOCOL aProtocol)
                : value(std::move(value)),
                  packetType(packetType),
                  startAngle(startAngle),
                  watchdogEnabled(watchdogEnabled),
                  watchdogTimeout(watchdogTimeout),
                  port(port),
                  hostname(std::move(hostname)),
                  protocol(aProtocol) {
        }

        virtual ~DeviceHandle() = default;

        [[maybe_unused]] const HandleType value{};
        [[maybe_unused]] const Data::PACKET_TYPE packetType{};
        [[maybe_unused]] const int startAngle{};
        [[maybe_unused]] const bool watchdogEnabled{};
        [[maybe_unused]] const std::chrono::milliseconds watchdogTimeout{};
        [[maybe_unused]] const unsigned short port{};
        [[maybe_unused]] const boost::asio::ip::address hostname{boost::asio::ip::address::from_string("0.0.0.0")};
        [[maybe_unused]] const PROTOCOL protocol{};
    };

    inline std::string packetTypeToString(Data::PACKET_TYPE type) {
        switch (type) {
            case Data::PACKET_TYPE::A:
                return "A";
            case Data::PACKET_TYPE::B:
                return "B";
            case Data::PACKET_TYPE::C:
                return "C";
        }
        return "C";
    }

} // namespace Device