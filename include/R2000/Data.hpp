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

namespace Device::Data {
    /**
     * Get the underlying value of an enum.
     * @tparam E The enum type.
     * @param enumerator The instance of enum to get the underlying value.
     * @return The value of the enum given its type.
     */
    template<typename E>
    constexpr auto underlyingType(E enumerator) {
        return static_cast<std::underlying_type_t<E>>(enumerator);
    }

    namespace internals {
        /**
         *
         * @tparam T
         * @tparam Advance
         */
        template<typename T, bool Advance>
        struct InterpretLittleEndianByteRange {
            /**
             *
             * @tparam Iterator
             */
            template<typename Iterator>
            [[maybe_unused]] static inline void interpret(Iterator &, T &) {};
        };

        /**
         *
         * @tparam Advance
         */
        template<bool Advance>
        struct InterpretLittleEndianByteRange<std::uint16_t, Advance> {
            /**
             *
             * @tparam Iterator
             * @param position
             * @param v
             */
            template<typename Iterator>
            static inline void interpret(Iterator &position, std::uint16_t &v) {
                v = uint16_t(*position) | (uint16_t(*(position + 1)) << 8);
                if constexpr(Advance) {
                    constexpr auto byteSizeOf16Bits{sizeof(std::uint16_t)};
                    std::advance(position, byteSizeOf16Bits);
                }
            };
        };

        /**
         *
         * @tparam Advance
         */
        template<bool Advance>
        struct InterpretLittleEndianByteRange<std::uint32_t, Advance> {
            /**
             *
             * @tparam Iterator
             * @param position
             * @param v
             */
            template<typename Iterator>
            static inline void interpret(Iterator &position, std::uint32_t &v) {
                v = uint32_t(*position) |
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
         *
         * @tparam Advance
         */
        template<bool Advance>
        struct InterpretLittleEndianByteRange<std::int32_t, Advance> {
            /**
             *
             * @tparam Iterator
             * @param position
             * @param v
             */
            template<typename Iterator>
            static inline void interpret(Iterator &position, std::int32_t &v) {
                v = int32_t(*position) |
                    (int32_t(*(position + 1)) << 8) |
                    (int32_t(*(position + 2)) << 16) |
                    (int32_t(*(position + 3)) << 24);
                if constexpr(Advance) {
                    constexpr auto byteSizeOf32Bits{sizeof(std::int32_t)};
                    std::advance(position, byteSizeOf32Bits);
                }
            };
        };

        template<bool Advance>
        struct InterpretLittleEndianByteRange<std::uint64_t, Advance> {
            template<typename Iterator>
            static inline void interpret(Iterator &position, std::uint64_t &v) {
                v = uint64_t(*position) |
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
         *
         * @tparam Iterator
         * @param begin
         * @return
         */
        template<typename Iterator>
        static Header fromByteRange(Iterator begin) {
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
        Scan() = default;

        /**
         *
         * @param distances
         * @param amplitudes
         * @param headers
         * @param timestamp
         */
        Scan(std::vector<std::uint32_t> distances, std::vector<std::uint32_t> amplitudes,
             std::vector<Header> headers,
             const std::chrono::steady_clock::time_point &timestamp) : distances(std::move(distances)),
                                                                       amplitudes(std::move(amplitudes)),
                                                                       headers(std::move(headers)),
                                                                       timestamp(timestamp) {}

        /**
         *
         * @return
         */
        [[maybe_unused]] [[nodiscard]] const std::vector<std::uint32_t> &getDistances() const { return distances; }

        /**
         *
         * @return
         */
        [[maybe_unused]] [[nodiscard]] const std::vector<std::uint32_t> &getAmplitudes() const { return amplitudes; }

        /**
         *
         * @return
         */
        [[maybe_unused]] [[nodiscard]] const std::vector<Header> &getHeaders() const { return headers; }

        /**
         *
         * @return
         */
        [[maybe_unused]] [[nodiscard]] const auto &getTimestamp() const { return timestamp; }

        /**
         *
         * @return
         */
        [[maybe_unused]] [[nodiscard]] inline bool empty() const { return headers.empty(); }

        /**
         *
         * @return
         */
        [[maybe_unused]] [[nodiscard]] inline bool isComplete() const {
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