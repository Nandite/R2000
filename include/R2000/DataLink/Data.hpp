//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#pragma once

#include <chrono>
#include <cstdint>
#include <utility>
#include <vector>
#include <system_error>
#include "R2000/NotImplementedException.hpp"

namespace Device::Data {
    /**
     * Get the underlying value of an enum.
     * @tparam E The enum type.
     * @param enumerator The instance of enum to get the underlying value.
     * @return The value of the enum given its type.
     */
    template<typename E>
    constexpr auto underlyingType(E enumerator) noexcept {
        return static_cast<std::underlying_type_t<E>>(enumerator);
    }

    namespace internals {
        /**
         * Helper struct that interpret a range byte as a type T.
         * @tparam T The type to interpret the range into.
         * @tparam Advance True if the iterator of the range must be advanced to the next byte
         * following the type T byte range representation.
         */
        template<typename T, bool Advance>
        struct InterpretLittleEndianByteRange {
            /**
             * Always throws a Device::NotImplemented exception.
             * @tparam Iterator The type of iterator of the range.
             */
            template<typename Iterator>
            [[maybe_unused]] static inline void interpret(Iterator &, T &) noexcept(false) {
                throw Device::NotImplemented("InterpretLittleEndianByteRange<T>::Not implemented.");
            };
        };

        /**
         * Specialization that interpret a little endian range of byte as a 16-bits length unsigned int.
         * @tparam Advance True if the iterator of the range must be advanced by two bytes.
         */
        template<bool Advance>
        struct InterpretLittleEndianByteRange<std::uint16_t, Advance> {
            /**
             * Interpret an iterator pointing over a little endian byte range as a 16-bits length unsigned integer.
             * @tparam Iterator An iterator that meet the requirements of LegacyInputIterator.
             * @param position Iterator over the byte range.
             * @param interpretedValue The value obtained as the interpretation of the iterator.
             */
            template<typename Iterator>
            static inline void interpret(Iterator &position, std::uint16_t &interpretedValue) noexcept {
                interpretedValue = uint16_t(*position) | (uint16_t(*(position + 1)) << 8);
                if constexpr(Advance) {
                    constexpr auto byteSizeOf16Bits{sizeof(std::uint16_t)};
                    std::advance(position, byteSizeOf16Bits);
                }
            };
        };

        /**
         * Specialization that interpret a little endian range of byte as a 32-bits length unsigned int.
         * @tparam Advance True if the iterator of the range must be advanced by four bytes.
         */
        template<bool Advance>
        struct InterpretLittleEndianByteRange<std::uint32_t, Advance> {
            /**
             * Interpret an iterator pointing over a little endian byte range as a 32-bits length unsigned integer.
             * @tparam Iterator An iterator that meet the requirements of LegacyInputIterator.
             * @param position Iterator over the byte range.
             * @param interpretedValue The value obtained as the interpretation of the iterator.
             */
            template<typename Iterator>
            static inline void interpret(Iterator &position, std::uint32_t &interpretedValue) noexcept {
                interpretedValue = uint32_t(*position) |
                                   (uint32_t(*(position + 1)) << 8) |
                                   (uint32_t(*(position + 2)) << 16) |
                                   (uint32_t(*(position + 3)) << 24);
                if constexpr(Advance) {
                    constexpr auto byteSizeOf32Bits{sizeof(std::uint32_t)};
                    std::advance(position, byteSizeOf32Bits);
                }
            };
        };

        /**
         * Specialization that interpret a little endian range of byte as a 32-bits length signed int.
         * @tparam Advance True if the iterator of the range must be advanced by four bytes.
         */
        template<bool Advance>
        struct InterpretLittleEndianByteRange<std::int32_t, Advance> {
            /**
             * Interpret an iterator pointing over a little endian byte range as a 32-bits length signed integer.
             * @tparam Iterator An iterator that meet the requirements of LegacyInputIterator.
             * @param position Iterator over the byte range.
             * @param interpretedValue The value obtained as the interpretation of the iterator.
             */
            template<typename Iterator>
            static inline void interpret(Iterator &position, std::int32_t &interpretedValue) noexcept {
                interpretedValue = int32_t(*position) |
                                   (int32_t(*(position + 1)) << 8) |
                                   (int32_t(*(position + 2)) << 16) |
                                   (int32_t(*(position + 3)) << 24);
                if constexpr(Advance) {
                    constexpr auto byteSizeOf32Bits{sizeof(std::int32_t)};
                    std::advance(position, byteSizeOf32Bits);
                }
            };
        };

