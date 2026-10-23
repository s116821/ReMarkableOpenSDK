#pragma once
// Fixed research coordinator. No startup installer, transport, navigation or unload.
#include "reader_insertion_guard_hooks.h"
#include "reader_native_worker_bridge.h"
#include "reader_joined_creation_witness.h"
#include "reader_worker_callback_witness.h"
#include "reader_insertion_post_order.h"
#include <QList>

namespace reader_insertion_entry {
using Permit = std::function<bool()>;
struct Config {
    reader_joined_creation::Request request;
    int wrongSourceIndex = -1;
    int budgetMs = 30000;
    std::chrono::steady_clock::time_point absoluteDeadline{};
    bool wantPostRejectProbe = false;
    bool wantConditionalForward = false;
    bool valid() const {
        if (!request.valid() || request.expectedPages.size() > 16 || budgetMs <= 0 || budgetMs > 30000) return false;
        if (wantConditionalForward) return !wantPostRejectProbe && wrongSourceIndex == -1;
        return request.sourceIndex + 1 < request.expectedPages.size() && wrongSourceIndex >= 0 &&
            wrongSourceIndex < request.expectedPages.size() && wrongSourceIndex != request.sourceIndex;
    }
};
struct Attribution {
    void *map = nullptr; // Profile-qualified opaque receiver; Coordinator interprets no member offsets.
    void *context = nullptr; // Immutable, process-resident storage; never a facade/GUI stack pointer.
    bool (*read)(void *, std::uint64_t &) noexcept = nullptr;
    reader_insertion_mutex_probe::Probe *postRejectProbe = nullptr;
    reader_insertion_condition::Calls conditionCalls{};
};
struct Runtime {
    reader_insertion_hooks::Core *core = nullptr; // Already installed and independently qualified.
    reader_insertion_predicate::Calls calls;
    reader_insertion_predicate::CallerAdmission admission;
    std::uintptr_t outerReturnPC = 0, innerReturnPC = 0;
    // GUI callout. Its returned context and all captures must survive candidate exit.
    std::function<Attribution(QObject *)> makeAttribution;
    reader_insertion_condition::CallerAdmission conditionAdmission{};
};
struct Result {
    bool started = false, finished = false, observed = false, canceled = false, unknown = false;
    bool ownersRetained = false, registrationAccepted = false;
    QString stage, refusal;
    std::uint64_t probeJob = 0, job = 0;
    unsigned matchingCompletions = 0, pageAdded = 0;
    reader_selected::WorkerCallbackResult probe;
    reader_joined_creation::Result creation;
    reader_insertion_hooks::Outcome core;
    bool forwardObserved = false, reconciliationRequired = false;
    int pageAddedIndex = -1;
    reader_insertion_post_order::Result postOrder;
    static constexpr bool nativeAuthority = false, renderAuthority = false, durableAuthority = false;
};

class Coordinator final : public QObject {
    Q_OBJECT
public:
#ifdef READER_INSERTION_GUARD_ENTRY_TESTING
    struct TestBoundary {
        std::function<reader_selected::Observation(QQuickWindow *, const Permit &, int,
            const reader_selected::ValidatedSourceAction &)> sample;
        std::function<bool(QObject *, const QVariant &, QObject *, const Permit &)> acquire;
        std::function<reader_worker_native::Submission(const std::function<void()> &, const Permit &)> submitProbe;
        std::function<bool(reader_insertion_hooks::Registration &&)> registerOnce;
        std::function<reader_insertion_hooks::Outcome()> snapshot;
        std::function<void()> cancel;
    };
    void testing(TestBoundary value) { if (!started_) test_ = std::move(value); }
#endif
    explicit Coordinator(Runtime runtime, QObject *parent = nullptr)
        : QObject(parent), resident_(new Resident(std::move(runtime))), probe_(new reader_selected::WorkerCallbackWitness(this)),
          creation_(new reader_joined_creation::Witness(this)) {
        timer_.setParent(this); timer_.setSingleShot(true);
        postTimer_.setParent(this); postTimer_.setSingleShot(true);
        connect(&timer_, &QTimer::timeout, this, [this] { refuse(QStringLiteral("deadline")); });
        connect(&postTimer_, &QTimer::timeout, this, [this] { schedule(); });
        connect(probe_, &reader_selected::WorkerCallbackWitness::finished, this, [this] { schedule(); });
        connect(creation_, &reader_joined_creation::Witness::finished, this, [this] { schedule(); });
    }
    ~Coordinator() override {
        // Deliberately leak Resident, including native owners and plain callback state.
        // QObject children disarm their observers; no native lease closes here.
        if (started_) cancelCore();
    }
    bool start(QQuickWindow *window, Config config, Permit permit) {
        if (!gui()) return false;
        if (started_) { if (!result_.finished) refuse(QStringLiteral("start-reentered")); return false; }
        started_ = true; result_.started = true; config_ = std::move(config); permit_ = std::move(permit);
        deadline_ = Clock::now() + std::chrono::milliseconds(config_.budgetMs);
        if (config_.absoluteDeadline != Clock::time_point{} && config_.absoluteDeadline < deadline_)
            deadline_ = config_.absoluteDeadline;
        window_ = window; engine_ = window ? qmlEngine(window) : nullptr;
        const QPointer<Coordinator> weak(this);
        if (!config_.valid() || !runtimeValid() || !window_ || !engine_ || !allowed()) {
            if (weak) refuse(QStringLiteral("entry-contract"));
            return false;
        }
        if (!weak) return false;
        connect(window_, &QObject::destroyed, this, [this] { refuse(QStringLiteral("window-destroyed")); });
        connect(engine_, &QObject::destroyed, this, [this] { refuse(QStringLiteral("engine-destroyed")); });
        timer_.start(remaining()); phase_ = Phase::First; schedule(); return true;
    }
    void cancel() {
        if (!gui() || result_.finished) return;
        result_.canceled = true; refuse(QStringLiteral("canceled"));
    }
    Result snapshot() const { return result_; } // GUI-only scalar snapshot, no publication authority.
signals:
    void finished();
private slots:
    void completed(qulonglong id) { record(id, false); }
    void added(qulonglong id, int index) { record(id, true, index); }
private:
    using Clock = std::chrono::steady_clock;
    enum class Phase { Idle, First, Probe, Second, Task, Post };
    struct ThreadState {
        std::atomic<unsigned> claim{0};
        std::atomic<bool> ready{false};
        std::thread::id id;
    };
    struct Resident {
        Runtime runtime;
        std::unique_ptr<reader_worker_native::Bridge> bridge;
        std::unique_ptr<reader_worker_native::Prepared> prepared;
        std::shared_ptr<ThreadState> thread = std::make_shared<ThreadState>();
        QPointer<QObject> worker;
        Attribution attribution;
        explicit Resident(Runtime value) : runtime(std::move(value)) {}
    };
    struct Event { qulonglong id; bool added; int index; };
    struct Frame {
        QPointer<Coordinator> p;
        explicit Frame(Coordinator *value) : p(value) { ++p->depth_; }
        ~Frame() { if (p && --p->depth_ == 0 && p->dirty_) p->schedule(); }
    };
    Resident *const resident_; // No destructor path may delete this process-resident record.
    reader_selected::WorkerCallbackWitness *probe_;
    reader_joined_creation::Witness *creation_;
    QTimer timer_, postTimer_;
    QPointer<QQuickWindow> window_;
    QPointer<QQmlEngine> engine_;
    QPointer<QObject> document_;
    QMetaObject::Connection completed_, added_, destroyed_;
    Config config_;
    Permit permit_;
    Result result_;
    Clock::time_point deadline_;
    Phase phase_ = Phase::Idle;
    bool started_ = false, queued_ = false, dirty_ = false;
    int depth_ = 0;
    unsigned postAttempts_ = 0;
    QList<Event> events_;
#ifdef READER_INSERTION_GUARD_ENTRY_TESTING
    TestBoundary test_;
#endif
    bool gui() const {
        auto *app = qobject_cast<QGuiApplication *>(QCoreApplication::instance());
        return app && !QCoreApplication::closingDown() && QThread::currentThread() == app->thread() && thread() == app->thread();
    }
    int remaining() const {
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(deadline_ - Clock::now()).count();
        return ms <= 0 ? 0 : (ms > 30000 ? 30000 : int(ms));
    }
    bool allowed() const {
        QPointer<Coordinator> weak(const_cast<Coordinator *>(this));
        QPointer<QQuickWindow> w = window_; QPointer<QQmlEngine> e = engine_;
        if (!gui() || !started_ || result_.finished || !result_.refusal.isEmpty() || !remaining() || !w || !e || !permit_ ||
            w->thread() != thread() || e->thread() != thread() || qmlEngine(w) != e ||
            QGuiApplication::focusWindow() != w || !w->isActive() || !w->isVisible()) return false;
        const Permit permit = permit_;
        bool yes = false; try { yes = permit(); } catch (...) {}
        return weak && w && e && yes && weak->gui() && !weak->result_.finished && weak->result_.refusal.isEmpty() &&
            weak->remaining() > 0 && weak->window_ == w && weak->engine_ == e && qmlEngine(w) == e &&
            QGuiApplication::focusWindow() == w && w->isActive() && w->isVisible();
    }
    bool runtimeValid() const {
        const auto &r = resident_->runtime; const auto &a = r.admission;
        bool core = r.core != nullptr;
#ifdef READER_INSERTION_GUARD_ENTRY_TESTING
        core = core || (test_.registerOnce && test_.snapshot && test_.cancel);
#endif
        if (!core || !r.makeAttribution || !r.outerReturnPC || !r.innerReturnPC) return false;
        if (config_.wantConditionalForward) {
            const auto &c = r.conditionAdmission;
            return c.profileAndProviders && c.originalCallsite && c.operationScope && c.workerThread && c.nativeOwners && c.finalLock;
        }
        return r.calls.readWords && r.calls.search && r.calls.equal && a.profileAndProviders && a.originalCallsite &&
            a.operationScope && a.workerThread && a.nativeOwners && a.finalLock;
    }
    void cancelCore() {
#ifdef READER_INSERTION_GUARD_ENTRY_TESTING
        if (test_.cancel) { const auto fn = test_.cancel; fn(); return; }
#endif
        if (resident_->runtime.core) resident_->runtime.core->cancel();
    }
    reader_insertion_hooks::Outcome coreSnapshot() const {
#ifdef READER_INSERTION_GUARD_ENTRY_TESTING
        if (test_.snapshot) { const auto fn = test_.snapshot; return fn(); }
#endif
        return resident_->runtime.core ? resident_->runtime.core->snapshot() : reader_insertion_hooks::Outcome{};
    }
    static bool forwardGood(const reader_insertion_hooks::Outcome &o) {
        return o.terminal && o.witness && o.attributed && o.jobId && o.outerCalls == 1 && o.innerCalls == 1 &&
            o.trackedForwards == 1 && o.forwardCommitted && o.innerReturned && o.outerReturned && o.condition.matched &&
            !o.unknown && !o.canceled && !o.interference && !o.reconciliationRequired;
    }
    void updateForward(const reader_insertion_hooks::Outcome &o) {
        result_.core = o; result_.job = o.jobId;
        // Factual normal-forward conjunction is independent of post-order success.
        result_.forwardObserved = o.terminal && o.forwardCommitted && o.innerReturned && o.outerReturned &&
            o.trackedForwards == 1 && o.condition.matched;
        // An in-flight snapshot is intentionally conservative; it is not a
        // terminal unknown outcome. A later normally settled snapshot may pass.
        result_.reconciliationRequired = result_.reconciliationRequired || (o.terminal && (o.reconciliationRequired || o.unknown));
        result_.unknown = result_.unknown || (o.terminal && o.unknown);
    }
    bool indexedEvents() {
        unsigned complete = 0, pages = 0; int index = -1;
        for (const auto &event : events_) if (result_.job && event.id == result_.job) {
            if (event.added) { ++pages; index = event.index; } else ++complete;
        }
        result_.matchingCompletions = complete; result_.pageAdded = pages; result_.pageAddedIndex = index;
        return complete == 1 && pages == 1 && index == config_.request.sourceIndex + 1;
    }
    void refuse(const QString &reason) {
        if (result_.finished) return;
        if (result_.refusal.isEmpty()) result_.refusal = reason.left(128);
        const QPointer<Coordinator> weak(this);
        cancelCore(); if (!weak) return;
        probe_->cancel(reason); if (!weak) return;
        creation_->cancel(reason); if (!weak) return;
        timer_.stop(); postTimer_.stop(); dirty_ = true; schedule();
    }
    void schedule() {
        dirty_ = true;
        if (queued_ || depth_ || result_.finished) return;
        queued_ = true;
        const QPointer<Coordinator> weak(this);
        if (!QMetaObject::invokeMethod(this, [weak] { if (weak) { weak->queued_ = false; weak->step(); } }, Qt::QueuedConnection))
            queued_ = false; // Original deadline remains armed; no alternate submission.
    }
    void finish(bool positive) {
        timer_.stop(); postTimer_.stop();
        const QPointer<Coordinator> weak(this);
        const auto completion = completed_, addition = added_, destruction = destroyed_;
        QObject::disconnect(completion); if (!weak) return;
        QObject::disconnect(addition); if (!weak) return;
        QObject::disconnect(destruction); if (!weak) return;
        result_.probe = probe_->snapshot(); result_.creation = creation_->snapshot();
        if (config_.wantConditionalForward) {
            if (positive) { const bool permission = allowed(); if (!weak) return; positive = permission; }
            const auto latest = coreSnapshot(); if (!weak) return;
            updateForward(latest);
            const auto &c = result_.creation;
            positive = positive && result_.refusal.isEmpty() && !result_.canceled && !result_.unknown && remaining() &&
                forwardGood(latest) && indexedEvents() && result_.postOrder.observed && c.finished &&
                c.attempted && c.returnKnown && c.accepted && c.callbackObserved && c.callbackCount == 1 &&
                !c.unknown && !c.duplicate && !c.canceled;
            result_.reconciliationRequired = result_.reconciliationRequired || (latest.forwardCommitted && !positive);
            result_.unknown = result_.unknown || latest.unknown || (latest.forwardCommitted && !positive);
        }
        result_.unknown = result_.unknown || (result_.creation.attempted && !positive);
        result_.observed = positive; result_.finished = true;
        result_.stage = config_.wantConditionalForward ? (positive ? QStringLiteral("insertion-guard-forward-observed") :
            QStringLiteral("insertion-guard-forward-incomplete")) :
            (positive ? QStringLiteral("insertion-guard-refusal-observed") : QStringLiteral("insertion-guard-refused"));
        emit finished();
    }
    void record(qulonglong id, bool added, int index = -1) {
        if (!gui() || result_.finished || !result_.refusal.isEmpty()) return;
        if (events_.size() >= 32) { refuse(QStringLiteral("worker-event-cap")); return; }
        events_.append({id, added, index}); schedule();
    }
    static bool ancestry(QObject *p, const char *name) { return reader_worker_native::detail::ancestry(p, name); }
    static QMetaMethod signal(QObject *p, const char *name, bool added) {
        if (!p || !p->metaObject() || p->metaObject()->methodCount() > 512) return {};
        QMetaMethod found; int matches = 0;
        for (int i = 0; i < p->metaObject()->methodCount(); ++i) {
            const auto m = p->metaObject()->method(i); if (m.name() == name) { ++matches; found = m; }
        }
        if (matches != 1 || found.methodType() != QMetaMethod::Signal || found.returnMetaType() != QMetaType::fromType<void>() ||
            qstrcmp(found.enclosingMetaObject()->className(), "DocumentWorker") || found.parameterCount() != (added ? 2 : 1) ||
            found.parameterMetaType(0) != QMetaType::fromType<qulonglong>() ||
            (added && found.parameterMetaType(1) != QMetaType::fromType<int>())) return {};
        return found;
    }
    QObject *readWorker(QObject *reader, const Permit &guard) {
        QPointer<QObject> root(reader);
        const QPointer<Coordinator> weak(this);
        if (!root || !guard || !guard() || !weak || !root) return nullptr;
        const int ri = root->metaObject()->indexOfProperty("sceneController");
        if (ri < 0) return nullptr;
        const auto rp = root->metaObject()->property(ri);
        if (!rp.isReadable() || !rp.metaType().flags().testFlag(QMetaType::PointerToQObject)) return nullptr;
        const auto rv = rp.read(root); QPointer<QObject> scene = rv.value<QObject *>();
        if (!guard() || !weak || !root || !scene || !ancestry(scene, "SceneController") || scene->thread() != thread()) return nullptr;
        const int wi = scene->metaObject()->indexOfProperty("worker");
        if (wi < 0) return nullptr;
        const auto wp = scene->metaObject()->property(wi);
        if (!wp.isReadable() || !wp.isConstant() || !wp.metaType().flags().testFlag(QMetaType::PointerToQObject)) return nullptr;
        const auto wv = wp.read(scene); QPointer<QObject> worker = wv.value<QObject *>();
        if (!guard() || !weak || !scene || !worker || worker->thread() != thread() || !ancestry(worker, "DocumentWorker") ||
            !qobject_cast<QThread *>(worker.data())) return nullptr;
        return worker;
    }
    bool expected(const reader_selected::Observation &o) const {
        const auto &r = config_.request;
        return o.observed && o.creationJoinObserved && o.mapCoherent && o.ready && !o.loading && !o.pageError && !o.documentError &&
            !o.awaitingTiles && !o.pendingTiles && !o.working && o.documentId == r.documentId && o.pageId == r.sourcePageId &&
            o.pageIndex == r.sourceIndex && o.pageCount == r.expectedPages.size();
    }
    reader_selected::Observation sample(const reader_selected::ValidatedSourceAction &action) {
        const QPointer<Coordinator> weak(this);
        const Permit permit = [weak] { return weak && weak->allowed(); };
        const int ms = qMin(remaining(), reader_selected::MaxObservationMs);
#ifdef READER_INSERTION_GUARD_ENTRY_TESTING
        if (test_.sample) { const auto fn = test_.sample; return fn(window_, permit, ms, action); }
#endif
        return reader_selected::observeCreationJoin(window_, permit, ms, {}, action);
    }
    void first(QObject *reader, QObject *controller, const QVariant &id,
               const reader_selected::Observation &before, const Permit &guard) {
        const QPointer<Coordinator> weak(this);
        QPointer<QObject> controllerPin(controller);
        Resident *const resident = resident_;
        if (!expected(before)) { refuse(QStringLiteral("first-source")); return; }
        QPointer<QObject> worker = readWorker(reader, guard);
        if (!weak) return;
        const auto completion = signal(worker, "jobCompleted", false);
        if (!worker || !completion.isValid() || !guard() || !weak || !worker) { if (weak) refuse(QStringLiteral("worker-contract")); return; }
        bool acquired = false;
#ifdef READER_INSERTION_GUARD_ENTRY_TESTING
        if (test_.acquire) { const auto fn = test_.acquire; acquired = fn(controllerPin, id, worker, guard); }
        else
#endif
        {
            auto loading = reader_worker_native::loadPinned(guard);
            if (!weak) return;
            if (loading.bridge) {
                resident_->bridge = std::move(loading.bridge);
                auto preparation = resident_->bridge->prepare(controllerPin, id, worker, guard);
                if (!weak) { if (preparation.prepared) resident->prepared = std::move(preparation.prepared); return; }
                resident_->prepared = std::move(preparation.prepared); acquired = bool(resident_->prepared);
            }
        }
        if (!weak) return;
        result_.ownersRetained = acquired; // A later permit refusal never releases successful leases.
        if (!acquired || !worker || !guard() || !weak) { if (weak) refuse(QStringLiteral("native-lease")); return; }
        if (!worker || !controllerPin) { refuse(QStringLiteral("native-source-destroyed")); return; }
        resident_->worker = worker; phase_ = Phase::Probe;
        const auto state = resident_->thread;
        probe_->start(worker, completion, [weak, state, &guard](const std::function<void()> &tail) -> qulonglong {
            if (!weak || !guard() || !weak) return 0;
            const std::function<void()> callback = [state, tail] {
                unsigned zero = 0;
                if (state->claim.compare_exchange_strong(zero, 1, std::memory_order_acq_rel)) {
                    state->id = std::this_thread::get_id(); state->ready.store(true, std::memory_order_release);
                }
                tail();
            };
            reader_worker_native::Submission submitted;
#ifdef READER_INSERTION_GUARD_ENTRY_TESTING
            if (weak->test_.submitProbe) { const auto fn = weak->test_.submitProbe; submitted = fn(callback, guard); }
            else
#endif
                submitted = weak->resident_->prepared->submit(callback, guard);
            if (weak && submitted.unknown) weak->result_.unknown = true;
            return submitted.accepted ? submitted.jobId : 0;
        }, qMin(remaining(), 20000));
    }
    void second(QObject *reader, QObject *document, QObject *controller, const QVariant &id,
                const reader_selected::Observation &before, const Permit &guard) {
        const QPointer<Coordinator> weak(this);
        QPointer<QObject> documentPin(document), controllerPin(controller);
        if (!expected(before)) { refuse(QStringLiteral("second-source")); return; }
        QPointer<QObject> worker = readWorker(reader, guard);
        if (!weak) return;
        if (!worker || worker != resident_->worker ||
            (resident_->prepared && resident_->prepared->worker() != worker) || !guard() || !weak) {
            if (weak) refuse(QStringLiteral("worker-incarnation"));
            return;
        }
        const auto factory = resident_->runtime.makeAttribution;
        const Attribution attribution = factory(worker);
        if (!weak) return;
        if (!worker || !attribution.map || !attribution.context || !attribution.read || !guard() || !weak || !documentPin || !controllerPin) {
            if (weak) refuse(QStringLiteral("attribution-context"));
            return;
        }
        if (config_.wantPostRejectProbe != bool(attribution.postRejectProbe)) {
            refuse(config_.wantPostRejectProbe ? QStringLiteral("post-reject-probe-missing") :
                QStringLiteral("post-reject-probe-unrequested")); return;
        }
        const auto &calls = attribution.conditionCalls;
        const bool anyCondition = calls.readWords || calls.search || calls.equal || calls.readTree || calls.find;
        const bool allCondition = calls.readWords && calls.search && calls.equal && calls.readTree && calls.find;
        if (config_.wantConditionalForward ? !allCondition : anyCondition) {
            refuse(config_.wantConditionalForward ? QStringLiteral("condition-calls-missing") : QStringLiteral("condition-calls-unrequested")); return;
        }
        resident_->attribution = attribution;
        const auto r = config_.request; // Survives facade deletion inside a borrowed guard.
        reader_insertion_hooks::Registration registration;
        registration.target = QUuid(r.targetPageId);
        if (config_.wantConditionalForward) {
            auto freezing = reader_insertion_condition::freeze({r.sourceIndex, int(r.expectedPages.size()), r.sourcePageId, r.targetPageId,
                r.sourceIndex + 1 < r.expectedPages.size() ? r.expectedPages[r.sourceIndex + 1] : QString()});
            if (!freezing.frozen) { refuse(QStringLiteral("condition-config")); return; }
            registration.policy = reader_insertion_hooks::Policy::ForwardIfCurrent;
            registration.frozenCondition = std::move(freezing.frozen);
            registration.conditionCalls = attribution.conditionCalls;
            registration.conditionAdmission = resident_->runtime.conditionAdmission;
        } else {
            auto freezing = reader_insertion_predicate::freeze({r.sourceIndex, config_.wrongSourceIndex, int(r.expectedPages.size()),
                r.sourcePageId, r.expectedPages[r.sourceIndex + 1], r.expectedPages[config_.wrongSourceIndex], r.targetPageId});
            if (!freezing.frozen) { refuse(QStringLiteral("predicate-config")); return; }
            registration.frozen = std::move(freezing.frozen);
        }
        registration.binding = {worker.data(), attribution.map, resident_->thread->id,
            resident_->runtime.outerReturnPC, resident_->runtime.innerReturnPC, r.sourceIndex + 1, 0, nullptr, 1, 0,
            r.paperSize, reader_insertion_hooks::CallerPolicy::BorrowedExplicitSize};
        registration.calls = resident_->runtime.calls; registration.admission = resident_->runtime.admission;
        registration.deadline = deadline_; registration.residentNativeOwners = resident_;
        registration.attributionContext = attribution.context; registration.attribute = attribution.read;
        registration.postRejectProbe = config_.wantPostRejectProbe ? attribution.postRejectProbe : nullptr;
        if (!guard() || !weak) { if (weak) refuse(QStringLiteral("registration-permission")); return; }
        bool registered = false;
#ifdef READER_INSERTION_GUARD_ENTRY_TESTING
        if (test_.registerOnce) { const auto fn = test_.registerOnce; registered = fn(std::move(registration)); }
        else
#endif
            registered = resident_->runtime.core->registerOnce(std::move(registration));
        if (!weak) return;
        result_.registrationAccepted = registered;
        if (!registered || !guard() || !weak) { if (weak) refuse(QStringLiteral("registration-refused")); return; }
        if (config_.wantConditionalForward) document_ = documentPin;
        phase_ = Phase::Task;
        creation_->submit(controllerPin, documentPin, id, before, r, guard, remaining());
    }
    bool connectTask() {
        const QPointer<Coordinator> weak(this); QPointer<QObject> worker = resident_->worker;
        const auto completion = signal(worker, "jobCompleted", false), addition = signal(worker, "pageAdded", true);
        if (!worker || !completion.isValid() || !addition.isValid()) return false;
        auto connection = QObject::connect(worker, completion, this, metaObject()->method(metaObject()->indexOfSlot("completed(qulonglong)")), Qt::QueuedConnection);
        if (!weak) { QObject::disconnect(connection); return false; } completed_ = connection;
        connection = QObject::connect(worker, addition, this, metaObject()->method(metaObject()->indexOfSlot("added(qulonglong,int)")), Qt::QueuedConnection);
        if (!weak) { QObject::disconnect(connection); return false; } added_ = connection;
        connection = connect(worker, &QObject::destroyed, this, [this] { refuse(QStringLiteral("worker-destroyed")); }, Qt::QueuedConnection);
        if (!weak) { QObject::disconnect(connection); return false; } destroyed_ = connection;
        return completed_ && added_ && destroyed_ && worker;
    }
    void conditionalTask() {
        const QPointer<Coordinator> weak(this);
        const auto outcome = coreSnapshot(); if (!weak) return;
        updateForward(outcome); result_.creation = creation_->snapshot();
        if (outcome.terminal && !forwardGood(outcome)) { refuse(QStringLiteral("core-forward-refused")); return; }
        if (outcome.terminal) {
            indexedEvents();
            if (result_.matchingCompletions > 1 || result_.pageAdded > 1 ||
                (result_.pageAdded && result_.pageAddedIndex != config_.request.sourceIndex + 1)) {
                refuse(QStringLiteral("forward-worker-events")); return;
            }
        }
        const auto &c = result_.creation;
        if (c.finished && (c.unknown || !c.attempted || !c.returnKnown || !c.accepted || !c.callbackObserved ||
            c.duplicate || c.callbackCount != 1 || c.canceled)) { refuse(QStringLiteral("ordinary-callback-refused")); return; }
        if (outcome.terminal && indexedEvents() && c.finished) { phase_ = Phase::Post; schedule(); }
    }
    void post() {
        const QPointer<Coordinator> weak(this);
        const auto latest = coreSnapshot(); if (!weak) return;
        updateForward(latest);
        if (!forwardGood(latest) || !indexedEvents()) { refuse(QStringLiteral("post-core-or-events")); return; }
        if (!document_) { refuse(QStringLiteral("post-document")); return; }
        if (++postAttempts_ > 128) { refuse(QStringLiteral("post-observation-cap")); return; }
        bool called = false, ready = false, waiting = false;
        const auto observed = sample([weak, &called, &ready, &waiting](QObject *, QObject *document, QObject *, const QVariant &id,
                const reader_selected::Observation &joined, const Permit &guard) {
            if (!weak) return;
            called = true;
            QPointer<QObject> pin(document);
            if (!pin || pin != weak->document_) { weak->refuse(QStringLiteral("post-document")); return; }
            // Readiness may settle without a new Task. Every retry is observation only.
            if ((!joined.ready || joined.loading || joined.working || joined.awaitingTiles || joined.pendingTiles) &&
                !joined.pageError && !joined.documentError) { waiting = true; return; }
            const auto order = reader_insertion_post_order::observe(weak->engine_, pin, id, weak->config_.request, joined, guard);
            if (!weak) return;
            weak->result_.postOrder = order;
            if (!order.observed) { weak->refuse(QStringLiteral("post-order") + QStringLiteral(":") + order.refusal); return; }
            ready = true;
        });
        if (!weak || !result_.refusal.isEmpty()) return;
        if (observed.observed && called && ready) { finish(true); return; }
        // Real Sample suppresses SourceAction for its fixed unready/error gate.
        // That exact refusal is retryable; lookup/metadata/epoch failures are not.
        const bool retry = (!called && observed.refusal == QStringLiteral("worker-source-unready")) ||
            (called && waiting && observed.observed);
        if (!retry) { refuse(QStringLiteral("post-join:") + observed.refusal.left(96)); return; }
        if (!allowed() || !weak) { if (weak) refuse(QStringLiteral("post-permission")); return; }
        postTimer_.start(qMin(100, remaining()));
    }
    void step() {
        if (result_.finished) return;
        if (depth_) { dirty_ = true; return; }
        dirty_ = false;
        const QPointer<Coordinator> weak(this);
        if (!result_.refusal.isEmpty()) { finish(false); return; }
        if (!allowed()) { if (weak) refuse(QStringLiteral("permission-or-deadline")); return; }
        if (!weak) return;
        Frame frame(this);
        if (phase_ == Phase::First) {
            const auto observed = sample([weak](QObject *reader, QObject *, QObject *c, const QVariant &id,
                    const reader_selected::Observation &o, const Permit &guard) { if (weak) weak->first(reader, c, id, o, guard); });
            if (weak && (!observed.observed || phase_ == Phase::First)) refuse(QStringLiteral("first-join"));
        } else if (phase_ == Phase::Probe) {
            const auto probe = probe_->snapshot(); if (!probe.finished) return;
            result_.probe = probe;
            if (!probe.deliveryObserved || !resident_->thread->ready.load(std::memory_order_acquire) ||
                resident_->thread->id == std::this_thread::get_id()) { refuse(QStringLiteral("thread-probe")); return; }
            bool ok = false; result_.probeJob = probe.jobId.toULongLong(&ok);
            if (!ok || !result_.probeJob) { refuse(QStringLiteral("probe-job")); return; }
            if (!creation_->prepare(engine_) || !weak) { if (weak) refuse(QStringLiteral("callback-preparation")); return; }
            if (!connectTask() || !weak) { if (weak) refuse(QStringLiteral("task-signals")); return; }
            phase_ = Phase::Second; schedule();
        } else if (phase_ == Phase::Second) {
            const auto observed = sample([weak](QObject *reader, QObject *d, QObject *c, const QVariant &id,
                    const reader_selected::Observation &o, const Permit &guard) { if (weak) weak->second(reader, d, c, id, o, guard); });
            if (weak && (!observed.observed || phase_ == Phase::Second)) refuse(QStringLiteral("second-join"));
            if (weak) schedule();
        } else if (phase_ == Phase::Task) {
            if (config_.wantConditionalForward) { conditionalTask(); return; }
            const auto outcome = coreSnapshot(); if (!weak) return;
            result_.core = outcome;
            result_.creation = creation_->snapshot();
            if (outcome.terminal) {
                result_.job = outcome.jobId; unsigned complete = 0, pages = 0;
                for (const auto &event : events_) if (outcome.jobId && event.id == outcome.jobId) { if (event.added) ++pages; else ++complete; }
                result_.matchingCompletions = complete; result_.pageAdded = pages;
                if (!outcome.witness || !outcome.attributed || !outcome.jobId || outcome.outerCalls != 1 || outcome.innerCalls != 1 || outcome.trackedForwards != 0) {
                    refuse(QStringLiteral("core-refused")); return;
                }
                if (config_.wantPostRejectProbe && !reader_insertion_mutex_probe::successful(outcome.postReject)) {
                    refuse(QStringLiteral("post-reject-probe-incomplete")); return;
                }
                if (complete > 1 || pages) { refuse(pages ? QStringLiteral("page-added") : QStringLiteral("completion-repeated")); return; }
            }
            const auto &c = result_.creation;
            if (c.finished && (c.unknown || !c.attempted || !c.returnKnown || !c.accepted || !c.callbackObserved || c.duplicate || c.callbackCount != 1 || c.canceled)) {
                refuse(QStringLiteral("ordinary-callback-refused")); return;
            }
            if (outcome.terminal && result_.matchingCompletions == 1 && c.finished) { finish(true); return; }
        } else if (phase_ == Phase::Post) {
            post();
        }
    }
};
} // namespace reader_insertion_entry
