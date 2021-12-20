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
            inline void addPacket(const Data::Header &header, std::vector<std::uint32_t> &inputDistances,
                                  std::vector<std::uint32_t> &inputAmplitudes) override {
                numberOfPoints += inputDistances.size();
                packets.emplace_back(std::make_tuple(
                        header,
                        std::move(inputDistances),
                        std::move(inputAmplitudes)
                        ));
            }
            [[nodiscard]] inline bool empty() const override { return packets.empty(); }
            [[nodiscard]] inline bool isComplete() const override {
                if(empty())
                    return false;
                const auto &header{std::get<0>(packets.back())};
                const auto numPointsScan{header.numPointsScan};
                return !empty() && numberOfPoints >= numPointsScan;
            }

            inline Data::Scan operator*() override {
                std::sort(std::begin(packets),
                          std::end(packets),
                          [](const Packet &lhs, const Packet &rhs) {
                              const auto &lhsHeader{std::get<0>(lhs)};
                              const auto &rhsHeader{std::get<0>(rhs)};
                              return lhsHeader.packetNumber < rhsHeader.packetNumber;
                          });
                std::vector<uint32_t> accumulatedDistances, accumulatedAmplitudes;
                std::vector<Data::Header> accumulatedHeaders;
                for (const auto &packet : packets) {
                    const auto &header{std::get<0>(packet)};
                    const auto &distances{std::get<1>(packet)};
                    const auto &amplitudes{std::get<2>(packet)};
                    accumulatedHeaders.push_back(header);
                    accumulatedDistances.insert(std::end(accumulatedDistances),
                                                std::cbegin(distances), std::cend(distances));
                    accumulatedAmplitudes.insert(std::end(accumulatedAmplitudes),
                                                 std::cbegin(amplitudes), std::cend(amplitudes));
                }
                clear();
                Data::Scan scan{accumulatedDistances, accumulatedAmplitudes, accumulatedHeaders,
                                std::chrono::steady_clock::now()};
                return scan;
            }
            inline void clear() override {
                numberOfPoints = 0u;
                packets.clear();
            }
            [[nodiscard]] inline bool isDifferentScan(const Data::Header &header) const override {
                return (*this) != header.scanNumber;
            }
            [[nodiscard]] inline bool isNewScan(const Data::Header &header) override {
                return header.scanNumber == 1;
            }
            inline bool operator==(const unsigned int scanNumber) const override {
                if(empty())
                    return false;
                const auto &header{std::get<0>(packets.back())};
                return header.scanNumber == scanNumber;
            }
            inline bool operator!=(const unsigned int scanNumber) const override { return !((*this) == scanNumber); }
            inline void getHeaders(std::vector<Data::Header> & outputHeaders) const override {
                if(empty())
                    return;
                outputHeaders.reserve(packets.size());
                for(const auto & packet : packets)
                {
                    const auto & header{std::get<0>(packet)};
                    outputHeaders.emplace_back(header);
                }
            }

        private:
            std::vector<Packet> packets;
            unsigned int numberOfPoints{0u};
        };
    public:
        UDPLink(std::shared_ptr<R2000> device, std::shared_ptr<DeviceHandle> handle);
        [[nodiscard]] inline Data::Scan getLastScan() const override{
            RealtimeScan::ScopedAccess <farbot::ThreadType::nonRealtime> scanGuard(*realtimeScan);
            return *scanGuard;
        };
        ~UDPLink() override;
    private:
        /**
         *
         * @param error
         * @param byteTransferred
         */
        void handleBytesReception(const boost::system::error_code &error, unsigned int byteTransferred);

    private:
        boost::asio::io_service ioService{};
        std::unique_ptr<boost::asio::ip::udp::socket> socket{nullptr};
        boost::asio::ip::udp::endpoint endPoint{};
        internals::Types::Buffer receptionByteBuffer{};
        internals::Types::Buffer extractionByteBuffer{};
        std::thread ioServiceThread{};
        std::unique_ptr<RealtimeScan> realtimeScan{std::make_unique<RealtimeScan>(Data::Scan())};
        UdpScanFactory scanFactory{};
    };
}