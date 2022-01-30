//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#include "R2000/DataLink/TCPLink.hpp"
#include "R2000/Control/DeviceHandle.hpp"
#include <iostream>
#include <utility>

Device::TCPLink::TCPLink(std::shared_ptr<R2000> iDevice,
                         std::shared_ptr<DeviceHandle> iHandle) noexcept(false)
        : DataLink(std::move(iDevice), std::move(iHandle), 5s),
          socket(std::make_unique<boost::asio::ip::tcp::socket>(ioService)),
          receptionByteBuffer(DEFAULT_RECEPTION_BUFFER_SIZE, 0)//,
          //extractionByteBuffer(DEFAULT_EXTRACTION_BUFFER_SIZE, 0)
          {
    extractionByteBuffer.reserve(DEFAULT_EXTRACTION_BUFFER_SIZE);
    asynchronousResolveEndpoints();
    ioServiceTask = std::async(std::launch::async, [this]() {
        ioService.run();
        std::clog << device->getName() << "::TCPLink::Exiting io service task" << std::endl;
    });
}

void Device::TCPLink::asynchronousResolveEndpoints() {
    const auto hostname{deviceHandle->getAddress()};
    const auto port{deviceHandle->getPort()};
    boost::asio::ip::tcp::resolver::query query{hostname.to_string(), std::to_string(port)};
    resolver.async_resolve(query, [this](const auto &error, auto queriedEndpoints) {
        if (!error) {
            endPoints = std::move(queriedEndpoints);
            asynchronousConnectSocket(std::begin(endPoints), std::end(endPoints));
        } else {
            if (error == boost::asio::error::operation_aborted) {
                std::clog << device->getName() << "::TCPLink::Cancelling endpoints resolution" << std::endl;
                return;
            }
            std::unique_lock<LockType> interruptGuard{interruptSocketAsyncOpsCvLock, std::adopt_lock};
            interruptSocketAsyncOpsCv.wait_for(interruptGuard, 10s, [this]() -> bool {
                return interruptSocketAsyncOpsFlag.load(std::memory_order_acquire);
            });
            asynchronousResolveEndpoints();
        }
    });
}

void Device::TCPLink::onSocketConnectionAttemptCompleted(boost::system::error_code error,
                                                         const boost::asio::ip::tcp::resolver::iterator &endpoint) {
    if (!error) {
        std::cout << device->getName() << "::TCPLink::Connected to the device at endpoint (" << endpoint->endpoint()
                  << ") / and service (" << endpoint->service_name() << ")" << std::endl;
        isConnected.store(true, std::memory_order_release);
        boost::asio::async_read(*socket, boost::asio::buffer(receptionByteBuffer),
                                boost::asio::transfer_exactly(
                                        DEFAULT_RECEPTION_BUFFER_SIZE),
                                [&](const auto &error, const auto byteTransferred) {
                                    onBytesReceived(error, byteTransferred);
                                });
    } else if (error || endpoint == endPoints.end()) {
        if (error == boost::asio::error::operation_aborted) {
            std::clog << device->getName() << "::TCPLink::Cancelling socket connection to device" << std::endl;
            return;
        }
        isConnected.store(false, std::memory_order_release);
        std::unique_lock<LockType> interruptGuard{interruptSocketAsyncOpsCvLock, std::adopt_lock};
        interruptSocketAsyncOpsCv.wait_for(interruptGuard, 20s, [this]() -> bool {
            return interruptSocketAsyncOpsFlag.load(std::memory_order_acquire);
        });
        asynchronousResolveEndpoints();
    }
}

void Device::TCPLink::onBytesReceived(const boost::system::error_code &error, unsigned int byteTransferred) {
    if (error) {
        if (error != boost::asio::error::operation_aborted) {
            std::clog << device->getName() << "::TCPLink::Network error (" << error.message() << ")" << std::endl;
        } else {
            std::clog << device->getName() << "::TCPLink::Cancelling operations on request." << std::endl;
        }
        isConnected.store(false, std::memory_order_release);
        return;
    }
    std::cout << "Transferred " << byteTransferred << " byte(s)"<< std::endl;
    std::cout << "Reception buffer size/capacity : " << receptionByteBuffer.size() << "/"
              << receptionByteBuffer.capacity() << std::endl;
    const auto begin{std::cbegin(receptionByteBuffer)};
    const auto end{std::next(begin, byteTransferred)};
    insertByteRangeIntoExtractionBuffer(begin, end);
    const auto from{std::cbegin(extractionByteBuffer)};
    const auto to{std::cend(extractionByteBuffer)};
    const auto until{tryExtractingScanFromByteRange(from, to)};
    removeUsedByteRangeFromExtractionBuffer(until.first);
    boost::asio::async_read(*socket, boost::asio::buffer(receptionByteBuffer),
                            boost::asio::transfer_exactly(until.second),
                            [&](const auto &error, const auto byteTransferred) {
                                onBytesReceived(error, byteTransferred);
                            });
}

Device::TCPLink::~TCPLink() {
    if (!interruptSocketAsyncOpsFlag.load(std::memory_order_acquire)) {
        {
            std::unique_lock<LockType> guard{interruptSocketAsyncOpsCvLock, std::adopt_lock};
            interruptSocketAsyncOpsFlag.store(true, std::memory_order_release);
            interruptSocketAsyncOpsCv.notify_one();
        }
    }
    resolver.cancel();
    {
        boost::system::error_code error{};
        socket->shutdown(boost::asio::ip::tcp::socket::shutdown_receive, error);
        if (error) {
            std::clog << device->getName() << "::TCPLink::An error has occurred on socket shutdown (" << error.message()
                      << ")" << std::endl;
        }
    }
    {
        boost::system::error_code error{};
        socket->close(error);
        if (error) {
            std::clog << device->getName() << "::TCPLink::An error has occurred on socket closure (" << error.message()
                      << ")" << std::endl;
        }
    }

    if (!ioService.stopped()) {
        ioService.stop();
        ioServiceTask.wait();
    }
}




