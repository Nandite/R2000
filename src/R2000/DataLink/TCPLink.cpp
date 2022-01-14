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
        : DataLink(std::move(iDevice), std::move(iHandle)),
          socket(std::make_unique<boost::asio::ip::tcp::socket>(ioService)),
          receptionByteBuffer(DEFAULT_RECEPTION_BUFFER_SIZE, 0),
          extractionByteBuffer(EXTRACTION_BUFFER_SIZE, 0) {
    asynchronousResolveEndpoints();
    ioServiceTask = std::async(std::launch::async, [this]() {
        ioService.run();
        std::cout << "TCPLink::Exiting io service task" << std::endl;
    });
}

void Device::TCPLink::asynchronousResolveEndpoints() {
    const auto hostname{device->getHostname()};
    const auto port{deviceHandle->port};
    boost::asio::ip::tcp::resolver::query query{hostname.to_string(), std::to_string(port)};
    resolver.async_resolve(query, [this](const auto &error, auto queriedEndpoints) {
        if (!error) {
            endPoints = std::move(queriedEndpoints);
            asynchronousConnectSocket(std::begin(endPoints), std::end(endPoints));
        } else {
            if (error == boost::asio::error::operation_aborted) {
                std::clog << "TCPLink::Cancelling endpoints resolution" << std::endl;
                return;
            }
            std::unique_lock<LockType> interruptGuard{interruptAsyncOpsCvLock, std::adopt_lock};
            interruptAsyncOpsCv.wait_for(interruptGuard, 10s, [this]() -> bool {
                return interruptAsyncOpsFlag.load(std::memory_order_acquire);
            });
            asynchronousResolveEndpoints();
        }
    });
}

void Device::TCPLink::handleSocketConnection(boost::system::error_code error,
                                             const boost::asio::ip::tcp::resolver::iterator &endpoint) {
    if (!error) {
        std::cout << "TCPLink::Connected to the device at endpoint (" << endpoint->endpoint() << ") / and service ("
                  << endpoint->service_name() << ")" << std::endl;
        isConnected.store(true, std::memory_order_release);
        boost::asio::async_read(*socket, boost::asio::buffer(receptionByteBuffer),
                                boost::asio::transfer_exactly(
                                        DEFAULT_RECEPTION_BUFFER_SIZE),
                                [this](const auto &error, const auto byteTransferred) {
                                    handleBytesReception(error, byteTransferred);
                                });
    } else if (error || endpoint == endPoints.end()) {
        if (error == boost::asio::error::operation_aborted) {
            std::clog << "TCPLink::Cancelling socket connection to device" << std::endl;
            return;
        }
        isConnected.store(false, std::memory_order_release);
        std::unique_lock<LockType> interruptGuard{interruptAsyncOpsCvLock, std::adopt_lock};
        interruptAsyncOpsCv.wait_for(interruptGuard, 20s, [this]() -> bool {
            return interruptAsyncOpsFlag.load(std::memory_order_acquire);
        });
        asynchronousResolveEndpoints();
    }
}

void Device::TCPLink::handleBytesReception(const boost::system::error_code &error,
                                           const unsigned int byteTransferred) {
    if (error) {
        if (error != boost::asio::error::operation_aborted) {
            std::clog << "TCPLink::Network error (" << error.message() << ")" << std::endl;
        } else {
            std::cout << "TCPLink::Cancelling operations on request." << std::endl;
        }
        isConnected.store(false, std::memory_order_release);
        return;
    }
    const auto begin{std::cbegin(receptionByteBuffer)};
    const auto end{std::next(begin, byteTransferred)};
    const auto range{insertByteRangeIntoExtractionBuffer(begin, end)};
    const auto extractionResult{tryExtractingScanFromByteRange(range.first, range.second)};
    removeUsedByteRange(range.first, extractionResult.first, range.second);
    boost::asio::async_read(*socket, boost::asio::buffer(receptionByteBuffer),
                            boost::asio::transfer_exactly(extractionResult.second),
                            [this](const auto &error, const auto byteTransferred) {
                                handleBytesReception(error, byteTransferred);
                            });
}

Device::TCPLink::~TCPLink() {
    if (!interruptAsyncOpsFlag.load(std::memory_order_acquire)) {
        {
            std::unique_lock<LockType> guard(interruptAsyncOpsCvLock, std::adopt_lock);
            interruptAsyncOpsFlag.store(true, std::memory_order_release);
            interruptAsyncOpsCv.notify_one();
        }
    }
    resolver.cancel();
    boost::system::error_code placeholder;
    socket->cancel(placeholder);
    socket->shutdown(boost::asio::ip::tcp::socket::shutdown_receive, placeholder);
    socket->close(placeholder);
    if (!ioService.stopped()) {
        ioService.stop();
        ioServiceTask.wait();
    }
    isConnected.store(false, std::memory_order_release);
}




