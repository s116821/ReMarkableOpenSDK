#pragma once
// Exact typed native enqueue. Opt-in diagnostic; no bootstrap selection or unload proof.
#include "composer_scene_attachment.h"
#include "resident_worker_slot.h"

namespace remarkable::composer_scene {
namespace worker_detail = reader_worker_native::detail;
using SceneRequest = qulonglong (*)(void *, const QString &, const Callback &);
struct DeliverySnapshot {
    bool available = false, canceled = false, ownersPinned = false;
    unsigned entered = 0, returned = 0;
    reader_worker_native::Submission submission;
    Snapshot attachment;
};
#ifdef COMPOSER_DELIVERY_TESTING
namespace testing { struct DeliveryAccess; }
#endif
class Delivery final {
    using Slot = reader_worker_native::ResidentSlot;
    using Submission = reader_worker_native::Submission;
    using Permit = reader_worker_native::Permit;
    struct Counters { std::atomic<unsigned> entered{0}, returned{0}; };
    struct State {
        Slot *const owners; // Process-resident, never captured by the native callback.
        const Slot::Token token;
        const QString page;
        const SceneRequest request;
        std::shared_ptr<Attachment> attachment;
        const std::shared_ptr<std::atomic<bool>> canceled;
        const std::shared_ptr<Counters> counters = std::make_shared<Counters>();
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
            SceneRequest request, std::optional<composer_text::ReadRange> range = {}, Mode mode = Mode::Observe) {
        using namespace reader_worker_native;
        if (!worker_detail::gui() || worker_detail::busy || !request || !pageValid(page)) return {};
        const auto owner = slot.match(token);
        if (!owner || !owner->ready || owner->consumed || owner->sceneCancellation || !owner->live())
            return {};
        auto cancellation = std::make_shared<std::atomic<bool>>(false);
        auto attachment = Attachment::prepare(profile,budget,range,cancellation,mode);
        if (!attachment) return {};
        return std::unique_ptr<Delivery>(new Delivery(std::make_shared<State>(
            slot,token,page,request,std::move(attachment),std::move(cancellation))));
    }
    static void close(const std::shared_ptr<State> &s) noexcept {
        s->canceled->store(true,std::memory_order_release);
        s->attachment->stop();
        if (s->ownsSlot && worker_detail::gui()) s->owners->cancel(s->token);
    }
public:
    Delivery(const Delivery &) = delete;
    Delivery &operator=(const Delivery &) = delete;
    // Handle methods/destruction are GUI-only and serialized. Native copies race
    // only against shared cancellation/counters and the accepted attachment state.
    ~Delivery() { close(state_); }
    static std::unique_ptr<Delivery> prepare(const Profile &profile, Slot &slot,
            const Slot::Token &token, const QString &page, std::chrono::milliseconds budget,
            std::optional<composer_text::ReadRange> range = {}, Mode mode = Mode::Observe) {
#if defined(__linux__) && defined(__arm__) && defined(__ARM_PCS_VFP) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) && QT_VERSION == QT_VERSION_CHECK(6,10,3)
        static_assert(sizeof(Callback) == 16 && alignof(Callback) == 4 && sizeof(qulonglong) == 8);
        // Distinct reviewed scene request. Never reinterpret the generic request.
        return make(profile,slot,token,page,budget,reinterpret_cast<SceneRequest>(0x6d6ce4U),range,mode);
#else
        (void)profile; (void)slot; (void)token; (void)page; (void)budget; (void)range; (void)mode;
        return {};
#endif
    }
    Submission submit(const Permit &permit) {
        using namespace reader_worker_native;
        const auto s = state_; // Survives destructive/reentrant permit callbacks.
        worker_detail::Gate gate;
        Submission result;
        if (!gate.entered || !worker_detail::gui()) {
            result.refusal=QStringLiteral("scene-reentered-or-thread"); return result;
        }
        if (s->consumed) {
            result.refusal=QStringLiteral("scene-already-consumed"); return result;
        }
        s->consumed=true;
        const auto owner=s->owners->match(s->token);
        auto refused=[&](const char *reason) {
            result.refusal=QString::fromLatin1(reason); close(s); s->result=result; return result;
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
            retained=[observe=std::move(observe),canceled,counts](::Scene *scene) noexcept {
                counts->entered.fetch_add(1,std::memory_order_acq_rel);
                struct Returned {
                    std::shared_ptr<Counters> counts;
                    ~Returned() { counts->returned.fetch_add(1,std::memory_order_release); }
                } returning{counts};
                if (!canceled->load(std::memory_order_acquire)) observe(scene);
                return false;
            };
        } catch (...) { return refused("scene-callback-copy-refused"); }
        if (!worker_detail::allowed(permit) || !owner->live() ||
            s->canceled->load(std::memory_order_acquire) ||
            Clock::now() >= s->attachment->deadline())
            return refused("scene-permit-refused");

        // Installed before native entry, even if it throws after retaining a copy.
        // This permanently excludes the generic physical-retirement path.
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
        result.accepted=true; s->result=result; return result;
    }
    void stop() noexcept { close(state_); }
    DeliverySnapshot snapshot() {
        const auto s=state_;
        DeliverySnapshot result;
        if (!worker_detail::gui()) return result;
        if (s->canceled->load(std::memory_order_acquire)) s->attachment->stop();
        result.attachment=s->attachment->snapshot();
        result.canceled=s->canceled->load(std::memory_order_acquire);
        result.entered=s->counters->entered.load(std::memory_order_acquire);
        result.returned=s->counters->returned.load(std::memory_order_acquire);
        result.ownersPinned=s->pinned;
        result.submission=s->result; result.available=true;
        return result;
    }
};
} // namespace remarkable::composer_scene
