//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#pragma once

#include "Data.hpp"
#include "Farbot.hpp"
#include "R2000/R2000.hpp"
#include "R2000/Control/Parameters.hpp"
#include <atomic>
#include <cassert>
#include <climits>
#include <cmath>
#include <condition_variable>
#include <future>
#include <mutex>
#include <optional>
#include <utility>

namespace Device {

    class DeviceHandle;
    namespace internals {
        namespace Types {
            using Byte = unsigned char;
            using Buffer = std::vector<Byte>;
        } // namespace Types
        const static std::uint16_t PACKET_MAGIC_START = 0xa25c;

        /**
         * Search a packet start magic through a range of byte delimited by two iterators.
         * @tparam Iterator An iterator type that meet the requirements of LegacyInputIterator.
         * @param begin Start of the byte range to search through.
         * @param end End of the byte range to search through.
         * @return An optional containing a pair iterators:
         * - A first iterator pointing at the start of the magic bytes.
         * - A second iterator pointing at the byte immediately following the magic bytes.
         * If no magic has been found, an empty optional is returned.
         */
        template<typename Iterator>
        static std::optional<std::pair<Iterator, Iterator>> retrievePacketMagic(Iterator begin, Iterator end) {
            constexpr auto byteSizeOf8Bits{sizeof(std::uint8_t)};
            static_assert(byteSizeOf8Bits == 1, "The size of uint8_t must be exactly 1 byte");
            auto position{begin};
            auto lastByteBeforeEnd{std::prev(end, 1)};
            while (position != lastByteBeforeEnd) {
                const auto uint16Value{std::uint16_t(*position) | (std::uint16_t(*(position + 1)) << 8)};
                if (uint16Value == PACKET_MAGIC_START)
                    return {std::make_pair(position, std::next(position, 2))};
                std::advance(position, byteSizeOf8Bits);
            }
            return std::nullopt;
        }

        /**
         * Search a packet header through a range of byte delimited by two iterators.
         * @tparam Iterator An iterator type that meet the requirements of LegacyInputIterator.
         * @param begin Start of the byte range to search through.
         * @param end End of the byte range to search through.
         * @return An optional containing a pair of:
         * - An iterator pointing at the start of the header.
         * - The header object found.
         * if no header has been found, an empty optional is returned.
         */
        template<typename Iterator>
        static std::optional<std::pair<Iterator, Data::Header>> retrievePacketHeader(Iterator begin, Iterator end) {
            constexpr auto headerSize{sizeof(Data::Header)};
            const auto numberOfBytes{(unsigned int) std::distance(begin, end)};
            if (numberOfBytes < headerSize)
                return std::nullopt;
            const auto magic{retrievePacketMagic(begin, end)};
            if (!magic)
                return std::nullopt;
            const auto headerStart{(*magic).first};
            const auto header{Data::Header::fromByteRange(headerStart)};
            return {{headerStart, header}};
        }

        /**
         * Extract up to a given number of 32 bits unsigned integers from a byte range.
         * @tparam Iterator An iterator type that meet the requirements of LegacyInputIterator.
         * @tparam Callback Callback type to call for every integer extracted.
         * @param begin Start of the byte range to extract from.
         * @param end End of the byte range to extract from.
         * @param numberOfIntegersToExtract The maximum number of integers to extract.
         * @param callback Callback called for every integer extracted.
         * @return Iterator pointing on:
         * - The next byte following the last integer extracted if the maximum number of integers to extract has
         * been reached or if the remaining bytes after the last extraction is less than 4 bytes.
         * - end iterator if the length of the range contained exactly the number of integers to extract.
         */
        template<typename Iterator, typename Callback>
        static Iterator
        retrieve32BitsUnsignedIntegersFromByteRange(Iterator begin, Iterator end,
                                                    const unsigned int numberOfIntegersToExtract,
                                                    Callback callback) {
            constexpr auto byteSizeOf32Bits{sizeof(std::uint32_t)};
            static_assert(byteSizeOf32Bits == 4, "The size of uint32_t must be 4 bytes");
            const auto numberOfBytes{(long unsigned int) std::distance(begin, end)};
            auto position{begin};
            for (auto remainingBytes{numberOfBytes}, blockCount{(long unsigned int) 0u};
                 remainingBytes >= byteSizeOf32Bits && blockCount < numberOfIntegersToExtract;
                 remainingBytes = std::distance(position, end), ++blockCount) {
                const auto uint32Value{std::uint32_t(std::uint32_t(*position) |
                                                     (std::uint32_t(*(position + 1)) << 8) |
                                                     (std::uint32_t(*(position + 2)) << 16) |
                                                     (std::uint32_t(*(position + 3)) << 24))};
                std::advance(position, byteSizeOf32Bits);
                std::invoke(callback, uint32Value);
            }
            return position;
        }

