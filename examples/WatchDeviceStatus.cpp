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

#include "R2000.hpp"
#include "StatusWatcher.hpp"
#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
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
    stream << "Execute a device status watcher over the network. Program usage:" << std::endl
           << "./WatchDeviceStatus <ipv4>";
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

    Device::DeviceConfiguration configuration{"R2000", deviceAddress};
    std::unique_ptr<Device::StatusWatcher> statusWatcher{std::make_unique<Device::StatusWatcher>(5s, configuration)};
    statusWatcher->addOnStatusAvailableCallback(
        [](auto status)
        {
            std::stringstream stream{};
            stream << "+ Cpu Load: " << status->getCpuLoad() << std::endl;
            stream << "+ Current temperature: " << status->getCurrentTemperature() << std::endl;
            stream << "+ Operation time: " << status->getOperationTime() << std::endl;
            stream << "+ Raw system time: " << status->getRawSystemTime() << std::endl;
            std::cout << stream.str() << std::endl;
        });

    while (!interruptProgram)
    {
        std::this_thread::sleep_for(1s);
    }
    std::cout << std::endl << "Stopping device status watcher." << std::endl;
    return EXIT_SUCCESS;
}
