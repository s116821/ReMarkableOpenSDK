#pragma once
// Exact-build diagnostic attachment. No native dispatch or draft authority.
#include "composer_document_observer.h"
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
    DocumentThread, QtTransaction, Observer, Exception
};
struct Facts {
    std::uint32_t idHigh = 0, idLow = 0;
    std::uint32_t operationScope = 0, imeScope = 0, imeNesting = 0;
    std::int32_t composeStart = 0, composeLength = 0, virtualExtension = 0;
    std::int32_t qtEditDepth = 0;
    std::uintptr_t documentAddress = 0; // Correspondence only; never dereferenced later.
};
struct Join { QTextDocument *document = nullptr; Facts facts; Refusal refusal = Refusal::None; };
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
    if (!participant || !item || !word(scene, 0x194)) {
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
#else
    (void)borrowed;
    result.refusal = Refusal::Unsupported;
#endif
    return result;
}
} // namespace detail

class Attachment;
#ifdef COMPOSER_SCENE_TESTING
namespace testing { struct Access; }
#endif
class Profile final {
    Profile() = default;
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
        return std::unique_ptr<Profile>(new Profile);
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
};
class Attachment final {
    struct State {
        const Clock::time_point deadline;
        std::atomic<bool> canceled{false}, claimed{false};
        std::atomic<Refusal> refusal{Refusal::None};
        std::mutex mutex;
        bool entered = false, retirementSettled = false;
#ifdef COMPOSER_SCENE_TESTING
        void *hookContext = nullptr;
        void (*stopContended)(void *) = nullptr;
        void (*beforePublish)(void *) = nullptr;
#endif
        Facts facts;
        std::unique_ptr<composer_diagnostic::Observer> observer;
        Join (*join)(::Scene *) = detail::join;
        explicit State(Clock::time_point limit) : deadline(limit) {}
        void reject(Refusal reason) noexcept {
            Refusal none = Refusal::None;
            refusal.compare_exchange_strong(none, reason, std::memory_order_acq_rel);
            canceled.store(true, std::memory_order_release);
        }
        void expire() noexcept { if (Clock::now() >= deadline) reject(Refusal::Deadline); }
        void retireLocked() {
            if (observer) observer->stop();
            retirementSettled = true;
        }
    };
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
    static std::unique_ptr<Attachment> prepare(const Profile &, std::chrono::milliseconds budget) {
        if (budget.count() < 1 || budget > std::chrono::milliseconds(120000)) return {};
        return std::unique_ptr<Attachment>(new Attachment(std::make_shared<State>(Clock::now() + budget)));
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
                s->facts = joined.facts; // Scalars only; never store the borrowed pointer.
                if (joined.refusal != Refusal::None || !joined.document) {
                    s->reject(joined.refusal == Refusal::None ? Refusal::Observer : joined.refusal);
                    return false;
                }
                s->expire();
                if (s->canceled.load(std::memory_order_acquire)) return false;
                auto observer = composer_diagnostic::Observer::attachUntil(*joined.document, s->deadline);
                if (!observer) { s->reject(Refusal::Observer); return false; }
                s->observer = std::move(observer);
#ifdef COMPOSER_SCENE_TESTING
                if (s->beforePublish) s->beforePublish(s->hookContext);
#endif
                s->expire();
                if (s->canceled.load(std::memory_order_acquire)) s->retireLocked();
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
        result.refusal = s->refusal.load(std::memory_order_acquire);
        std::unique_lock<std::mutex> lock(s->mutex, std::try_to_lock);
        if (!lock.owns_lock()) return result;
        if (s->canceled.load(std::memory_order_acquire)) s->retireLocked();
        if (s->observer) s->observer->poll();
        result.callbackEntered = s->entered;
        result.facts = s->facts;
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
