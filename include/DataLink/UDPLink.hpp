//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#pragma once

#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/ip/udp.hpp>
#include "DataLink.hpp"

namespace Device {
    class R2000;

    class UDPLink : public DataLink {
        static constexpr unsigned int DATAGRAM_SIZE{65535u};
    private:
        class UdpScanFactory : public ScanFactory {
            using Packet = std::tuple<Data::Header, std::vector<uint32_t>, std::vector<uint32_t>>;
        public:
            UdpScanFactory() = default;

            /**
             * Add a new packet to the scan being assembled.
             * @param header The header of the packet.
             * @param inputDistances The distances vector  of the points contained into the packet.
             * @param inputAmplitudes The amplitudes vector of the points contained into the packet.
             */
            inline void addPacket(const Data::Header &header, std::vector<std::uint32_t> &inputDistances,
                                  std::vector<std::uint32_t> &inputAmplitudes) override {
                numberOfPoints += inputDistances.size();
                packets.emplace_back(std::make_tuple(
                        header,
                        std::move(inputDistances),
                        std::move(inputAmplitudes)
                ));
            }

            /**
             * @return True if the factory is empty (no packet stored), False otherwise.
             */
            [[nodiscard]] inline bool empty() const override { return packets.empty(); }

            /**
             * @return True if the factory holds enough packet to generate a complete scan, False otherwise.
             */
            [[nodiscard]] inline bool isComplete() const override {
                if (empty())
                    return false;
                const auto &header{std::get<0>(packets.back())};
                const auto numPointsScan{header.numPointsScan};
                return !empty() && numberOfPoints >= numPointsScan;
            }

            /**
             * @return Assembles and returns a scan from the stored packets. Subsequent call to this method will not
             * yield a scan as the internals buffers are cleared every time this function is called.
             */
            inline SharedScan operator*() override {
                std::sort(std::begin(packets),
                          std::end(packets),
                          [](const Packet &lhs, const Packet &rhs) {
                              const auto &lhsHeader{std::get<0>(lhs)};
                              const auto &rhsHeader{std::get<0>(rhs)};
                              return lhsHeader.packetNumber < rhsHeader.packetNumber;
                          });
                const auto numberOfHeaders{packets.size()};
                std::vector<uint32_t> accumulatedDistances{}, accumulatedAmplitudes{};
                std::vector<Data::Header> accumulatedHeaders{};
                accumulatedHeaders.reserve(numberOfHeaders);
                accumulatedAmplitudes.reserve(numberOfPoints);
                accumulatedAmplitudes.reserve(numberOfPoints);
                for (const auto &packet: packets) {
                    const auto &header{std::get<0>(packet)};
                    const auto &distances{std::get<1>(packet)};
                    const auto &amplitudes{std::get<2>(packet)};
                    accumulatedHeaders.push_back(header);
                    accumulatedDistances.insert(std::end(accumulatedDistances),
                                                std::cbegin(distances), std::cend(distances));
                    accumulatedAmplitudes.insert(std::end(accumulatedAmplitudes),
                                                 std::cbegin(amplitudes), std::cend(amplitudes));
                }
                auto scan{
                        std::make_shared<Data::Scan>(accumulatedDistances, accumulatedAmplitudes, accumulatedHeaders,
                                                     std::chrono::steady_clock::now())};
                clear();
                return scan;
            }

            /**
             * Clear the internals buffers and remove all packets stored if any.
             */
            inline void clear() override {
                numberOfPoints = 0u;
                packets.clear();
            }

            /**
             * @param scanNumber The scan number to test.
             * @return True if the packets stored by this factory belong to a given scan number.
             */
            inline bool operator==(const unsigned int scanNumber) const override {
                if (empty())
                    return false;
                const auto &header{std::get<0>(packets.back())};
                return header.scanNumber == scanNumber;
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
                outputHeaders.reserve(packets.size());
                for (const auto &packet: packets) {
                    const auto &header{std::get<0>(packet)};
                    outputHeaders.emplace_back(header);
                }
            }

        private:
            std::vector<Packet> packets{};
            unsigned int numberOfPoints{0u};
        };

    public:
        /**
         * @param iDevice : The device to attach the DataLink to.
         * @param iHandle : The device handle obtained from the device.
         */
        UDPLink(std::shared_ptr<R2000> iDevice, std::shared_ptr<DeviceHandle> iHandle);

        /**
         * Shutdown and close the socket, then wait for the io service task to finish.
         */
        ~UDPLink() override;

    private:
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
        inline void insertByteRangeIntoExtractionBuffer(Iterator begin, Iterator end) noexcept {
            extractionByteBuffer.insert(std::cend(extractionByteBuffer), begin, end);
        }

        /**
         * Remove a range of byte from the extraction buffer between the start of the range and a position within
         * the buffer. If the position is equals to end, the buffer is cleared.
         * @tparam Iterator Iterator must  meet the requirements of MoveAssignable.
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
         * @return An iterator pointing to:
         * - The position to start the next extraction of the next scan if the extraction has succeeded.
         * - The start of the scan if the extraction has failed due to a lack of bytes to contain the full scan.
         * - The end of the range if no scan has been found.
         */
        template<typename Iterator>
        Iterator tryExtractingScanFromByteRange(Iterator begin, Iterator end) noexcept(false) {
            auto position{begin};
            for (;;) {
                const auto extractionResult{extractScanPacketFromByteRange(position, end, scanFactory)};
                const auto hadEnoughBytes{std::get<0>(extractionResult)};
                position = std::get<1>(extractionResult);
                if (scanFactory.isComplete()) {
                    setOutputScanFromCompletedFactory(*scanFactory);
                }
                if (!hadEnoughBytes) {
                    break;
                }
            }
            return position;
        }

    private:
        boost::asio::io_service ioService{};
        boost::asio::ip::udp::socket socket{ioService, boost::asio::ip::udp::v4()};
        boost::asio::ip::udp::endpoint endPoint{};
        internals::Types::Buffer receptionByteBuffer{};
        internals::Types::Buffer extractionByteBuffer{};
        std::future<void> ioServiceTask{};
        UdpScanFactory scanFactory{};
    };
}