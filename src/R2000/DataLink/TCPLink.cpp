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
    socketConnectionTask = std::async(std::launch::async, &Device::TCPLink::backoffStrategySocketConnection, this);
}

bool Device::TCPLink::tryConnectSocketOnce() {
    const auto hostname{device->getHostname()};
    const auto port{deviceHandle->port};
    boost::asio::ip::tcp::resolver resolver{ioService};
    boost::asio::ip::tcp::resolver::query query{hostname.to_string(), std::to_string(port)};
    auto endpoints{resolver.resolve(query)};
    boost::system::error_code error{boost::asio::error::host_not_found};
    for (boost::asio::ip::tcp::resolver::iterator end{};
         error && endpoints != end && !interruptConnectionFlag.load(std::memory_order_acquire);
         ++endpoints) {
        boost::system::error_code placeholder{};
        socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, placeholder);
        socket->close(placeholder);
        //socket = std::make_unique<boost::asio::ip::tcp::socket>(ioService);
        socket->connect(*endpoints, error);
    }
    if (!error) {
        isConnected.store(true, std::memory_order_release);
        boost::asio::async_read(*socket, boost::asio::buffer(receptionByteBuffer),
                                boost::asio::transfer_exactly(
                                        DEFAULT_RECEPTION_BUFFER_SIZE),
                                [this](const boost::system::error_code &error,
                                       const unsigned int byteTransferred) {
                                    handleBytesReception(error, byteTransferred);
                                });
        ioServiceTask = std::async(std::launch::async, [this]() {
            ioService.run();
        });
    } else {
        std::clog << "Could not join the device (" << error.message() << ")"
                  << std::endl;
        isConnected.store(false, std::memory_order_release);
    }

    return !isConnected.load(std::memory_order_acquire) &&
           !interruptConnectionFlag.load(std::memory_order_acquire);
}

void Device::TCPLink::backoffStrategySocketConnection() {
    for (; !interruptConnectionFlag.load(std::memory_order_acquire);) {
        Retry::ExponentialBackoff(std::numeric_limits<unsigned int>::max(), 100ms, 20000ms,
                                  [this](const auto &duration) {
                                      std::unique_lock<LockType> interruptGuard{interruptConnectionCvLock,
                                                                                std::adopt_lock};
                                      interruptConnectionCv.wait_for(interruptGuard, duration, [this]() -> bool {
                                          return interruptConnectionFlag.load(std::memory_order_acquire);
                                      });
                                  },
                                  Retry::ForwardStatus,
                                  &Device::TCPLink::tryConnectSocketOnce, this);
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
    if (!interruptConnectionFlag.load(std::memory_order_acquire)) {
        {
            std::unique_lock<LockType> guard(interruptConnectionCvLock, std::adopt_lock);
            interruptConnectionFlag.store(true, std::memory_order_release);
            interruptConnectionCv.notify_one();
        }
        socketConnectionTask.wait();
    }
    if (!ioService.stopped()) {
        ioService.stop();
        ioServiceTask.wait();
    }
    boost::system::error_code placeholder;
    socket->shutdown(boost::asio::ip::tcp::socket::shutdown_receive, placeholder);
    socket->close(placeholder);
    isConnected.store(false, std::memory_order_release);
}