        /**
         * Specialization that interpret a little endian range of byte as a 64-bits length unsigned int.
         * @tparam Advance True if the iterator of the range must be advanced by eight bytes.
         */
        template<bool Advance>
        struct InterpretLittleEndianByteRange<std::uint64_t, Advance> {
            /**
             * Interpret an iterator pointing over a little endian byte range as a 64-bits length unsigned integer.
             * @tparam Iterator An iterator that meet the requirements of LegacyInputIterator.
             * @param position Iterator over the byte range.
             * @param interpretedValue The value obtained as the interpretation of the iterator.
             */
            template<typename Iterator>
            static inline void interpret(Iterator &position, std::uint64_t &interpretedValue) noexcept {
                interpretedValue = uint64_t(*position) |
                                   (uint64_t(*(position + 1)) << 8) |
                                   (uint64_t(*(position + 2)) << 16) |
                                   (uint64_t(*(position + 3)) << 24) |
                                   (uint64_t(*(position + 4)) << 32) |
                                   (uint64_t(*(position + 5)) << 40) |
                                   (uint64_t(*(position + 6)) << 48) |
                                   (uint64_t(*(position + 7)) << 56);
                if constexpr(Advance) {
                    constexpr auto byteSizeOf64Bits{sizeof(std::uint64_t)};
                    std::advance(position, byteSizeOf64Bits);
                }
            };
        };
    }

    struct Header {
        /**
         * Construct a header by interpreting a little endian byte range given an input iterator.
         * @tparam Iterator The type of the iterator over the range. It must meet the requirements
         * of LegacyInputIterator.
         * @param begin The starting position of the byte range.
         * @return A Header constructed from the byte range.
         */
        template<typename Iterator>
        static Header fromByteRange(Iterator begin) noexcept {
            Data::Header header{};
            internals::InterpretLittleEndianByteRange<uint16_t, true>::interpret(begin, header.magic);
            internals::InterpretLittleEndianByteRange<uint16_t, true>::interpret(begin, header.packetType);
            internals::InterpretLittleEndianByteRange<uint32_t, true>::interpret(begin, header.packetSize);
            internals::InterpretLittleEndianByteRange<uint16_t, true>::interpret(begin, header.headerSize);
            internals::InterpretLittleEndianByteRange<uint16_t, true>::interpret(begin, header.scanNumber);
            internals::InterpretLittleEndianByteRange<uint16_t, true>::interpret(begin, header.packetNumber);
            internals::InterpretLittleEndianByteRange<uint64_t, true>::interpret(begin, header.timestampRaw);
            internals::InterpretLittleEndianByteRange<uint64_t, true>::interpret(begin, header.timestampSync);
            internals::InterpretLittleEndianByteRange<uint32_t, true>::interpret(begin, header.statusFlags);
            internals::InterpretLittleEndianByteRange<uint32_t, true>::interpret(begin, header.scanFrequency);
            internals::InterpretLittleEndianByteRange<uint16_t, true>::interpret(begin, header.numPointsScan);
            internals::InterpretLittleEndianByteRange<uint16_t, true>::interpret(begin, header.numPointsPacket);
            internals::InterpretLittleEndianByteRange<uint16_t, true>::interpret(begin, header.firstIndex);
            internals::InterpretLittleEndianByteRange<int32_t, true>::interpret(begin, header.firstAngle);
            internals::InterpretLittleEndianByteRange<int32_t, true>::interpret(begin, header.angularIncrement);
            internals::InterpretLittleEndianByteRange<uint32_t, true>::interpret(begin, header.iqInput);
            internals::InterpretLittleEndianByteRange<uint32_t, true>::interpret(begin, header.iqOverload);
            internals::InterpretLittleEndianByteRange<uint64_t, true>::interpret(begin, header.iqTimestampRaw);
            internals::InterpretLittleEndianByteRange<uint64_t, false>::interpret(begin, header.iqTimestampSync);
            return header;
        }

