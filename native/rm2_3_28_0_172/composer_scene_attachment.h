#pragma once
// Exact-build diagnostic attachment. No native dispatch or draft authority.
#include "composer_document_observer.h"
#include "composer_text_copy.h"
#include <array>
#include <optional>
#if defined(__linux__)
#include "../resident/loaded_artifact.h"
#include <dlfcn.h>
#include <link.h>
#endif
#include <cstring>
#include <functional>
#include <limits>

// This name matches the retained std::function<bool(Scene*)> metatype.
class Scene;

namespace remarkable::composer_scene {
using Callback = std::function<bool(::Scene *)>;
using Clock = composer_diagnostic::Clock;
enum class Refusal {
    None, Unsupported, Canceled, Deadline, Duplicate, Busy, NullScene, MissingRoot,
    SceneBackReference, FocusMismatch, ItemType, NativeTransaction, Composition,
    DocumentThread, QtTransaction, Observer, Text, Exception, Scalar
};
struct Facts {
    std::uint32_t idHigh = 0, idLow = 0;
    std::uint32_t operationScope = 0, imeScope = 0, imeNesting = 0;
    std::int32_t composeStart = 0, composeLength = 0, virtualExtension = 0;
    std::int32_t qtEditDepth = 0;
    std::uintptr_t documentAddress = 0; // Correspondence only; never dereferenced later.
};
enum class Mode { Observe, Scalars };
enum class ScalarStatus { NotRequested, Pending, Observed, Refused };
struct ScalarSample {
    bool undoRedoEnabled, undoAvailable, redoAvailable;
    int maximumBlockCount, revision;
    bool signalsBlocked, cachedWasUndoAvailable, cachedWasRedoAvailable;

