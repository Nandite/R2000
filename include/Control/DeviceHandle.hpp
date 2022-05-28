// Copyright (c) 2022 Papa Libasse Sow.
// https://github.com/Nandite/R2000
// Distributed under the MIT Software License (X11 license).
//
// SPDX-License-Identifier: MIT
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of
// the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include "DataLink/Data.hpp"
#include <boost/asio.hpp>

namespace Device {
    // Base type of handle returned by the sensor.
    using HandleType = std::string;
    /**
     * The type of protocol supported by the device to transmit scans.
     */
    enum class PROTOCOL {
        TCP,
        UDP
    };

    class DeviceHandle {
    public:
        /**
         * Construct a new device handle given the parameters sent to the device to configure a new data stream.
         * @param value The alphanumeric value of the handle returned by the device.
         * @param address For TCP, the address of the device, for UDP, the address of the receiver of the data stream.
         * @param port For TCP, the port given by the device while requesting the handle. For UDP, the port at which
         * the device must send the data.
         * @param watchdogEnabled Either or not the watchdog must be enabled.
         * @param watchdogTimeout The maximum period of the timeout.
         */
        DeviceHandle(HandleType value, boost::asio::ip::address address, const unsigned short port,
                     const bool watchdogEnabled, const std::chrono::milliseconds watchdogTimeout)
                : value(std::move(value)),
                  watchdogEnabled(watchdogEnabled),
                  watchdogTimeout(watchdogTimeout),
                  port(port),
                  ipv4Address(std::move(address)) {
        }

        /**
         * @return Get the alphanumeric value of the handle.
         */
        [[nodiscard]] inline const HandleType &getValue() const noexcept { return value; }

        /**
         * @return True if the watchdog is enabled for the handle stream, False otherwise.
         */
        [[nodiscard]] inline bool isWatchdogEnabled() const noexcept { return watchdogEnabled; }

        /**
         * @return Get the timeout of the watchdog.
         */
        [[nodiscard]] inline auto getWatchdogTimeout() const noexcept { return watchdogTimeout; }

        /**
         * @return Get the port at which:
         * - for TCP connection, the DataLink must connect.
         * - for UDP connection, The local port where the DataLink receive the data.
         */
        [[nodiscard]] inline unsigned short getPort() const noexcept { return port; }

        /**
         * @return Get the address at which:
         * - for TCP connection, the DataLink must connect (the device address)
         * - for UDP connection, the address at which the device must send the data (usually the address of
         * the local machine).
         */
        [[nodiscard]] inline const boost::asio::ip::address &getAddress() const noexcept { return ipv4Address; }

        virtual ~DeviceHandle() = default;

    private:
        const HandleType value{};
        const bool watchdogEnabled{};
        const std::chrono::milliseconds watchdogTimeout{};
        const unsigned short port{};
        const boost::asio::ip::address ipv4Address{};
    };

} // namespace Device