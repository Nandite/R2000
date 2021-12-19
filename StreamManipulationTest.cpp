#include "DataLink/DataLink.hpp"
#include "DataLinkBuilder.hpp"
#include "R2000.hpp"
#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <jsoncpp/json/json.h>
#include <random>
#include <thread>

#define assertm(exp, msg) assert(((void)msg, exp))

using namespace std::chrono_literals;
namespace basio = boost::asio;

template<typename Iterator>
void alignBuffer32Bits(Device::internals::Types::Buffer &buffer, Iterator begin, Iterator end) {
    constexpr auto byteSizeOf32Bits{sizeof(uint32_t)};
    static_assert(byteSizeOf32Bits == 4, "The size of uint32_t must be 4 bytes");
    const auto size{std::distance(begin, end)};
    const auto isAlreadyAligned{!(size % byteSizeOf32Bits)};
    if (isAlreadyAligned)
        return;
    auto smallestNumberGreaterThanNDivisibleByK = [](unsigned int N, unsigned int K) {
        unsigned int rem = (N + K) % K;
        if (rem == 0)
            return N;
        else
            return N + K - rem;
    };
    const auto alignedSize{smallestNumberGreaterThanNDivisibleByK(size, byteSizeOf32Bits)};
    assertm(alignedSize > size, "alignedSize must be > size");
    const auto remaining{alignedSize - size};
    for (auto u{0u}; u < remaining; ++u)
        buffer.push_back(0x00);
}

Device::internals::Types::Buffer randomBuffer(unsigned int size, const Device::internals::Types::Buffer::value_type low,
                                              const Device::internals::Types::Buffer::value_type high) {
    using value_type = Device::internals::Types::Buffer::value_type;
    // We use static in order to instantiate the random engine
    // and the distribution once only.
    // It may provoke some thread-safety issues.
    std::uniform_int_distribution<value_type> distribution(low, high);
    std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
    std::vector<value_type> data(size);
    std::generate(std::begin(data), std::end(data), [&distribution, &generator]() { return distribution(generator); });
    return data;
}

Device::internals::Types::Buffer randomMarkedBuffer(const Device::internals::Types::Buffer &mark,
                                                    unsigned int numberOfMarks,
                                                    const Device::internals::Types::Buffer::value_type low,
                                                    const Device::internals::Types::Buffer::value_type high,
                                                    const unsigned int maxFragmentLength) {
    Device::internals::Types::Buffer buffer{mark};
    std::uniform_int_distribution<unsigned int> distribution(0, maxFragmentLength);
    std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
    for (auto i{0u}; i < numberOfMarks; ++i) {
        const auto fragment{randomBuffer(distribution(generator), low, high)};
        buffer.insert(std::end(buffer), std::begin(fragment), std::end(fragment));
        buffer.insert(std::end(buffer), std::begin(mark), std::end(mark));
    }
    return buffer;
}

Device::internals::Types::Buffer randomHeaderMarkedBuffer(const Device::internals::Types::Buffer &magic,
                                                          unsigned int numberOfHeaders,
                                                          const Device::internals::Types::Buffer::value_type low,
                                                          const Device::internals::Types::Buffer::value_type high,
                                                          const unsigned int fragmentLength) {
    constexpr auto headerSize{sizeof(Device::Data::Header)};
    Device::internals::Types::Buffer buffer{};
    const auto headerWithoutMagicSize = headerSize - magic.size();
    for (auto i{0u}; i < numberOfHeaders; ++i) {
        const auto fragment{randomBuffer(fragmentLength, low, high)};
        const auto headerWithoutMagic{randomBuffer(headerWithoutMagicSize, low, high)};
        buffer.insert(std::end(buffer), std::begin(magic), std::end(magic));
        buffer.insert(std::end(buffer), std::begin(headerWithoutMagic), std::end(headerWithoutMagic));
        buffer.insert(std::end(buffer), std::begin(fragment), std::end(fragment));
    }
    return buffer;
}

