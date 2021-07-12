//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#pragma once

#include <memory>
#include "DataLink.hpp"
#include <boost/asio.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <thread>

namespace R2000 {
    class HttpController;
    struct DeviceHandle;
    class TCPLink : public DataLink {
        using Byte = unsigned char;
    public:
        TCPLink(std::shared_ptr<HttpController> controller, std::shared_ptr<DeviceHandle> handle) noexcept(false);
        [[nodiscard]] unsigned int available() const override;
        Scan pop() override;
        ~TCPLink() override;

    private:
        void handleReception(const boost::system::error_code &error, std::size_t bytesTransferred);
    private:
        std::unique_ptr<boost::asio::io_service> mIoService{nullptr};
        std::unique_ptr<boost::asio::ip::tcp::socket> mSocket{nullptr};
        boost::circular_buffer<Byte> mRingBuffer{};
        boost::asio::streambuf mStreamBuffer{4096};
        std::unique_ptr<std::thread> mIoServiceThread{nullptr};
    };
}