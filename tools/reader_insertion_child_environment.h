#pragma once
// Current-process environment only. Existing mappings/bindings stay resident.
#include <cstdlib>

namespace reader_child_environment {
inline bool clearInheritedInjection() noexcept {
    const int preload = ::unsetenv("LD_PRELOAD");
    const int bindNow = ::unsetenv("LD_BIND_NOW");
    const bool preloadAbsent = std::getenv("LD_PRELOAD") == nullptr;
    const bool bindNowAbsent = std::getenv("LD_BIND_NOW") == nullptr;
    return preload == 0 && bindNow == 0 && preloadAbsent && bindNowAbsent;
}
} // namespace reader_child_environment