    bool operator==(const ScalarSample &other) const noexcept {
        return undoRedoEnabled == other.undoRedoEnabled && undoAvailable == other.undoAvailable &&
            redoAvailable == other.redoAvailable && maximumBlockCount == other.maximumBlockCount &&
            revision == other.revision && signalsBlocked == other.signalsBlocked &&
            cachedWasUndoAvailable == other.cachedWasUndoAvailable &&
            cachedWasRedoAvailable == other.cachedWasRedoAvailable;
    }
};
struct ScalarResult {
    ScalarStatus status = ScalarStatus::NotRequested;
    std::optional<ScalarSample> sample;
};
struct Join {
    QTextDocument *document = nullptr;
    Facts facts;
    Refusal refusal = Refusal::None;
    // Borrow-local correspondence. Never copied into the published snapshot.
    std::uintptr_t root = 0, control = 0, participant = 0, privateData = 0;
};
namespace detail {

#if defined(__linux__) && defined(__arm__)
inline bool nativeImageMapped() {
    bool matched = false;
    ::dl_iterate_phdr([](dl_phdr_info *info, std::size_t, void *context) {
        if (info->dlpi_addr || (info->dlpi_name && *info->dlpi_name)) return 0;
        struct Span { std::uintptr_t start; std::size_t size; unsigned flags; };
        const Span spans[] = {{0x6d6ce4,8,PF_R|PF_X}, {0x1513188,24,PF_R},
            {0x121a9d4,sizeof("std::function<bool(Scene*)>"),PF_R},
            {0x133e6f8,12,PF_R}, {0x133e750,8,PF_R}};
        for (const auto &span : spans) {
            bool found = false;
            for (unsigned i = 0; i < info->dlpi_phnum; ++i) {
                const auto &segment = info->dlpi_phdr[i];
                if (segment.p_type != PT_LOAD || (segment.p_flags & span.flags) != span.flags)
                    continue;
                if (span.start >= segment.p_vaddr &&
                    span.start - segment.p_vaddr <= segment.p_memsz &&
                    span.size <= segment.p_memsz - (span.start - segment.p_vaddr))
                    found = true;
            }
            if (!found) return 0;
        }
        *static_cast<bool *>(context) = true;
        return 1;
    }, &matched);
    return matched;
}
#endif

inline std::uint32_t word(std::uintptr_t base, std::size_t offset) {
    std::uint32_t value;
    std::memcpy(&value, reinterpret_cast<const void *>(base + offset), sizeof value);
    return value;
}
// Preconditions: the exact native callback supplied a live Scene on its actual
// owner thread; provider layout has been pinned. No arbitrary-pointer safety is
// implied. Nothing in this function is a native owner acquisition or release.
inline Join join(::Scene *borrowed) {
    Join result;
#if defined(__linux__) && defined(__arm__) && defined(__ARM_PCS_VFP) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    static_assert(sizeof(void *) == 4 && sizeof(QTextDocument) == 8);
    const auto scene = reinterpret_cast<std::uintptr_t>(borrowed);
    if (!scene) { result.refusal = Refusal::NullScene; return result; }
    const auto participant = word(scene, 0x26c);
    const auto item = word(scene, 0x190);
    const auto control = word(scene, 0x194);
    if (!participant || !item || !control) {
        result.refusal = Refusal::MissingRoot; return result;
    }
    if (word(participant, 8) != scene) {
        result.refusal = Refusal::SceneBackReference; return result;
    }
    // Exact most-derived SceneTextItem vptrs; subclasses are unsupported.
    if (word(item, 0) != 0x133e700 || word(item, 8) != 0x133e758) {
        result.refusal = Refusal::ItemType; return result;
    }
    auto &f = result.facts;
    f.idHigh = word(item, 0x10); f.idLow = word(item, 0x14);
    if (word(participant, 0x10) != f.idHigh || word(participant, 0x14) != f.idLow) {
        result.refusal = Refusal::FocusMismatch; return result;
    }
    f.operationScope = word(participant, 0x30);
    f.imeScope = word(participant, 0x34); f.imeNesting = word(participant, 0x44);
    if (f.operationScope || f.imeScope || f.imeNesting) {
        result.refusal = Refusal::NativeTransaction; return result;
    }
    f.composeStart = static_cast<std::int32_t>(word(item, 0x260));
    f.composeLength = static_cast<std::int32_t>(word(item, 0x264));
    f.virtualExtension = static_cast<std::int32_t>(word(item, 0x268));
    if (f.composeStart != -1 || f.composeLength || f.virtualExtension) {
        result.refusal = Refusal::Composition; return result;
    }
    auto *document = reinterpret_cast<QTextDocument *>(item + 0x1ec);
    if (document->thread() != QThread::currentThread()) {
        result.refusal = Refusal::DocumentThread; return result;
    }
    const auto privateData = word(item + 0x1ec, 4);
    if (!privateData) { result.refusal = Refusal::QtTransaction; return result; }
    f.qtEditDepth = static_cast<std::int32_t>(word(privateData, 0x74));
    if (f.qtEditDepth) { result.refusal = Refusal::QtTransaction; return result; }
    f.documentAddress = reinterpret_cast<std::uintptr_t>(document);
    result.document = document;
    result.root = item;
    result.control = control;
    result.participant = participant;
    result.privateData = privateData;
#else
    (void)borrowed;
    result.refusal = Refusal::Unsupported;
#endif
    return result;
}
using CacheBytes = std::optional<std::array<unsigned char, 2>>;

inline CacheBytes readScalarCaches(const Join &joined) {
#if defined(__linux__) && defined(__arm__) && defined(__ARM_PCS_VFP) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) && QT_VERSION == QT_VERSION_CHECK(6,10,3)
    std::array<unsigned char, 2> bytes;
    std::memcpy(bytes.data(), reinterpret_cast<const void *>(joined.privateData + 0x40), bytes.size());
    return bytes;
#else
    (void)joined;
    return {};
#endif
}

inline std::optional<ScalarSample> readScalars(const Join &joined, CacheBytes (*readCaches)(const Join &)) {
    if (!joined.root || !joined.control || !joined.participant || !joined.privateData ||
        joined.facts.documentAddress != reinterpret_cast<std::uintptr_t>(joined.document) ||
        joined.document->thread() != QThread::currentThread())
        return {};
    const auto cache = readCaches(joined);
    if (!cache || (*cache)[0] > 1 || (*cache)[1] > 1) return {};
    const auto &doc = *joined.document;
    return ScalarSample{doc.isUndoRedoEnabled(), doc.isUndoAvailable(), doc.isRedoAvailable(),
        doc.maximumBlockCount(), doc.revision(), doc.signalsBlocked(),
        (*cache)[0] == 1, (*cache)[1] == 1};
}

inline bool sameScalarBorrow(const Join &first, const Join &again) {
    return again.refusal == Refusal::None && again.document == first.document &&
        again.root == first.root && again.control == first.control &&
        again.participant == first.participant && again.privateData == first.privateData &&
        again.facts.idHigh == first.facts.idHigh && again.facts.idLow == first.facts.idLow;
}

// Shared final read check. Both callers must still hold their admitted native
// borrow; this helper does not extend it or acquire native ownership.
inline void revalidateText(const Join &first, const Join &again, composer_text::Result &candidate) {
    const auto &f = again.facts;
    if (again.refusal != Refusal::None || again.document != first.document ||
        f.documentAddress != first.facts.documentAddress ||
        f.idHigh != first.facts.idHigh || f.idLow != first.facts.idLow) {
        candidate.refuse(composer_text::Failure::Join);
    } else if (first.document->revision() != candidate.revisionAfter ||
               first.document->characterCount() != candidate.characterCount) {
        candidate.refuse(composer_text::Failure::Revision);
    }
}
} // namespace detail

class Attachment;
#ifdef COMPOSER_SCENE_TESTING
namespace testing { struct Access; }
#endif
class Profile final {
    explicit Profile(bool scalarLayout = false) : scalarLayout_(scalarLayout) {}
    const bool scalarLayout_;
    friend class Attachment;
#ifdef COMPOSER_SCENE_TESTING
    friend struct testing::Access;
#endif
public:
    // File/mapping identity only, within a trusted process. No device contact or
    // runtime selection follows. Call before callback dispatch, never in a signal.
    Profile(const Profile &) = delete;
    Profile &operator=(const Profile &) = delete;
    static std::unique_ptr<Profile> load() {
#if defined(__linux__) && defined(__arm__) && defined(__ARM_PCS_VFP) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) && QT_VERSION == QT_VERSION_CHECK(6,10,3)
        static_assert(sizeof(Callback) == 16 && alignof(Callback) == 4);
        if (!detail::nativeImageMapped()) return {};
        using resident::loadedArtifactDigest;
        const auto *request = reinterpret_cast<const void *>(0x6d6ce4U);
        void *revision = ::dlsym(RTLD_DEFAULT, "_ZNK13QTextDocument8revisionEv");
        void *thread = ::dlsym(RTLD_DEFAULT, "_ZN7QThread13currentThreadEv");
        void *badFunction = ::dlsym(RTLD_DEFAULT, "_ZSt25__throw_bad_function_callv");
        if (loadedArtifactDigest(request).toHex() != "071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df" ||
            loadedArtifactDigest(revision).toHex() != "93fe582cc61673342ca49e12306d7f689860016582fa46ce135ffa280a972839" ||
            loadedArtifactDigest(thread).toHex() != "43b0e210d64e59b534490d78c4c82cc1d2958999b0969aa0f77e11a082704e5d" ||
            loadedArtifactDigest(badFunction).toHex() != "826f225815cb21bbfb5290dacc4bfdc21f913fc0f0194d6b8869948c4467de01")
            return {};
        // This ET_EXEC profile uses fixed addresses. Check live critical bytes,
        // metatype spelling and type vtable RTTI in addition to backing files.
        const unsigned char requestPrefix[] = {0xf0,0x43,0x2d,0xe9,0x00,0x80,0xa0,0xe1};
        const unsigned char revisionBytes[] = {0x04,0x30,0x90,0xe5,0x68,0x00,0x93,0xe5,0x1e,0xff,0x2f,0xe1};
        if (std::memcmp(request, requestPrefix, sizeof requestPrefix) ||
            std::memcmp(revision, revisionBytes, sizeof revisionBytes) ||
            detail::word(0x1513188, 0) != 0x40001 ||
            detail::word(0x1513188, 4) != 16 ||
            detail::word(0x1513188, 20) != 0x121a9d4 ||
            std::memcmp(reinterpret_cast<const void *>(0x121a9d4), "std::function<bool(Scene*)>", sizeof("std::function<bool(Scene*)>")) ||
            detail::word(0x133e700, 0) != 0xa7e65c ||
            detail::word(0x133e6f8, 4) != 0x133e618 ||
            detail::word(0x133e750, 0) != 0xfffffff8U ||
            detail::word(0x133e750, 4) != 0x133e618)
            return {};
        // Scalar mode admits the additional public getters only from the
        // exact QtGui provider already established by the revision anchor.
        Dl_info gui{};
        bool scalarLayout = ::dladdr(revision, &gui) && gui.dli_fbase;
        for (const auto *name : {"_ZNK13QTextDocument17isUndoRedoEnabledEv",
                "_ZNK13QTextDocument15isUndoAvailableEv", "_ZNK13QTextDocument15isRedoAvailableEv",
                "_ZNK13QTextDocument17maximumBlockCountEv"}) {
            Dl_info getter{};
            void *symbol = ::dlsym(RTLD_DEFAULT, name);
            scalarLayout = scalarLayout && symbol && ::dladdr(symbol, &getter) &&
                getter.dli_fbase == gui.dli_fbase;
        }
        return std::unique_ptr<Profile>(new Profile(scalarLayout));
#else
        return {};
#endif
    }
};
struct Snapshot {
    bool available = false, callbackEntered = false, attached = false, retired = false;
    Refusal refusal = Refusal::None;
    Facts facts;
    composer_diagnostic::Snapshot observations;
    composer_text::Result text;
    ScalarResult scalar;
};
class Attachment final {
    struct State {
        const Clock::time_point deadline;
        const Mode mode;
        const std::optional<composer_text::ReadRange> range;
        const std::shared_ptr<std::atomic<bool>> externalCancellation;
        composer_text::Result text;
        ScalarResult scalar;
        std::atomic<bool> canceled{false}, claimed{false};
        std::atomic<Refusal> refusal{Refusal::None};
        std::mutex mutex;
        bool entered = false, retirementSettled = false;
#ifdef COMPOSER_SCENE_TESTING
        void *hookContext = nullptr;
        void (*stopContended)(void *) = nullptr;
        void (*beforePublish)(void *) = nullptr;
#ifdef COMPOSER_TEXT_TESTING
        void (*afterTextCopy)(void *) = nullptr;
#endif
#endif
        Facts facts;
        std::unique_ptr<composer_diagnostic::Observer> observer;
        Join (*join)(::Scene *) = detail::join;
        detail::CacheBytes (*readCaches)(const Join &) = detail::readScalarCaches;
        explicit State(Clock::time_point limit,
                std::optional<composer_text::ReadRange> read = {},
                std::shared_ptr<std::atomic<bool>> external = {}, Mode requested = Mode::Observe)
            : deadline(limit), mode(requested), range(read), externalCancellation(std::move(external)) {
            if (mode == Mode::Scalars) scalar.status = ScalarStatus::Pending;
            if (range) { text.status = composer_text::Status::Pending; text.range = *range; }
        }
        void reject(Refusal reason) noexcept {
            Refusal none = Refusal::None;
            refusal.compare_exchange_strong(none, reason, std::memory_order_acq_rel);
            canceled.store(true, std::memory_order_release);
        }
        void expire() noexcept {
            if (externalCancellation && externalCancellation->load(std::memory_order_acquire))
                reject(Refusal::Canceled);
            if (Clock::now() >= deadline) reject(Refusal::Deadline);
        }
        void retireLocked() {
            if (scalar.status == ScalarStatus::Pending) scalar.status = ScalarStatus::Refused;
            if (text.status == composer_text::Status::Pending) {
                const auto reason = refusal.load(std::memory_order_acquire);
                text.refuse(reason == Refusal::Deadline ? composer_text::Failure::Deadline :
                    reason == Refusal::Canceled ? composer_text::Failure::Canceled :
                    composer_text::Failure::Attachment);
            }
            if (observer) observer->stop();
            retirementSettled = true;
        }
    };
    static void sampleScalars(State &s, ::Scene *scene, const Join &first) {
        const auto sample = detail::readScalars(first, s.readCaches);
        if (!sample) { s.reject(Refusal::Scalar); return; }
#ifdef COMPOSER_SCENE_TESTING
        if (s.beforePublish) s.beforePublish(s.hookContext);
#endif
        s.expire();
        if (s.canceled.load(std::memory_order_acquire)) return;
        const auto again = s.join(scene);
        if (!detail::sameScalarBorrow(first, again)) {
            s.reject(again.refusal == Refusal::None ? Refusal::Scalar : again.refusal);
            return;
        }
        s.expire();
        if (s.canceled.load(std::memory_order_acquire)) return;
        const auto confirmed = detail::readScalars(again, s.readCaches);
        if (!confirmed || !(*sample == *confirmed)) { s.reject(Refusal::Scalar); return; }
        s.expire();
        if (!s.canceled.load(std::memory_order_acquire)) s.scalar = {ScalarStatus::Observed, sample};
    }

