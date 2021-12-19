//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#pragma once

#include "Data.hpp"
#include <atomic>
#include <cassert>
#include <climits>
#include <condition_variable>
#include <deque>
#include <future>
#include <mutex>
#include <optional>
#include <cmath>
#include "Farbot.hpp"

#define assertm(exp, msg) assert(((void)(msg), exp))
namespace Device {
    class R2000;

    struct DeviceHandle;
    namespace internals {

        template<typename F, typename... Ts>
        inline static std::future<typename std::result_of<F(Ts...)>::type> spawnAsync(F &&f, Ts &&... params) {
            return std::async(std::launch::async, std::forward<F>(f), std::forward<Ts>(params)...);
        }

        namespace Types {
            using Byte = unsigned char;
            using Buffer = std::vector<Byte>;
        } // namespace Types
        constexpr std::uint16_t PACKET_START = 0xa25c;

        template<typename Iterator>
        std::optional<std::pair<Iterator, Iterator>> retrievePacketMagic(Iterator begin, Iterator end) {
            constexpr auto byteSizeOf8Bits{sizeof(uint8_t)};
            static_assert(byteSizeOf8Bits == 1, "The size of uint8_t must be exactly 1 byte");
            auto position{begin};
            auto lastByteBeforeEnd{std::prev(end, 1)};
            while (position != lastByteBeforeEnd) {
                const auto uint16Value{*((uint16_t *) &(*position))};
                if (uint16Value == PACKET_START)
                    return {std::make_pair(position, std::next(position, 2))};
                std::advance(position, byteSizeOf8Bits);
            }
            return std::nullopt;
        }

        template<typename Iterator>
        std::optional<std::pair<Iterator, Data::Header>> retrievePacketHeader(Iterator begin, Iterator end) {
            constexpr auto headerSize{sizeof(Data::Header)};
            const auto numberOfBytes{(unsigned int) std::distance(begin, end)};
            if (numberOfBytes < headerSize)
                return std::nullopt;
            const auto magic{retrievePacketMagic(begin, end)};
            if (!magic)
                return std::nullopt;
            const auto headerStart{(*magic).first};
            const auto header{*((Data::Header *) &(*headerStart))};
            return {{headerStart, header}};
        }

        template<typename Iterator, typename Callback>
        Iterator retrieve32bitsFromAlignedSequence(Iterator begin, Iterator end, const unsigned int numberOf32bitsBlock,
                                                   Callback callback) {
            constexpr auto byteSizeOf32Bits{sizeof(uint32_t)};
            static_assert(byteSizeOf32Bits == 4, "The size of uint32_t must be 4 bytes");
            const auto numberOfBytes{std::distance(begin, end)};
            auto position = begin;
            for (auto remainingBytes{numberOfBytes}, blockCount{0u};
                 remainingBytes >= byteSizeOf32Bits && blockCount < numberOf32bitsBlock;
                 remainingBytes = std::distance(position, end), ++blockCount) {
                const auto uint32Value{*((uint32_t *) &(*position))};
                std::advance(position, byteSizeOf32Bits);
                callback(uint32Value);
            }
            return position;
        }

        template<typename Iterator, typename Callback>
        Iterator retrieve48bitsFromAlignedSequence(Iterator begin, Iterator end, const unsigned int numberOf48bitsBlock,
                                                   Callback callback) {
            constexpr auto byteSizeOf32Bits{sizeof(uint32_t)};
            constexpr auto byteSizeOf16Bits{sizeof(uint16_t)};
            static_assert(byteSizeOf32Bits == 4, "The size of uint32_t must be 4 bytes");
            static_assert(byteSizeOf16Bits == 2, "The size of uint16_t must be 2 bytes");
            const auto numberOfBytes{std::distance(begin, end)};
            auto position{begin};
            for (auto remainingBytes{numberOfBytes}, blockCount{0u};
                 remainingBytes >= byteSizeOf32Bits && blockCount < numberOf48bitsBlock;
                 remainingBytes = std::distance(position, end), ++blockCount) {
                const auto uint32Value{*((uint32_t *) &(*position))};
                std::advance(position, byteSizeOf32Bits);
                const auto uint16Value{*((uint16_t *) &(*position))};
                std::advance(position, byteSizeOf16Bits);
                callback(uint32Value, uint16Value);
            }
            return position;
        }

        template<unsigned Type>
        struct PayloadExtractionFromByteSequence {
            template<typename Iterator>
            static Iterator retrievePayload(Iterator, Iterator, const unsigned int, std::vector<uint32_t> &,
                                            std::vector<uint32_t> &) {
                throw std::runtime_error("Unsupported type of packet payload: " + Type);
            }
        };

