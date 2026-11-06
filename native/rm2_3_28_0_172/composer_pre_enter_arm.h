#pragma once
// Private exact-build route mechanics. No live-process installation admission.
#include <cstdint>
#include <limits>

namespace remarkable::composer_pre_enter::detail {
constexpr std::uintptr_t Site = 0xb4a9b0, Original = 0xb492a0;
constexpr std::uint32_t OriginalWord = 0xebfffa3a;
constexpr std::uint32_t VeneerLoad = 0xe51ff004; // ldr pc,[pc,#-4], A32 tail transfer

inline bool branch(std::uintptr_t site, std::uintptr_t target, std::uint32_t &word) noexcept {
    if ((site | target) & 3U || site > UINT32_MAX - 8U || target > UINT32_MAX) return false;
    const auto delta = static_cast<std::int64_t>(target) - static_cast<std::int64_t>(site + 8);
    if (delta < -33554432 || delta > 33554428) return false;
    word = 0xeb000000U | (static_cast<std::uint32_t>(delta / 4) & 0xffffffU);
    return true;
}

enum class Installation { Unsupported };
// No boolean, raw-pointer installer or same-thread bypass. The private pre-main
// loader must establish installation admission before this stub can change.
inline Installation installProduction() noexcept { return Installation::Unsupported; }
} // namespace remarkable::composer_pre_enter::detail
