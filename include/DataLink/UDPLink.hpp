//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#pragma once

#include <memory>
#include "DataLink.hpp"
#include "DeviceHandle.hpp"

class HttpController;

namespace R2000 {
    class UDPLink : public DataLink {
    public:
        UDPLink(std::shared_ptr<HttpController> controller, std::shared_ptr<DeviceHandle> handle);
        [[nodiscard]] unsigned int available() const override;
        Scan pop() override;
    };
}