int main() {
    const auto LOOP_TEST_COUNT = 20;
    using Buffer = Device::internals::Types::Buffer;
    const Buffer mark{0xa2, 0x5c};

    {
        const auto NumberOfMark{5u};
        auto buffer{randomMarkedBuffer(mark, NumberOfMark, 0, 0x2a, 4096)};
        alignBuffer32Bits(buffer, std::begin(buffer), std::end(buffer));
        std::reverse(std::begin(buffer), std::end(buffer)); // Little endian vector
        auto countedMark{0u};
        for (auto begin{std::begin(buffer)};;) {
            alignBuffer32Bits(buffer, begin, std::end(buffer));
            const auto result{Device::internals::retrievePacketMagic(begin, std::end(buffer))};
            if (!result)
                break;
            ++countedMark;
            begin = (*result).second;
            //            std::advance(begin, 2);
        };
        assertm(countedMark == NumberOfMark + 1, "Wrong number of counted mark");
    }

    for (auto n{0u}; n < LOOP_TEST_COUNT; ++n) {
        const auto unmarkedBuffer{randomBuffer(4096, 0, 0x2a)};
        assert(!Device::internals::retrievePacketMagic(std::begin(unmarkedBuffer), std::end(unmarkedBuffer)));
    }

    for (auto n{0u}; n < LOOP_TEST_COUNT; ++n) {
        auto callCounter{0u};
        const auto bufferSize{4096};
        const auto numberOf32BitsChunk{bufferSize / 4};
        const auto buffer{randomBuffer(bufferSize, 0, 0x2a)};
        Device::internals::retrieve32bitsFromAlignedSequence(std::begin(buffer), std::end(buffer), numberOf32BitsChunk,
                                                             [&callCounter](const uint32_t &) { ++callCounter; });
        assertm(callCounter == numberOf32BitsChunk, "callCounter != numberOf32BitsChunk");
    }

    for (auto n{0u}; n < LOOP_TEST_COUNT; ++n) {
        const auto unmarkedBuffer{randomBuffer(4096, 0, 0x2a)};
        assert(!Device::internals::retrievePacketHeader(std::begin(unmarkedBuffer), std::end(unmarkedBuffer)));
    }

    {
        const auto NumberOfHeader{10u};
        for (auto n{0u}; n < LOOP_TEST_COUNT; ++n) {
            constexpr auto headerSize{sizeof(Device::Data::Header)};
            Device::internals::Types::Buffer magic{0x5c, 0xa2};
            auto buffer{randomHeaderMarkedBuffer(magic, NumberOfHeader, 0, 0, 128)};
            auto countedHeader{0u};
            for (auto begin{std::begin(buffer)};;) {
                // alignBuffer32Bits(buffer, begin, std::end(buffer));
                const auto result{Device::internals::retrievePacketHeader(begin, std::end(buffer))};
                if (!result)
                    break;
                ++countedHeader;
                begin = (*result).first;
                std::advance(begin, headerSize);
            };
            assertm(countedHeader == NumberOfHeader + 1, "Wrong number of counted header");
        }
    }

    //    Device::DeviceConfiguration deviceConfiguration(basio::ip::address::from_string("192.168.1.30"));
    //    const auto controller = Device::R2000::makeShared(deviceConfiguration);
    //
    //    const auto result = controller->reboot(60s);
    //    if (result == Device::RebootStatus::FAILED || result == Device::RebootStatus::TIMEOUT)
    //    {
    //        std::clog << "Failed to reboot the device" << std::endl;
    //        return EXIT_FAILURE;
    //    }
    //    if (!controller->displayText({"Gorkem+Keser", "IUT+de+Rennes"}))
    //    {
    //        std::clog << "Could not display the required text on the sensor" << std::endl;
    //    }
    //    std::this_thread::sleep_for(5s);
    //
    //    try
    //    {
    //        const auto dataLink = Device::DataLinkBuilder()
    //                                  .withProtocol(Device::DeviceHandle::PROTOCOL::TCP)
    //                                  .withPacketType(Device::DeviceHandle::PACKET_TYPE::C)
    //                                  .withStartAngle(-180)
    //                                  .withPort(43000)
    //                                  .withTargetHostname("192.168.1.40")
    //                                  .withWatchdogEnabled()
    //                                  .withWatchdogTimeoutInMs(200ms)
    //                                  .build(controller);
    //        std::this_thread::sleep_for(20s);
    //        std::cout << "Handle opened" << std::endl;
    //    }
    //    catch (...)
    //    {
    //        std::clog << "Error, could not request any handle" << std::endl;
    //        return EXIT_FAILURE;
    //    }

    return EXIT_SUCCESS;
}
