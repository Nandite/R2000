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

#include <Control/Commands.hpp>
#include <any>
#include <chrono>
#include <future>
#include <utility>

using namespace std::chrono_literals;

namespace Device {

    class R2000;

    class DataLink;

    class DeviceHandle;

    enum class RequestResult;

    class builderException : public std::invalid_argument {
    public:
        explicit builderException(const std::string &what) : std::invalid_argument(what) {};
    };
    namespace internals {
        /**
         * Helper class to map different parameters inside a map with a Key type.
         * @tparam KeyType The type of key to use for the underlying map.
         */
        template<typename KeyType>
        class Requirements {
        public:
            /**
             * Put a new element into the map.
             * @tparam ValueType The type of the value to put.
             * @param key The key of the value.
             * @param value The value to put inside the map.
             */
            template<typename ValueType>
            void put(KeyType key, ValueType &&value) {
                requirements[key] = std::any(std::forward<ValueType>(value));
            }

            /**
             * Get a value stored inside the map as an any.
             * @param key The key of the value.
             * @return an any containing the value is found, empty otherwise.
             */
            std::any get(KeyType key) {
                if (requirements.count(key))
                    return requirements.at(key);
                return {};
            }

        private:
            std::unordered_map<KeyType, std::any> requirements{};
        };

        /**
         * Cast an any to a value if not empty, otherwise throws an exception with a custom message.
         * @tparam T The type to cast the any into.
         * @param any The any to cast.
         * @param throwMessage The message to throw if the any is empty.
         * @return The casted value.
         */
        template<typename T>
        inline auto castAnyOrThrow(const std::any &any, const std::string &throwMessage) noexcept(false) {
            if (!any.has_value())
                throw builderException("Any is empty -> " + throwMessage);
            return std::any_cast<T>(any);
        }

        /**
         * Find a value inside a map or throws an exception with a custom message.
         * @param map The map to search into.
         * @param key The key of the value to get.
         * @param throwMessage The message to throw if the key is not found.
         * @return The found value.
         */
        inline auto findValueOrThrow(const Parameters::ParametersMap &map, const std::string &key,
                                     const std::string &throwMessage) noexcept(false)
        -> const Parameters::ParametersMap::mapped_type & {
            if (!map.count(key))
                throw builderException("Key not found -> " + throwMessage);
            return map.at(key);
        }

        /**
         * Find a value inside a map or return a default value.
         * @param map The map to search into.
         * @param key The key of the value to get.
         * @param defaultValue The default fallback value if the key is not found.
         * @return The found value or the default one.
         */
        inline auto findValueOrDefault(const Parameters::ParametersMap &map, const std::string &key,
                                       const std::string &defaultValue) noexcept(false)
        -> const Parameters::ParametersMap::mapped_type & {
            if (!map.count(key))
                return defaultValue;
            return map.at(key);
        }

    } // namespace internals

    class DataLinkBuilder {
    private:
        using WatchdogConfiguration = std::pair<bool, std::chrono::milliseconds>;
    public:
        using AsyncBuildResult = std::pair<RequestResult, std::shared_ptr<DataLink>>;

    public:
        /**
         * Construct a new DataLinkBuilder geared to build a TCPLink.
         * @param builder configuration of the TCPLink data stream.
         */
        explicit DataLinkBuilder(const Parameters::ReadWriteParameters::TcpHandle &builder);

        /**
         * Construct a new DataLinkBuilder geared to build a UDPLink.
         * @param builder configuration of the UDPLink data stream.
         */
        explicit DataLinkBuilder(const Parameters::ReadWriteParameters::UdpHandle &builder);

        /**
         * Construct a new DataLink for a device using the stored configuration. Be careful, this method will
         * block if the device cannot be joined over the network until it fails and return. The length of the
         * blocking depends on the underlying OS socket configuration.
         * @param device The device to build the link with.
         * @return A shared pointer containing the built DataLink or nullptr if an error has occurred.
         */
        std::shared_ptr<DataLink> build(const std::shared_ptr<Device::R2000> &device) noexcept(false);

