#pragma once
#include "composer_scene_attachment.h"
#include "composer_pre_enter_arm.h"

namespace remarkable::composer_pre_enter {
namespace detail {
// Internal native ABI view, never publicly admitted or retained after entry.
struct StockPair { std::uintptr_t item, control; };
using Insert = void (*)(void *, const StockPair *, const QString *);
inline composer_scene::Join stockJoin(void *raw, const StockPair *owner) {
    composer_scene::Join result;
#if defined(__linux__) && defined(__arm__) && defined(__ARM_PCS_VFP)
    if (!raw || !owner || !owner->item || !owner->control) {
        result.refusal = composer_scene::Refusal::MissingRoot; return result;
    }
    const auto p = reinterpret_cast<std::uintptr_t>(raw);
    const auto scene = composer_scene::detail::word(p,8);
    if (!scene || composer_scene::detail::word(scene,0x26c) != p ||
        composer_scene::detail::word(scene,0x190) != owner->item ||
        composer_scene::detail::word(scene,0x194) != owner->control) {
        result.refusal = composer_scene::Refusal::FocusMismatch; return result;
    }
    return composer_scene::detail::join(reinterpret_cast<::Scene *>(scene));
#else
    (void)raw; (void)owner;
    result.refusal = composer_scene::Refusal::Unsupported;
    return result;
#endif
}

struct State {
    const composer_text::ReadRange range;
    const composer_text::Clock::time_point deadline;
    std::atomic<bool> canceled{false}, claimed{false}, finished{false};
    std::atomic<bool> originalReturned{false}, originalThrew{false};
    std::atomic<composer_scene::Refusal> nativeRefusal{composer_scene::Refusal::None};
    std::mutex mutex;
    composer_text::Result result;
#ifdef COMPOSER_PRE_ENTER_TESTING
    void (*beforeRead)()=nullptr;
    void (*beforeRecheck)()=nullptr;
#endif
    State(composer_text::ReadRange r, std::chrono::milliseconds budget)
        : range(r), deadline(composer_text::Clock::now()+budget) {
        result.range=r; result.status=composer_text::Status::Pending;
    }
    composer_text::detail::Guard guard() const { return {canceled,nullptr,deadline}; }
    composer_text::Result attempt(void *participant, const StockPair *owner) noexcept {
        composer_text::Result candidate; candidate.range=range;
        candidate.status=composer_text::Status::Pending;
        try {
            if (const auto fail=guard().check(); fail!=composer_text::Failure::None) {
                candidate.refuse(fail); return candidate;
            }
#ifdef COMPOSER_PRE_ENTER_TESTING
            if (beforeRead) beforeRead();
#endif
            const auto first=stockJoin(participant,owner);
            if (first.refusal!=composer_scene::Refusal::None || !first.document) {
                nativeRefusal.store(first.refusal,std::memory_order_release);
                candidate.refuse(composer_text::Failure::Join); return candidate;
            }
            candidate=composer_text::detail::copy(*first.document,range,guard());
            if (candidate.status==composer_text::Status::Copied) {
#ifdef COMPOSER_PRE_ENTER_TESTING
                if (beforeRecheck) beforeRecheck();
#endif
                const auto again=stockJoin(participant,owner);
                nativeRefusal.store(again.refusal,std::memory_order_release);
                composer_scene::detail::revalidateText(first,again,candidate);
            }
        } catch (...) { candidate.refuse(composer_text::Failure::Exception); }
        return candidate;
    }
    void publish(composer_text::Result &candidate, bool returned) noexcept {
        if (!returned) {
            originalThrew.store(true,std::memory_order_release);
            candidate.refuse(composer_text::Failure::Exception);
        } else originalReturned.store(true,std::memory_order_release);
        if (const auto fail=guard().check(); fail!=composer_text::Failure::None)
            candidate.refuse(fail);
        // No mutex is held across the stock call. A contended publication drops
        // the copy; snapshot reports the finished-but-unpublished refusal.
        try {
            std::unique_lock<std::mutex> lock(mutex,std::try_to_lock);
            if (lock.owns_lock()) result=candidate;
        } catch (...) {} // Publication failure cannot replace a stock exception.
        finished.store(true,std::memory_order_release);
    }
};
// Prepared state is retained by never-destroyed SDK route allocations; entry
// only loads a published pointer. Owned tests bind only while idle, without a
// quiescence boolean.
extern "C" void remarkable_pre_enter_shim(void *,const StockPair *,const QString *);
// Preparation only: pins the already loaded shim DSO with RTLD_NODELETE. Never
// call from interception or the excluded patch interval. No installation proof.
bool pinPreparedRouteCode() noexcept;
namespace patch { class StartupBuilder; class ConstructorStartup; }
class Route final {
    std::shared_ptr<State> retained_;
    explicit Route(std::shared_ptr<State> s) : retained_(std::move(s)) {}
    ~Route()=default; // Only unpublished allocations can be destroyed.
    static std::atomic<Route *> current_;
    static_assert(std::atomic<Route *>::is_always_lock_free);
    friend void remarkable_pre_enter_shim(void *,const StockPair *,const QString *);
    friend class patch::ConstructorStartup;
#ifdef COMPOSER_PRE_ENTER_TESTING
    friend struct OwnedTestAccess;
#endif
    static Route *current() noexcept { return current_.load(std::memory_order_acquire); }
};
} // namespace detail

struct Snapshot {
    bool available=false, claimed=false, finished=false;
    bool originalReturned=false, originalThrew=false;
    composer_scene::Refusal nativeRefusal=composer_scene::Refusal::None;
    composer_text::Result text;
};
class Capture final {
    std::shared_ptr<detail::State> state_;
    explicit Capture(std::shared_ptr<detail::State> s) : state_(std::move(s)) {}
    friend class detail::patch::StartupBuilder;
#ifdef COMPOSER_PRE_ENTER_TESTING
    friend struct TestAccess;
#endif
public:
    Capture(const Capture &)=delete;
    Capture &operator=(const Capture &)=delete;
    ~Capture() { cancel(); }
    static std::unique_ptr<Capture> prepare(const composer_scene::Profile &,
            composer_text::ReadRange range,std::chrono::milliseconds budget) {
        if (!composer_text::detail::validRequest(range) || budget.count()<1 ||
            budget>std::chrono::milliseconds(120000)) return {};
        return std::unique_ptr<Capture>(new Capture(std::make_shared<detail::State>(range,budget)));
    }
    // Request only; no wait, unpatch, release or unload permission.
    void cancel() noexcept { state_->canceled.store(true,std::memory_order_release); }
    Snapshot snapshot() {
        Snapshot s;
        std::unique_lock<std::mutex> lock(state_->mutex,std::try_to_lock);
        if (!lock.owns_lock()) return s;
        s.available=true;
        s.claimed=state_->claimed.load(std::memory_order_acquire);
        s.finished=state_->finished.load(std::memory_order_acquire);
        s.originalReturned=state_->originalReturned.load(std::memory_order_acquire);
        s.originalThrew=state_->originalThrew.load(std::memory_order_acquire);
        s.nativeRefusal=state_->nativeRefusal.load(std::memory_order_acquire);
        s.text=state_->result;
        if (s.finished && s.text.status==composer_text::Status::Pending)
            s.text.refuse(composer_text::Failure::Join);
        return s;
    }
};
} // namespace remarkable::composer_pre_enter
