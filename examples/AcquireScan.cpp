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

#include "DataLink/DataLink.hpp"
#include "DataLink/DataLinkBuilder.hpp"
#include "R2000.hpp"
#include <thread>

using namespace std::chrono_literals;

/**
 * @param address The address to test.
 * @return True if the address has an ipv4 valid form, False otherwise.
 */
[[nodiscard]] bool isValidIpv4(const std::string& address)
{
    boost::system::error_code errorCode{};
    const auto ipv4Address{boost::asio::ip::address::from_string(address, errorCode)};
    return !errorCode && ipv4Address.is_v4();
}

bool interruptProgram{false};

void interrupt(int)
{
    interruptProgram = true;
}

void printUsage(std::ostream& stream)
{
    stream << "Acquire scan from the sensor. Program usage:" << std::endl << "./AcquireScan <ipv4>";
}

int main(int argc, char** argv)
{
    std::signal(SIGTERM, interrupt);
    std::signal(SIGKILL, interrupt);
    std::signal(SIGINT, interrupt);

    if (argc < 2)
    {
        printUsage(std::cout);
        return EXIT_SUCCESS;
    }
    const std::string deviceAddress{argv[1]};
    if (!isValidIpv4(deviceAddress))
    {
        std::clog << "The provided address is not valid [" << deviceAddress << "]" << std::endl;
        printUsage(std::clog);
        return EXIT_FAILURE;
    }

    const auto device{Device::R2000::makeShared({"R2000", deviceAddress})};
    auto handleParameters{Device::Parameters::ReadWriteParameters::TcpHandle{}
                              .withWatchdog()
                              .withWatchdogTimeout(5000)};
    auto future{Device::DataLinkBuilder(handleParameters).build(device, 1s)};
    auto [requestResult, dataLink]{future.get()};
    if (requestResult != Device::RequestResult::SUCCESS)
    {
        std::clog << "Could not establish a data link with sensor at " << device->getHostname() << " ("
                  << Device::requestResultToString(requestResult) << ")." << std::endl;
        return EXIT_FAILURE;
    }
    dataLink->addOnNewScanAvailableCallback(
        [](const auto& newScan)
        { std::cout << "Scan number [" << newScan->getHeaders()[0].scanNumber << "] has been received" << std::endl; });

    while (!interruptProgram)
    {
        std::this_thread::sleep_for(1s);
        if(dataLink->isStalled())
        {
            std::clog << "Data link has stalled" << std::endl;
            break;
        }
    }
    std::cout << std::endl << "Stopping scan acquisition." << std::endl;
    return EXIT_SUCCESS;
}