    std::shared_ptr<State> state_;
    explicit Attachment(std::shared_ptr<State> state) : state_(std::move(state)) {}
#ifdef COMPOSER_SCENE_TESTING
    friend struct testing::Access;
#endif
public:
    Attachment(const Attachment &) = delete;
    Attachment &operator=(const Attachment &) = delete;
    // Serialize handle methods/destruction; delivered callback copies may race.
    ~Attachment() { stop(); }
    static std::unique_ptr<Attachment> prepare(const Profile &profile, std::chrono::milliseconds budget,
            std::optional<composer_text::ReadRange> range = {},
            std::shared_ptr<std::atomic<bool>> externalCancellation = {}, Mode mode = Mode::Observe) {
        if (budget.count() < 1 || budget > std::chrono::milliseconds(120000) ||
            (mode != Mode::Observe && mode != Mode::Scalars) ||
            (mode == Mode::Scalars && (range || !profile.scalarLayout_)) ||
            (range && !composer_text::detail::validRequest(*range))) return {};
        return std::unique_ptr<Attachment>(new Attachment(std::make_shared<State>(
            Clock::now() + budget, range, std::move(externalCancellation), mode)));
    }
    Clock::time_point deadline() const noexcept { return state_->deadline; }
    Callback callback() const {
        const auto s = state_;
        return [s](::Scene *scene) noexcept -> bool {
            s->expire();
            if (s->canceled.load(std::memory_order_acquire)) return false;
            if (s->claimed.exchange(true, std::memory_order_acq_rel)) {
                s->reject(Refusal::Duplicate);
                std::unique_lock<std::mutex> lock(s->mutex, std::try_to_lock);
                if (lock.owns_lock()) s->retireLocked();
                return false;
            }
            std::unique_lock<std::mutex> lock(s->mutex, std::try_to_lock);
            if (!lock.owns_lock()) { s->reject(Refusal::Busy); return false; }
            s->entered = true;
            if (s->canceled.load(std::memory_order_acquire)) return false;
            try {
                auto joined = s->join(scene);
                if (s->mode == Mode::Observe) s->facts = joined.facts;
                if (joined.refusal != Refusal::None || !joined.document) {
                    s->reject(joined.refusal == Refusal::None ? Refusal::Observer : joined.refusal);
                    return false;
                }
                s->expire();
                if (s->canceled.load(std::memory_order_acquire)) return false;
                if (s->mode == Mode::Scalars) {
                    sampleScalars(*s, scene, joined);
                    if (s->canceled.load(std::memory_order_acquire)) s->retireLocked();
                    return false;
                }
                auto observer = composer_diagnostic::Observer::attachUntil(*joined.document, s->deadline);
                if (!observer) { s->reject(Refusal::Observer); return false; }
                s->observer = std::move(observer);
                composer_text::Result candidate;
                if (s->range) {
                    candidate = composer_text::detail::copy(*joined.document,*s->range,
                        {s->canceled,s->externalCancellation.get(),s->deadline}
#ifdef COMPOSER_TEXT_TESTING
                        ,s->hookContext,s->afterTextCopy
#endif
                    );
                    if (candidate.status != composer_text::Status::Copied) {
                        s->text = candidate; s->reject(Refusal::Text); s->retireLocked(); return false;
                    }
                }
#ifdef COMPOSER_SCENE_TESTING
                if (s->beforePublish) s->beforePublish(s->hookContext);
#endif
                s->expire();
                if (s->canceled.load(std::memory_order_acquire)) { s->retireLocked(); return false; }
                if (s->range) {
                    const auto again = s->join(scene);
                    detail::revalidateText(joined,again,candidate);
                    s->expire();
                    if (s->canceled.load(std::memory_order_acquire)) { s->retireLocked(); return false; }
                    s->text = candidate;
                    if (candidate.status != composer_text::Status::Copied) {
                        s->reject(Refusal::Text); s->retireLocked();
                    }
                }
            } catch (...) {
                s->reject(Refusal::Exception); s->retireLocked();
            }
            return false; // Never requests native damage flushing.
        };
    }
    void stop() noexcept {
        state_->reject(Refusal::Canceled);
        // Caller-side stop waits for an admitted attachment to settle; callbacks
        // never wait for this mutex. Stop must not be reentered from native join.
        std::unique_lock<std::mutex> lock(state_->mutex, std::defer_lock);
        if (!lock.try_lock()) {
#ifdef COMPOSER_SCENE_TESTING
            if (state_->stopContended) state_->stopContended(state_->hookContext);
#endif
            lock.lock();
        }
        state_->retireLocked();
    }
    Snapshot snapshot() {
        const auto s = state_;
        s->expire();
        Snapshot result;
        if (s->mode == Mode::Scalars) result.scalar.status = ScalarStatus::Pending;
        result.refusal = s->refusal.load(std::memory_order_acquire);
        std::unique_lock<std::mutex> lock(s->mutex, std::try_to_lock);
        if (!lock.owns_lock()) return result;
        if (s->canceled.load(std::memory_order_acquire)) s->retireLocked();
        if (s->observer) s->observer->poll();
        result.callbackEntered = s->entered;
        result.facts = s->facts;
        result.text = s->text;
        result.scalar = s->scalar;
        if (s->observer) result.observations = s->observer->snapshot();
        result.retired = s->retirementSettled;
        result.refusal = s->refusal.load(std::memory_order_acquire);
        result.attached = s->observer && !s->canceled.load(std::memory_order_acquire) &&
                         !result.retired &&
                         result.observations.available &&
                         result.observations.end == composer_diagnostic::End::Open;
        result.available = true;
        return result;
    }
};
} // namespace remarkable::composer_scene
