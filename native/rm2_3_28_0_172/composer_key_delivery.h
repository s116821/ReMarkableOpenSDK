#pragma once
// One actual Qt key delivery -> the existing typed Scene request. Unenabled.
// This preserves event provenance, not semantic Submit or draft/edit authority.
#include "composer_scene_delivery.h"
#include "reader_selected_view_probe.h"
#include <QKeyEvent>

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
        const reader_worker_native::Permit &)>;
    struct State {
        const Clock::time_point deadline;
        bool entered = false, canceled = false;
        QString refusal;
        std::optional<KeyFacts> key;
        std::unique_ptr<Delivery> delivery;
        Acquire acquire;
        explicit State(Clock::time_point limit, Acquire fn) : deadline(limit), acquire(std::move(fn)) {}
        void cancel() {
            canceled = true;
            if (delivery) delivery->stop();
        }
    };
    const std::shared_ptr<State> state_;
    QPointer<QQuickItem> receiver_;
    QPointer<QQuickWindow> window_;
    KeyDelivery(QQuickItem *receiver, std::chrono::milliseconds budget, Acquire acquire)
        : state_(std::make_shared<State>(Clock::now() + budget, std::move(acquire))),
          receiver_(receiver), window_(receiver->window()) {}
    bool watch(QObject *object, const char *signal) {
        if (!object) return false;
        const int from = object->metaObject()->indexOfSignal(signal);
        const int to = metaObject()->indexOfSlot("invalidate()");
        return from >= 0 && to >= 0 && bool(QObject::connect(object,
            object->metaObject()->method(from), this, metaObject()->method(to), Qt::DirectConnection));
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
        if (!reader_worker_native::detail::gui() || !receiver || receiver->thread() != QThread::currentThread() || !acquire ||
                budget.count() < 1 || budget.count() > reader_selected::MaxObservationMs) return {};
        auto result = std::unique_ptr<KeyDelivery>(new KeyDelivery(receiver, budget, std::move(acquire)));
        if (!result->install()) return {};
        return result;
    }
#ifdef COMPOSER_KEY_DELIVERY_TESTING
    friend struct testing::KeyAccess;
#endif
    bool eventFilter(QObject *receiver, QEvent *event) override {
        const auto state = state_; // Survives reentrant destruction during admission.
        if (receiver != receiver_ || !event) return false;
        if (event->type() == QEvent::InputMethod || event->type() == QEvent::FocusOut ||
                event->type() == QEvent::Hide || event->type() == QEvent::ParentChange) state->cancel();
        if (event->type() != QEvent::KeyPress || state->canceled) return false;
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
        facts.text = text; state->key = std::move(facts);
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(state->deadline - Clock::now());
        if (remaining.count() < 1) {
            state->refusal = QStringLiteral("key-deadline"); state->cancel(); return false;
        }
        QPointer<KeyDelivery> self(this);
        QPointer<QQuickItem> target(receiver_);
        const auto allowed = [state, self, target] {
            return self && target && !state->canceled && Clock::now() < state->deadline;
        };
        try {
            auto delivery = state->acquire(target, state->deadline, allowed);
            state->delivery = std::move(delivery);
            if (!allowed() || !state->delivery) {
                state->refusal = QStringLiteral("key-source-refused"); state->cancel(); return !target;
            }
            const auto sent = state->delivery->snapshot().submission;
            if (!sent.accepted) { state->refusal = sent.refusal; state->cancel(); }
        } catch (...) { state->refusal = QStringLiteral("key-handoff-exception"); state->cancel(); }
        // Always pass through; no accept(), key substitution, grammar or replay.
        // If a callout destroyed the receiver, Qt requires true to avoid UAF.
        return !target;
    }
private slots:
    void invalidate() { state_->cancel(); }
public:
    ~KeyDelivery() override {
        state_->cancel();
        if (receiver_) receiver_->removeEventFilter(this);
    }
    // GUI-only. Profile ownership is transferred; no borrowed provider pointer
    // or QKeyEvent survives arming. The Slot is already process-resident.
    static std::unique_ptr<KeyDelivery> prepare(QQuickItem *receiver, QObject *creationController,
            std::unique_ptr<Profile> profile, Slot &slot, std::chrono::milliseconds budget,
            std::optional<composer_text::ReadRange> range = {}) {
        if (!reader_worker_native::detail::gui() || !profile || !creationController ||
                creationController->thread() != QThread::currentThread()) return {};
        QPointer<QObject> creation(creationController);
        auto ownedProfile = std::shared_ptr<Profile>(std::move(profile));
        return make(receiver, budget, [creation, ownedProfile, &slot, range](QQuickItem *target, Clock::time_point deadline,
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
                        deadline, range);
                    // Submit must happen while the synchronous join is still live.
                    if (result) {
                        // Keep attempted/unknown submission and pinned-owner
                        // evidence, including a throw-after-retain outcome.
                        result->submit(joined);
                    } else slot.cancel(preparation.token);
                });
            return result;
        });
    }
    KeySnapshot snapshot() {
        KeySnapshot result;
        if (!reader_worker_native::detail::gui()) return result;
        const auto state = state_;
        if (Clock::now() >= state->deadline) state->cancel();
        result.entered = state->entered; result.canceled = state->canceled;
        result.refusal = state->refusal; result.key = state->key;
        if (state->delivery) result.scene = state->delivery->snapshot();
        return result;
    }
};
} // namespace remarkable::composer_scene