        /**
         * Asynchronously construct a new DataLink for a device using the stored configuration. The method returns a
         * future upon which the client can wait for the DataLink to be build. A timeout can be provided so that the
         * built is aborted if the device cannot be joined over the network.
         * @param device The device to build the link with.
         * @param timeout Maximum time allowed for the build to takes place.
         * @return a future of pair containing the result of the build request and if successful and the
         * built DataLink. If the request has failed, the DataLink evaluates to nullptr.
         */
        std::future<AsyncBuildResult> build(const std::shared_ptr<Device::R2000> &device,
                                            std::chrono::milliseconds timeout) noexcept(false);

    private:
        /**
         * Extract the watchdog parameters from a parameters map.
         * @param map The parameters map to search the watchdog data in.
         * @return a pair containing:
         * - A flag at true if the watchdog is enabled, False otherwise (False also if the watchdog parameters is not
         * found within the map).
         * - The timeout of the watchdog (defaulted to 1 minute).
         */
        [[nodiscard]] static std::pair<bool, std::chrono::milliseconds>
        extractWatchdogParameters(const Parameters::ParametersMap &map);

        /**
         * Extract the TCP Handle and watchdog configuration parameters from a map of requirements.
         * @param requirements The map of requirements to search in.
         * @return A pair of:
         * - The TCP handle parameters.
         * - The watchdog configuration.
         */
        static std::pair<Parameters::ReadWriteParameters::TcpHandle, WatchdogConfiguration>
        extractTcpDataLinkParameters(internals::Requirements<std::string> &requirements) noexcept(false);

        /**
         * Extract the UDP Handle, watchdog configuration, and the address and port parameters
         * from a map of requirements.
         * @param requirements The map of requirements to search in.
         * @return A tuple of of:
         * - The UDP handle parameters.
         * - The watchdog configuration.
         * - The address at which the data stream is sent.
         * - The port at which the data stream is sent.
         */
        static std::tuple<Parameters::ReadWriteParameters::UdpHandle, WatchdogConfiguration, boost::asio::ip::address,
                unsigned int>
        extractUdpDataLinkParameters(internals::Requirements<std::string> &requirements) noexcept(false);

        /**
         * Make a DeviceHandle given a result from the request of a TCP handle to the device.
         * @param commandResult The result of the request handle command.
         * @param address The address of the device.
         * @param watchdog The watchdog configuration.
         * @return A shared pointer containing a DeviceHandle.
         */
        static std::shared_ptr<DeviceHandle>
        makeTcpDeviceHandle(const Commands::RequestTcpHandleCommand::ResultType &commandResult,
                            const boost::asio::ip::address &address,
                            WatchdogConfiguration watchdog) noexcept;

        /**
         * Make a DeviceHandle given a result from the request of a UDP handle to the device.
         * @param commandResult The result of the request handle command.
         * @param address The address at which the data is sent.
         * @param port The port at which the data is sent.
         * @param watchdog The watchdog configuration.
         * @return A shared pointer containing a DeviceHandle.
         */
        static std::shared_ptr<DeviceHandle>
        makeUdpDeviceHandle(const Commands::RequestUdpHandleCommand::ResultType &commandResult,
                            const boost::asio::ip::address &address, unsigned int port,
                            WatchdogConfiguration watchdog) noexcept;

        /**
         * Synchronously request a data stream to the device and construct a DataLink to handle it.
         * @param device The device to make the link with.
         * @return A shared pointer containing the DataLink or nullptr if the link could not be established.
         */
        std::shared_ptr<DataLink>
        constructDataLink(const std::shared_ptr<Device::R2000> &device) noexcept(false);

        /**
         * Asynchronously request a data stream to the device and construct a DataLink to handle it.
         * @param device The device to make the link with.
         * @param timeout The maximum time allowed for the device to be joined and the DataLink constructed.
         * @return a future that contains the result of the request and a DataLink (or nullptr if the link could not
         * be established).
         */
        std::future<AsyncBuildResult> constructDataLink(const std::shared_ptr<Device::R2000> &device,
                                                        std::chrono::milliseconds timeout) noexcept(true);

    private:
        internals::Requirements<std::string> requirements{};
    };

} // namespace Device