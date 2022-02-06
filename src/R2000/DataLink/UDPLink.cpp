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
          receptionByteBuffer(DATAGRAM_SIZE, 0),
          extractionByteBuffer(std::floor(DATAGRAM_SIZE * 1.5), 0) {
    const auto port{deviceHandle->getPort()};
    boost::asio::ip::udp::endpoint listenEndpoint{boost::asio::ip::udp::v4(), port};
    boost::system::error_code openError{};
    if (!socket.is_open()) {
        socket.open(listenEndpoint.protocol(), openError);
        if (openError) {
            std::clog << device->getName() << "::UDPLink::Could not open the socket to read from "
                      << device->getHostname() << ":" << port << " with ("
                      << openError.message() << ")" << std::endl;
        }
    }
    boost::system::error_code optionError{};
    socket.template set_option(boost::asio::ip::udp::socket::reuse_address(true), optionError);
    if (optionError) {
        std::clog << device->getName() << "::UDPLink::Could not set the reuse option on the socket to read from "
                  << device->getHostname() << ":" << port << " with (" << optionError.message() << ")" << std::endl;
    }
    boost::system::error_code bindError{};
    socket.bind(listenEndpoint, bindError);
    if (bindError) {
        std::clog << device->getName() << "::UDPLink::Could not bind the socket to read from " << device->getHostname()
                  << ":" << port << " with (" << bindError.message() << ")" << std::endl;
    }
    if (openError || optionError || bindError) {
        std::clog << device->getName() << "::UDPLink::Could not setup the Udp socket." << std::endl;
        isConnected.store(false, std::memory_order_release);
    } else {
        isConnected.store(true, std::memory_order_release);
        socket.async_receive_from(boost::asio::buffer(receptionByteBuffer), endPoint,
                                  [&](const boost::system::error_code &error, const unsigned int byteTransferred) {
                                      onBytesReceived(error, byteTransferred);
                                  });
        ioServiceTask = std::async(std::launch::async, [&]() {
            ioService.run();
        });
    }
}

void Device::UDPLink::onBytesReceived(const boost::system::error_code &error, unsigned int byteTransferred) {
    if (error) {
        if (error != boost::asio::error::operation_aborted) {
            std::clog << device->getName() << "::UDPLink::Network error (" << error.message() << ")" << std::endl;
        } else {
            std::clog << device->getName() << "::UDPLink::Cancelling operations on request." << std::endl;
        }
        isConnected.store(false, std::memory_order_release);
        return;
    }
    const auto begin{std::cbegin(receptionByteBuffer)};
    const auto end{std::next(begin, byteTransferred)};
    insertByteRangeIntoExtractionBuffer(begin, end);
    const auto from{std::cbegin(extractionByteBuffer)};
    const auto to{std::cend(extractionByteBuffer)};
    auto until{tryExtractingScanFromByteRange(from, to)};
    removeUsedByteRangeFromExtractionBufferBeginningUntil(until);
    socket.async_receive_from(boost::asio::buffer(receptionByteBuffer), endPoint,
                              [&](const auto &error, const auto byteTransferred) {
                                  onBytesReceived(error, byteTransferred);
                              });
}

Device::UDPLink::~UDPLink() {
    {
        boost::system::error_code error{};
        socket.shutdown(boost::asio::ip::tcp::socket::shutdown_receive, error);
        if (error) {
            std::clog << device->getName() << "::UDPLink::An error has occurred on socket shutdown (" << error.message()
                      << ")" << std::endl;
        }
    }
    {
        boost::system::error_code error{};
        socket.close(error);
        if (error) {
            std::clog << device->getName() << "::UDPLink::An error has occurred on socket closure (" << error.message()
                      << ")" << std::endl;
        }
    }
    if (!ioService.stopped()) {
        ioService.stop();
        ioServiceTask.wait();
    }
}
