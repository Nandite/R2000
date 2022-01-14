//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#pragma once

#include "R2000/DataLink/Data.hpp"
#include <boost/asio.hpp>

namespace Device {
    using HandleType = std::string;
    enum class PROTOCOL {
        TCP,
        UDP
    };
    struct DeviceHandle {
    public:
        /**
         *
         * @param value
         * @param watchdogEnabled
         * @param watchdogTimeout
         * @param port
         */
        DeviceHandle(HandleType value,
                     const bool watchdogEnabled,
                     const std::chrono::milliseconds watchdogTimeout,
                     const unsigned short port)
                : value(std::move(value)),
                  watchdogEnabled(watchdogEnabled),
                  watchdogTimeout(watchdogTimeout),
                  port(port) {
        }

        virtual ~DeviceHandle() = default;

        const HandleType value{};
        const bool watchdogEnabled{};
        const std::chrono::milliseconds watchdogTimeout{};
        const unsigned short port{};
    };

} // namespace Device