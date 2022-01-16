//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#include <utility>
#include <iostream>
#include "R2000/Control/DeviceHandle.hpp"
#include "R2000/DataLink/UDPLink.hpp"

Device::UDPLink::UDPLink(std::shared_ptr<R2000> iDevice, std::shared_ptr<DeviceHandle> iHandle)
        : DataLink(std::move(iDevice), std::move(iHandle), 1s),
          socket(std::make_unique<boost::asio::ip::udp::socket>(ioService, boost::asio::ip::udp::v4())),
          receptionByteBuffer(DATAGRAM_SIZE, 0),
          extractionByteBuffer(std::floor(DATAGRAM_SIZE * 1.5), 0) {

    const auto port{deviceHandle->port};
    boost::asio::ip::udp::endpoint listenEndpoint{boost::asio::ip::udp::v4(), port};
    boost::system::error_code openError{};
    if (!socket->is_open()) {
        socket->open(listenEndpoint.protocol(), openError);
        if (openError) {
            std::clog << "Could not open the socket to read from " << device->getHostname() << ":" << port << " with ("
                      << openError.message() << ")" << std::endl;
        }
    }
    boost::system::error_code optionError{};
    socket->template set_option(boost::asio::ip::udp::socket::reuse_address(true), optionError);
    if (optionError) {
        std::clog << "Could not set the reuse option on the socket to read from " << device->getHostname() << ":"
                  << port << " with ("
                  << optionError.message() << ")" << std::endl;
    }
    boost::system::error_code bindError{};
    socket->bind(listenEndpoint, bindError);
    if (bindError) {
        std::clog << "Could not bind the socket to read from " << device->getHostname() << ":"
                  << port << " with ("
                  << bindError.message() << ")" << std::endl;
    }
    if (openError || optionError || bindError) {
        std::clog << __func__ << ":: Could not setup the Udp socket." << std::endl;
        isConnected.store(false, std::memory_order_release);
    } else {
        isConnected.store(true, std::memory_order_release);
        socket->async_receive_from(boost::asio::buffer(receptionByteBuffer), endPoint,
                                   [this](const boost::system::error_code &error, const unsigned int byteTransferred) {
                                       handleBytesReception(error, byteTransferred);
                                   });
        ioServiceTask = std::async(std::launch::async, [this]() {
            ioService.run();
        });
    }
}

void Device::UDPLink::handleBytesReception(const boost::system::error_code &error, unsigned int byteTransferred) {
    if (error) {
        std::clog << __func__ << ":: Network error (" << error.message() << ")" << std::endl;
        isConnected.store(false, std::memory_order_release);
        return;
    }
    const auto begin{std::cbegin(receptionByteBuffer)};
    const auto end{std::next(begin, byteTransferred)};
    const auto range{insertByteRangeIntoExtractionBuffer(begin, end)};
    auto position{tryExtractingScanFromByteRange(range.first, range.second)};
    removeUsedByteRange(range.first, position, range.second);
    socket->async_receive_from(boost::asio::buffer(receptionByteBuffer), endPoint,
                               [this](const auto &error, const auto byteTransferred) {
                                   handleBytesReception(error, byteTransferred);
                               });
}

Device::UDPLink::~UDPLink() {
    boost::system::error_code placeholder;
    socket->shutdown(boost::asio::ip::udp::socket::shutdown_receive, placeholder);
    socket->close(placeholder);
    if (!ioService.stopped()) {
        ioService.stop();
        ioServiceTask.wait();
    }
}
