// Original owned model only. No firmware layout, address, function or body.
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEvent>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QSizeF>
#include <QString>
#include <QThread>
#include <dobby.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#define ENTRY __attribute__((noinline, noclone, noipa))
static_assert(sizeof(qreal) == 8 && sizeof(QSizeF) == 16, "owned hard-float double-size model");
static unsigned checks = 0;
static void check(bool value, const char *message) {
    ++checks;
    if (!value) { std::fprintf(stderr, "CHECK FAILED: %s\n", message); std::abort(); }
}
static std::thread::id guiThread;
static std::atomic<int> temporaries{0};
struct Temporary {
    QString text{QStringLiteral("owned QString temporary: \u03bb")};
    Temporary() { ++temporaries; }
    ~Temporary() { --temporaries; }
};
class Events : public QObject {
    Q_OBJECT
signals:
    void pageAdded(qulonglong job, int index);
    void jobCompleted(qulonglong job);
    void finished();
};
struct Worker;
struct Map {
    std::mutex mutex;
    std::vector<QString> pages{QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")};
    std::atomic<bool> held{false};
    std::thread::id owner;
    Worker *worker = nullptr;
    int insertions = 0;
    int index = -1;
    QString target, receivedTemplate;
    const QString *templateAddress = nullptr;
    QSizeF receivedSize;
    int receivedCookie = 0;
};
struct Worker {
    Map map;
    Events *events;
    qulonglong job;
    int outerCalls = 0, earlyEffects = 0;
    Worker *foreign = nullptr;
    explicit Worker(Events *e, qulonglong id) : events(e), job(id) { map.worker = this; }
};
enum Mode {
    Normal = 0, EarlyEffect = 1, ThrowBefore = 2, ThrowAfter = 4,
    DuplicateInner = 8, NestedForeignMap = 16, NestedForeignThread = 32,
    DuplicateOuter = 64
};
enum class Phase { Pending, CancelWon, InsertClaimed, Rejected };
struct Observer {};
struct Record {
    Worker *const worker;
    Map *const map;
    const std::vector<QString> expected;
    const int requestedIndex;
    const QString requestedTarget;
    const int sourceIndex = 0;
    const QString sourceId{QStringLiteral("a")};
    const std::chrono::steady_clock::time_point deadline;
    const std::weak_ptr<Observer> observer;
    std::atomic<Phase> phase{Phase::Pending};
    std::atomic<int> outerAttempts{0};
    std::atomic<int> gate{0};
    bool pauseBeforeClaim = false, pauseAfterClaim = false;
    bool refused = false, cleanupAtCatch = false, mutexFreeAtCatch = false;
    bool claimWon = false, innerReturned = false, outerOffGui = false, innerOffGui = false;
    bool eligibleScope = false, duplicateOuterRefused = false;
    int eligibleInner = 0;
    QString reason;
    Record(Worker *w, int index, QString target, std::shared_ptr<Observer> observerValue,
           bool expired = false)
        : worker(w), map(&w->map), expected{QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")},
          requestedIndex(index), requestedTarget(std::move(target)),
          deadline(std::chrono::steady_clock::now() + (expired ? std::chrono::seconds(-1) : std::chrono::seconds(10))),
          observer(observerValue) {}
    bool cancel() {
        auto expectedPhase = Phase::Pending;
        return phase.compare_exchange_strong(expectedPhase, Phase::CancelWon);
    }
};
struct Scope {
    std::shared_ptr<Record> record;
    Scope *previous;
    bool innerVisited = false;
};
static thread_local std::shared_ptr<Record> registration;
static thread_local Scope *activeScope = nullptr;
struct ScopeFrame {
    Scope scope;
    explicit ScopeFrame(std::shared_ptr<Record> record)
        : scope{std::move(record), activeScope, false} { activeScope = &scope; }
    ~ScopeFrame() { activeScope = scope.previous; }
};
struct GuardRejected { const char *reason; };
struct OriginalFailure { const char *reason; };
using Handler = void (*)(Worker *, int, const QString &, QSizeF, const QString &, int, unsigned);
using Insert = void (*)(Map *, int, const QString &, const QString &, QSizeF, int);
static Handler originalHandler = nullptr;
static Insert originalInsert = nullptr;

extern "C" ENTRY void owned_insert(Map *map, int index, const QString &target,
                                    const QString &pageTemplate, QSizeF size, int cookie) {
    Temporary local;
    map->index = index;
    map->target = target;
    map->receivedTemplate = pageTemplate;
    map->templateAddress = &pageTemplate;
    map->receivedSize = size;
    map->receivedCookie = cookie;
    map->pages.insert(map->pages.begin() + index, target);
    ++map->insertions;
}
struct HeldMarker {
    Map *map;
    explicit HeldMarker(Map *m) : map(m) { map->owner = std::this_thread::get_id(); map->held = true; }
    ~HeldMarker() { map->held = false; }
};
extern "C" ENTRY void owned_handler(Worker *worker, int requestedIndex, const QString &pageTemplate,
                                     QSizeF size, const QString &requestedTarget, int cookie, unsigned mode) {
    ++worker->outerCalls;
    Temporary earlyTemporary;
    if (mode & EarlyEffect) ++worker->earlyEffects;
    if (mode & ThrowBefore) throw OriginalFailure{"before-insert"};
    int earlierCount;
    { std::lock_guard<std::mutex> early(worker->map.mutex); earlierCount = int(worker->map.pages.size()); }
    int index = requestedIndex;
    if (index < 0 || index > earlierCount) index = earlierCount;
    {
        std::unique_lock<std::mutex> finalLock(worker->map.mutex);
        HeldMarker held(&worker->map);
        Temporary finalTemporary;
        QString effectiveTarget = requestedTarget;
        for (const auto &id : worker->map.pages) if (id == effectiveTarget) {
            effectiveTarget = QStringLiteral("owned-fallback-id"); break;
        }
        owned_insert(&worker->map, index, effectiveTarget, pageTemplate, size, cookie);
        if (mode & DuplicateInner) owned_insert(&worker->map, index, effectiveTarget, pageTemplate, size, cookie);
        if (mode & ThrowAfter) throw OriginalFailure{"after-insert"};
    }
    emit worker->events->pageAdded(worker->job, index);
}
static void reject(Record &record, const char *reason) {
    auto expectedPhase = Phase::Pending;
    record.phase.compare_exchange_strong(expectedPhase, Phase::Rejected);
    throw GuardRejected{reason};
}
static void waitGate(Record &record, int value) {
    record.gate.store(value);
    while (record.gate.load() == value) std::this_thread::yield();
}
extern "C" ENTRY void insert_hook(Map *map, int index, const QString &target,
                                   const QString &pageTemplate, QSizeF size, int cookie) {
    Scope *scope = activeScope;
    if (!scope || map != scope->record->map) {
        originalInsert(map, index, target, pageTemplate, size, cookie); return;
    }
    auto &record = *scope->record;
    ++record.eligibleInner;
    record.innerOffGui = std::this_thread::get_id() != guiThread;
    if (scope->innerVisited) throw OriginalFailure{"duplicate-inner-after-claim"};
    scope->innerVisited = true;
    if (!map->held.load() || map->owner != std::this_thread::get_id()) throw OriginalFailure{"missing-owned-final-lock"};
    if ((map->worker->foreign) && (cookie == 0x13579)) {
        auto *other = map->worker->foreign;
        const QString foreignTarget = QStringLiteral("foreign-new");
        const unsigned foreignMode = unsigned(other->earlyEffects);
        if (foreignMode == NestedForeignThread) {
            std::thread thread([&] { owned_handler(other, 1, pageTemplate, size, foreignTarget, 77, 0); });
            thread.join();
        } else owned_handler(other, 1, pageTemplate, size, foreignTarget, 77, 0);
    }
    if (target != record.requestedTarget) reject(record, "effective-target-changed");
    if (index != record.requestedIndex) reject(record, "normalized-index-changed");
    if (map->pages != record.expected || map->pages.at(record.sourceIndex) != record.sourceId)
        reject(record, "source-order-mismatch");
    if (record.pauseBeforeClaim) waitGate(record, 1);
    if (record.observer.expired()) reject(record, "observer-lost");
    if (std::chrono::steady_clock::now() >= record.deadline) reject(record, "original-deadline");
    auto pending = Phase::Pending;
    if (!record.phase.compare_exchange_strong(pending, Phase::InsertClaimed)) {
        if (pending == Phase::CancelWon) throw GuardRejected{"cancel-won"};
        throw OriginalFailure{"claim-not-pending"};
    }
    record.claimWon = true;
    if (record.pauseAfterClaim) waitGate(record, 2);
    originalInsert(map, index, target, pageTemplate, size, cookie);
    record.innerReturned = true;
}
extern "C" ENTRY void handler_hook(Worker *worker, int index, const QString &pageTemplate,
                                    QSizeF size, const QString &target, int cookie, unsigned mode) {
    const auto record = registration;
    if (!record || worker != record->worker || index != record->requestedIndex || target != record->requestedTarget) {
        originalHandler(worker, index, pageTemplate, size, target, cookie, mode); return;
    }
    if (record->outerAttempts.fetch_add(1) != 0) {
        record->duplicateOuterRefused = true; return;
    }
    ScopeFrame frame(record);
    record->eligibleScope = true;
    record->outerOffGui = std::this_thread::get_id() != guiThread;
    try { originalHandler(worker, index, pageTemplate, size, target, cookie, mode); }
    catch (const GuardRejected &rejection) {
        record->refused = true;
        record->reason = QString::fromLatin1(rejection.reason);
        record->cleanupAtCatch = temporaries.load() == 0;
        record->mutexFreeAtCatch = worker->map.mutex.try_lock();
        if (record->mutexFreeAtCatch) worker->map.mutex.unlock();
    }
}
struct Outcome {
    int added = 0, completed = 0, finished = 0, foreignAdded = 0;
    bool outsideCaught = false, unexpected = false, workerScopeRestored = false;
    const char *outsideReason = "";
};
static void processQueued() { QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall); }
static void runCase(const char *name, unsigned mode, bool eligible, int index, QString target,
                    bool orderMismatch, int pause, bool observerLoss, bool expired,
                    int expectedInsertions, int expectedAdded, int expectedCompletion,
                    const char *expectedRefusal, const char *expectedOutside = "") {
    std::printf("CASE %s\n", name); std::fflush(stdout);
    Outcome outcome;
    Events events;
    Worker worker(&events, 0x12345678abcdef01ULL), foreign(&events, 0x12345678abcdef02ULL);
    if (orderMismatch) std::swap(worker.map.pages[0], worker.map.pages[1]);
    if (mode & (NestedForeignMap | NestedForeignThread)) {
        worker.foreign = &foreign; foreign.earlyEffects = int(mode & (NestedForeignMap | NestedForeignThread));
    }
    auto observer = std::make_shared<Observer>();
    auto record = std::make_shared<Record>(&worker, index, target, observer, expired);
    record->pauseBeforeClaim = pause == 1;
    record->pauseAfterClaim = pause == 2;
    const auto deadline = record->deadline;
    const QString pageTemplate = QStringLiteral("owned template \u03bb, ref and heap-sharing");
    const QSizeF size(1404.25, 1872.5);
    QObject::connect(&events, &Events::pageAdded, &events, [&](qulonglong job, int) {
        check(std::this_thread::get_id() == guiThread, "pageAdded queued to GUI");
        if (job == worker.job) ++outcome.added; else if (job == foreign.job) ++outcome.foreignAdded;
    }, Qt::QueuedConnection);
    QObject::connect(&events, &Events::jobCompleted, &events, [&](qulonglong job) {
        check(std::this_thread::get_id() == guiThread && job == worker.job, "generic completion exact job on GUI");
        ++outcome.completed; emit events.finished();
    }, Qt::QueuedConnection);
    QObject::connect(&events, &Events::finished, &events, [&] { ++outcome.finished; });
    QThread *thread = QThread::create([&] {
        registration = eligible ? record : nullptr;
        try {
            owned_handler(&worker, index, pageTemplate, size, target, 0x13579, mode);
            if (mode & DuplicateOuter) owned_handler(&worker, index, pageTemplate, size, target, 0x13579, mode);
            emit events.jobCompleted(worker.job);
        } catch (const OriginalFailure &failure) {
            outcome.outsideCaught = true; outcome.outsideReason = failure.reason;
        } catch (...) { outcome.unexpected = true; }
        outcome.workerScopeRestored = activeScope == nullptr;
        registration.reset();
    });
    thread->start();
    if (pause) {
        QElapsedTimer wait; wait.start();
        while (record->gate.load() != pause && wait.elapsed() < 5000) QThread::yieldCurrentThread();
        check(record->gate.load() == pause, "bounded gate arrived");
        bool canceled = false;
        if (observerLoss) observer.reset(); else canceled = record->cancel();
        if (!observerLoss) check(canceled == (pause == 1), "single cancellation versus insertion CAS winner");
        record->gate.store(3);
    }
    check(thread->wait(10000), "owned worker joined without renewing deadline");
    delete thread;
    processQueued();
    check(!outcome.unexpected, "no unexpected exception");
    check(outcome.workerScopeRestored, "worker ScopeFrame restored after original or outside catch");
    check(worker.map.insertions == expectedInsertions, "exact insertion count");
    check(outcome.added == expectedAdded, "page-added independent count");
    check(outcome.completed == expectedCompletion && outcome.finished == expectedCompletion, "generic terminal independent count");
    check(temporaries.load() == 0, "all original QString RAII temporaries destroyed");
    check(!worker.map.held.load(), "original final-lock marker unwound");
    check(worker.map.mutex.try_lock(), "original mutex available after job"); worker.map.mutex.unlock();
    check(activeScope == nullptr && registration == nullptr, "GUI does not borrow worker TLS scope");
    check(record->deadline == deadline, "original deadline unchanged");
    check(record.use_count() == 1, "worker TLS/scope released retained record only after unwind and join");
    check(outcome.outsideCaught == (*expectedOutside != '\0'), "only expected unrelated exception propagated");
    if (*expectedOutside) check(QString::fromLatin1(outcome.outsideReason) == QString::fromLatin1(expectedOutside), "unrelated exception identity preserved");
    check(record->refused == (*expectedRefusal != '\0'), "explicit refusal distinct from completion");
    if (*expectedRefusal) {
        check(record->reason == QString::fromLatin1(expectedRefusal), "exact guard rejection");
        check(record->cleanupAtCatch && record->mutexFreeAtCatch, "original RAII cleanup before private outer catch");
        check(!record->claimWon && !record->innerReturned, "refused operation never won insertion claim");
    }
    if (expectedInsertions) {
        check(worker.map.receivedSize == size, "hard-float QSizeF exact doubles");
        check(worker.map.receivedTemplate == pageTemplate && worker.map.templateAddress == &pageTemplate, "QString reference identity and value forwarded");
        check(worker.map.receivedCookie == 0x13579, "integer stack argument forwarded");
    }
    if (eligible) check(record->outerOffGui && record->eligibleScope, "eligible outer original on actual owned worker");
    if (record->eligibleInner) check(record->innerOffGui, "eligible inner predicate on actual owned worker");
    if (pause == 2) check(record->phase == Phase::InsertClaimed && !record->refused, "claim winner never relabeled pre-effect cancellation");
    if (mode & EarlyEffect) check(worker.earlyEffects == 1, "early side effect survives later refusal");
    if (mode & DuplicateOuter) check(record->duplicateOuterRefused && worker.outerCalls == 1 && worker.map.insertions == 1, "no second eligible submission reaches original");
    if (mode & (NestedForeignMap | NestedForeignThread)) check(foreign.map.insertions == 1 && outcome.foreignAdded == 1 && record->eligibleInner == 1, "foreign map/thread transparent without eligible permission");
    if (mode & DuplicateInner) check(record->eligibleInner == 2 && worker.map.insertions == 1 && !record->refused, "duplicate after claim stays unrelated and cannot insert twice");
}
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv); guiThread = std::this_thread::get_id();
    void *handlerTrampoline = nullptr, *insertTrampoline = nullptr;
    const auto bytes = [](const void *address) { return QString::fromLatin1(QByteArray(static_cast<const char *>(address), 16).toHex()); };
    const QString handlerBefore = bytes(reinterpret_cast<void *>(&owned_handler));
    const QString insertBefore = bytes(reinterpret_cast<void *>(&owned_insert));
    check((reinterpret_cast<uintptr_t>(&owned_handler) & 1U) == 0, "owned handler ARM-mode entry");
    check((reinterpret_cast<uintptr_t>(&owned_insert) & 1U) == 0, "owned insert ARM-mode entry");
    check(DobbyHook(reinterpret_cast<void *>(&owned_handler), reinterpret_cast<void *>(&handler_hook), &handlerTrampoline) == 0, "install outer function-entry hook before threads");
    originalHandler = reinterpret_cast<Handler>(handlerTrampoline);
    check(DobbyHook(reinterpret_cast<void *>(&owned_insert), reinterpret_cast<void *>(&insert_hook), &insertTrampoline) == 0, "install inner function-entry hook before threads");
    originalInsert = reinterpret_cast<Insert>(insertTrampoline);
    check(originalHandler && originalInsert, "relocated original pointers available");
    runCase("accepted", Normal, true, 1, "new", false, 0, false, false, 1, 1, 1, "");
    runCase("transparent-no-record", Normal, false, 1, "new", false, 0, false, false, 1, 1, 1, "");
    runCase("source-order-mismatch", Normal, true, 1, "new", true, 0, false, false, 0, 0, 1, "source-order-mismatch");
    runCase("collision-keeps-outer-eligibility", Normal, true, 1, "b", false, 0, false, false, 0, 0, 1, "effective-target-changed");
    runCase("normalized-index", Normal, true, 99, "new", false, 0, false, false, 0, 0, 1, "normalized-index-changed");
    runCase("cancel-wins", Normal, true, 1, "new", false, 1, false, false, 0, 0, 1, "cancel-won");
    runCase("claim-wins", Normal, true, 1, "new", false, 2, false, false, 1, 1, 1, "");
    runCase("observer-lost-before-claim", Normal, true, 1, "new", false, 1, true, false, 0, 0, 1, "observer-lost");
    runCase("observer-lost-after-claim", Normal, true, 1, "new", false, 2, true, false, 1, 1, 1, "");
    runCase("original-deadline", Normal, true, 1, "new", false, 0, false, true, 0, 0, 1, "original-deadline");
    runCase("early-effect-then-refusal", EarlyEffect, true, 1, "new", true, 0, false, false, 0, 0, 1, "source-order-mismatch");
    runCase("unrelated-before", ThrowBefore, true, 1, "new", false, 0, false, false, 0, 0, 0, "", "before-insert");
    runCase("unrelated-after", ThrowAfter, true, 1, "new", false, 0, false, false, 1, 0, 0, "", "after-insert");
    runCase("duplicate-inner", DuplicateInner, true, 1, "new", false, 0, false, false, 1, 0, 0, "", "duplicate-inner-after-claim");
    runCase("nested-foreign-map", NestedForeignMap, true, 1, "new", false, 0, false, false, 1, 1, 1, "");
    runCase("nested-foreign-thread", NestedForeignThread, true, 1, "new", false, 0, false, false, 1, 1, 1, "");
    runCase("duplicate-outer-no-resubmit", DuplicateOuter, true, 1, "new", false, 0, false, false, 1, 1, 1, "");
    QJsonObject result{{"classification", "owned-arm-interception-fixture"}, {"cases", 17},
        {"checks", int(checks)}, {"passed", true}, {"native_qualification", false},
        {"hooks_resident_until_process_exit", true}, {"runtime_unhook_attempted", false},
        {"owned_handler_before16", handlerBefore}, {"owned_insert_before16", insertBefore},
        {"owned_handler_after16", bytes(reinterpret_cast<void *>(&owned_handler))},
        {"owned_insert_after16", bytes(reinterpret_cast<void *>(&owned_insert))},
        {"owned_handler_trampoline16", bytes(handlerTrampoline)}, {"owned_insert_trampoline16", bytes(insertTrampoline)}};
    std::puts(QJsonDocument(result).toJson(QJsonDocument::Compact).constData());
    return 0;
}
#include "owned_fixture.moc"
