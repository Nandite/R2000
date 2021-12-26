#pragma once
#include <string>

namespace Backtrace
{
    std::string makeBacktrace(int skip);

    [[maybe_unused]] void printBacktraceAndExitHandler(int signal);
}
