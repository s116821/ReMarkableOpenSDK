#pragma once
// Private exact-build route mechanics. No live-process installation admission.
#include <cstdint>
#include <limits>
#ifdef RSDK_DEVELOPMENT_PRE_ENTER_DIAGNOSTIC_V1
#include <string_view>
#if RSDK_DEVELOPMENT_PRE_ENTER_DIAGNOSTIC_V1 != 1 || !defined(COMPOSER_TEXT_PRIVATE_QT) || !defined(RSDK_PROFILE_SHA256)
#error Pre-Enter diagnostic requires the exact build gate, manifest and private Qt.
#endif
#if defined(RSDK_INSTALLED_READER_INSERT_V1) || defined(RSDK_DEVELOPMENT_EFFECT_PROBE_V1) || \
    defined(RSDK_DEVELOPMENT_REPEAT_INSERT_V1) || defined(RSDK_DEVELOPMENT_ORDINARY_CLOSE_V1) || \
    defined(RSDK_DEVELOPMENT_FIXTURE_REOPEN_V1) || defined(COMPOSER_PRE_ENTER_TESTING) || \
    defined(COMPOSER_PATCH_OWNED_TESTING) || defined(RSDK_PRE_ENTER_MARKER_TESTING) || \
    defined(COMPOSER_TEXT_TESTING) || defined(COMPOSER_SCENE_TESTING) || \
    defined(COMPOSER_OBSERVER_TESTING) || defined(COMPOSER_DELIVERY_TESTING)
#error Pre-Enter diagnostic cannot be combined with mutation or fixture gates.
#endif
#endif

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

// Only the dedicated build may select startup. Runtime admission remains
// independent and still checks the exact manifest and artifact digests.
#ifdef RSDK_DEVELOPMENT_PRE_ENTER_DIAGNOSTIC_V1
static_assert(std::string_view(RSDK_PROFILE_SHA256) ==
              "3aff53ec9aa52ecc0ab61e1190dae6cd2dc8e0d78055473ff78a5b5be0d0f2d9",
              "Incorrect pre-Enter diagnostic manifest");
constexpr bool StartupSelected = true;
#else
constexpr bool StartupSelected = false;
#endif
} // namespace remarkable::composer_pre_enter::detail