        /**
         * Extract up to a given number of sequentially ordered 32 bits unsigned integers and 16 bits unsigned integers
         * from a byte range.
         * @tparam Iterator An iterator type that meet the requirements of LegacyInputIterator.
         * @tparam Callback Callback type to call for every pair of integers extracted.
         * @param begin Start of the byte range to extract from.
         * @param end End of the byte range to extract from.
         * @param numberOfIntegersToExtract The maximum number of pair of integers to extract.
         * @param callback Callback called for every integer extracted.
         * @return Iterator pointing on:
         * - The next byte following the last pair of integer extracted if the maximum number of integers to extract has
         * been reached or if the remaining bytes after the last extraction is less than 6 bytes.
         * - end iterator if the length of the range contained exactly the number of integers to extract.
         */
        template<typename Iterator, typename Callback>
        static Iterator
        retrieve48bitsUnsignedIntegersFromByteRange(Iterator begin, Iterator end,
                                                    const unsigned int numberOfIntegersToExtract,
                                                    Callback callback) {
            constexpr auto byteSizeOf32Bits{sizeof(std::uint32_t)};
            constexpr auto byteSizeOf16Bits{sizeof(std::uint16_t)};
            static_assert(byteSizeOf32Bits == 4, "The size of uint32_t must be 4 bytes");
            static_assert(byteSizeOf16Bits == 2, "The size of uint16_t must be 2 bytes");
            const auto numberOfBytes{(long unsigned int) std::distance(begin, end)};
            auto position{begin};
            for (auto remainingBytes{numberOfBytes}, blockCount{(long unsigned int) 0u};
                 remainingBytes >= byteSizeOf32Bits && blockCount < numberOfIntegersToExtract;
                 remainingBytes = std::distance(position, end), ++blockCount) {
                const auto uint32Value{std::uint32_t(std::uint32_t((*position)) |
                                                     (std::uint32_t(*(position + 1)) << 8) |
                                                     (std::uint32_t(*(position + 2)) << 16) |
                                                     (std::uint32_t(*(position + 3)) << 24))};
                std::advance(position, byteSizeOf32Bits);
                const auto uint16Value{std::uint16_t(*position) | (std::uint16_t(*(position + 1)) << 8)};
                std::advance(position, byteSizeOf16Bits);
                std::invoke(callback, uint32Value, uint16Value);
            }
            return position;
        }

        /**
         * Helper struct that extract a scan payload from a byte range.
         * @tparam Type The type of payload to extract from the range.
         */
        template<unsigned Type>
        struct PayloadExtractionFromByteRange {
            /**
             * Always throws a Device::NotImplementedException exception.
             * @tparam Iterator The type of iterator of the range.
             * @return Nothing, it throws.
             */
            template<typename Iterator>
            static inline Iterator retrievePayload(Iterator, Iterator, const unsigned int, std::vector<uint32_t> &,
                                                   std::vector<uint32_t> &) {
                throw Device::NotImplementedException("Unsupported type of packet payload: " + Type);
            }
        };

