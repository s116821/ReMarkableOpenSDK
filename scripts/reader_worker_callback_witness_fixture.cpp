// Entirely owned queue interfaces; not a native ABI or worker authority test.
#include "reader_worker_callback_witness.h"
#include <QEvent>
#include <QEventLoop>
#include <QTextStream>
#include <chrono>
#include <limits>

class CallbackEmitter final : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    std::function<void()> onDisconnect;
    void complete(qulonglong id) { emit jobCompleted(id); }
signals:
    void jobCompleted(qulonglong);
    void wrongCompleted(int);
protected:
    void disconnectNotify(const QMetaMethod &) override {
        const auto callback = std::move(onDisconnect); onDisconnect = {};
        if (callback) callback(); // No observer access after this owned hook.
    }
};
static void requireCallback(bool ok, const char *reason) {
    if (!ok) qFatal("FAIL worker callback fixture: %s", reason);
}
static QMetaMethod callbackSignal(CallbackEmitter *source) {
    return source->metaObject()->method(source->metaObject()->indexOfSignal("jobCompleted(qulonglong)"));
}
static void pumpCallback() {
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCoreApplication::processEvents();
}
static void awaitCallback(reader_selected::WorkerCallbackWitness &witness) {
    QElapsedTimer clock; clock.start();
    while (!witness.snapshot().finished && clock.elapsed() < 3000) pumpCallback();
    requireCallback(witness.snapshot().finished, "owned bounded GUI delivery");
}
struct OwnedWorker {
    QThread thread;
    CallbackEmitter *source = new CallbackEmitter;
    OwnedWorker() {
        source->moveToThread(&thread);
        QObject::connect(&thread, &QThread::finished, source, &QObject::deleteLater);
        thread.start();
    }
    ~OwnedWorker() { thread.quit(); requireCallback(thread.wait(3000), "owned worker exit"); }
    void enqueue(std::function<void()> callback, qulonglong id, bool complete = true) {
        QPointer<CallbackEmitter> guarded = source;
        requireCallback(QMetaObject::invokeMethod(source, [guarded, callback = std::move(callback), id, complete] {
            callback(); // Value copy remains alive through completion emission.
            if (complete && guarded) guarded->complete(id);
        }, Qt::QueuedConnection), "owned value-copy enqueue");
    }
};
struct FinalSubmitCapture {
    std::function<void()> onDestroy;
    explicit FinalSubmitCapture(std::function<void()> callback) : onDestroy(std::move(callback)) {}
    ~FinalSubmitCapture() { if (onDestroy) onDestroy(); }
};
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    using Witness = reader_selected::WorkerCallbackWitness;
    constexpr qulonglong large = std::numeric_limits<qulonglong>::max() - 3;
    {
        CallbackEmitter source; Witness witness; int submissions = 0;
        witness.cancel(QStringLiteral("canceled-before-start"));
        requireCallback(!witness.start(&source, callbackSignal(&source), [&](const auto &) { ++submissions; return 1; }, 1000),
            "prestart cancellation consumes observer admission");
        awaitCallback(witness);
        requireCallback(submissions == 0 && !witness.snapshot().attempted && witness.snapshot().refusal == QStringLiteral("canceled-before-start"),
            "prestart cancellation cannot later enqueue");
    }
    {
        CallbackEmitter source; Witness witness;
        bool startReturned = false, destructorPumped = false;
        int publications = 0;
        QObject::connect(&witness, &Witness::finished, &app, [&] {
            requireCallback(startReturned, "throwing Submit cannot publish during final capture destruction");
            ++publications;
        });
        const bool submitted = witness.start(&source, callbackSignal(&source),
            [last = std::make_shared<FinalSubmitCapture>([&] {
                destructorPumped = true; pumpCallback();
                requireCallback(publications == 0 && !witness.snapshot().finished,
                    "exceptional Submit capture destructor reentry stays behind submit barrier");
            })](const std::function<void()> &) -> qulonglong { (void)last; throw 1; }, 1000);
        startReturned = true;
        requireCallback(!submitted && destructorPumped, "throwing Submit releases final capture before returning");
        awaitCallback(witness);
        requireCallback(publications == 1 && !witness.snapshot().deliveryObserved &&
            witness.snapshot().refusal == QStringLiteral("submission-exception"), "throwing Submit finalizes once after unwind");
    }
    {
        CallbackEmitter source; Witness witness;
        witness.start(&source, callbackSignal(&source), [&](const auto &callback) {
            std::thread worker(callback); worker.join(); source.complete(0); return 0;
        }, 1000);
        awaitCallback(witness);
        requireCallback(!witness.snapshot().deliveryObserved && witness.snapshot().refusal == QStringLiteral("submission-id-invalid"),
            "zero returned native job ID refuses even with offGUI callback and matching zero signal");
    }
    {
        OwnedWorker worker; Witness witness;
        int submissions = 0, publications = 0;
        QObject::connect(&witness, &Witness::finished, &app, [&] { ++publications; });
        const bool accepted = witness.start(worker.source, callbackSignal(worker.source), [&](const std::function<void()> &callback) {
            ++submissions; worker.enqueue(callback, large); return large;
        }, 1000);
        requireCallback(accepted && !witness.snapshot().finished, "no synchronous publication on submit stack");
        awaitCallback(witness);
        const auto result = witness.snapshot();
        requireCallback(result.deliveryObserved && result.jobId == QStringLiteral("18446744073709551612") &&
            result.enteredCount == 1 && result.returningCount == 1 && result.offGuiCount == 1 && result.matchingCompletionCount == 1,
            "worker execution with post-return completion and lossless64bit ID");
        requireCallback(!witness.start(worker.source, callbackSignal(worker.source), [&](const auto &) { ++submissions; return 2; }, 1000), "finished witness cannot resubmit");
        worker.source->complete(large); pumpCallback();
        requireCallback(submissions == 1 && publications == 1 && witness.snapshot().deliveryObserved, "snapshot freezes once despite late duplicate signal");
    }
    for (bool callbackRuns : {true, false}) {
        CallbackEmitter source; Witness witness;
        witness.start(&source, callbackSignal(&source), [&](const std::function<void()> &callback) {
            if (callbackRuns) callback();
            source.complete(7); return 7;
        }, 1000);
        awaitCallback(witness);
        requireCallback(!witness.snapshot().deliveryObserved && witness.snapshot().matchingCompletion,
            "inlineGUI or skipped callback cannot become delivery success from generic completion");
        requireCallback(witness.snapshot().enteredCount == unsigned(callbackRuns), "truthful entry count");
    }
    {
        OwnedWorker worker; Witness witness;
        witness.start(worker.source, callbackSignal(worker.source), [&](const auto &callback) {
            worker.source->complete(5); // Foreign ID remains unrelated.
            worker.enqueue(callback, 8); return 8;
        }, 1000);
        awaitCallback(witness); requireCallback(witness.snapshot().deliveryObserved, "foreign completion ignored after returned ID known");
    }
    {
        CallbackEmitter source; Witness witness; int publications = 0;
        QObject::connect(&witness, &Witness::finished, &app, [&] { ++publications; });
        bool submitActive = false;
        witness.start(&source, callbackSignal(&source), [&](const auto &callback) {
            submitActive = true;
            std::thread worker([&] { callback(); source.complete(17); }); worker.join();
            // Only this fixture pumps: force queued completion before ID return.
            pumpCallback();
            requireCallback(publications == 0 && !witness.snapshot().finished, "early nested completion deferred until submit unwind");
            submitActive = false; return 17;
        }, 1000);
        awaitCallback(witness);
        requireCallback(!submitActive && publications == 1 && witness.snapshot().deliveryObserved, "buffered early ID attributed after returned ID");
    }
    {
        CallbackEmitter source; Witness witness;
        std::atomic<bool> reachedTail{false}, allowCompletion{false};
        std::thread worker;
        witness.start(&source, callbackSignal(&source), [&](const auto &callback) {
            worker = std::thread([&, callback] {
                callback(); reachedTail.store(true, std::memory_order_release);
                while (!allowCompletion.load(std::memory_order_acquire)) std::this_thread::yield();
                source.complete(21);
            });
            return 21;
        }, 1000);
        QElapsedTimer clock; clock.start();
        while (!reachedTail.load(std::memory_order_acquire) && clock.elapsed() < 500) pumpCallback();
        requireCallback(reachedTail.load(std::memory_order_acquire) && !witness.snapshot().finished,
            "entry/tail markers alone do not publish completion or actual return");
        allowCompletion.store(true, std::memory_order_release); worker.join();
        awaitCallback(witness); requireCallback(witness.snapshot().deliveryObserved, "attributed post-dispatch completion required in addition to tail marker");
    }
    {
        CallbackEmitter source; Witness witness;
        witness.start(&source, callbackSignal(&source), [&](const auto &callback) {
            std::thread worker([&] { callback(); }); worker.join();
            source.complete(9); source.complete(9); return 9;
        }, 1000);
        awaitCallback(witness);
        requireCallback(!witness.snapshot().deliveryObserved && witness.snapshot().matchingCompletionCount == 2, "duplicate matching completions observed before freeze are diagnostic failure");
    }
    {
        CallbackEmitter source; Witness witness;
        witness.start(&source, callbackSignal(&source), [&](const auto &callback) {
            std::thread worker([&] { callback(); callback(); }); worker.join();
            source.complete(9); return 9;
        }, 1000);
        awaitCallback(witness);
        requireCallback(!witness.snapshot().deliveryObserved && witness.snapshot().enteredCount == 2, "multiple callback entries are not qualified once-delivery");
    }
    for (const QByteArray &mode : {QByteArray("cancel-in-submit"), QByteArray("cancel-queued"), QByteArray("reenter"),
            QByteArray("deadline-submit"), QByteArray("deadline-delivery"), QByteArray("source-delete"), QByteArray("throw-after-copy")}) {
        auto source = std::make_unique<CallbackEmitter>(); Witness witness;
        std::function<void()> retained;
        int publications = 0, submissions = 0;
        QObject::connect(&witness, &Witness::finished, &app, [&] { ++publications; });
        const int budget = mode.startsWith("deadline") ? 5 : 1000;
        witness.start(source.get(), callbackSignal(source.get()), [&](const std::function<void()> &callback) -> qulonglong {
            ++submissions; retained = callback;
            if (mode == "cancel-in-submit") { witness.cancel(); pumpCallback(); requireCallback(publications == 0, "cancel defers publication during submit"); }
            if (mode == "reenter") witness.start(source.get(), callbackSignal(source.get()), [](const auto &) { return 0; }, 1000);
            if (mode == "deadline-submit") std::this_thread::sleep_for(std::chrono::milliseconds(10));
            if (mode == "source-delete") source.reset();
            if (mode == "throw-after-copy") throw 1;
            return 12;
        }, budget);
        if (mode == "cancel-queued") { source->complete(12); witness.cancel(); }
        if (mode == "deadline-delivery") { source->complete(12); std::this_thread::sleep_for(std::chrono::milliseconds(10)); }
        awaitCallback(witness);
        const auto frozen = witness.snapshot();
        std::thread worker(retained); worker.join(); // Benign late native value copy.
        if (source) source->complete(12);
        pumpCallback();
        requireCallback(frozen.finished && !frozen.deliveryObserved && publications == 1 && submissions == 1 &&
            witness.snapshot().enteredCount == frozen.enteredCount && witness.snapshot().refusal == frozen.refusal,
            "cancel/reentry/deadline/source-loss/unknown submission freeze once; late callback cannot republish");
    }
    {
        CallbackEmitter source; auto witness = std::make_unique<Witness>();
        std::function<void()> nativeCopy; int publications = 0;
        QObject::connect(witness.get(), &Witness::finished, &app, [&] { ++publications; });
        witness->start(&source, callbackSignal(&source), [&](const std::function<void()> &callback) {
            witness.reset(); // Actual function object must remain alive on submit stack.
            nativeCopy = callback;
            std::thread worker(nativeCopy); worker.join(); return 3;
        }, 1000);
        source.complete(3); pumpCallback();
        requireCallback(!witness && publications == 0, "observer destruction during submit preserves borrowed function until native copy and late benign execution");
    }
    for (bool atFreeze : {false, true}) {
        CallbackEmitter source; auto witness = std::make_unique<Witness>();
        std::function<void()> nativeCopy; int publications = 0; bool notified = false;
        QObject::connect(witness.get(), &Witness::finished, &app, [&] {
            ++publications;
            if (atFreeze) witness.reset();
        });
        witness->start(&source, callbackSignal(&source), [&](const auto &callback) {
            nativeCopy = callback;
            if (atFreeze) { std::thread worker(callback); worker.join(); source.complete(34); }
            return 34;
        }, 1000);
        // Qt forbids QObject reentry from disconnectNotify. The hook records
        // only plain state; destruction occurs at a valid Qt boundary instead.
        source.onDisconnect = [&] { notified = true; };
        if (!atFreeze) { witness->cancel(); witness.reset(); }
        else {
            QElapsedTimer clock; clock.start();
            while (witness && clock.elapsed() < 500) pumpCallback();
        }
        requireCallback(!witness && notified && publications == int(atFreeze),
            "notification bookkeeping and observer destruction after cancel/from finished are safe");
        std::thread late(nativeCopy); late.join(); pumpCallback();
        requireCallback(publications == int(atFreeze), "retained atomic-only callback cannot republish after observer destruction");
    }
    {
        CallbackEmitter source; Witness witness; std::function<void()> retained;
        witness.start(&source, callbackSignal(&source), [&](const auto &callback) {
            retained = callback;
            for (qulonglong id = 0; id < 17; ++id) source.complete(id);
            pumpCallback(); return 3;
        }, 1000);
        awaitCallback(witness);
        requireCallback(witness.snapshot().refusal == QStringLiteral("early-completion-cap"), "bounded early ID buffer refuses overflow without resubmit");
    }
    {
        CallbackEmitter source; Witness witness; int submissions = 0;
        const auto wrong = source.metaObject()->method(source.metaObject()->indexOfSignal("wrongCompleted(int)"));
        requireCallback(!witness.start(&source, wrong, [&](const auto &) { ++submissions; return 0; }, 1000), "exact queued signal contract enforced");
        awaitCallback(witness); requireCallback(submissions == 0 && !witness.snapshot().attempted, "invalid signal never submits");
    }
    QTextStream(stdout) << "PASS owned resident worker callback witness fixtures\n";
    return 0;
}
#include "reader_worker_callback_witness_fixture.moc"
