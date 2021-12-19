#pragma once
#include <string>

namespace LocalizationToolkit::program
{
    std::string makeBacktrace(int skip);

    [[maybe_unused]] void printBacktraceAndExitHandler(int signal);
}