        template<>
        struct PayloadExtractionFromByteSequence<Data::underlyingType(Data::PACKET_TYPE::A)> {
            template<typename Iterator>
            static Iterator retrievePayload(Iterator begin, Iterator end, const unsigned int numberOfPoints,
                                            std::vector<uint32_t> &distances, std::vector<uint32_t> &amplitudes) {
                return retrieve32bitsFromAlignedSequence(begin, end, numberOfPoints,
                                                         [&distances, &amplitudes](const uint32_t &distance) {
                                                             const auto nanOrDistance =
                                                                     distance != 0xFFFFFFFF ? distance
                                                                                            : std::numeric_limits<uint32_t>::quiet_NaN();
                                                             distances.push_back(nanOrDistance);
                                                             amplitudes.push_back(0);
                                                         });
            }
        };

        template<>
        struct PayloadExtractionFromByteSequence<Data::underlyingType(Data::PACKET_TYPE::B)> {
            template<typename Iterator>
            static Iterator retrievePayload(Iterator begin, Iterator end, const unsigned int numberOfPoints,
                                            std::vector<uint32_t> &distances, std::vector<uint32_t> &amplitudes) {
                return retrieve48bitsFromAlignedSequence(begin, end, numberOfPoints,
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

        template<>
        struct PayloadExtractionFromByteSequence<Data::underlyingType(Data::PACKET_TYPE::C)> {
            template<typename Iterator>
            static Iterator retrievePayload(Iterator begin, Iterator end, const unsigned int numberOfPoints,
                                            std::vector<uint32_t> &distances, std::vector<uint32_t> &amplitudes) {
                return retrieve32bitsFromAlignedSequence(begin, end, numberOfPoints,
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
        class ScanFactory {
        public:
            inline virtual void addPacket(const Data::Header &header, std::vector<std::uint32_t> &inputDistances,
                                  std::vector<std::uint32_t> &inputAmplitudes) = 0;
            [[nodiscard]] virtual inline bool empty() const =0;
            [[nodiscard]] virtual inline bool isComplete() const =0;
            virtual inline Data::Scan operator*() =0;
            virtual inline void clear() = 0;
            [[nodiscard]] virtual inline bool isDifferentScan(const Data::Header & header) const = 0;
            [[nodiscard]] virtual  inline bool isNewScan(const Data::Header & header) =0;
            virtual inline bool operator==(unsigned int scanNumber) const = 0;
            virtual inline bool operator!=(unsigned int scanNumber) const = 0;
            virtual inline void getHeaders(std::vector<Data::Header> & outputHeaders) const  = 0;
        };
    protected:
        using LockType = std::mutex;
        using Cv = std::condition_variable;
        using RealtimeScan = farbot::RealtimeObject<Data::Scan, farbot::RealtimeObjectOptions::realtimeMutatable>;
        static constexpr unsigned int MAX_RESERVE_POINTS_BUFFER{1024u};
    protected:
        DataLink(std::shared_ptr<R2000> controller, std::shared_ptr<DeviceHandle> handle);
/**
         *
         * @tparam Iterator
         * @param begin
         * @param end
         * @param scanFactory
         * @return
         */
        template<typename Iterator>
        static std::tuple<bool, Iterator, unsigned int>
        extractScanPacketFromByteRange(Iterator begin, Iterator end, ScanFactory &scanFactory) {
            static constexpr auto A{Data::underlyingType(Data::PACKET_TYPE::A)};
            static constexpr auto B{Data::underlyingType(Data::PACKET_TYPE::B)};
            static constexpr auto C{Data::underlyingType(Data::PACKET_TYPE::C)};

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
                case A:
                    payloadEndPosition = internals::PayloadExtractionFromByteSequence<A>::retrievePayload(
                            payloadStart, end, numberOfPoints, distances, amplitudes);
                    break;
                case B:
                    payloadEndPosition = internals::PayloadExtractionFromByteSequence<B>::retrievePayload(
                            payloadStart, end, numberOfPoints, distances, amplitudes);
                    break;
                case C:
                    payloadEndPosition = internals::PayloadExtractionFromByteSequence<C>::retrievePayload(
                            payloadStart, end, numberOfPoints, distances, amplitudes);
                    break;
                default:
                    throw std::runtime_error("TCPLink::Unsupported payload type: " + packetType);
            }
            scanFactory.addPacket(header, distances, amplitudes);
            if (scanFactory.isComplete()) {
                return {true, payloadEndPosition, 0};
            }
            return {true, payloadEndPosition, 0};
        }
    public:
        [[nodiscard]] virtual bool isAlive() const;
        [[nodiscard]] virtual Device::Data::Scan getLastScan() const = 0;
        virtual ~DataLink();

    private:
        std::shared_ptr<R2000> mDevice{nullptr};
        std::future<void> mWatchdogTask{};
        Cv mCv{};
        LockType mInterruptCvLock{};
        std::atomic_bool mInterruptFlag{false};
    protected:
        std::shared_ptr<DeviceHandle> mHandle{nullptr};
        std::atomic_bool mIsConnected{false};
    };
} // namespace Device
