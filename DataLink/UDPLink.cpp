//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#include <utility>

#include "DataLink/UDPLink.hpp"

R2000::UDPLink::UDPLink(std::shared_ptr<HttpController> controller, std::shared_ptr<DeviceHandle> handle) : DataLink(
        std::move(controller), std::move(handle)) {}


unsigned int R2000::UDPLink::available() const {
    return 0;
}

R2000::Scan R2000::UDPLink::pop() {
    return R2000::Scan();
}
