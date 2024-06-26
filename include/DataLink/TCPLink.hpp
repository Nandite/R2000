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

#include "DataLink.hpp"
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <memory>
#include <thread>

namespace Device {
    class R2000;

    class DeviceHandle;

    class TCPLink : public DataLink {
        // Default size of the bytes reception buffer
        static constexpr unsigned int DEFAULT_RECEPTION_BUFFER_SIZE{4096u};
        // Maximum size of the bytes reception buffer.
        static constexpr unsigned int MAX_RECEPTION_BUFFER_SIZE{32768u};
        // Default size of the extraction buffer.
        static constexpr unsigned int DEFAULT_EXTRACTION_BUFFER_SIZE{DEFAULT_RECEPTION_BUFFER_SIZE * 4};
    private:
        /**
         * Helper class that assembles a scan received through packets on a TCP connection from the device.
         */
        class TcpScanFactory : public ScanFactory {
        public:
            TcpScanFactory() = default;

            /**
             * Add a new packet to the scan being assembled.
             * @param header The header of the packet.
             * @param inputDistances The distances vector  of the points contained into the packet.
             * @param inputAmplitudes The amplitudes vector of the points contained into the packet.
             */
            inline void addPacket(const Data::Header &header, std::vector<std::uint32_t> &inputDistances,
                                  std::vector<std::uint32_t> &inputAmplitudes) override {
                distances.insert(std::end(distances), std::cbegin(inputDistances), std::cend(inputDistances));
                amplitudes.insert(std::end(amplitudes), std::cbegin(inputAmplitudes), std::cend(inputAmplitudes));
                headers.emplace_back(header);
            }

            /**
             * @return True if the factory is empty (no packet stored), False otherwise.
             */
            [[nodiscard]] inline bool empty() const override { return headers.empty(); }

            /**
             * @return True if the factory holds enough packet to generate a complete scan, False otherwise.
             */
            [[nodiscard]] inline bool isComplete() const override {
                return !empty() && distances.size() >= headers[0].numPointsScan;
            }

            /**
             * @return Assembles and returns a scan from the stored packets. Subsequent call to this method will not
             * yield a scan as the internals buffers are cleared every time this function is called.
             */
            inline SharedScan operator*() override {
                auto scan{
                        std::make_shared<Data::Scan>(distances, amplitudes, headers, std::chrono::steady_clock::now())};
                clear();
                return scan;
            }

            /**
             * Clear the internals buffers and remove all packets stored if any.
             */
            inline void clear() override {
                distances.clear();
                amplitudes.clear();
                headers.clear();
            }

            /**
             * @param scanNumber The scan number to test.
             * @return True if the packets stored by this factory belong to a given scan number.
             */
            inline bool operator==(const unsigned int scanNumber) const override {
                return !empty() && headers.back().scanNumber == scanNumber;
            }

            /**
             * @param scanNumber The scan number to test.
             * @return True if the packets stored by this factory don't belong to a given scan number.
             */
            inline bool operator!=(const unsigned int scanNumber) const override { return !((*this) == scanNumber); }

            /**
             * Get the headers of all the packet currently stored by the factory.
             * @param outputHeaders The vector to store the headers into.
             */
            inline void getHeaders(std::vector<Data::Header> &outputHeaders) const override {
                if (empty()) {
                    return;
                }
                const auto numberOfHeaders{headers.size()};
                outputHeaders.reserve(numberOfHeaders);
                outputHeaders.insert(std::cend(outputHeaders),
                                     std::cbegin(headers),
                                     std::cend(headers));
            }

        private:
            // Distance data in polar form in millimeter
            std::vector<std::uint32_t> distances{};
            // Amplitude data in the range 32-4095, values lower than 32 indicate an error or undefined values
            std::vector<std::uint32_t> amplitudes{};
            // Header received with the distance and amplitude data
            std::vector<Data::Header> headers{};
        };

    public:
        /**
         * @param iDevice : The device to attach the DataLink to.
         * @param iHandle : The device handle obtained from the device.
         */
        TCPLink(std::shared_ptr<R2000> iDevice, std::shared_ptr<DeviceHandle> iHandle) noexcept(false);

