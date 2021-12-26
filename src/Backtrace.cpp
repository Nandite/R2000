#include "Backtrace.hpp"
#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <sstream>
#include <cstring>
#include <iostream>

std::string Backtrace::makeBacktrace(int skip) {
    void *callStack[128];
    const int nMaxFrames = sizeof(callStack) / sizeof(callStack[0]);
    char buf[1024];
    const int nFrames = backtrace(callStack, nMaxFrames);
    char **symbols = backtrace_symbols(callStack, nFrames);

    std::ostringstream trace_buf;
    for (int i = skip; i < nFrames; i++) {
        printf("%s\n", symbols[i]);

        Dl_info info;
        if (dladdr(callStack[i], &info) && info.dli_sname) {
            char *demangled = nullptr;
            int status = -1;
            if (info.dli_sname[0] == '_')
                demangled = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);
            snprintf(buf, sizeof(buf), "%-3d %*p %s + %zd\n", i, int(2 + sizeof(void *) * 2), callStack[i],
                     status == 0 ? demangled
                                 : info.dli_sname == nullptr ? symbols[i]
                                                             : info.dli_sname,
                     (char *) callStack[i] - (char *) info.dli_saddr);
            free(demangled);
        } else {
            snprintf(buf, sizeof(buf), "%-3d %*p %s\n", i, int(2 + sizeof(void *) * 2), callStack[i], symbols[i]);
        }
        trace_buf << buf;
    }
    free(symbols);
    if (nFrames == nMaxFrames)
        trace_buf << "[truncated]\n";
    return trace_buf.str();
}

void Backtrace::printBacktraceAndExitHandler(int signal) {
    std::cout << "Signal " << signal << " -> " << strsignal(signal) << " caught." << std::endl;
    makeBacktrace(1);
    exit(signal);
}
