//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#include "DeviceHandle.hpp"

std::string R2000::packetTypeToString(DeviceHandle::PACKET_TYPE type){
    switch (type) {
        case DeviceHandle::PACKET_TYPE::A:
            return "A";
        case DeviceHandle::PACKET_TYPE::B:
            return "B";
        case DeviceHandle::PACKET_TYPE::C:
            return "C";
    }
    return "C";
}