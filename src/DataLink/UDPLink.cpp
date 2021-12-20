//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#include <utility>
#include <iostream>
#include "DeviceHandle.hpp"
#include "DataLink/UDPLink.hpp"

Device::UDPLink::UDPLink(std::shared_ptr<R2000> device, std::shared_ptr<DeviceHandle> handle)
        : DataLink(std::move(device), std::move(handle)),
          socket(std::make_unique<boost::asio::ip::udp::socket>(ioService, boost::asio::ip::udp::v4())),
          receptionByteBuffer(DATAGRAM_SIZE, 0),
          extractionByteBuffer(std::floor(DATAGRAM_SIZE * 1.5), 0) {

    const auto port{mHandle->port};
    boost::asio::ip::udp::endpoint listenEndpoint{boost::asio::ip::udp::v4(), port};
    boost::system::error_code openError{};
    if (!socket->is_open()) {
        socket->open(listenEndpoint.protocol(), openError);
        if (openError) {
            std::clog << "Could not open the socket to read from " << mDevice->getHostname() << ":" << port << " with ("
                      << openError.message() << ")" << std::endl;
        }
    }
    boost::system::error_code optionError{};
    socket->template set_option(boost::asio::ip::udp::socket::reuse_address(true), optionError);
    if (optionError) {
        std::clog << "Could not set the reuse option on the socket to read from " << mDevice->getHostname() << ":"
                  << port << " with ("
                  << optionError.message() << ")" << std::endl;
    }
    boost::system::error_code bindError{};
    socket->bind(listenEndpoint, bindError);
    if (bindError) {
        std::clog << "Could not bind the socket to read from " << mDevice->getHostname() << ":"
                  << port << " with ("
                  << bindError.message() << ")" << std::endl;
    }
    if (openError || optionError || bindError) {
        std::clog << __func__ << ":: Could not setup the Udp socket." << std::endl;
        mIsConnected.store(false, std::memory_order_release);
    } else {
        mIsConnected.store(true, std::memory_order_release);
        socket->async_receive_from(boost::asio::buffer(receptionByteBuffer), endPoint,
                                   [this](const boost::system::error_code &error, const unsigned int byteTransferred) {
                                       handleBytesReception(error, byteTransferred);
                                   });
        ioServiceThread = std::thread([this]() {
            ioService.run();
        });
    }
}

void Device::UDPLink::handleBytesReception(const boost::system::error_code &error, unsigned int byteTransferred) {
    if (!error) {
        const auto streamBegin{std::begin(receptionByteBuffer)};
        const auto streamEnd{std::next(streamBegin, byteTransferred)};
        extractionByteBuffer.insert(std::end(extractionByteBuffer), streamBegin, streamEnd);
        const auto extractionBegin{std::begin(extractionByteBuffer)};
        const auto extractionEnd{std::end(extractionByteBuffer)};
        auto position{extractionBegin};
        for (;;) {
            const auto extractionResult{extractScanPacketFromByteRange(position, extractionEnd, scanFactory)};
            const auto hadEnoughBytes{std::get<0>(extractionResult)};
            position = std::get<1>(extractionResult);
            if (scanFactory.isComplete()) {
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
        socket->async_receive_from(boost::asio::buffer(receptionByteBuffer), endPoint,
                                   [this](const boost::system::error_code &error,
                                          const unsigned int byteTransferred) {
                                       handleBytesReception(error, byteTransferred);
                                   });
    } else {
        std::clog << __func__ << ":: Network error (" << error.message() << ")" << std::endl;
        mIsConnected.store(false, std::memory_order_release);
    }
}

Device::UDPLink::~UDPLink() {
    if (!ioService.stopped())
        ioService.stop();
    if (ioServiceThread.joinable())
        ioServiceThread.join();
    boost::system::error_code placeholder;
    socket->shutdown(boost::asio::ip::udp::socket::shutdown_receive, placeholder);
    socket->close(placeholder);
    mIsConnected.store(false, std::memory_order_release);
}
