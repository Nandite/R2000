//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#include "R2000/DataLink/TCPLink.hpp"
#include "R2000/DeviceHandle.hpp"
#include <iostream>

Device::TCPLink::TCPLink(std::shared_ptr<R2000> iDevice,
                         std::shared_ptr<DeviceHandle> iHandle) noexcept(false)
        : DataLink(std::move(iDevice), std::move(iHandle)),
          socket(std::make_unique<boost::asio::ip::tcp::socket>(ioService)),
          receptionByteBuffer(DEFAULT_RECEPTION_BUFFER_SIZE, 0),
          extractionByteBuffer(EXTRACTION_BUFFER_SIZE, 0) {
    const auto hostname{device->getHostname()};
    const auto port{deviceHandle->port};
    boost::asio::ip::tcp::resolver resolver{ioService};
    boost::asio::ip::tcp::resolver::query query{hostname.to_string(), std::to_string(port)};
    auto endpoints{resolver.resolve(query)};

    boost::system::error_code error{boost::asio::error::host_not_found};
    for (boost::asio::ip::tcp::resolver::iterator end{};
         error && endpoints != end;
         ++endpoints) {
        boost::system::error_code placeholder{};
        socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, placeholder);
        socket->close(placeholder);
        socket = std::make_unique<boost::asio::ip::tcp::socket>(ioService);
        socket->connect(*endpoints, error);
    }
    if (!error) {
        isConnected.store(true, std::memory_order_release);
        boost::asio::async_read(*socket, boost::asio::buffer(receptionByteBuffer),
                                boost::asio::transfer_exactly(DEFAULT_RECEPTION_BUFFER_SIZE),
                                [this](const boost::system::error_code &error, const unsigned int byteTransferred) {
                                    handleBytesReception(error, byteTransferred);
                                });
        ioServiceThread = std::thread([this]() {
            ioService.run();
        });
    } else {
        std::clog << "Could not join the device (" << error.message() << ")" << std::endl;
        isConnected.store(false, std::memory_order_release);
    }
}

void Device::TCPLink::handleBytesReception(const boost::system::error_code &error,
                                           const unsigned int byteTransferred) {
    if (error) {
        std::clog << __func__ << ":: Network error (" << error.message() << ")" << std::endl;
        isConnected.store(false, std::memory_order_release);
        return;
    }
    const auto begin{std::cbegin(receptionByteBuffer)};
    const auto end{std::next(begin, byteTransferred)};
    const auto range {insertByteRangeIntoExtractionBuffer(begin, end)};
    const auto extractionResult{tryExtractingScanFromByteRange(range.first, range.second)};
    removeUsedByteRange(range.first, extractionResult.first, range.second);
    boost::asio::async_read(*socket, boost::asio::buffer(receptionByteBuffer),
                            boost::asio::transfer_exactly(extractionResult.second),
                            [this](const auto &error, const auto byteTransferred) {
                                handleBytesReception(error, byteTransferred);
                            });
}

Device::TCPLink::~TCPLink() {
    if (!ioService.stopped())
        ioService.stop();
    if (ioServiceThread.joinable())
        ioServiceThread.join();
    boost::system::error_code placeholder;
    socket->shutdown(boost::asio::ip::tcp::socket::shutdown_receive, placeholder);
    socket->close(placeholder);
    isConnected.store(false, std::memory_order_release);
}
