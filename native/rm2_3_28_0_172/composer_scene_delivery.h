#pragma once
// Exact typed native enqueue. Opt-in diagnostic; no bootstrap selection or unload proof.
#include "composer_scene_attachment.h"
#include "resident_worker_slot.h"
#include "composer_snapshot_retirement.h"
#include <thread>

namespace remarkable::composer_scene {
namespace worker_detail = reader_worker_native::detail;
using SceneRequest = qulonglong (*)(void *, const QString &, const Callback &);
struct DeliverySnapshot {
    bool available = false, canceled = false, ownersPinned = false;
    unsigned entered = 0, returned = 0;
    reader_worker_native::Submission submission;
    Snapshot attachment;
    Refusal callbackRefusal = Refusal::None;
    SnapshotRetirementStatus retirement;
};
#ifdef COMPOSER_DELIVERY_TESTING
namespace testing { struct DeliveryAccess; }
#endif
class Delivery final {
    using Slot = reader_worker_native::ResidentSlot;
    using Submission = reader_worker_native::Submission;
    using Permit = reader_worker_native::Permit;
    struct Counters {
        const std::thread::id gui=std::this_thread::get_id();
        std::uintptr_t workerThread=0; // Comparison only; never dereferenced by a callback.
        std::atomic<unsigned> entered{0}, returned{0}, offGui{0}, onWorker{0};
        std::atomic<bool> wrongThread{false};
        std::atomic<bool> borrowClosed{false}; // Published only after the private mutex seal.
    };
    struct State {
        Slot *const owners; // Process-resident, never captured by the native callback.
        const Slot::Token token;
        const QString page;
        const SceneRequest request;
        std::shared_ptr<Attachment> attachment;
        const std::shared_ptr<std::atomic<bool>> canceled;
        const std::shared_ptr<Counters> counters = std::make_shared<Counters>();
        std::shared_ptr<SnapshotRetirement> retirement;
        bool consumed = false, pinned = false, ownsSlot = false;
        Submission result;
        State(Slot &slot, Slot::Token owner, QString value, SceneRequest fn,
              std::unique_ptr<Attachment> observer, std::shared_ptr<std::atomic<bool>> cancellation)
            : owners(&slot), token(std::move(owner)), page(std::move(value)), request(fn),
              attachment(std::move(observer)), canceled(std::move(cancellation)) {}
    };
    std::shared_ptr<State> state_;
    explicit Delivery(std::shared_ptr<State> s) : state_(std::move(s)) {}
#ifdef COMPOSER_DELIVERY_TESTING
    friend struct testing::DeliveryAccess;
#endif
    static bool pageValid(const QString &page) {
        return !page.isEmpty() && page.size() <= 256 && !page.contains(QChar(0));
    }
    static std::unique_ptr<Delivery> make(const Profile &profile, Slot &slot,
            const Slot::Token &token, const QString &page, std::chrono::milliseconds budget,
            SceneRequest request, std::optional<composer_text::ReadRange> range = {}, Mode mode = Mode::Observe,
            std::optional<Clock::time_point> deadline = {},
            std::shared_ptr<std::atomic<bool>> cancellation = {},
            std::shared_ptr<SnapshotRetirement> retirement = {}) {
        using namespace reader_worker_native;
        if (!worker_detail::gui() || worker_detail::busy || !request || !pageValid(page)) return {};
        const auto owner = slot.match(token);
        if (!owner || !owner->ready || owner->consumed || owner->sceneCancellation || !owner->live())
            return {};
        if (!cancellation) cancellation = std::make_shared<std::atomic<bool>>(false);
        if (cancellation->load(std::memory_order_acquire)) return {};
        auto attachment = deadline ? Attachment::prepareUntil(profile,*deadline,range,cancellation,mode)
                                   : Attachment::prepare(profile,budget,range,cancellation,mode);
        if (!attachment) return {};
        if(retirement && (mode!=Mode::Snapshot || retirement->deadline_!=attachment->deadline()))return {};
        auto state=std::make_shared<State>(slot,token,page,request,std::move(attachment),std::move(cancellation));
        state->counters->workerThread=reinterpret_cast<std::uintptr_t>(owner->publicWorker.data());
        state->retirement=std::move(retirement);
        return std::unique_ptr<Delivery>(new Delivery(std::move(state)));
    }
    static void close(const std::shared_ptr<State> &s) noexcept {
        s->canceled->store(true,std::memory_order_release);
        if (s->attachment->snapshotOnly()) s->attachment->revoke();
        else s->attachment->stop();
        if (s->ownsSlot && worker_detail::gui()) s->owners->cancel(s->token);
    }
public:
    Delivery(const Delivery &) = delete;
    Delivery &operator=(const Delivery &) = delete;
    // Handle methods/destruction are GUI-only and serialized. Native copies race
    // only against shared cancellation/counters and the accepted attachment state.
    ~Delivery() {
        const reader_request_scope::Scope scope(state_->retirement?state_->retirement->binding_:reader_request_scope::Binding{});
        close(state_);
    }
    static std::unique_ptr<Delivery> prepare(const Profile &profile, Slot &slot,
            const Slot::Token &token, const QString &page, std::chrono::milliseconds budget,
            std::optional<composer_text::ReadRange> range = {}, Mode mode = Mode::Observe) {
        if (budget.count() < 1 || budget > std::chrono::milliseconds(120000)) return {};
        return prepareUntil(profile,slot,token,page,Clock::now()+budget,range,mode);
    }
    static std::unique_ptr<Delivery> prepareUntil(const Profile &profile, Slot &slot,
            const Slot::Token &token, const QString &page, Clock::time_point deadline,
            std::optional<composer_text::ReadRange> range = {}, Mode mode = Mode::Observe,
            std::shared_ptr<std::atomic<bool>> cancellation = {},
            std::shared_ptr<SnapshotRetirement> retirement = {}) {
#if defined(__linux__) && defined(__arm__) && defined(__ARM_PCS_VFP) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) && QT_VERSION == QT_VERSION_CHECK(6,10,3)
        static_assert(sizeof(Callback) == 16 && alignof(Callback) == 4 && sizeof(qulonglong) == 8);
        // Distinct reviewed scene request. Never reinterpret the generic request.
        return make(profile,slot,token,page,std::chrono::milliseconds(0),reinterpret_cast<SceneRequest>(0x6d6ce4U),range,mode,deadline,std::move(cancellation),std::move(retirement));
#else
        (void)profile; (void)slot; (void)token; (void)page; (void)deadline; (void)range; (void)mode; (void)cancellation; (void)retirement;
        return {};
#endif
    }
    Submission submit(const Permit &permit) {
        const reader_request_scope::Scope scope(state_->retirement?state_->retirement->binding_:reader_request_scope::Binding{});
        using namespace reader_worker_native;
        const auto s = state_; // Survives destructive/reentrant permit callbacks.
        worker_detail::Gate gate;
        Submission result;
        if (!scope.active() || !gate.entered || !worker_detail::gui()) {
            result.refusal=QStringLiteral("scene-reentered-or-thread"); return result;
        }
        if (s->consumed) {
            result.refusal=QStringLiteral("scene-already-consumed"); return result;
        }
        s->consumed=true;
        const auto owner=s->owners->match(s->token);
        auto refused=[&](const char *reason) {
            result.refusal=QString::fromLatin1(reason); close(s); s->result=result;
            if(s->retirement)s->retirement->returned(result);
            return result;
        };
        if (!owner || !owner->ready || owner->consumed || owner->sceneCancellation ||
            !owner->live() || s->canceled->load(std::memory_order_acquire) ||
            Clock::now() >= s->attachment->deadline())
            return refused("scene-submission-refused");
        owner->consumed=true; owner->submitting=true; s->ownsSlot=true;
        struct Finished {
            decltype(owner) value;
            ~Finished() { value->submitting=false; }
        } finished{owner};
        const QString page=s->page;
        Callback retained;
        try {
            auto observe=s->attachment->callback();
            auto canceled=s->canceled;
            auto counts=s->counters;
            const bool requireExactWorker=bool(s->retirement);
            retained=[observe=std::move(observe),canceled,counts,requireExactWorker](::Scene *scene) noexcept {
                counts->entered.fetch_add(1,std::memory_order_acq_rel);
                const bool offGui=std::this_thread::get_id()!=counts->gui;
                const bool onWorker=reinterpret_cast<std::uintptr_t>(QThread::currentThread())==counts->workerThread;
                if(offGui)counts->offGui.fetch_add(1,std::memory_order_relaxed);
                if(onWorker)counts->onWorker.fetch_add(1,std::memory_order_relaxed);
                struct Returned {
                    std::shared_ptr<Counters> counts;
                    ~Returned() { counts->returned.fetch_add(1,std::memory_order_release); }
                } returning{counts};
                // After the linearized seal a retained copy is inert on every
                // thread. This cached fact never authorizes owner release.
                if(requireExactWorker && counts->borrowClosed.load(std::memory_order_acquire))return false;
                // Admission precedes every Scene dereference, including the
                // raw native join fields before QTextDocument thread checking.
                // The original diagnostic modes keep their existing contract.
                if(requireExactWorker && (!offGui || !onWorker)) {
                    counts->wrongThread.store(true,std::memory_order_release);
                    canceled->store(true,std::memory_order_release);
                    return false;
                }
                if (!canceled->load(std::memory_order_acquire)) observe(scene);
                return false;
            };
        } catch (...) { return refused("scene-callback-copy-refused"); }
        if(s->retirement) {
            const auto worker=owner->publicWorker;
            if(!s->retirement->watch(worker,[s] {
                if(s->counters->returned.load(std::memory_order_acquire)!=1 ||
                        s->counters->entered.load(std::memory_order_acquire)!=1 ||
                        s->counters->offGui.load(std::memory_order_acquire)!=1 ||
                        s->counters->onWorker.load(std::memory_order_acquire)!=1)return false;
                if(!s->attachment->closeSnapshotBorrow())return false;
                s->counters->borrowClosed.store(true,std::memory_order_release);
                return true;
            },[s,worker] {
                const auto current=s->owners->match(s->token);
                if(!worker || !current || current->publicWorker!=worker)return false;
                if(s->counters->returned.load(std::memory_order_acquire)!=1 ||
                        s->counters->entered.load(std::memory_order_acquire)!=1 ||
                        s->counters->onWorker.load(std::memory_order_acquire)!=1)return false;
                const auto released=s->owners->retireSnapshot(s->token,s->canceled);
                if(released.reclaimed)s->pinned=false;
                return released.reclaimed;
            },s->retirement))return refused("scene-completion-watch-refused");
        }
        if (!worker_detail::allowed(permit) || !owner->live() ||
            s->canceled->load(std::memory_order_acquire) ||
            Clock::now() >= s->attachment->deadline())
            return refused("scene-permit-refused");

        // Installed before native entry, even if it throws after retaining a copy.
        // This excludes the generic physical-retirement path.
        owner->sceneCancellation=s->canceled; s->pinned=true; result.attempted=true;
        try { result.jobId=s->request(reinterpret_cast<void *>(owner->worker.words[0]),page,retained); }
        catch (...) {
            owner->unknown=true; result.unknown=true;
            return refused("scene-request-threw");
        }
        // No QObject getters, stale selection guard or native-owner reads after
        // enqueue. The job ID proves only that this native call returned nonzero.
        if (!result.jobId || s->canceled->load(std::memory_order_acquire) ||
            owner->canceled || owner->unknown || worker_detail::reentered) {
            owner->unknown=true; result.unknown=true;
            return refused("scene-request-outcome-unknown");
        }
        result.accepted=true; s->result=result;
        if(s->retirement)s->retirement->returned(result);
        return result;
    }
    void stop() noexcept {
        const reader_request_scope::Scope scope(state_->retirement?state_->retirement->binding_:reader_request_scope::Binding{});
        close(state_);
    }
    DeliverySnapshot snapshot() {
        const reader_request_scope::Scope scope(state_->retirement?state_->retirement->binding_:reader_request_scope::Binding{});
        const auto s=state_;
        DeliverySnapshot result;
        if (!worker_detail::gui()) return result;
        if (s->canceled->load(std::memory_order_acquire)) {
            if (s->attachment->snapshotOnly()) s->attachment->revoke();
            else s->attachment->stop();
        }
        result.attachment=s->attachment->snapshot();
        result.canceled=s->canceled->load(std::memory_order_acquire);
        result.entered=s->counters->entered.load(std::memory_order_acquire);
        result.returned=s->counters->returned.load(std::memory_order_acquire);
        if(s->counters->wrongThread.load(std::memory_order_acquire))result.callbackRefusal=Refusal::DocumentThread;
        result.ownersPinned=s->pinned;
        if(s->retirement)result.retirement=s->retirement->status_;
        result.submission=s->result; result.available=true;
        return result;
    }
};
} // namespace remarkable::composer_scene