        /**
         * Specialization that extract payload to for scan of type A.
         * Such scan contains only distance values.
         */
        template<>
        struct PayloadExtractionFromByteRange<Data::underlyingType(Parameters::PACKET_TYPE::A)> {
            /**
             * Extract up to N number of points from a payload of type A through a byte range. This type contains only
             * the distance value for each point as 32 bits long unsigned integer.
             * @tparam Iterator An iterator type that meet the requirements of LegacyInputIterator.
             * @param begin Start of the byte range to extract from.
             * @param end End of the byte range to extract from.
             * @param numberOfPoints Maximum number of point to extract from the range.
             * @param distances Distances values extracted from the byte range. Invalid measure are represented by
             * NaN values.
             * @param amplitudes Amplitudes of the points. Contains only zeros.
             * @return Iterator pointing on:
             * - The next byte following the last point extracted if the maximum number of points to extract has
             * been reached or if the remaining bytes after the last extraction is less than the bytes size of a point.
             * - end iterator if the length of the range contained exactly the number of point to extract.
             */
            template<typename Iterator>
            static inline Iterator retrievePayload(Iterator begin, Iterator end, const unsigned int numberOfPoints,
                                                   std::vector<uint32_t> &distances,
                                                   std::vector<uint32_t> &amplitudes) {
                return retrieve32BitsUnsignedIntegersFromByteRange(begin, end, numberOfPoints,
                                                                   [&distances, &amplitudes](const uint32_t &distance) {
                                                                       const auto nanOrDistance =
                                                                               distance != 0xFFFFFFFF ? distance
                                                                                                      : std::numeric_limits<uint32_t>::quiet_NaN();
                                                                       distances.push_back(nanOrDistance);
                                                                       amplitudes.push_back(0);
                                                                   });
            }
        };

        /**
         * Specialization that extract payload to for scan of type B.
         * Such scan contains distances and amplitudes as 32 bits and 16 bits respectively long values.
         */
        template<>
        struct PayloadExtractionFromByteRange<Data::underlyingType(Parameters::PACKET_TYPE::B)> {
            /**
             * Extract up to N number of points from a payload of type B through a byte range.
             * @tparam Iterator An iterator type that meet the requirements of LegacyInputIterator.
             * @param begin Start of the byte range to extract from.
             * @param end End of the byte range to extract from.
             * @param numberOfPoints Maximum number of point to extract from the range.
             * @param distances Distances values extracted from the byte range. Invalid measure are represented
             * by NaN values.
             * @param amplitudes Amplitudes values extracted from the byte range.
             * @return Iterator pointing on:
             * - The next byte following the last point extracted if the maximum number of points to extract has
             * been reached or if the remaining bytes after the last extraction is less than the bytes size of a point
             * (distance + amplitude).
             * - end iterator if the length of the range contained exactly the number of point to extract.
             */
            template<typename Iterator>
            static inline Iterator retrievePayload(Iterator begin, Iterator end, const unsigned int numberOfPoints,
                                                   std::vector<uint32_t> &distances,
                                                   std::vector<uint32_t> &amplitudes) {
                return retrieve48bitsUnsignedIntegersFromByteRange(begin, end, numberOfPoints,
                                                                   [&distances, &amplitudes](const uint32_t &distance,
                                                                                             const uint16_t &amplitude) {
                                                                       const auto nanOrDistance =
                                                                               distance != 0xFFFFFFFF ? distance
                                                                                                      : std::numeric_limits<uint32_t>::quiet_NaN();
                                                                       distances.push_back(nanOrDistance);
                                                                       amplitudes.push_back(amplitude);
                                                                   });
            }
        };