        /**
         * Interrupt all the asynchronous IO requests, shutdown and close the socket, then wait for the
         * io service task to finish.
         */
        ~TCPLink() override;

    private:

        /**
         * Asynchronously resolve and find the endpoints to connect on the remote device. On success,
         * this method will initiate an asynchronous connection of the socket to one of the found endpoints.
         */
        void asynchronousResolveEndpoints();

        /**
         * Asynchronously try to connect the reception socket to any endpoint given a range of endpoints.
         * @tparam Iterator The type of iterator of the range of endpoints.
         * @param begin The start of the range of endpoints.
         * @param end The end of the range of endpoints.
         */
        template<typename Iterator>
        void asynchronousConnectSocket(Iterator begin, Iterator end) {
            boost::system::error_code placeholder{};
            socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, placeholder);
            socket->close(placeholder);
            boost::asio::async_connect(*socket, begin, end, [&](auto error, auto endpoint) {
                onSocketConnectionAttemptCompleted(error, endpoint);
            });
        }

        /**
         * Handle called when the socket connection is completed, either it has succeeded or failed.
         * @param error The error of connection if any.
         * @param endpoint The endpoint to which the connection has been performed if successful,
         * or the end iterator if no endpoint could be used for connection.
         */
        void onSocketConnectionAttemptCompleted(boost::system::error_code error,
                                                const boost::asio::ip::tcp::resolver::iterator &endpoint);

        /**
         * Handle called when bytes have been received through the socket.
         * @param error The error that has occurred if any during the bytes reading.
         * @param byteTransferred The number of byte transferred during the reading.
         */
        void onBytesReceived(const boost::system::error_code &error, unsigned int byteTransferred);

        /**
         * Insert a range of byte inside the extraction buffer.
         * @tparam Iterator The type of iterator of the byte range:
         * Iterator must meet the requirements of EmplaceConstructible in order to use overload.
         * Iterator must meet the requirements of MoveAssignable and MoveInsertable in order to use overload (4).
         * required only if InputIt satisfies LegacyInputIterator but not LegacyForwardIterator (until C++17).
         * Iterator must meet the requirements of Swappable, MoveAssignable, MoveConstructible and MoveInsertable
         * in order to use overload (4,5) (since C++17).
         * @param begin The start of the byte range to insert.
         * @param end The end of the byte range to insert.
         */
        template<typename Iterator>
        auto insertByteRangeIntoExtractionBuffer(Iterator begin, Iterator end) {
            extractionByteBuffer.insert(std::cend(extractionByteBuffer), begin, end);
            return std::make_pair(std::cbegin(extractionByteBuffer),
                                  std::cend(extractionByteBuffer));
        }

        /**
         * Remove a range of byte from the extraction buffer between the start of the range and a position within
         * the buffer. If the position is equals to end, the buffer is cleared.
         * @tparam Iterator Iterator must  meet the requirements of MoveAssignable.
         * @param begin Beginning of the range within the extraction buffer to remove.
         * @param position Position within the extraction buffer until which to remove the bytes.
         */
        template<typename Iterator>
        void removeUsedByteRangeFromExtractionBufferBeginningUntil(Iterator position) noexcept {
            const auto remainingBytes{std::distance(position, std::cend(extractionByteBuffer))};
            if (remainingBytes) {
                extractionByteBuffer.erase(std::cbegin(extractionByteBuffer), position);
            } else {
                extractionByteBuffer.clear();
            }
        }

