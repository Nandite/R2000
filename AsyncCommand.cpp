//
// Created by chazz on 10/01/2022.
//
#include "Backtrace.hpp"
#include "R2000/R2000.hpp"
#include <boost/asio.hpp>
#include "R2000/Commands.hpp"

using namespace std::chrono_literals;
namespace basio = boost::asio;

int main()
{
    std::signal(SIGSEGV, Backtrace::printBacktraceAndExitHandler);
    std::signal(SIGABRT, Backtrace::printBacktraceAndExitHandler);
    std::signal(SIGILL, Backtrace::printBacktraceAndExitHandler);
    std::signal(SIGFPE, Backtrace::printBacktraceAndExitHandler);
    std::signal(SIGPIPE, Backtrace::printBacktraceAndExitHandler);
    std::signal(SIGTERM, Backtrace::printBacktraceAndExitHandler);

    const auto device{Device::R2000::makeShared({"R2000", "192.168.2.30"})};
    Device::Commands::FetchParametersCommand fetchParametersCommand{*device};
    auto future{fetchParametersCommand.execute(5s)};
    if(!future)
        return EXIT_FAILURE;
    future->wait();
    auto asyncCommandResult{future->get()};
    if(asyncCommandResult.second)
    {
        for(const auto &name : *asyncCommandResult.second)
            std::cout << name << std::endl;
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}
