//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#include "DataLink/TCPLink.hpp"
#include "DeviceHandle.hpp"
#include <iostream>

Device::TCPLink::TCPLink(std::shared_ptr<R2000> device,
                         std::shared_ptr<DeviceHandle> handle) noexcept(false)
        : DataLink(std::move(device), std::move(handle)),
          socket(std::make_unique<boost::asio::ip::tcp::socket>(ioService)),
          receptionByteBuffer(DEFAULT_RECEPTION_BUFFER_SIZE, 0),
          extractionByteBuffer(EXTRACTION_BUFFER_SIZE, 0) {
    const auto hostname{mDevice->getHostname()};
    const auto port{mHandle->port};
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
        mIsConnected.store(true, std::memory_order_release);
        boost::asio::async_read(*socket, boost::asio::buffer(receptionByteBuffer),
                                boost::asio::transfer_exactly(DEFAULT_RECEPTION_BUFFER_SIZE),
                                [this](const boost::system::error_code &error, const unsigned int byteTransferred) {
                                    handleBytesReception(error, byteTransferred);
                                });
        ioServiceThread = std::thread([this]() {
            ioService.run();
        });
    } else {
        std::clog << "Could not join device (" << error.message() << ")" << std::endl;
        mIsConnected.store(false, std::memory_order_release);
    }
}

void Device::TCPLink::handleBytesReception(const boost::system::error_code &error,
                                           const unsigned int byteTransferred) {
    if (!error) {
        const auto streamBegin{std::begin(receptionByteBuffer)};
        const auto streamEnd{std::next(streamBegin, byteTransferred)};
        extractionByteBuffer.insert(std::end(extractionByteBuffer), streamBegin, streamEnd);
        const auto extractionBegin{std::begin(extractionByteBuffer)};
        const auto extractionEnd{std::end(extractionByteBuffer)};
        auto position{extractionBegin};
        auto numberOfMissingBytes{0u};
        for (;;) {
            const auto extractionResult{extractScanPacketFromByteRange(position, extractionEnd, scanFactory)};
            const auto hadEnoughBytes{std::get<0>(extractionResult)};
            position = std::get<1>(extractionResult);
            numberOfMissingBytes = std::get<2>(extractionResult);
            if (scanFactory.isComplete()) {
                const auto bufferSizeNeededToContainAFullScan{computeBoundedRequiredBufferSize(scanFactory)};
                resizeReceptionBuffer(bufferSizeNeededToContainAFullScan);
                {
                    RealtimeScan::ScopedAccess <farbot::ThreadType::realtime> scanGuard(*realtimeScan);
                    *scanGuard = *scanFactory;
                }
            }
            if (!hadEnoughBytes)
                break;
        }
        const auto remainingBytes{std::distance(position, extractionEnd)};
        if (remainingBytes) {
            extractionByteBuffer.erase(extractionBegin, position);
        } else {
            extractionByteBuffer.clear();
        }
        const auto bufferCapacity{(unsigned int) receptionByteBuffer.capacity()};
        const auto numberOfBytesToTransfer{
                numberOfMissingBytes ? std::min(numberOfMissingBytes, bufferCapacity)
                                     : bufferCapacity};
        boost::asio::async_read(*socket, boost::asio::buffer(receptionByteBuffer),
                                boost::asio::transfer_exactly(numberOfBytesToTransfer),
                                [this](const boost::system::error_code &error,
                                       const unsigned int byteTransferred) {
                                    handleBytesReception(error, byteTransferred);
                                });
    } else if (error == boost::asio::error::eof) {

        boost::asio::async_read(*socket, boost::asio::buffer(receptionByteBuffer),
                                boost::asio::transfer_exactly(DEFAULT_RECEPTION_BUFFER_SIZE),
                                [this](const boost::system::error_code &error, const unsigned int byteTransferred) {
                                    handleBytesReception(error, byteTransferred);
                                });
    } else {
        std::clog << __func__ << ":: Network error (" << error.message() << ")" << std::endl;
        mIsConnected.store(false, std::memory_order_release);
    }
}

Device::TCPLink::~TCPLink() {
    if (!ioService.stopped())
        ioService.stop();
    if (ioServiceThread.joinable())
        ioServiceThread.join();
    boost::system::error_code placeholder;
    socket->shutdown(boost::asio::ip::tcp::socket::shutdown_receive, placeholder);
    socket->close(placeholder);
    mIsConnected.store(false, std::memory_order_release);
}
