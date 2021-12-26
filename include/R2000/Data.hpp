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

    struct Header {
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
    } __attribute__((packed));

    class Scan {
    public:
        Scan() = default;
        Scan(std::vector<std::uint32_t> distances, std::vector<std::uint32_t> amplitudes,
             std::vector<Header> headers,
             const std::chrono::steady_clock::time_point &timestamp) : distances(std::move(distances)),
                                                                       amplitudes(std::move(amplitudes)),
                                                                       headers(std::move(headers)),
                                                                       timestamp(timestamp) {}

        [[nodiscard]] const std::vector<std::uint32_t> &getDistances() const { return distances; }
        [[nodiscard]] const std::vector<std::uint32_t> &getAmplitudes() const { return amplitudes; }
        [[nodiscard]] const std::vector<Header> &getHeaders() const { return headers; }
        [[nodiscard]] const auto &getTimestamp() const { return timestamp; }
        [[nodiscard]] inline bool empty() const { return headers.empty(); }
        [[nodiscard]] inline bool isComplete() const { return !empty() && distances.size() >= headers[0].numPointsScan; }
    private:
        // Distance data in polar form in millimeter
        std::vector<std::uint32_t> distances{};
        // Amplitude data in the range 32-4095, values lower than 32 indicate an error or undefined values
        std::vector<std::uint32_t> amplitudes{};
        // Header received with the distance and amplitude data
        std::vector<Header> headers{};
        // Timestamp of the scan
        std::chrono::steady_clock::time_point timestamp;
    };
} // namespace Device