        // Magic bytes, must be  5C A2 (hex)
        std::uint16_t magic{};
        // Packet type, must be 43 00 (hex)
        std::uint16_t packetType{};
        // Overall packet size (header+payload), 1404 bytes with maximum payload
        std::uint32_t packetSize{};
        // Header size, defaults to 60 bytes
        std::uint16_t headerSize{};
        // Sequence for scan (incremented for every scan, starting with 0, overflows)
        std::uint16_t scanNumber{};
        // Sequence number for packet (counting packets of a particular scan, starting with 1)
        std::uint16_t packetNumber{};
        // Raw timestamp of internal clock in NTP time format
        std::uint64_t timestampRaw{};
        // With an external NTP server synced Timestamp  (currently not available and and set to zero)
        std::uint64_t timestampSync{};
        // Status flags
        std::uint32_t statusFlags{};
        // Frequency of scan-head rotation in mHz (Milli-Hertz)
        std::uint32_t scanFrequency{};
        // Total number of scan points (samples) within complete scan
        std::uint16_t numPointsScan{};
        // Total number of scan points within this packet
        std::uint16_t numPointsPacket{};
        // Index of first scan point within this packet
        std::uint16_t firstIndex{};
        // Absolute angle of first scan point within this packet in 1/10000�
        std::int32_t firstAngle{};
        // Delta between two successive scan points 1/10000�
        std::int32_t angularIncrement{};
        // Bit field for switching input state
        std::uint32_t iqInput{};
        // Bit field for switching output overload warning
        std::uint32_t iqOverload{};
        // Raw timestamp for systemStatusMap of switching I/Q
        std::uint64_t iqTimestampRaw{};
        // Synchronized timestamp for systemStatusMap of switching I/Q
        std::uint64_t iqTimestampSync{};
        // 0-3 bytes padding (to align the header size to a 32bit boundary)
        // uint8_t padding[4] = {0, 0, 0, 0};
    };

    class Scan {
    public:
        /**
         * Construct a default scan with empty distances and amplitudes and no timestamp.
         */
        Scan() = default;

        /**
         * Construct a new scan given a distances, amplitudes vectors, headers and timestamp.
         * @param distances The distances values of the scan.
         * @param amplitudes The amplitudes values of the scan.
         * @param headers The headers of the scan.
         * @param timestamp The timestamp  of the scan.
         */
        Scan(std::vector<std::uint32_t> distances, std::vector<std::uint32_t> amplitudes,
             std::vector<Header> headers,
             const std::chrono::steady_clock::time_point &timestamp) : distances(std::move(distances)),
                                                                       amplitudes(std::move(amplitudes)),
                                                                       headers(std::move(headers)),
                                                                       timestamp(timestamp) {}

        /**
         * @return The distances vector of the scan.
         */
        [[maybe_unused]] [[nodiscard]] inline const std::vector<std::uint32_t> &
        getDistances() const noexcept { return distances; }

        /**
         * @return The amplitudes vector of the scan.
         */
        [[maybe_unused]] [[nodiscard]] inline const std::vector<std::uint32_t> &
        getAmplitudes() const noexcept { return amplitudes; }

        /**
         * @return The headers of each packet of the scan.
         */
        [[maybe_unused]] [[nodiscard]] inline const std::vector<Header> &getHeaders() const noexcept { return headers; }

        /**
         * @return The timestamp of the scan.
         */
        [[maybe_unused]] [[nodiscard]] inline const auto &getTimestamp() const noexcept { return timestamp; }

        /**
         * @return True if the scan is empty, False otherwise.
         */
        [[maybe_unused]] [[nodiscard]] inline bool empty() const noexcept { return headers.empty(); }

        /**
         * @return True if the scan is complete, False otherwise. A scan is complete if the number of points
         * specified by the first header is equals to the size of the distances vector.
         */
        [[maybe_unused]] [[nodiscard]] inline bool isComplete() const noexcept {
            return !empty() && distances.size() >= headers[0].numPointsScan;
        }

    private:
        // Distance data in polar form in millimeter
        std::vector<std::uint32_t> distances{};
        // Amplitude data in the range 32-4095, values lower than 32 indicate an error or undefined values
        std::vector<std::uint32_t> amplitudes{};
        // Header received with the distance and amplitude data
        std::vector<Header> headers{};
        // Timestamp of the scan
        std::chrono::steady_clock::time_point timestamp{};
    };
} // namespace Device