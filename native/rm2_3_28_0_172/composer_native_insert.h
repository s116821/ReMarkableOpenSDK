#pragma once
// Private native insertion owner. Returned IDs still require live readback.
#include "composer_native_action.h"
#include "composer_anchor_walk.h"

namespace remarkable::composer_writer::detail {
struct InsertedInterval { NativeId first, last; };
class InsertProfile final {
    using Make = void (*)(ActionOwner *, void *, int, QString *, const std::uint32_t *);
    using Submit = void (*)(void *, const ActionOwner *);
    using Release = void (*)(void *);
    using Matches = bool (*)(const ActionOwner &, std::uintptr_t, int, const QString &);
    using ReadIds = InsertedInterval (*)(const ActionOwner &);
    Make make_; Submit submit_; Release release_; Matches matches_; ReadIds ids_;
    InsertProfile(Make make, Submit submit, Release release, Matches matches, ReadIds ids)
        : make_(make), submit_(submit), release_(release), matches_(matches), ids_(ids) {}
    friend class Insertion;
    friend class remarkable::composer_writer::OwnedEdit;
#ifdef COMPOSER_WRITER_TESTING
    friend struct remarkable::composer_writer::testing::Access;
#endif
    static bool matches(const ActionOwner &owner, std::uintptr_t item, int start, const QString &text) {
#if defined(__linux__) && defined(__arm__) && defined(__ARM_PCS_VFP) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
        using composer_scene::detail::word;
        if (!owner.control || owner.object != owner.control + 0x10 ||
            word(owner.control,0) != 0x13420f0 || word(owner.object,0) != 0x1341940 ||
            word(owner.object,4) != item || word(owner.object,8) != static_cast<std::uint32_t>(start) ||
            word(owner.object,0xc) != 0)
            return false;
        const auto &native = *reinterpret_cast<const QString *>(owner.object + 0x20);
        return native == text;
#else
        (void)owner; (void)item; (void)start; (void)text;
        return false;
#endif
    }
    static InsertedInterval ids(const ActionOwner &owner) {
        using composer_scene::detail::word;
        return {{word(owner.object,0x10),word(owner.object,0x14)},
                {word(owner.object,0x18),word(owner.object,0x1c)}};
    }
    static std::unique_ptr<InsertProfile> load() {
#if defined(__linux__) && defined(__arm__) && defined(__ARM_PCS_VFP) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) && QT_VERSION == QT_VERSION_CHECK(6,10,3)
        static_assert(sizeof(ActionOwner) == 8 && sizeof(QString) == 12);
        auto providers = composer_scene::Profile::load();
        if (!providers) return {};
        using composer_scene::detail::word;
        if (word(0xb6eaf4,0) != 0xe92d4ff0 || word(0xaac1f0,0) != 0xe92d43f0 ||
            word(0x1341940,8) != 0xb59b58 || word(0x1341940,12) != 0xb5dd30)
            return {};
        return std::unique_ptr<InsertProfile>(new InsertProfile(
            reinterpret_cast<Make>(0xb6eaf4), reinterpret_cast<Submit>(0xaac1f0),
            reinterpret_cast<Release>(0x9d620), matches, ids));
#else
        return {};
#endif
    }
};
class Insertion final {
    const InsertProfile &profile_;
    ActionOwner owner_;
    ActionReceipt receipt_;
    QString text_;
    bool finished_ = false;
    friend class remarkable::composer_writer::OwnedEdit;
#ifdef COMPOSER_WRITER_TESTING
    friend struct remarkable::composer_writer::testing::Access;
#endif
    explicit Insertion(const InsertProfile &profile) : profile_(profile) {}
    static bool supported(const QString &text) noexcept {
        if (text.isEmpty() || text.size() > static_cast<qsizetype>(composer_text::Result::Capacity)) return false;
        for (qsizetype i = 0; i < text.size(); ++i) {
            const auto c = text.at(i);
            const auto unit = c.unicode();
            if (!unit || unit == 0x0d || unit == 0x2029 || unit == 0xfeff || unit == 0xfffe ||
                unit == 0xfffc || unit == 0xfdd0 || unit == 0xfdd1)
                return false;
            if (c.isHighSurrogate()) {
                if (++i == text.size() || !text.at(i).isLowSurrogate()) return false;
            } else if (c.isLowSurrogate()) return false;
        }
        return true;
    }
    bool prepare(std::uintptr_t item, int start, const QString &text) noexcept {
        if (finished_ || receipt_.constructionAttempted || !item || start < 0 || !supported(text) ||
            start > std::numeric_limits<int>::max() - text.size()) return false;
        try { text_ = text; } catch (...) { return false; }
        receipt_.constructionAttempted = true;
        try {
            // Stock's fifth argument is an 8-byte optional flags value, passed
            // by address; byte+4 is its engagement flag. Null is not valid.
            const std::uint32_t absentFormat[2] = {0,0};
            profile_.make_(&owner_, reinterpret_cast<void *>(item), start, &text_, absentFormat);
            receipt_.constructed = text_ == text && profile_.matches_(owner_,item,start,text);
        } catch (...) {}
        if (!receipt_.constructed) {
            receipt_.quarantined = owner_.object || owner_.control;
            finished_ = true;
        }
        return receipt_.constructed;
    }
    void submit(std::uintptr_t stack) noexcept {
        if (finished_ || !receipt_.constructed || receipt_.submissionAttempted || !stack) return;
        receipt_.submissionAttempted = true;
        try {
            profile_.submit_(reinterpret_cast<void *>(stack), &owner_);
            receipt_.submissionReturned = true;
        } catch (...) {}
    }
    std::optional<InsertedInterval> interval() const noexcept {
        if (finished_ || !receipt_.constructed || !receipt_.submissionReturned) return {};
        try {
            const auto result = profile_.ids_(owner_);
            if (result.first.empty() || result.last.empty() || result.first.author() != result.last.author() ||
                result.last.counter() < result.first.counter() ||
                result.last.counter() - result.first.counter() + 1 != static_cast<std::uint64_t>(text_.size()))
                return {};
            return result;
        } catch (...) { return {}; }
    }
    void finish() noexcept {
        if (finished_) return;
        finished_ = true;
        if (!receipt_.constructed) return;
        receipt_.releaseAttempted = true;
        try {
            profile_.release_(reinterpret_cast<void *>(owner_.control));
            receipt_.releaseReturned = true; owner_ = {};
        } catch (...) { receipt_.quarantined = true; }
    }
    ActionReceipt receipt() const noexcept { return receipt_; }
public:
    Insertion(const Insertion &) = delete;
    Insertion &operator=(const Insertion &) = delete;
    Insertion(Insertion &&) = delete;
    Insertion &operator=(Insertion &&) = delete;
    ~Insertion() { finish(); }
};
} // namespace remarkable::composer_writer::detail
