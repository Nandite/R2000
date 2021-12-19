//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#pragma once

#include "DataLink.hpp"
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <memory>
#include <thread>

namespace Device {
    class R2000;

    struct DeviceHandle;

    class TCPLink : public DataLink {
        static constexpr unsigned int DEFAULT_RECEPTION_BUFFER_SIZE{4096u};
        static constexpr unsigned int MAX_RECEPTION_BUFFER_SIZE{32768u};
        static constexpr unsigned int EXTRACTION_BUFFER_SIZE{DEFAULT_RECEPTION_BUFFER_SIZE * 4};
    private:
        class TcpScanFactory : public ScanFactory {
        public:
            TcpScanFactory() = default;
            inline void addPacket(const Data::Header &header, std::vector<std::uint32_t> &inputDistances,
                                   std::vector<std::uint32_t> &inputAmplitudes) override {
                distances.insert(std::end(distances), std::cbegin(inputDistances), std::cend(inputDistances));
                amplitudes.insert(std::end(amplitudes), std::cbegin(inputAmplitudes), std::cend(inputAmplitudes));
                headers.emplace_back(header);
            }
            [[nodiscard]] inline bool empty() const override { return headers.empty(); }
            [[nodiscard]] inline bool isComplete() const override {
                return !empty() && distances.size() >= headers[0].numPointsScan;
            }
            inline Data::Scan operator*() override {
                Data::Scan scan{distances, amplitudes, headers, std::chrono::steady_clock::now()};
                distances.clear();
                amplitudes.clear();
                headers.clear();
                return scan;
            }
            inline void clear() override {
                distances.clear();
                amplitudes.clear();
                headers.clear();
            }
            [[nodiscard]] inline bool isDifferentScan(const Data::Header &header) const override {
                return (*this) != header.scanNumber;
            }
            [[nodiscard]] inline bool isNewScan(const Data::Header &header) override {
                return header.scanNumber == 1;
            }
            inline bool operator==(const unsigned int scanNumber) const override {
                return !empty() && headers.back().scanNumber == scanNumber;
            }
            inline bool operator!=(const unsigned int scanNumber) const override { return !((*this) == scanNumber); }
            inline void getHeaders(std::vector<Data::Header> & outputHeaders) const override {
                outputHeaders.insert(std::end(outputHeaders),
                                     std::begin(headers),
                                     std::end(headers));
            }

        private:

            // Distance data in polar form in millimeter
            std::vector<std::uint32_t> distances{};
            // Amplitude data in the range 32-4095, values lower than 32 indicate an error or undefined values
            std::vector<std::uint32_t> amplitudes{};
            // Header received ParameterChaining the distance and amplitude data
            std::vector<Data::Header> headers{};
        };
    public:
        TCPLink(std::shared_ptr<R2000> device, std::shared_ptr<DeviceHandle> handle) noexcept(false);
        [[nodiscard]] inline Data::Scan getLastScan() const override {
            RealtimeScan::ScopedAccess <farbot::ThreadType::nonRealtime> scanGuard(*realtimeScan);
            return *scanGuard;
        }
        ~TCPLink() override;

    private:

        /**
         *
         * @param error
         * @param byteTransferred
         */
        void handleBytesReception(const boost::system::error_code &error, unsigned int byteTransferred);


        /**
         *
         * @return
         */
        inline void resizeReceptionBuffer(const unsigned int newSize) {
            const auto bufferCapacity{receptionByteBuffer.capacity()};
            if (bufferCapacity != newSize) {
                receptionByteBuffer = internals::Types::Buffer(newSize, 0);
                extractionByteBuffer.reserve(std::floor(newSize * 1.5));
            }
        }

        /**
         *
         * @param scanFactory
         * @return
         */
        [[nodiscard]] static inline unsigned int
        computeBoundedRequiredBufferSize(const ScanFactory &scanFactory) {
            std::vector<Data::Header> headers{};
            scanFactory.getHeaders(headers);
            auto byteSize{0u};
            for (const auto &header : headers)
                byteSize += header.packetSize;
            return std::min(MAX_RECEPTION_BUFFER_SIZE, std::max(byteSize, DEFAULT_RECEPTION_BUFFER_SIZE));
        }

        /**
         *
         */
        void inline releaseResources() {
            if (!ioService.stopped())
                ioService.stop();
            if (ioServiceThread.joinable())
                ioServiceThread.join();
            boost::system::error_code placeholder;
            socket->shutdown(boost::asio::ip::tcp::socket::shutdown_receive, placeholder);
            socket->close(placeholder);
            mIsConnected.store(false, std::memory_order_release);
        }

    private:
        boost::asio::io_service ioService{};
        std::unique_ptr<boost::asio::ip::tcp::socket> socket{nullptr};
        internals::Types::Buffer receptionByteBuffer{};
        internals::Types::Buffer extractionByteBuffer{};
        std::thread ioServiceThread{};
        std::unique_ptr<RealtimeScan> realtimeScan{std::make_unique<RealtimeScan>(Data::Scan())};
        TcpScanFactory scanFactory{};
    };
} // namespace Device