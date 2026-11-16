#pragma once
// Private stock action ownership. This is not an edit/range admission API.
#include "composer_scene_attachment.h"
#include <cstdint>
#include <memory>

namespace remarkable::composer_writer {
class OwnedEdit;
#ifdef COMPOSER_WRITER_TESTING
namespace testing { struct Access; }
#endif
namespace detail {
// The stock factory returns a non-trivial shared_ptr through the AAPCS hidden
// result pointer. Do not return this trivial struct by value from the FFI call.
struct ActionOwner { std::uintptr_t object = 0, control = 0; };
struct ActionReceipt {
    bool constructionAttempted = false, constructed = false;
    bool submissionAttempted = false, submissionReturned = false;
    bool releaseAttempted = false, releaseReturned = false;
    bool quarantined = false;
    // A returned void submission is not evidence that the edit was applied.
};
class ActionProfile final {
    using Make = void (*)(ActionOwner *, void *, int, int, int);
    using Submit = void (*)(void *, const ActionOwner *);
    using Release = void (*)(void *);
    using Matches = bool (*)(const ActionOwner &, std::uintptr_t, int, int);
    Make make_ = nullptr;
    Submit submit_ = nullptr;
    Release release_ = nullptr;
    Matches matches_ = nullptr;
    explicit ActionProfile(Make make, Submit submit, Release release, Matches matches)
        : make_(make), submit_(submit), release_(release), matches_(matches) {}
    friend class Removal;
    friend class remarkable::composer_writer::OwnedEdit;
#ifdef COMPOSER_WRITER_TESTING
    friend struct remarkable::composer_writer::testing::Access;
#endif
    static bool matches(const ActionOwner &owner, std::uintptr_t item, int start, int length) {
#if defined(__linux__) && defined(__arm__) && defined(__ARM_PCS_VFP) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
        using composer_scene::detail::word;
        return owner.control && owner.object == owner.control + 0x10 &&
            word(owner.control, 0) == 0x134187c &&
            word(owner.object, 0) == 0x1341980 && word(owner.object, 4) == item &&
            word(owner.object, 8) == static_cast<std::uint32_t>(start) &&
            word(owner.object, 0xc) == 0 &&
            word(owner.object, 0x10) == static_cast<std::uint32_t>(length);
#else
        (void)owner; (void)item; (void)start; (void)length;
        return false;
#endif
    }
    // Called before scheduling a native borrow, never from its callback. Uses
    // the accepted provider admission and additionally pins this action surface.
    static std::unique_ptr<ActionProfile> load() {
#if defined(__linux__) && defined(__arm__) && defined(__ARM_PCS_VFP) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) && QT_VERSION == QT_VERSION_CHECK(6,10,3)
        static_assert(sizeof(ActionOwner) == 8);
        auto providers = composer_scene::Profile::load();
        if (!providers) return {};
        using composer_scene::detail::word;
        // R37: normal kind-0 removal factory and normal SceneActionStack path.
        // The shortcut b68924 deliberately is not used: it bypasses history.
        if (word(0xb69f84, 0) != 0xe92d41f0 ||
            word(0xaac1f0, 0) != 0xe92d43f0 ||
            word(0x1341980, 8) != 0xb59e6c ||
            word(0x1341980, 12) != 0xb5d540)
            return {};
        return std::unique_ptr<ActionProfile>(new ActionProfile(
            reinterpret_cast<Make>(0xb69f84), reinterpret_cast<Submit>(0xaac1f0),
            reinterpret_cast<Release>(0x9d620), matches));
#else
        return {};
#endif
    }
};

// No raw-pointer constructor or submit method is public. The forthcoming
// OwnedEdit transaction must prove owner, anchors, revision and plain shape;
// constructing an action below grants none of those facts.
class Removal final {
    const ActionProfile &profile_;
    ActionOwner owner_;
    ActionReceipt receipt_;
    bool finished_ = false;
    friend class remarkable::composer_writer::OwnedEdit;
#ifdef COMPOSER_WRITER_TESTING
    friend struct remarkable::composer_writer::testing::Access;
#endif
    explicit Removal(const ActionProfile &profile) : profile_(profile) {}
    bool prepare(std::uintptr_t item, int start, int length) noexcept {
        if (finished_ || receipt_.constructionAttempted || !item || start < 0 || length <= 0 ||
            start > std::numeric_limits<int>::max() - length)
            return false;
        receipt_.constructionAttempted = true;
        try {
            profile_.make_(&owner_, reinterpret_cast<void *>(item), start, length, 0);
            receipt_.constructed = profile_.matches_(owner_, item, start, length);
        } catch (...) {
            // The factory may have run. A malformed/partial owner cannot be
            // passed to a deleter whose ownership contract was not established.
        }
        if (!receipt_.constructed) {
            receipt_.quarantined = owner_.object || owner_.control;
            finished_ = true;
        }
        return receipt_.constructed;
    }
    void submit(std::uintptr_t stack) noexcept {
        if (finished_ || !receipt_.constructed || receipt_.submissionAttempted || !stack) return;
        receipt_.submissionAttempted = true; // Sticky BEFORE the native call.
        try {
            profile_.submit_(reinterpret_cast<void *>(stack), &owner_);
            receipt_.submissionReturned = true;
        } catch (...) {
            // Unknown-after-attempt. Never retry or claim a no-op from a throw.
        }
    }
    void finish() noexcept {
        if (finished_) return;
        finished_ = true;
        if (!receipt_.constructed) return;
        receipt_.releaseAttempted = true;
        try {
            // Drop only the factory's local reference. SceneActionStack may
            // have retained/merged it; its native history owns that decision.
            profile_.release_(reinterpret_cast<void *>(owner_.control));
            receipt_.releaseReturned = true;
            owner_ = {};
        } catch (...) {
            receipt_.quarantined = true; // Do not attempt a second decrement.
        }
    }
    ActionReceipt receipt() const noexcept { return receipt_; }
public:
    Removal(const Removal &) = delete;
    Removal &operator=(const Removal &) = delete;
    Removal(Removal &&) = delete;
    Removal &operator=(Removal &&) = delete;
    ~Removal() { finish(); }
};
} // namespace detail
} // namespace remarkable::composer_writer
