// Copyright (c) 2022 Papa Libasse Sow.
// https://github.com/Nandite/R2000
// Distributed under the MIT Software License (X11 license).
//
// SPDX-License-Identifier: MIT
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of
// the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "DataLink/TCPLink.hpp"
#include "Control/DeviceHandle.hpp"
#include <iostream>
#include <utility>

Device::TCPLink::TCPLink(std::shared_ptr<R2000> iDevice,
                         std::shared_ptr<DeviceHandle> iHandle) noexcept(false)
        : DataLink(std::move(iDevice), std::move(iHandle), 5s),
          socket(std::make_unique<boost::asio::ip::tcp::socket>(ioService)),
          receptionByteBuffer(DEFAULT_RECEPTION_BUFFER_SIZE, 0) {
    extractionByteBuffer.reserve(DEFAULT_EXTRACTION_BUFFER_SIZE);
    asynchronousResolveEndpoints();
    ioServiceTask = std::async(std::launch::async, [&]() {
        ioService.run();
    });
}

void Device::TCPLink::asynchronousResolveEndpoints() {
    const auto hostname{deviceHandle->getAddress()};
    const auto port{deviceHandle->getPort()};
    boost::asio::ip::tcp::resolver::query query{hostname.to_string(), std::to_string(port)};
    resolver.async_resolve(query, [&](const auto &error, auto queriedEndpoints) {
        if (!error) {
            endPoints = std::move(queriedEndpoints);
            asynchronousConnectSocket(std::begin(endPoints), std::end(endPoints));
        } else {
            if (error == boost::asio::error::operation_aborted) {
                std::clog << device->getName() << "::TCPLink::Cancelling endpoints resolution" << std::endl;
                return;
            }
            std::unique_lock<LockType> interruptGuard{interruptSocketAsyncOpsCvLock, std::adopt_lock};
            interruptSocketAsyncOpsCv.wait_for(interruptGuard, 10s, [&]() -> bool {
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
        interruptSocketAsyncOpsCv.wait_for(interruptGuard, 20s, [&]() -> bool {
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
        fireDataLinkConnectionLostEvent();
        return;
    }
    const auto begin{std::cbegin(receptionByteBuffer)};
    const auto end{std::next(begin, byteTransferred)};
    insertByteRangeIntoExtractionBuffer(begin, end);
    const auto from{std::cbegin(extractionByteBuffer)};
    const auto to{std::cend(extractionByteBuffer)};
    const auto extractionResult{tryExtractingScanFromByteRange(from, to)};
    const auto until{std::get<0>(extractionResult)};
    const auto numberOfBytesToTransfer{std::get<1>(extractionResult)};
    const auto &sizeOfAFullScan{std::get<2>(extractionResult)};
    removeUsedByteRangeFromExtractionBufferBeginningUntil(until);
    if (sizeOfAFullScan) {
        resizeReceptionAndExtractionBuffers(*sizeOfAFullScan);
    }
    boost::asio::async_read(*socket, boost::asio::buffer(receptionByteBuffer),
                            boost::asio::transfer_exactly(numberOfBytesToTransfer),
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




