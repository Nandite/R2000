#include "R2000/R2000.hpp"
#include "Backtrace.hpp"
#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <jsoncpp/json/json.h>
#include <random>
#include <thread>

#define assertm(exp, msg) assert(((void)msg, exp))

int main() {

    std::signal(SIGSEGV, Backtrace::printBacktraceAndExitHandler);
    std::signal(SIGABRT, Backtrace::printBacktraceAndExitHandler);
    std::signal(SIGILL, Backtrace::printBacktraceAndExitHandler);
    std::signal(SIGFPE, Backtrace::printBacktraceAndExitHandler);
    std::signal(SIGPIPE, Backtrace::printBacktraceAndExitHandler);
    std::signal(SIGTERM, Backtrace::printBacktraceAndExitHandler);

    const auto device{Device::R2000::makeShared({"R2000", "192.168.1.30"})};

    Device::Commands::FetchParametersCommand fetchParametersCommand{*device};
    Device::Commands::GetParametersCommand getParametersCommand{*device};
    Device::Commands::FactoryResetParametersCommand factoryResetParametersCommand{*device};
    Device::Commands::SetParametersCommand setParametersCommand{*device};

    Device::Commands::FactoryResetDeviceCommand factoryResetDeviceCommand {*device};
    if(!factoryResetDeviceCommand.execute())
    {
        std::clog << "Could not reset the device to factory parameters." << std::endl;
        return EXIT_FAILURE;
    }

//    const auto roParameters {fetchParametersCommand.execute()};
//    for(const auto & parameter : roParameters)
//        std::cout << parameter << std::endl;
//
//    const auto rwHmiParameters{Device::RWParameters::HmiDisplay()
//                                       .lockHmiButton()
//                                       .lockHmiParameters()
//                                       .withHmiLanguage(Device::Language::GERMAN)};
//    if (!setParametersCommand.execute(rwHmiParameters)) {
//        std::clog << "Could not set the HMI parameters" << std::endl;
//        return EXIT_FAILURE;
//    }
//
//    {
//        const auto roHmiParameters{Device::RWParameters::HmiDisplay::AsList{}.withLockHmiButton()
//                                           .withLockHmiParameters()
//                                           .withHmiLanguage()};
//        const auto roParameters{getParametersCommand.execute(roHmiParameters)};
//        for (const auto &parameter : roParameters)
//            std::cout << parameter.first << "/" << parameter.second << std::endl;
//
//        if (!factoryResetParametersCommand.execute(roHmiParameters)) {
//            std::clog << "Could not reset the HMI parameters" << std::endl;
//            return EXIT_FAILURE;
//        }
//    }
//
//    {
//        const auto roHmiParameters{Device::RWParameters::HmiDisplay::AsList{}.withLockHmiButton()
//                                           .withLockHmiParameters()
//                                           .withHmiLanguage()};
//        const auto roParameters{getParametersCommand.execute(roHmiParameters)};
//        for (const auto &parameter : roParameters)
//            std::cout << parameter.first << "/" << parameter.second << std::endl;
//    }

    return EXIT_SUCCESS;
}