        /**
         * Try to extract a full scan given a range of bytes.
         * @tparam Iterator The type of iterator of the range.
         * @param begin Start of the range to extract from.
         * @param end End of the range to extract from.
         * @return A tuple containing:
         * - An iterator pointing to:
         *      - The position to start the next extraction of the next scan if the extraction has succeeded.
         *      - The start of the scan if the extraction has failed because there isn't enough byte to contain
         *      the full scan.
         *      - The end of the range if no scan has been found.
         * - The number of byte to transfer to receive the next scan. If there isn't enough byte to get the full scan,
         * this field is equals to the number of byte needed to transfer for getting the complete scan.
         * - An optional containing the number of bytes needed to contain a full scan. This value is set only when
         * a full scan has been received, so its size can be computed.
         */
        template<typename Iterator>
        std::tuple<Iterator, unsigned int, std::optional<unsigned int>>
        tryExtractingScanFromByteRange(Iterator begin, Iterator end) noexcept(false) {
            auto position{begin};
            auto numberOfMissingBytes{0u};
            std::optional<unsigned int> bufferSizeNeededToContainAFullScanOptional{std::nullopt};
            for (;;) {
                const auto extractionResult{extractScanPacketFromByteRange(position, end, scanFactory)};
                const auto hadEnoughBytes{std::get<0>(extractionResult)};
                position = std::get<1>(extractionResult);
                numberOfMissingBytes = std::get<2>(extractionResult);
                if (scanFactory.isComplete()) {
                    const auto bufferSizeNeededToContainAFullScan{
                            computeBoundedRequiredBufferSizeToHostAFullScan(scanFactory)};
                    bufferSizeNeededToContainAFullScanOptional = bufferSizeNeededToContainAFullScan;
                    setOutputScanFromCompletedFactory(*scanFactory);
                }
                if (!hadEnoughBytes) {
                    break;
                }
            }
            const auto bufferCapacity{(unsigned int) receptionByteBuffer.capacity()};
            const auto numberOfBytesToTransfer{
                    numberOfMissingBytes ? std::min(numberOfMissingBytes, bufferCapacity)
                                         : bufferCapacity};
            return std::make_tuple(position, numberOfBytesToTransfer, bufferSizeNeededToContainAFullScanOptional);
        }

        /**
         * Resize the internal reception buffer to a given size and the extraction buffer to 1.5 times the new size
         * if and only if the new size bigger than the current reception buffer capacity.
         * @param newSize : The new size to adopt if bigger than the current capacity.
         */
        inline void resizeReceptionAndExtractionBuffers(const unsigned int newSize) noexcept(false) {
            const auto bufferCapacity{receptionByteBuffer.capacity()};
            if (newSize > bufferCapacity) {
                receptionByteBuffer = internals::Types::Buffer(newSize, 0);
                extractionByteBuffer.reserve(std::floor(newSize * 1.5));
            }
        }

        /**
         * Compute the number of bytes needed to receive a full scan given a scan factory. The number of byte returned
         * is within the bound [DEFAULT_RECEPTION_BUFFER_SIZE;MAX_RECEPTION_BUFFER_SIZE].
         * @param scanFactory The scan factory to use as reference to estimate the number of bytes needed.
         * @return An estimation of the number of bytes needed to host a full scan. The returned value is bounded.
         */
        [[nodiscard]] static inline unsigned int
        computeBoundedRequiredBufferSizeToHostAFullScan(const ScanFactory &scanFactory) {
            std::vector<Data::Header> headers{};
            scanFactory.getHeaders(headers);
            auto byteSize{0u};
            for (const auto &header: headers) {
                byteSize += header.packetSize;
            }
            return std::min(MAX_RECEPTION_BUFFER_SIZE, std::max(byteSize, DEFAULT_RECEPTION_BUFFER_SIZE));
        }

    private:
        boost::asio::io_service ioService{};
        std::unique_ptr<boost::asio::ip::tcp::socket> socket{nullptr};
        internals::Types::Buffer receptionByteBuffer{};
        internals::Types::Buffer extractionByteBuffer{};
        std::future<void> ioServiceTask{};
        TcpScanFactory scanFactory{};
        ConditionVariable interruptSocketAsyncOpsCv{};
        LockType interruptSocketAsyncOpsCvLock{};
        std::atomic_bool interruptSocketAsyncOpsFlag{false};
        boost::asio::ip::tcp::resolver resolver{ioService};
        boost::asio::ip::tcp::resolver::results_type endPoints{};
    };
} // namespace Device