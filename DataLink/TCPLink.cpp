//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#include "DataLink/TCPLink.hpp"
#include "DeviceHandle.hpp"

R2000::TCPLink::TCPLink(std::shared_ptr<HttpController> controller, std::shared_ptr<DeviceHandle> handle) noexcept(false) : DataLink(
        std::move(controller), std::move(handle)) ,
        mIoService(std::make_unique<boost::asio::io_service>()),
        mSocket(std::make_unique<boost::asio::ip::tcp::socket>(*mIoService)){

    const auto hostname = mHandle->hostname;
    const auto port = mHandle->port;
    boost::asio::ip::tcp::resolver resolver(*mIoService);
    boost::asio::ip::tcp::resolver::query query(hostname.to_string(), std::to_string(port));
    auto endpoints = resolver.resolve(query);

    {
        boost::system::error_code error = boost::asio::error::host_not_found;
        for (boost::asio::ip::tcp::resolver::iterator end; error && endpoints != end; ++endpoints) {
            boost::system::error_code placeholder;
            mSocket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, placeholder);
            mSocket->close();
            mSocket = std::make_unique<boost::asio::ip::tcp::socket>(*mIoService);
            mSocket->connect(*endpoints, error);
        }
        if (error) {
            throw boost::system::system_error(error);
        }
        boost::asio::async_read(*mSocket, mStreamBuffer,
                                [this](const boost::system::error_code &error, std::size_t bytesTransferred) {
                                    handleReception(error, bytesTransferred);
                                });
    }
    mIoServiceThread = std::make_unique<std::thread>(
            [this]() {
                mIoService->run();
            });
}


void R2000::TCPLink::handleReception(const boost::system::error_code &error, std::size_t bytesTransferred) {

}

unsigned int R2000::TCPLink::available() const {
    return 0;
}

R2000::Scan R2000::TCPLink::pop() {
    return R2000::Scan();
}

R2000::TCPLink::~TCPLink() {
    if (!mIoService->stopped()) {
        mIoService->stop();
    }
    if (mIoServiceThread->joinable()) {
        mIoServiceThread->join();
    }
}
