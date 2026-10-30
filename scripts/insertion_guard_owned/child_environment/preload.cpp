// Owned constructor admission model. No target binary or native profile calls.
#include "reader_insertion_child_environment.h"
#include <climits>
#include <cstdint>
#include <cstring>
#include <unistd.h>

namespace {
pid_t admitted = 0;
bool clearedInConstructor = false;
[[noreturn]] void deny() noexcept {
    constexpr char text[] = "OWNED_PRELOAD_CHILD_DENIED\n";
    const auto written = ::write(STDERR_FILENO, text, sizeof(text) - 1);
    (void)written;
    std::_Exit(119);
}
__attribute__((constructor)) void qualify() {
    const char *expected = std::getenv("OWNED_EXPECTED_PARENT_PID");
    if (!expected || expected[0] < '1' || expected[0] > '9') deny();
    unsigned value = 0;
    for (const char *p = expected; *p; ++p) {
        if (*p < '0' || *p > '9' || value > (unsigned(INT_MAX) - unsigned(*p - '0')) / 10) deny();
        value = value * 10 + unsigned(*p - '0');
    }
    const char *bind = std::getenv("LD_BIND_NOW");
    if (value != unsigned(::getpid()) || !bind || std::strcmp(bind, "1") || !std::getenv("LD_PRELOAD")) deny();
    admitted = ::getpid();
    constexpr char text[] = "OWNED_PRELOAD_PARENT_QUALIFIED\n";
    if (::write(STDERR_FILENO, text, sizeof(text) - 1) != ssize_t(sizeof(text) - 1)) deny();
    const char *mode = std::getenv("OWNED_CLEAR_MODE");
    if (mode && std::strcmp(mode, "constructor-clear") == 0) {
        if (!reader_child_environment::clearInheritedInjection()) deny();
        clearedInConstructor = true;
        constexpr char clear[] = "OWNED_PRELOAD_ENV_CLEARED\n";
        if (::write(STDERR_FILENO, clear, sizeof(clear) - 1) != ssize_t(sizeof(clear) - 1)) deny();
    }
}
}
extern "C" __attribute__((visibility("default"))) std::uint64_t owned_injection_cookie() noexcept {
    return admitted == ::getpid() ? UINT64_C(0x5e1ec7edc001d00d) : 0;
}
extern "C" __attribute__((visibility("default"))) int owned_injection_cleared_in_constructor() noexcept {
    return clearedInConstructor ? 1 : 0;
}