        /**
         * Specialization that extract payload to for scan of type C.
         * Such scan contains distances and amplitudes packed into a 32 bits length unsigned integer.
         */
        template<>
        struct PayloadExtractionFromByteRange<Data::underlyingType(Parameters::PACKET_TYPE::C)> {
            /**
             * Extract up to N number of points from a payload of type C through a byte range.
             * @tparam Iterator An iterator type that meet the requirements of LegacyInputIterator.
             * @param begin Start of the byte range to extract from.
             * @param end End of the byte range to extract from.
             * @param numberOfPoints Maximum number of point to extract from the range.
             * @param distances Distances values extracted from the byte range. Invalid measure are represented
             * by NaN values.
             * @param amplitudes Amplitudes values extracted from the byte range.
             * @return Iterator pointing on:
             * - The next byte following the last point extracted if the maximum number of points to extract has
             * been reached or if the remaining bytes after the last extraction is less than the bytes size of a point
             * (distance + amplitude packed).
             * - end iterator if the length of the range contained exactly the number of point to extract.
             */
            template<typename Iterator>
            static inline Iterator retrievePayload(Iterator begin, Iterator end, const unsigned int numberOfPoints,
                                                   std::vector<uint32_t> &distances,
                                                   std::vector<uint32_t> &amplitudes) {
                return retrieve32BitsUnsignedIntegersFromByteRange(begin, end, numberOfPoints,
                                                                   [&distances, &amplitudes](const uint32_t &point) {
                                                                       const auto distance{point & 0x000FFFFF};
                                                                       const auto amplitude{(point & 0xFFFFF000) >> 20};
                                                                       const auto nanOrDistance =
                                                                               distance != 0xFFFFF ? distance
                                                                                                   : std::numeric_limits<uint32_t>::quiet_NaN();
                                                                       distances.push_back(nanOrDistance);
                                                                       amplitudes.push_back(amplitude);
                                                                   });
            }
        };
    } // namespace internals
    class DataLink {
    protected:

        /**
         * Helper class that assemble a scan from different packets received over a transport
         * medium (i.e TCP or UDP). Depending on the medium of transport, order of reception is not
         * guaranteed. Hence, each derivation of the DataLink for a given transport must also derive the
         * ScanFactory and provides the logic necessary to store an reassemble the scan from the different packets.
         * Using this approach, the DataLink provides base algorithm logic that can retrieve the packets and
         * assemble them using a Factory without taking care of how packet has to be assembled for any given medium.
         */
        class ScanFactory {
        public:
            /**
             * Add a new packet received to the factory for future assembly.
             * @param header The header of the packet received.
             * @param inputDistances The distance values of the packet.
             * @param inputAmplitudes The amplitude values of the packet.
             */
            inline virtual void addPacket(const Data::Header &header, std::vector<std::uint32_t> &inputDistances,
                                          std::vector<std::uint32_t> &inputAmplitudes) = 0;

            /**
             * @return True if the factory is empty (no packet stored), False otherwise.
             */
            [[nodiscard]] virtual inline bool empty() const = 0;

            /**
             * @return True if the factory holds enough packet to generate a complete scan, False otherwise.
             */
            [[nodiscard]] virtual inline bool isComplete() const = 0;

            /**
             * @return Assembles and returns a scan from the stored packets. If the isComplete method evaluates to True,
             * this method must return a full scan. Subsequent call of this method may not return a complete scan as
             * the internal stored packets can be flushed after a scan is assembled.
             */
            virtual inline Data::Scan operator*() = 0;

            /**
             * Clear any packet stored by the factory and prepare it to store packets from a different scan.
             */
            virtual inline void clear() = 0;

            /**
             * @param header A packet header to check against the stored packets.
             * @return True if the header belong to a scan different from the packets stored by the factory,
             * False otherwise.
             */
            [[nodiscard]] virtual inline bool isDifferentScan(const Data::Header &header) const {
                return (*this) != header.scanNumber;
            }

            /**
             * @param header The header to check.
             * @return True if the packet header is the beginning of a new scan.
             */
            [[nodiscard]] virtual inline bool isNewScan(const Data::Header &header) {
                return header.packetNumber == 1;
            }

            /**
             * @param scanNumber The scan number to test.
             * @return True if the packets stored by this factory belong to a given scan number.
             */
            virtual inline bool operator==(unsigned int scanNumber) const = 0;

            /**
             * @param scanNumber The scan number to test.
             * @return True if the packets stored by this factory don't belong to a given scan number.
             */
            virtual inline bool operator!=(unsigned int scanNumber) const = 0;

            /**
             * Get the headers of all the packet currently stored by the factory.
             * @param outputHeaders The vector to store the headers into.
             */
            virtual inline void getHeaders(std::vector<Data::Header> &outputHeaders) const = 0;
        };

    protected:
        using LockType = std::mutex;
        using Cv = std::condition_variable;
        using RealtimeScan = farbot::RealtimeObject<Data::Scan, farbot::RealtimeObjectOptions::realtimeMutatable>;
        static constexpr unsigned int MAX_RESERVE_POINTS_BUFFER{1024u};
    protected:

