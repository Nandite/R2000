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

    class DeviceHandle;

    class TCPLink : public DataLink {
        static constexpr unsigned int DEFAULT_RECEPTION_BUFFER_SIZE{4096u};
        static constexpr unsigned int MAX_RECEPTION_BUFFER_SIZE{32768u};
        static constexpr unsigned int EXTRACTION_BUFFER_SIZE{DEFAULT_RECEPTION_BUFFER_SIZE * 4};
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
            inline Data::Scan operator*() override {
                Data::Scan scan{distances, amplitudes, headers, std::chrono::steady_clock::now()};
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
        /**
         *
         * @param iDevice
         * @param iHandle
         */
        TCPLink(std::shared_ptr<R2000> iDevice, std::shared_ptr<DeviceHandle> iHandle) noexcept(false);

        /**
         *
         */
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
            boost::asio::async_connect(*socket, begin, end, [this](auto error, auto endpoint) {
                onSocketConnectionAttemptCompleted(error, endpoint);
            });
        }

        /**
         *
         * @param error
         * @param endpoint_iter
         */
        void onSocketConnectionAttemptCompleted(boost::system::error_code error,
                                                const boost::asio::ip::tcp::resolver::iterator &endpoint);

        /**
         *
         * @param error
         * @param byteTransferred
         */
        void onBytesReceived(const boost::system::error_code &error, unsigned int byteTransferred);

        /**
         *
         * @tparam Iterator
         * @param begin
         * @param end
         * @return
         */
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
                    const auto bufferSizeNeededToContainAFullScan{
                            computeBoundedRequiredBufferSizeToHostAFullScan(scanFactory)};
                    resizeReceptionBuffer(bufferSizeNeededToContainAFullScan);
                    setOutputScanFromCompletedFactory(*scanFactory);
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
        computeBoundedRequiredBufferSizeToHostAFullScan(const ScanFactory &scanFactory) {
            std::vector<Data::Header> headers{};
            scanFactory.getHeaders(headers);
            auto byteSize{0u};
            for (const auto &header : headers)
                byteSize += header.packetSize;
            return std::min(MAX_RECEPTION_BUFFER_SIZE, std::max(byteSize, DEFAULT_RECEPTION_BUFFER_SIZE));
        }

    private:
        boost::asio::io_service ioService{};
        std::unique_ptr<boost::asio::ip::tcp::socket> socket{nullptr};
        internals::Types::Buffer receptionByteBuffer{};
        internals::Types::Buffer extractionByteBuffer{};
        std::future<void> ioServiceTask{};
        TcpScanFactory scanFactory{};
        Cv interruptAsyncOpsCv{};
        LockType interruptAsyncOpsCvLock{};
        std::atomic_bool interruptAsyncOpsFlag{false};
        boost::asio::steady_timer deadline{ioService};
        boost::asio::ip::tcp::resolver resolver{ioService};
        boost::asio::ip::tcp::resolver::results_type endPoints{};
    };
} // namespace Device