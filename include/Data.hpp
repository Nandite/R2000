//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#pragma once
#include <vector>
namespace R2000 {
    struct DataHeader {

        //! Magic bytes, must be  5C A2 (hex)
        std::uint16_t magic;
        //! Packet type, must be 43 00 (hex)
        std::uint16_t packetType;
        //! Overall packet size (header+payload), 1404 bytes with maximum payload
        std::uint32_t packetSize;
        //! Header size, defaults to 60 bytes
        std::uint16_t headerSize;
        //! Sequence for scan (incremented for every scan, starting with 0, overflows)
        std::uint16_t scanNumber;
        //! Sequence number for packet (counting packets of a particular scan, starting with 1)
        std::uint16_t packetNumber;
        //! Raw timestamp of internal clock in NTP time format
        std::uint64_t timestampRaw;
        //! With an external NTP server synced Timestamp  (currenty not available and and set to zero)
        std::uint64_t timestampSync;
        //! Status flags
        std::uint32_t statusFlags;
        //! Frequency of scan-head rotation in mHz (Milli-Hertz)
        std::uint32_t scanFrequency;
        //! Total number of scan points (samples) within complete scan
        std::uint16_t numPointsScan;
        //! Total number of scan points within this packet
        std::uint16_t numPointsPacket;
        //! Index of first scan point within this packet
        std::uint16_t firstIndex;
        //! Absolute angle of first scan point within this packet in 1/10000°
        std::int32_t firstAngle;
        //! Delta between two succeding scan points 1/10000°
        std::int32_t angularIncrement;
        //! Output status
        std::uint32_t outputStatus;
        //! Field status
        std::uint32_t fieldStatus;
        //! Possible padding to align header size to 32bit boundary
        //std::uint8 padding[0];
    };

    struct Scan {
        //! Distance data in polar form in millimeter
        std::vector<std::uint32_t> distanceData{};
        //! Amplitude data in the range 32-4095, values lower than 32 indicate an error or undefined values
        std::vector<std::uint32_t> amplitudeData{};
        //! Header received with the distance and amplitude data
        std::vector<DataHeader> headers{};
    };
}