        /**
         * @param iDevice : The device to attach the DataLink to.
         * @param iHandle : The device handle obtained from the device.
         * @param connectionTimeout : Maximum time allowed for the command issued by DataLink to the device
         * to initiate the scan.
         */
        DataLink(std::shared_ptr<R2000> iDevice, std::shared_ptr<DeviceHandle> iHandle,
                 std::chrono::milliseconds connectionTimeout);

        /**
         * Watchdog task to keep alive the connection between the device and the host.
         * @param commandTimeout The commandTimeout at which a keep alive signal is emitted.
         */
        void watchdogTask(std::chrono::seconds commandTimeout);

        /**
         * Search through a byte range and extract a complete scan packet, then store it into a ScanFactory.
         * @tparam Iterator An iterator type that meet the requirements of LegacyInputIterator.
         * @param begin Start of the byte range to search and extract from.
         * @param end End of the byte range to search and extract from.
         * @param scanFactory An implementation of a scan factory to use for scan assembly.
         * @return A tuple of three elements containing:
         * - A bool at True if a packet has been extracted, False if a packet cannot be found or the byte range does not
         * contains the entirety of the packet.
         * - An iterator pointing to end if the range does not contains a packet, pointing to the start of the packet
         * if the range does not contains the entirety of the packet, pointing to end or to the next byte following the
         * end of the packet if the whole packet has been extracted.
         * - An integer at 0 if a packet cannot be found or if an entire packet has been extracted, at N if a packet has
         * been found but its not all contained in the range, N being the number of missing bytes needed to
         * extract the entire packet.
         */
        template<typename Iterator>
        static std::tuple<bool, Iterator, unsigned int>
        extractScanPacketFromByteRange(Iterator begin, Iterator end, ScanFactory &scanFactory) noexcept(false) {
            static constexpr auto A{Data::underlyingType(Parameters::PACKET_TYPE::A)};
            static constexpr auto B{Data::underlyingType(Parameters::PACKET_TYPE::B)};
            static constexpr auto C{Data::underlyingType(Parameters::PACKET_TYPE::C)};

            auto result{internals::retrievePacketHeader(begin, end)};
            if (!result)
                return {false, end, 0};

            const auto &header{result->second};
            const auto headerSize{header.headerSize};
            const auto payloadStart{std::next(result->first, headerSize)};
            const auto numberOfPoints{header.numPointsPacket};
            const auto packetType{header.packetType};
            const auto payloadSize{header.packetSize - headerSize};
            const auto numberOfBytesAvailable{std::distance(payloadStart, end)};

            if (!scanFactory.empty() && (scanFactory.isDifferentScan(header) || scanFactory.isNewScan(header))) {
                scanFactory.clear();
            }

            if (payloadSize > numberOfBytesAvailable) {
                const auto numberOfMissingBytes{payloadSize - numberOfBytesAvailable};
                return {false, result->first, numberOfMissingBytes};
            }
            Iterator payloadEndPosition{};
            std::vector<uint32_t> distances{}, amplitudes{};
            const auto numPointReserve{std::min((unsigned int) (numberOfPoints), MAX_RESERVE_POINTS_BUFFER)};
            distances.reserve(numPointReserve);
            amplitudes.reserve(numPointReserve);
            switch (packetType) {
                case A: {
                    payloadEndPosition = internals::PayloadExtractionFromByteRange<A>::retrievePayload(
                            payloadStart, end, numberOfPoints, distances, amplitudes);
                    break;
                }
                case B: {
                    payloadEndPosition = internals::PayloadExtractionFromByteRange<B>::retrievePayload(
                            payloadStart, end, numberOfPoints, distances, amplitudes);
                    break;
                }
                case C: {
                    payloadEndPosition = internals::PayloadExtractionFromByteRange<C>::retrievePayload(
                            payloadStart, end, numberOfPoints, distances, amplitudes);
                    break;
                }
                default:
                    throw std::runtime_error(
                            "DataLink::Unsupported payload type received from the device: " + packetType);
            }
            scanFactory.addPacket(header, distances, amplitudes);
            return {true, payloadEndPosition, 0};
        }

