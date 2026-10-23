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
    struct Job { qulonglong id; std::function<void()> callback; bool complete; bool duplicate; bool added; };
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
            if (job.added) emit pageAdded(job.id, 5);
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
    int probes = 0, acquisitions = 0, samples = 0, registrations = 0, canceled = 0;
    reader_insertion_hooks::Outcome snapshot() { std::lock_guard<std::mutex> lock(mutex); return outcome; }
    void complete(const QByteArray &mode) {
        std::lock_guard<std::mutex> lock(mutex);
        outcome.terminal = outcome.privateCaught = outcome.observedMismatch = outcome.attributed = outcome.witness = true;
        outcome.jobId = mode == "zero-job" ? 0 : UINT64_MAX - 2;
        outcome.outerCalls = outcome.innerCalls = 1;
        if (mode == "core-refused") { outcome.witness = false; outcome.refusal = reader_insertion_hooks::Refusal::Binding; }
        if (mode == "forwarded") outcome.trackedForwards = 1;
    }
};
void scenario(const QByteArray &mode) {
    QQmlEngine engine; QQuickWindow window;
    QQmlEngine::setContextForObject(&window, engine.rootContext());
    window.show(); window.requestActivate(); pump();
    DocumentWorker worker, otherWorker;
    SceneController scene; scene.value = &worker;
    Reader reader; reader.value = &scene;
    QmlDocumentWrapper doc; Controller controller; controller.mode = Controller::None;
    auto config = reader_insertion_entry::Config{requestFor(doc), 5, 500};
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
    runtime.outerReturnPC = 0x1000; runtime.innerReturnPC = 0x2000;
    runtime.makeAttribution = [&, owner, model](QObject *source) {
        check(source == &worker, "snapshot context bound to retained worker");
        if (mode == "factory-cancel") entry->cancel();
        if (mode == "factory-delete") { delete entry; entry = nullptr; }
        return reader_insertion_entry::Attribution{reinterpret_cast<void *>(0x30c8), owner.get(), attribute};
    };
    std::unique_ptr<Entry> holder(new Entry(runtime)); entry = holder.get(); QPointer<Entry> weak = entry;
    Entry::TestBoundary test;
    test.sample = [&](QQuickWindow *w, const reader_insertion_entry::Permit &guard, int budget,
                      const reader_selected::ValidatedSourceAction &action) {
        ++model->samples;
        check(w == &window && budget > 0 && budget <= config.budgetMs, "fresh sample uses original remaining budget");
        if (mode == "absolute-clamp") check(budget <= 25, "sample clamped to original external deadline");
        auto observed = before;
        if (model->samples == 2 && mode == "wrong-source") observed.pageId = doc.pages[0];
        if (model->samples == 2 && mode == "wrong-worker") scene.value = &otherWorker;
        if (model->samples == 2 && mode == "wrong-order") doc.pages.swapItemsAt(0, 1);
        if (model->samples == 2 && mode == "permission-loss") permit = false;
        if (guard()) action(&reader, &doc, &controller, QVariant::fromValue(doc.key), observed, guard);
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
        check(registration.binding.worker == &worker && registration.binding.policy == reader_insertion_hooks::CallerPolicy::BorrowedExplicitSize &&
            registration.binding.optional == nullptr && registration.binding.sizePresent == 1 && registration.binding.inherit == 0 &&
            registration.binding.index == 5 && registration.binding.size == config.request.paperSize, "borrowed explicit carrier bound");
        check(registration.binding.thread != std::this_thread::get_id() && registration.binding.thread != std::thread::id{}, "real off GUI thread identity frozen");
        if (mode == "absolute-valid") check(registration.deadline <= config.absoluteDeadline, "Core inherits original external deadline");
        check(registration.frozen->realSource() == config.request.sourcePageId && registration.frozen->wrongSource() == config.request.expectedPages[5] &&
            registration.frozen->rightNeighbor() == config.request.expectedPages[5] && registration.residentNativeOwners && registration.attribute,
            "wrong source only predicate and retained owner attribution");
        model->registration.reset(new reader_insertion_hooks::Registration(std::move(registration)));
        return mode != "registration-refused";
    };
    test.snapshot = [model] { return model->snapshot(); };
    test.cancel = [model] { ++model->canceled; };
    entry->testing(std::move(test));
    int finished = 0;
    QObject::connect(entry, &Entry::finished, &window, [&] {
        ++finished; if (nestedFinished) qFatal("publication on ordinary invoke stack");
        if (mode == "delete-finished") { saved = entry->snapshot(); delete entry; entry = nullptr; }
    });
    controller.onCall = [&] {
        nestedFinished = true;
        check(model->registrations == 1 && controller.index == 5 && controller.lastId == doc.key && controller.target == config.request.targetPageId,
            "one ordinary Task carries actual ID real index and fresh target");
        const bool complete = mode != "missing-completion";
        const auto job = qulonglong(UINT64_MAX - 2);
        worker.queue({mode == "foreign-completion" ? job - 10 : job,
            [model, mode] { model->complete(mode); }, complete, mode == "duplicate-completion", mode == "page-added"});
        if (mode == "early-completion") { check(worker.entered.tryAcquire(1, 1000), "worker probe returned"); check(worker.entered.tryAcquire(1, 1000), "worker Task returned"); pump(); }
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
    check(started == (mode != "invalid-config" && mode != "expired-before-start"), "start admission result");
    if (mode == "before-cancel") entry->cancel();
    QElapsedTimer elapsed; elapsed.start();
    while (weak && !weak->snapshot().finished && elapsed.elapsed() < 1300) pump();
    if (mode == "delete-probe" || mode == "factory-delete" || mode == "delete-finished") {
        check(!weak, "facade destroyed by guarded callout"); holder.release(); pump(10);
        check(controller.calls == (mode == "delete-finished" ? 1 : 0) && !ownerWeak.expired(), "facade destruction retains owner context and no additional Task");
        if (mode == "delete-finished") check(saved.observed && saved.finished && saved.job == UINT64_MAX - 2 && finished == 1, "terminal snapshot survives deletion in finished receiver");
    } else {
        check(weak && weak->snapshot().finished && finished == 1, "one deferred terminal result");
        const auto r = weak->snapshot();
        const bool positive = mode == "valid" || mode == "early-completion" || mode == "absolute-valid";
        if (r.observed != positive) QTextStream(stderr) << "mode=" << mode << " refusal=" << r.refusal << " probe=" << r.probe.refusal << " creation=" << r.creation.refusal << '\n';
        check(r.observed == positive && !r.nativeAuthority && !r.renderAuthority && !r.durableAuthority, "negative diagnostic only no authority");
        if (mode == "cancel-after-acquire") check(r.ownersRetained && r.canceled && model->probes == 0 && controller.calls == 0, "post-acquisition cancellation reports retained owners before any enqueue");
        if (positive) check(r.job == UINT64_MAX - 2 && r.probeJob == UINT64_MAX - 3 && r.matchingCompletions == 1 && !r.pageAdded &&
            r.creation.callbackCount == 1 && r.core.privateCaught && r.core.trackedForwards == 0 && r.ownersRetained, "full width job private catch callback zero added and owner retention");
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
            "expired-before-start", "absolute-clamp", "absolute-valid", "cancel-after-acquire", "delete-finished"}) scenario(mode);
    QTextStream(stdout) << "PASS owned coordinator checks=" << checks << '\n'; return 0;
}
#include "reader_insertion_guard_entry_fixture.moc"
