// Owned phase orchestration. Sample, leases and hook outcomes are explicit models;
// real Qt witnesses, copied callbacks, worker thread and typed ordinary method run.
#define READER_INSERTION_GUARD_ENTRY_TESTING 1
#include "reader_insertion_guard_entry.h"
#define READER_JOINED_CREATION_OWNED_CLASSES_ONLY
#include "reader_joined_creation_witness_fixture.cpp"
#include <QSemaphore>
#include <QTextStream>
#include <deque>
#include <mutex>

class DocumentWorker final : public QThread {
    Q_OBJECT
public:
    struct Job { qulonglong id; std::function<void()> callback; bool complete; bool duplicate; bool added; int addedIndex = 5; };
    std::mutex mutex;
    std::deque<Job> jobs;
    QSemaphore available, entered;
    bool stopping = false;
    DocumentWorker() { start(); }
    ~DocumentWorker() override { { std::lock_guard<std::mutex> lock(mutex); stopping = true; } available.release(); wait(); }
    void queue(Job job) { { std::lock_guard<std::mutex> lock(mutex); jobs.push_back(std::move(job)); } available.release(); }
    void run() override {
        for (;;) {
            available.acquire(); Job job;
            { std::lock_guard<std::mutex> lock(mutex); if (stopping) return; job = std::move(jobs.front()); jobs.pop_front(); }
            job.callback();
            if (job.added) emit pageAdded(job.id, job.addedIndex);
            if (job.complete) { emit jobCompleted(job.id); if (job.duplicate) emit jobCompleted(job.id); }
            entered.release();
        }
    }
signals:
    void jobCompleted(qulonglong);
    void pageAdded(qulonglong, int);
};
class SceneController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* worker READ worker CONSTANT)
public:
    QPointer<QObject> value;
    QObject *worker() const { return value; }
};
class Reader final : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QObject* sceneController READ sceneController CONSTANT)
public:
    QPointer<QObject> value;
    QObject *sceneController() const { return value; }
};
namespace {
using Entry = reader_insertion_entry::Coordinator;
using Controller = xofm::libs::library::DocumentController;
int checks = 0;
void check(bool yes, const char *what) { if (!yes) qFatal("FAIL %s", what); ++checks; }
void pump(int ms = 5) {
    QElapsedTimer elapsed; elapsed.start();
    do { QCoreApplication::processEvents(QEventLoop::AllEvents, 1); QThread::msleep(1); } while (elapsed.elapsed() < ms);
}
bool words(reader_insertion_predicate::Address, reader_insertion_predicate::Words &) { return false; }
reader_insertion_predicate::Address search(reader_insertion_predicate::Address, const QString &) { return 0; }
bool equal(const QString &a, const QString &b) { return a == b; }
bool conditionWords(reader_insertion_condition::Address, reader_insertion_condition::Words &) { return false; }
bool tree(reader_insertion_condition::Address, reader_insertion_condition::Tree &) { return false; }
reader_insertion_condition::Address find(reader_insertion_condition::Address, const QString &) { return 0; }
bool attribute(void *, std::uint64_t &id) noexcept { id = UINT64_MAX - 2; return true; }
reader_joined_creation::Request requestFor(const QmlDocumentWrapper &doc) {
    reader_joined_creation::Request request;
    request.documentId = doc.key.text; request.sourceIndex = 4; request.sourcePageId = doc.pages[4]; request.expectedPages = doc.pages;
    request.targetPageId = QStringLiteral("20000000-0000-0000-0000-000000000099"); return request;
}
reader_selected::Observation beforeFor(const reader_joined_creation::Request &r) {
    reader_selected::Observation o;
    o.documentId = r.documentId; o.pageId = r.sourcePageId; o.pageIndex = r.sourceIndex; o.pageCount = r.expectedPages.size();
    o.observed = o.creationJoinObserved = o.mapCoherent = o.ready = true;
    o.loading = o.pageError = o.documentError = o.awaitingTiles = o.pendingTiles = o.working = false; return o;
}
struct Model {
    std::mutex mutex;
    reader_insertion_hooks::Outcome outcome;
    std::unique_ptr<reader_insertion_hooks::Registration> registration;
    int probes = 0, acquisitions = 0, samples = 0, registrations = 0, canceled = 0, inflightSnapshots = 0;
    reader_insertion_hooks::Outcome snapshot() { std::lock_guard<std::mutex> lock(mutex); return outcome; }
    void complete(const QByteArray &mode) {
        std::lock_guard<std::mutex> lock(mutex);
        outcome.terminal = outcome.privateCaught = outcome.observedMismatch = outcome.attributed = outcome.witness = true;
        outcome.jobId = mode == "zero-job" ? 0 : UINT64_MAX - 2;
        outcome.outerCalls = outcome.innerCalls = 1;
        if (registration && registration->postRejectProbe && mode != "post-incomplete")
            outcome.postReject = registration->postRejectProbe->once();
        if (mode == "core-refused") { outcome.witness = false; outcome.refusal = reader_insertion_hooks::Refusal::Binding; }
        if (mode == "forwarded") outcome.trackedForwards = 1;
        if (mode.startsWith("conditional-")) {
            outcome.privateCaught = outcome.observedMismatch = false;
            outcome.unknown = outcome.reconciliationRequired = false;
            outcome.forwardCommitted = outcome.innerReturned = outcome.outerReturned = outcome.condition.matched = true;
            outcome.trackedForwards = 1;
            if (mode == "conditional-core-unknown") { outcome.unknown = outcome.reconciliationRequired = true; outcome.witness = false; }
        }
    }
};
void scenario(const QByteArray &mode) {
    const bool conditional = mode.startsWith("conditional-");
    QQmlEngine engine; QQuickWindow window;
    QQmlEngine::setContextForObject(&window, engine.rootContext());
    window.show(); window.requestActivate(); pump();
    DocumentWorker worker, otherWorker;
    SceneController scene; scene.value = &worker;
    Reader reader; reader.value = &scene;
    QmlDocumentWrapper doc; Controller controller; controller.mode = Controller::None;
    auto config = reader_insertion_entry::Config{requestFor(doc), 5, 500};
    if (conditional) {
        if (mode == "conditional-end") config.request.sourceIndex = doc.pages.size() - 1;
        if (mode == "conditional-single") { doc.pages = {doc.pages.first()}; config.request.expectedPages = doc.pages; config.request.sourceIndex = 0; }
        config.request.sourcePageId = doc.pages[config.request.sourceIndex];
        config.wrongSourceIndex = -1; config.wantConditionalForward = true;
        if (mode == "conditional-both-modes") config.wantPostRejectProbe = true;
        if (mode == "conditional-wrong-index-config") config.wrongSourceIndex = 0;
    }
    if (!conditional) config.wantPostRejectProbe = mode.startsWith("post-") && mode != "post-unrequested";
    if (mode == "invalid-config") config.wrongSourceIndex = config.request.sourceIndex;
    if (mode == "deadline") config.budgetMs = 25;
    if (mode == "expired-before-start") config.absoluteDeadline = std::chrono::steady_clock::now() - std::chrono::milliseconds(1);
    if (mode == "absolute-clamp") config.absoluteDeadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(25);
    if (mode == "absolute-valid") { config.budgetMs = 2000; config.absoluteDeadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(1000); }
    const auto before = beforeFor(config.request);
    auto model = std::make_shared<Model>();
    auto owner = std::make_shared<int>(42); std::weak_ptr<int> ownerWeak = owner;
    Entry *entry = nullptr;
    bool permit = true, nestedFinished = false, cancelAfterAcquire = false;
    reader_insertion_entry::Result saved;
    reader_insertion_entry::Runtime runtime;
    runtime.calls = {words, search, equal}; runtime.admission = {true, true, true, true, true, true};
    runtime.conditionAdmission = {true, true, true, true, true, true};
    runtime.outerReturnPC = 0x1000; runtime.innerReturnPC = 0x2000;
    auto *ownedMutex = new pthread_mutex_t{}; // Intentionally resident owned storage.
    check(::pthread_mutex_init(ownedMutex,nullptr)==0,"owned default mutex initialized");
    const auto mutexAt = reinterpret_cast<std::uintptr_t>(ownedMutex);
    auto *postProbe = reader_insertion_mutex_probe::Probe::create({ownedMutex,{mutexAt,mutexAt+sizeof(*ownedMutex)},::pthread_mutex_trylock,::pthread_mutex_unlock});
    check(postProbe != nullptr, "owned optional probe created outside guarded sample");
    runtime.makeAttribution = [&, owner, model](QObject *source) {
        check(source == &worker, "snapshot context bound to retained worker");
        if (mode == "factory-cancel") entry->cancel();
        if (mode == "factory-delete") { delete entry; entry = nullptr; }
        if (mode == "post-factory-cancel") entry->cancel();
        reader_insertion_entry::Attribution result{reinterpret_cast<void *>(0x30c8), owner.get(), attribute,
            mode.startsWith("post-") && mode != "post-missing" ? postProbe : nullptr};
        if (conditional || mode == "condition-unrequested") result.conditionCalls = {conditionWords, search, equal, tree, find};
        if (mode == "conditional-missing-call") result.conditionCalls.find = nullptr;
        return result;
    };
    std::unique_ptr<Entry> holder(new Entry(runtime)); entry = holder.get(); QPointer<Entry> weak = entry;
    Entry::TestBoundary test;
    test.sample = [&](QQuickWindow *w, const reader_insertion_entry::Permit &guard, int budget,
                      const reader_selected::ValidatedSourceAction &action) {
        ++model->samples;
        check(w == &window && budget > 0 && budget <= config.budgetMs, "fresh sample uses original remaining budget");
        if (mode == "absolute-clamp") check(budget <= 25, "sample clamped to original external deadline");
        auto observed = before;
        if (conditional && model->samples >= 3) {
            if (model->samples == 3 && (mode == "conditional-native-shape-retry" || mode == "conditional-native-shape-anchor")) {
                observed = {};
                observed.refusal = mode == "conditional-native-shape-retry" ? QStringLiteral("worker-source-unready") : QStringLiteral("lexical-root-missing");
                return observed; // Matches real Sample's no-action refusal shape.
            }
            if (mode == "conditional-no-action") return observed;
            observed.pageCount = doc.pages.size();
            if (mode == "conditional-target-selected") { observed.pageId = config.request.targetPageId; observed.pageIndex = config.request.sourceIndex + 1; }
            if (mode == "conditional-not-ready" || (mode == "conditional-readiness-retry" && model->samples == 3)) observed.ready = false;
            if (mode == "conditional-wrong-selection") observed.pageId = doc.pages[0];
            if (mode == "conditional-post-epoch") doc.onRead = [&] { permit = false; };
            if (mode == "conditional-post-cancel") doc.onRead = [&] { entry->cancel(); };
            if (mode == "conditional-post-delete") doc.onRead = [&] { delete entry; entry = nullptr; };
            if (mode == "conditional-wrong-doc") {
                QmlDocumentWrapper foreign;
                if (guard()) action(&reader, &foreign, &controller, QVariant::fromValue(foreign.key), observed, guard);
                return observed;
            }
        }
        if (model->samples == 2 && mode == "wrong-source") observed.pageId = doc.pages[0];
        if (model->samples == 2 && mode == "wrong-worker") scene.value = &otherWorker;
        if (model->samples == 2 && mode == "wrong-order") doc.pages.swapItemsAt(0, 1);
        if (model->samples == 2 && mode == "permission-loss") permit = false;
        if (guard()) action(&reader, &doc, &controller, QVariant::fromValue(doc.key), observed, guard);
        if (mode == "conditional-post-sample-epoch" && model->samples >= 3) { observed = {}; observed.refusal = QStringLiteral("relationship-refused"); }
        return observed;
    };
    test.acquire = [&](QObject *c, const QVariant &id, QObject *w, const reader_insertion_entry::Permit &guard) {
        ++model->acquisitions;
        const bool acquired = c && id.isValid() && w && guard();
        if (mode == "cancel-after-acquire") cancelAfterAcquire = true;
        return acquired;
    };
    test.submitProbe = [&](const std::function<void()> &callback, const reader_insertion_entry::Permit &guard) {
        check(guard(), "probe permit only on synchronous sample stack"); ++model->probes;
        reader_worker_native::Submission submitted;
        submitted.attempted = true; submitted.accepted = mode != "zero-probe"; submitted.jobId = submitted.accepted ? UINT64_MAX - 3 : 0;
        if (mode == "inline-probe") { callback(); emit worker.jobCompleted(submitted.jobId); }
        else {
            const auto copy = callback;
            worker.queue({submitted.jobId, [copy, mode] { if (mode == "deadline" || mode == "absolute-clamp") QThread::msleep(45); copy(); if (mode == "duplicate-probe") copy(); }, true, false, false});
        }
        if (mode == "cancel-probe") entry->cancel();
        if (mode == "delete-probe") { delete entry; entry = nullptr; }
        if (mode == "reentry") check(!entry->start(&window, config, [] { return true; }), "nested start consumed once");
        return submitted;
    };
    test.registerOnce = [&, model](reader_insertion_hooks::Registration &&registration) {
        ++model->registrations;
        check(bool(registration.postRejectProbe) == config.wantPostRejectProbe, "only requested registration carries typed probe");
        check(registration.binding.worker == &worker && registration.binding.policy == reader_insertion_hooks::CallerPolicy::BorrowedExplicitSize &&
            registration.binding.optional == nullptr && registration.binding.sizePresent == 1 && registration.binding.inherit == 0 &&
            registration.binding.index == config.request.sourceIndex + 1 && registration.binding.size == config.request.paperSize, "borrowed explicit carrier bound");
        check(registration.binding.thread != std::this_thread::get_id() && registration.binding.thread != std::thread::id{}, "real off GUI thread identity frozen");
        if (mode == "absolute-valid") check(registration.deadline <= config.absoluteDeadline, "Core inherits original external deadline");
        if (conditional) check(!registration.frozen && registration.frozenCondition &&
            registration.policy == reader_insertion_hooks::Policy::ForwardIfCurrent &&
            registration.frozenCondition->source() == config.request.sourcePageId && registration.conditionCalls.find &&
            registration.residentNativeOwners && registration.attribute, "separate correct-source conditional registration");
        else check(registration.frozen->realSource() == config.request.sourcePageId && registration.frozen->wrongSource() == config.request.expectedPages[5] &&
            registration.frozen->rightNeighbor() == config.request.expectedPages[5] && registration.residentNativeOwners && registration.attribute,
            "wrong source only predicate and retained owner attribution");
        model->registration.reset(new reader_insertion_hooks::Registration(std::move(registration)));
        return mode != "registration-refused";
    };
    test.snapshot = [&, model] {
        auto value = model->snapshot();
        if (mode == "conditional-inflight" && value.forwardCommitted && !value.terminal) ++model->inflightSnapshots;
        if (conditional && model->samples >= 3 && (mode == "conditional-late-core" || mode == "conditional-snapshot-delete")) {
            if (mode == "conditional-snapshot-delete") { delete entry; entry = nullptr; }
            value.canceled = value.unknown = value.reconciliationRequired = true; value.witness = false;
        }
        return value;
    };
    test.cancel = [model] { ++model->canceled; };
    if (mode == "conditional-no-core") { test.registerOnce = {}; test.snapshot = {}; test.cancel = {}; }
    entry->testing(std::move(test));
    int finished = 0;
    QObject::connect(entry, &Entry::finished, &window, [&] {
        ++finished; if (nestedFinished) qFatal("publication on ordinary invoke stack");
        if (mode == "delete-finished") { saved = entry->snapshot(); delete entry; entry = nullptr; }
    });
    controller.onCall = [&] {
        nestedFinished = true;
        check(model->registrations == 1 && controller.index == config.request.sourceIndex + 1 && controller.lastId == doc.key && controller.target == config.request.targetPageId,
            "one ordinary Task carries actual ID real index and fresh target");
        const bool complete = mode != "missing-completion";
        const auto job = qulonglong(UINT64_MAX - 2);
        if (mode == "conditional-inflight") {
            std::lock_guard<std::mutex> lock(model->mutex);
            model->outcome.forwardCommitted = model->outcome.unknown = model->outcome.reconciliationRequired = true;
        }
        if (conditional && mode != "conditional-no-insertion") {
            doc.pages.insert(config.request.sourceIndex + 1, config.request.targetPageId);
            if (mode == "conditional-wrong-order") doc.pages.swapItemsAt(0, 1);
        }
        worker.queue({mode == "foreign-completion" || mode == "conditional-foreign-events" ? job - 10 : job,
            [model, mode, &worker, job] { if (mode == "conditional-inflight") QThread::msleep(50);
                model->complete(mode); if (mode == "conditional-duplicate-added") {
                emit worker.pageAdded(job, 5); emit worker.pageAdded(job, 5);
            } }, complete, mode == "duplicate-completion" || mode == "conditional-duplicate-completion",
            mode == "page-added" || (conditional && mode != "conditional-no-added" && mode != "conditional-duplicate-added"),
            config.request.sourceIndex + (mode == "conditional-wrong-added-index" ? 2 : 1)});
        if (mode == "early-completion" || mode == "conditional-early") { check(worker.entered.tryAcquire(1, 1000), "worker probe returned"); check(worker.entered.tryAcquire(1, 1000), "worker Task returned"); pump(); }
        // Both duplicate events must exist before the original invoke frame can
        // finish. Otherwise this fixture races publication against a future signal.
        if (mode == "duplicate-completion" || mode == "conditional-duplicate-completion" || mode == "conditional-duplicate-added") {
            if (!worker.entered.tryAcquire(1,1000) || !worker.entered.tryAcquire(1,1000)) qFatal("duplicate worker did not return");
        }
        if (mode == "cancel-task") entry->cancel();
        if (mode == "false-accept") controller.mode = Controller::False;
        else if (mode == "duplicate-js") controller.mode = Controller::Duplicate;
        else if (mode != "missing-js") controller.mode = Controller::Async;
        nestedFinished = false;
    };
    const bool started = entry->start(&window, config, [&] {
        if (cancelAfterAcquire) { cancelAfterAcquire = false; entry->cancel(); }
        return permit;
    });
    check(started == (mode != "invalid-config" && mode != "expired-before-start" && mode != "conditional-both-modes" &&
        mode != "conditional-wrong-index-config" && mode != "conditional-no-core"), "start admission result");
    if (mode == "before-cancel") entry->cancel();
    QElapsedTimer elapsed; elapsed.start();
    while (weak && !weak->snapshot().finished && elapsed.elapsed() < 1300) pump();
    if (mode == "delete-probe" || mode == "factory-delete" || mode == "delete-finished" ||
        mode == "conditional-post-delete" || mode == "conditional-snapshot-delete") {
        check(!weak, "facade destroyed by guarded callout"); holder.release(); pump(10);
        check(controller.calls == (mode == "delete-finished" || conditional ? 1 : 0) && !ownerWeak.expired(), "facade destruction retains owner context and no additional Task");
        if (mode == "delete-finished") check(saved.observed && saved.finished && saved.job == UINT64_MAX - 2 && finished == 1, "terminal snapshot survives deletion in finished receiver");
    } else {
        check(weak && weak->snapshot().finished && finished == 1, "one deferred terminal result");
        const auto r = weak->snapshot();
        const bool positive = mode == "valid" || mode == "early-completion" || mode == "absolute-valid" || mode == "post-valid" ||
            mode == "conditional-middle" || mode == "conditional-end" || mode == "conditional-single" || mode == "conditional-early" ||
            mode == "conditional-target-selected" || mode == "conditional-readiness-retry" || mode == "conditional-inflight" ||
            mode == "conditional-native-shape-retry";
        if (r.observed != positive) QTextStream(stderr) << "mode=" << mode << " refusal=" << r.refusal << " probe=" << r.probe.refusal << " creation=" << r.creation.refusal << '\n';
        check(r.observed == positive && !r.nativeAuthority && !r.renderAuthority && !r.durableAuthority, "negative diagnostic only no authority");
        if (mode == "cancel-after-acquire") check(r.ownersRetained && r.canceled && model->probes == 0 && controller.calls == 0, "post-acquisition cancellation reports retained owners before any enqueue");
        if (positive && conditional) check(r.job == UINT64_MAX - 2 && r.probeJob == UINT64_MAX - 3 && r.matchingCompletions == 1 &&
            r.pageAdded == 1 && r.pageAddedIndex == config.request.sourceIndex + 1 && r.forwardObserved && !r.reconciliationRequired &&
            r.postOrder.observed && r.postOrder.ids == doc.pages && r.creation.callbackCount == 1 && r.ownersRetained,
            "conditional forward plus full postorder and indexed event conjunction");
        else if (positive) check(r.job == UINT64_MAX - 2 && r.probeJob == UINT64_MAX - 3 && r.matchingCompletions == 1 && !r.pageAdded &&
            r.creation.callbackCount == 1 && r.core.privateCaught && r.core.trackedForwards == 0 && r.ownersRetained, "full width job private catch callback zero added and owner retention");
        if (conditional && controller.calls && !positive) check(r.reconciliationRequired && r.unknown && r.forwardObserved && !r.observed,
            "normal modeled forward with incomplete post-observation requires reconciliation");
        if (mode == "conditional-target-selected") check(r.postOrder.targetSelected, "post snapshot records already selected target");
        if (mode == "conditional-readiness-retry") check(model->samples >= 4 && controller.calls == 1 && model->registrations == 1,
            "readiness retry cannot repeat creation or registration");
        if (mode == "conditional-native-shape-retry") check(model->samples == 4 && controller.calls == 1 && model->registrations == 1,
            "exact native unready refusal may rejoin without another effect");
        if (mode == "conditional-native-shape-anchor" || mode == "conditional-no-action" || mode == "conditional-post-sample-epoch")
            check(model->samples == 3 && r.refusal.startsWith(QStringLiteral("post-join:")), "nonreadiness join failures cannot retry");
        if (mode == "conditional-inflight") check(model->inflightSnapshots >= 1 && !r.unknown && !r.reconciliationRequired,
            "provisional committed unknown snapshot can normally settle before publication");
        if (mode == "conditional-missing-call" || mode == "condition-unrequested") check(controller.calls == 0 && model->registrations == 0 &&
            r.refusal == (conditional ? QStringLiteral("condition-calls-missing") : QStringLiteral("condition-calls-unrequested")),
            "typed condition calls are required exactly in requested mode");
        if (mode == "post-valid") check(reader_insertion_mutex_probe::successful(r.core.postReject), "requested complete post-rejection facts required");
        if (mode == "post-missing" || mode == "post-unrequested") check(model->registrations == 0 && controller.calls == 0 &&
            r.refusal == (mode == "post-missing" ? QStringLiteral("post-reject-probe-missing") : QStringLiteral("post-reject-probe-unrequested")), "configuration and factory result must agree before registration");
        if (mode == "post-incomplete") check(r.refusal == QStringLiteral("post-reject-probe-incomplete") && !r.observed, "incomplete requested facts cannot inherit modeled core witness");
        if (mode == "post-factory-cancel") check(model->registrations == 0 && controller.calls == 0 && r.canceled, "factory cancellation prevents requested registration");
        check(model->probes <= 1 && controller.calls <= 1 && model->registrations <= 1, "once-only all submission stages");
        check(!entry->start(&window, config, [] { return true; }), "terminal entry cannot rearm");
        holder.reset();
    }
    runtime.makeAttribution = {}; owner.reset();
    check(!ownerWeak.expired(), "resident record never implicitly closes fake strong holder");
    // Kept intentionally small and scalar; no document UUIDs in test output.
    QTextStream(stdout) << "PASS " << mode << '\n';
}
}
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    qRegisterMetaType<entry::Id>("entry::Id");
    QMetaType::registerConverter<entry::Id, QString>([](const entry::Id &id) { return id.text; });
    for (const QByteArray mode : {"valid", "early-completion", "invalid-config", "before-cancel", "wrong-source", "wrong-worker", "wrong-order",
            "permission-loss", "factory-cancel", "factory-delete", "registration-refused", "zero-probe", "inline-probe", "duplicate-probe",
            "cancel-probe", "delete-probe", "reentry", "deadline", "missing-completion", "foreign-completion", "duplicate-completion",
            "page-added", "missing-js", "duplicate-js", "false-accept", "cancel-task", "core-refused", "forwarded", "zero-job",
            "expired-before-start", "absolute-clamp", "absolute-valid", "cancel-after-acquire", "delete-finished",
            "post-valid", "post-missing", "post-unrequested", "post-incomplete", "post-factory-cancel"}) scenario(mode);
    if (checks != 537) qFatal("legacy owned check count changed: %d", checks);
    QTextStream(stdout) << "PASS owned coordinator checks=" << checks << '\n';
    checks = 0;
    for (const QByteArray mode : {"conditional-middle", "conditional-end", "conditional-single", "conditional-target-selected",
            "conditional-early", "conditional-readiness-retry", "conditional-inflight", "conditional-not-ready", "conditional-no-insertion", "conditional-wrong-order",
            "conditional-wrong-selection", "conditional-wrong-doc", "conditional-wrong-added-index", "conditional-duplicate-added",
            "conditional-duplicate-completion", "conditional-foreign-events", "conditional-no-added", "conditional-core-unknown",
            "conditional-late-core", "conditional-post-epoch", "conditional-post-cancel", "conditional-post-delete", "conditional-snapshot-delete",
            "conditional-both-modes", "conditional-wrong-index-config", "conditional-missing-call", "conditional-no-core", "condition-unrequested",
            "conditional-native-shape-retry", "conditional-native-shape-anchor", "conditional-no-action", "conditional-post-sample-epoch"}) scenario(mode);
    QTextStream(stdout) << "PASS conditional coordinator checks=" << checks << '\n'; return 0;
}
#include "reader_insertion_guard_entry_fixture.moc"
