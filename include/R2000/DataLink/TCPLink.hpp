//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#pragma once

#include "DataLink.hpp"
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
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

            inline void getHeaders(std::vector<Data::Header> &outputHeaders) const override {
                outputHeaders.insert(std::end(outputHeaders),
                                     std::begin(headers),
                                     std::end(headers));
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
        TCPLink(std::shared_ptr<R2000> iDevice, std::shared_ptr<DeviceHandle> queriedEndpoints) noexcept(false);

        ~TCPLink() override;

    private:

        /**
         *
         */
        void asynchronousResolveEndpoints();

        /**
         *
         * @tparam Iterator
         * @param begin
         * @param end
         */
        template<typename Iterator>
        void asynchronousConnectSocket(Iterator begin, Iterator end) {
            boost::system::error_code placeholder{};
            socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, placeholder);
            socket->close(placeholder);
            boost::asio::async_connect(*socket, begin, end,
                                       [this](boost::system::error_code error,
                                              boost::asio::ip::tcp::resolver::iterator endpoint) {
                                           handleSocketConnection(error, endpoint);
                                       });
        }

        /**
         *
         * @param error
         * @param endpoint_iter
         */
        void handleSocketConnection(boost::system::error_code error,
                                    const boost::asio::ip::tcp::resolver::iterator &endpoint);

        /**
         *
         * @param error
         * @param byteTransferred
         */
        void handleBytesReception(const boost::system::error_code &error, unsigned int byteTransferred);

        template<typename Iterator>
        auto insertByteRangeIntoExtractionBuffer(Iterator begin, Iterator end) {
            extractionByteBuffer.insert(std::cend(extractionByteBuffer), begin, end);
            return std::make_pair(std::cbegin(extractionByteBuffer),
                                  std::cend(extractionByteBuffer));
        }

        /**
         *
         * @tparam Iterator
         * @param begin
         * @param position
         * @param end
         */
        template<typename Iterator>
        void removeUsedByteRange(Iterator begin, Iterator position, Iterator end) {
            const auto remainingBytes{std::distance(position, end)};
            if (remainingBytes) {
                extractionByteBuffer.erase(begin, position);
            } else {
                extractionByteBuffer.clear();
            }
        }

        /**
         *
         * @tparam Iterator
         * @param begin
         * @param end
         * @return
         */
        template<typename Iterator>
        auto tryExtractingScanFromByteRange(Iterator begin, Iterator end) {
            auto position{begin};
            auto numberOfMissingBytes{0u};
            for (;;) {
                const auto extractionResult{extractScanPacketFromByteRange(position, end, scanFactory)};
                const auto hadEnoughBytes{std::get<0>(extractionResult)};
                position = std::get<1>(extractionResult);
                numberOfMissingBytes = std::get<2>(extractionResult);
                if (scanFactory.isComplete()) {
                    const auto bufferSizeNeededToContainAFullScan{computeBoundedRequiredBufferSize(scanFactory)};
                    resizeReceptionBuffer(bufferSizeNeededToContainAFullScan);
                    setOutputScanFromCompletedFactory(scanFactory);
                }
                if (!hadEnoughBytes)
                    break;
            }
            const auto bufferCapacity{(unsigned int) receptionByteBuffer.capacity()};
            const auto numberOfBytesToTransfer{
                    numberOfMissingBytes ? std::min(numberOfMissingBytes, bufferCapacity)
                                         : bufferCapacity};
            return std::make_pair(position, numberOfBytesToTransfer);
        }

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

    private:
        std::future<void> socketConnectionTask{};
        boost::asio::io_service ioService{};
        std::unique_ptr<boost::asio::ip::tcp::socket> socket{nullptr};
        internals::Types::Buffer receptionByteBuffer{};
        internals::Types::Buffer extractionByteBuffer{};
        std::future<void> ioServiceTask{};
        TcpScanFactory scanFactory{};
        Cv interruptAsyncOpsCv{};
        LockType interruptAsyncOpsCvLock{};
        std::atomic_bool interruptAsyncOpsFlag{false};
        /////////////////////////////////////////////////////////////
        boost::asio::steady_timer deadline{ioService};
        boost::asio::ip::tcp::resolver resolver{ioService};
        boost::asio::ip::tcp::resolver::results_type endPoints{};
    };
} // namespace Device