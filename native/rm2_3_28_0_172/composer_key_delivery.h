#pragma once
// One actual Qt key delivery -> the existing typed Scene request. Unenabled.
// This preserves event provenance, not semantic Submit or draft/edit authority.
#include "composer_scene_delivery.h"
#include "reader_selected_view_probe.h"
#include <QKeyEvent>
#if QT_VERSION < QT_VERSION_CHECK(6,10,0) && defined(COMPOSER_KEY_DELIVERY_TESTING)
// Older owned host fixture only. Production uses Qt 6.10's public dynamic
// signal-to-functor overload and never this private compatibility path.
#include <QtCore/private/qobject_p.h>
#endif

namespace remarkable::composer_scene {
struct KeyFacts {
    static constexpr qsizetype MaxTextUnits = 64;
    int key = 0, count = 0;
    Qt::KeyboardModifiers modifiers;
    quint32 nativeScanCode = 0, nativeVirtualKey = 0, nativeModifiers = 0;
    bool autoRepeat = false, spontaneous = false;
    QString text;
    // Qt's spontaneous/native-code fields do not identify folio versus OSK.
};
struct KeySnapshot {
    bool entered = false, canceled = false;
    QString refusal;
    std::optional<KeyFacts> key;
    DeliverySnapshot scene;
};
#ifdef COMPOSER_KEY_DELIVERY_TESTING
namespace testing { struct KeyAccess; }
#endif
class KeyDelivery final : public QObject {
    Q_OBJECT
    using Slot = reader_worker_native::ResidentSlot;
    using Acquire = std::function<std::unique_ptr<Delivery>(QQuickItem *, Clock::time_point,
        const std::shared_ptr<std::atomic<bool>> &,
        const reader_worker_native::Permit &)>;
    struct State {
        const Clock::time_point deadline;
        bool entered = false;
        const std::shared_ptr<std::atomic<bool>> revoked = std::make_shared<std::atomic<bool>>(false);
        QString refusal;
        std::optional<KeyFacts> key;
        std::unique_ptr<Delivery> delivery;
        Acquire acquire;
        const reader_request_scope::Binding binding;
#ifdef COMPOSER_KEY_DELIVERY_TESTING
        std::function<void()> beforeSceneSnapshot;
#endif
        explicit State(Clock::time_point limit, Acquire fn,reader_request_scope::Binding frames={})
            : deadline(limit), acquire(std::move(fn)),binding(std::move(frames)) {}
        void cancel() { revoked->store(true,std::memory_order_release); }
        bool canceled() const { return revoked->load(std::memory_order_acquire); }
    };
    const std::shared_ptr<State> state_;
    QPointer<QQuickItem> receiver_;
    QPointer<QQuickWindow> window_;
    QList<QMetaObject::Connection> revocations_;
    KeyDelivery(QQuickItem *receiver, Clock::time_point deadline, Acquire acquire,reader_request_scope::Binding binding)
        : state_(std::make_shared<State>(deadline, std::move(acquire),std::move(binding))),
          receiver_(receiver), window_(receiver->window()) {}
    bool watch(QObject *object, const char *signal) {
        if (!object) return false;
        const int from = object->metaObject()->indexOfSignal(signal);
        if (from < 0) return false;
        const auto method = object->metaObject()->method(from);
        // A Qt-owned functor retains only the atomic latch. An off-thread
        // emission or an in-flight activation after disconnect cannot touch
        // KeyDelivery, its GUI state, or the native observer's mutex.
        auto revoke = [gate=state_->revoked] { gate->store(true,std::memory_order_release); };
#if QT_VERSION >= QT_VERSION_CHECK(6,10,0)
        const auto connection=QMetaObject::connect(object,method,object,std::move(revoke),Qt::DirectConnection);
#elif defined(COMPOSER_KEY_DELIVERY_TESTING)
        const auto connection=QObjectPrivate::connect(object,method.methodIndex(),object,
            new QtPrivate::QFunctorSlotObject<decltype(revoke),0,QtPrivate::List<>,void>(std::move(revoke)),Qt::DirectConnection);
#else
        (void)method; (void)revoke;
        return false;
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6,10,0) || defined(COMPOSER_KEY_DELIVERY_TESTING)
        if(!connection)return false;
        try { revocations_.append(connection); }
        catch(...) { QObject::disconnect(connection); throw; }
        return true;
#endif
    }
    bool install() {
        auto *app = qobject_cast<QGuiApplication *>(QCoreApplication::instance());
        if (!app || !receiver_ || !window_ || receiver_->thread() != app->thread() ||
                QThread::currentThread() != app->thread() || QCoreApplication::closingDown()) return false;
        // Retained until cancellation/destruction: ABA notifications also cancel.
        for (const auto *signal : {"destroyed(QObject*)", "documentChanged()", "controllerChanged()",
                "pageIdChanged()", "activeFocusChanged(bool)", "parentChanged(QQuickItem*)",
                "windowChanged(QQuickWindow*)", "visibleChanged()", "enabledChanged()"})
            if (!watch(receiver_, signal)) return false;
        for (const auto *signal : {"destroyed(QObject*)", "activeFocusItemChanged()", "activeChanged()", "visibleChanged(bool)"})
            if (!watch(window_, signal)) return false;
        if (!watch(app, "focusWindowChanged(QWindow*)") || !watch(app, "aboutToQuit()")) return false;
        receiver_->installEventFilter(this);
        return true;
    }
    static std::unique_ptr<KeyDelivery> make(QQuickItem *receiver, std::chrono::milliseconds budget, Acquire acquire) {
        if(budget.count()<1 || budget.count()>reader_selected::MaxObservationMs)return {};
        return makeUntil(receiver,Clock::now()+budget,std::move(acquire),{});
    }
    static std::unique_ptr<KeyDelivery> makeUntil(QQuickItem *receiver,Clock::time_point deadline,Acquire acquire,
                                                reader_request_scope::Binding binding) {
        if (!reader_worker_native::detail::gui() || !receiver || receiver->thread() != QThread::currentThread() || !acquire ||
                Clock::now()>=deadline) return {};
        auto result = std::unique_ptr<KeyDelivery>(new KeyDelivery(receiver, deadline, std::move(acquire),std::move(binding)));
        if (!result->install()) return {};
        return result;
    }
#ifdef COMPOSER_KEY_DELIVERY_TESTING
    friend struct testing::KeyAccess;
#endif
    bool eventFilter(QObject *receiver, QEvent *event) override {
        const reader_request_scope::Scope scope(state_->binding);
        const auto state = state_; // Survives reentrant destruction during admission.
        if (receiver != receiver_ || !event) return false;
        if(!scope.active()) { state->cancel(); return false; }
        if (event->type() == QEvent::InputMethod || event->type() == QEvent::FocusOut ||
                event->type() == QEvent::Hide || event->type() == QEvent::ParentChange) state->cancel();
        if (event->type() != QEvent::KeyPress || state->canceled()) return false;
        if (state->entered) { state->cancel(); return false; }
        state->entered = true; // No second admission, including nested dispatch.
        const auto &key = *static_cast<QKeyEvent *>(event);
        KeyFacts facts;
        facts.key = key.key(); facts.count = key.count(); facts.modifiers = key.modifiers();
        facts.autoRepeat = key.isAutoRepeat(); facts.spontaneous = key.spontaneous();
        facts.nativeScanCode = key.nativeScanCode(); facts.nativeVirtualKey = key.nativeVirtualKey();
        facts.nativeModifiers = key.nativeModifiers();
        const QString text = key.text();
        if (text.size() > KeyFacts::MaxTextUnits) {
            state->refusal = QStringLiteral("key-text-limit"); state->cancel(); return false;
        }
        facts.text = text; // Remains private until a joined native attempt exists.
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(state->deadline - Clock::now());
        if (remaining.count() < 1) {
            state->refusal = QStringLiteral("key-deadline"); state->cancel(); return false;
        }
        QPointer<KeyDelivery> self(this);
        QPointer<QQuickItem> target(receiver_);
        const auto allowed = [state, self, target] {
            return self && target && !state->canceled() && Clock::now() < state->deadline;
        };
        try {
            auto delivery = state->acquire(target, state->deadline, state->revoked, allowed);
            state->delivery = std::move(delivery);
            const auto sent = state->delivery ? state->delivery->snapshot().submission : reader_worker_native::Submission{};
            if (sent.attempted) state->key = std::move(facts);
            if (!allowed() || !state->delivery) {
                state->refusal = QStringLiteral("key-source-refused"); state->cancel(); return !target;
            }
            if (!sent.accepted) { state->refusal = sent.refusal; state->cancel(); }
        } catch (...) { state->refusal = QStringLiteral("key-handoff-exception"); state->cancel(); }
        // Always pass through; no accept(), key substitution, grammar or replay.
        // If a callout destroyed the receiver, Qt requires true to avoid UAF.
        return !target;
    }
public:
    ~KeyDelivery() override {
        const reader_request_scope::Scope scope(state_->binding);
        state_->cancel();
        // Do not rely on a short-lived context's QObject destructor to detach
        // direct functors while another thread emits. Explicit disconnect keeps
        // any in-flight functor's independent latch alive and its tag intact.
        // The emitting object is the context; it outlives its own activation.
        for(const auto &connection:revocations_)QObject::disconnect(connection);
        revocations_.clear();
        if (receiver_) receiver_->removeEventFilter(this);
    }
    // GUI-only. Profile ownership is transferred; no borrowed provider pointer
    // or QKeyEvent survives arming. The Slot is already process-resident.
    static std::unique_ptr<KeyDelivery> prepare(QQuickItem *receiver, QObject *creationController,
            std::unique_ptr<Profile> profile, Slot &slot, std::chrono::milliseconds budget,
            std::optional<composer_text::ReadRange> range = {}) {
        if (!reader_worker_native::detail::gui() || !profile || !creationController ||
                creationController->thread() != QThread::currentThread() ||
                budget.count()<1 || budget.count()>reader_selected::MaxObservationMs) return {};
        const auto deadline=Clock::now()+budget;
        // Construct the QObject/fence before the selected-view Sample exists.
        const auto retirement=SnapshotRetirement::create(deadline);
        if(!retirement)return {};
        QPointer<QObject> creation(creationController);
        auto ownedProfile = std::shared_ptr<Profile>(std::move(profile));
        return makeUntil(receiver, deadline, [creation, ownedProfile, &slot, range,retirement](QQuickItem *target, Clock::time_point deadline,
                const std::shared_ptr<std::atomic<bool>> &revoked,
                const reader_worker_native::Permit &permit) -> std::unique_ptr<Delivery> {
            if (!target || !creation || !permit()) return {};
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - Clock::now()).count();
            if (ms < 1) return {};
            std::unique_ptr<Delivery> result;
            reader_selected::observeInputReceiver(target->window(), creation, target, permit, int(ms),
                [&](QObject *controller, const QVariant &id, QObject *worker,
                        const reader_selected::Observation &before, const std::function<bool()> &joined) {
                    auto preparation = slot.prepare(controller, id, worker, joined);
                    if (!preparation.ready) return;
                    if (!joined()) { slot.cancel(preparation.token); return; }
                    result = Delivery::prepareUntil(*ownedProfile, slot, preparation.token, before.pageId,
                        deadline, range, Mode::Snapshot, revoked,retirement);
                    // Submit must happen while the synchronous join is still live.
                    if (result) {
                        // Keep attempted/unknown submission and pinned-owner
                        // evidence, including a throw-after-retain outcome.
                        result->submit(joined);
                    } else slot.cancel(preparation.token);
                });
            return result;
        },retirement->binding_);
    }
    KeySnapshot snapshot() {
        const reader_request_scope::Scope scope(state_->binding);
        KeySnapshot result;
        if (!reader_worker_native::detail::gui()) return result;
        const auto state = state_;
        if (Clock::now() >= state->deadline) state->cancel();
        result.entered = state->entered; result.canceled = state->canceled();
        result.refusal = state->refusal; result.key = state->key;
#ifdef COMPOSER_KEY_DELIVERY_TESTING
        if(state->beforeSceneSnapshot)state->beforeSceneSnapshot();
#endif
        if (state->delivery) result.scene = state->delivery->snapshot();
        // Copied text may remain as historical evidence. Revocation dominates
        // the complete receipt; neither Copied nor a past revision is a lease.
        result.canceled = result.canceled || result.scene.canceled || state->canceled();
        return result;
    }
};
} // namespace remarkable::composer_scene