    public:
        /**
         * @return True if the connection to the device is still alive by checking if the last watchdog signal has been
         * successfully sent to the device.
         */
        [[maybe_unused]] [[nodiscard]] virtual bool isAlive() const noexcept;

        /**
         * @return The last scan received by this DataLink. This method is wait-free and lock-free.
         */
        [[maybe_unused]] [[nodiscard]] virtual inline Device::Data::Scan getLastScan() const {
            RealtimeScan::ScopedAccess <farbot::ThreadType::nonRealtime> scanGuard{*realtimeScan};
            return *scanGuard;
        };

        /**
         * Wait for the next scan to be received by this DataLink.
         * @tparam Duration The duration of type std::chrono::duration.
         * @param timeout Maximum length of time to wait for the next scan.
         * @return A pair containing :
         * - A flag at True if the next scan has been received, False if a timeout has occurred or the
         * DataLink is being destroyed.
         * - The scan received. If the flag is False, an empty scan is set to this field.
         */
        template<typename Duration>
        [[maybe_unused]] [[nodiscard]] inline std::optional<Device::Data::Scan> waitForNextScan(Duration timeout) {
            if (!isConnected.load(std::memory_order_acquire)) {
                return std::nullopt;
            }
            std::unique_lock<LockType> guard{waitForScanLock, std::adopt_lock};
            const auto scanCountBeforeWaiting{scanCounter};
            if (scanAvailableCv.template wait_for(guard, timeout, [&]() {
                const auto scanCountWhileWaiting{scanCounter};
                return interruptFlag.load(std::memory_order_acquire) ||
                       (scanCountBeforeWaiting != scanCountWhileWaiting);
            })) {
                return {getLastScan()};
            }
            return std::nullopt;
        }

        /**
         * Wait indefinitely for the next scan to arrive.
         * @return A pair containing :
         * - A flag at True if the next scan has been received, False if the DataLink is being destroyed.
         * - The scan received. If the flag is False, an empty scan is set to this field.
         */
        [[maybe_unused]] [[nodiscard]] virtual inline std::optional<Device::Data::Scan> waitForNextScan() {
            // Approximately 490293 years, should be sufficient as a long timeout ^^
            const auto aLongTimeout{std::chrono::hours{std::numeric_limits<uint32_t>::max()}};
            return waitForNextScan(aLongTimeout);
        }

        /**
         * Stop the watchdog task and wait for it to exit, then instruct the device to stop
         * and release the stream of data.
         */
        virtual ~DataLink();

    protected:
        /**
         * Template method pattern defining how a scan is set to the output of the DataLink.
         * Subclasses of the DataLink can call this method in order to set an output scan. This method
         * will set the output scan in a lock-free wait-free manner, then awake every thread waiting
         * on the arrival of a new scan. We must precise that awaking the waiting thread still requires
         * to acquire a mutex.
         * @param scan The new scan to set as output.
         */
        inline void setOutputScanFromCompletedFactory(Data::Scan scan) {
            {
                RealtimeScan::ScopedAccess <farbot::ThreadType::realtime> scanGuard{*realtimeScan};
                *scanGuard = std::move(scan);
            }
            std::unique_lock<LockType> guard{waitForScanLock, std::adopt_lock};
            ++scanCounter;
            scanAvailableCv.notify_all();
        }

    private:
        std::optional<std::future<void>> watchdogTaskFuture{std::nullopt};
        std::uint64_t scanCounter{0u};
        Cv interruptCv{};
        LockType interruptCvLock{};
        std::atomic_bool interruptFlag{false};
    protected:
        std::unique_ptr<RealtimeScan> realtimeScan{std::make_unique<RealtimeScan>(Data::Scan{})};
        Cv scanAvailableCv{};
        LockType waitForScanLock{};
        std::shared_ptr<R2000> device{nullptr};
        std::shared_ptr<DeviceHandle> deviceHandle{nullptr};
        std::atomic_bool isConnected{false};
    };
} // namespace Device
