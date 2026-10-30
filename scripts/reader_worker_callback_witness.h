#pragma once
#include "reader_request_scope.h"
// Original resident delivery diagnostic. No worker/document/action lease.
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QMetaMethod>
#include <QPointer>
#include <QList>
#include <QThread>
#include <QTimer>
#include <atomic>
#include <functional>
#include <memory>
#include <thread>

namespace reader_selected {
struct WorkerCallbackResult {
    bool finished = false, attempted = false, submissionReturned = false;
    bool matchingCompletion = false, deliveryObserved = false, canceled = false;
    QString jobId; // Exact decimal, never narrowed through a JSON number.
    QString refusal;
    unsigned enteredCount = 0, returningCount = 0, offGuiCount = 0, canceledCount = 0;
    unsigned matchingCompletionCount = 0;
};

// All public methods and destruction belong on the application GUI thread.
// Native code must copy the function value and emit this job's completion after
// invocation/dispatcher return with a nonzero native job ID. The payload stays loaded through process
// exit: native code may retain copies after this observer is gone. Those are
// adapter contracts, not properties established by this generic witness.
// Sources must also obey Qt's no-QObject-reentry notification-hook contract.
class WorkerCallbackWitness final : public QObject {
    Q_OBJECT
public:
    using Submit = std::function<qulonglong(const std::function<void()> &)>;
    explicit WorkerCallbackWitness(QObject *parent = nullptr,reader_request_scope::Binding binding={})
        : QObject(parent),binding_(std::move(binding)) {
        timer_.setParent(this); timer_.setSingleShot(true);
        connect(&timer_, &QTimer::timeout, this, [this] { cancel(QStringLiteral("deadline")); });
    }
    ~WorkerCallbackWitness() override {
        if (state_) state_->canceled.store(true, std::memory_order_release);
        QObject::disconnect(completionConnection_);
        QObject::disconnect(destroyedConnection_);
    }
    bool start(QObject *source, QMetaMethod completion, Submit submit, int remainingMs) {
        if(!gui())return false;
        const reader_request_scope::Scope scope(binding_);
        if(!scope.active())return false;
        if (!gui()) return false;
        if (started_) {
            if (!terminal_) cancel(QStringLiteral("witness-reentered"));
            return false;
        }
        started_ = true; elapsed_.start(); budget_ = remainingMs;
        const QPointer<WorkerCallbackWitness> weak(this);
        if (remainingMs < 1 || remainingMs > 20000) { stop(QStringLiteral("budget-invalid")); return false; }
        source_ = source;
        const bool completionValid = source_ && submit && validCompletion(source_, completion);
        if (!weak) return false;
        if (!completionValid) {
            stop(QStringLiteral("completion-contract")); return false;
        }
        const int slotIndex = metaObject()->indexOfSlot("completed(qulonglong)");
        const auto completionConnection = QObject::connect(source_, completion, this,
            metaObject()->method(slotIndex), Qt::QueuedConnection);
        if (!weak) { QObject::disconnect(completionConnection); return false; }
        completionConnection_ = completionConnection;
        if (terminal_ || !pendingReason_.isEmpty() || !source_) {
            stop(QStringLiteral("source-destroyed")); return false;
        }
        const auto destroyedConnection = QObject::connect(source_, &QObject::destroyed, this,
            [weak] { if (weak) weak->cancel(QStringLiteral("source-destroyed")); }, Qt::QueuedConnection);
        if (!weak) { QObject::disconnect(destroyedConnection); return false; }
        destroyedConnection_ = destroyedConnection;
        if (!completionConnection_ || !destroyedConnection_ || !source_) {
            stop(QStringLiteral("completion-connect")); return false;
        }
        state_ = std::make_shared<State>(std::this_thread::get_id());
        const auto state = state_;
        callback_ = std::make_shared<std::function<void()>>([state] {
            // No QObject/QML/Qt getters, GUI calls, file I/O or observer pointer.
            state->entered.fetch_add(1, std::memory_order_relaxed);
            if (std::this_thread::get_id() != state->guiThread)
                state->offGui.fetch_add(1, std::memory_order_relaxed);
            if (state->canceled.load(std::memory_order_acquire))
                state->canceledEntries.fetch_add(1, std::memory_order_relaxed);
            // This is a tail marker, NOT proof that operator() has returned.
            state->returning.fetch_add(1, std::memory_order_release);
        });
        // These local values survive observer deletion inside a reentrant submit.
        // In particular its const-reference argument cannot dangle mid-submit.
        const auto callback = callback_;
        if (terminal_ || !pendingReason_.isEmpty() || !source_) {
            stop(QStringLiteral("source-destroyed")); return false;
        }
        if (expired()) { stop(QStringLiteral("deadline")); return false; }
        submitting_ = true; result_.attempted = true; // Consume before native call.
        timer_.start(qMax(1, budget_ - int(elapsed_.elapsed())));
        qulonglong returnedId = 0;
        try {
            returnedId = submit(*callback);
            // Release adapter captures while still deferring GUI finalization.
            // An adapter capture destructor may itself reenter the event loop.
            submit = {};
        } catch (...) {
            // Match normal unwind: a last adapter capture can pump GUI events
            // or delete the observer while its destructor releases ownership.
            submit = {};
            if (weak) {
                weak->submitting_ = false;
                weak->stop(QStringLiteral("submission-exception"));
            }
            return false; // Submission may nevertheless have queued a value copy.
        }
        if (!weak) return false;
        weak->submitting_ = false;
        weak->result_.submissionReturned = true;
        weak->result_.jobId = QString::number(returnedId);
        weak->expected_ = returnedId;
        if (!returnedId) weak->stop(QStringLiteral("submission-id-invalid"));
        else if (!weak->source_) weak->stop(QStringLiteral("source-destroyed"));
        else if (weak->expired()) weak->stop(QStringLiteral("deadline"));
        else if (!weak->pendingReason_.isEmpty()) weak->queueFinish();
        else {
            weak->matchingCount_ = weak->early_.count(returnedId);
            weak->early_.clear();
            if (weak->matchingCount_) weak->queueFinish();
        }
        return true; // A returned submission ID, never delivery success.
    }
    void cancel(QString reason = QStringLiteral("canceled")) {
        if(!gui())return;
        const reader_request_scope::Scope scope(binding_);
        if(!scope.entered())return;
        if (!gui() || terminal_) return;
        if (!started_) { started_ = true; stop(reason.left(128)); return; }
        stop(expired() ? QStringLiteral("deadline") : reason.left(128));
    }
    WorkerCallbackResult snapshot() const {
        if (!gui()) { WorkerCallbackResult refused; refused.refusal = QStringLiteral("application-thread"); return refused; }
        return result_; // Atomic facts are copied only once, at finalization.
    }
signals:
    void finished();
private slots:
    void completed(qulonglong id) {
        if(!gui())return;
        const reader_request_scope::Scope scope(binding_);
        if(!scope.active())return;
        if (!gui() || terminal_ || !started_ || !pendingReason_.isEmpty()) return;
        if (expired()) { stop(QStringLiteral("deadline")); return; }
        if (!source_) { stop(QStringLiteral("source-destroyed")); return; }
        if (submitting_) {
            // A native nested GUI loop could deliver before the returned ID is
            // available. Count only a bounded prefix; never finalize that stack.
            if (early_.size() >= 16) { stop(QStringLiteral("early-completion-cap")); return; }
            early_.append(id); return;
        }
        if (!result_.submissionReturned || id != expected_) return;
        ++matchingCount_; queueFinish();
    }
private:
    struct State {
        const std::thread::id guiThread;
        std::atomic<unsigned> entered{0}, returning{0}, offGui{0}, canceledEntries{0};
        std::atomic<bool> canceled{false};
        explicit State(std::thread::id id) : guiThread(id) {}
    };
    bool gui() const {
        auto app = qobject_cast<QGuiApplication *>(QCoreApplication::instance());
        return app && !QCoreApplication::closingDown() && QThread::currentThread() == app->thread() && thread() == app->thread();
    }
    static bool validCompletion(QObject *source, const QMetaMethod &completion) {
        if (!source || !completion.isValid() || completion.methodType() != QMetaMethod::Signal ||
            completion.name() != "jobCompleted" || completion.parameterCount() != 1 ||
            completion.parameterMetaType(0) != QMetaType::fromType<qulonglong>() ||
            completion.returnMetaType() != QMetaType::fromType<void>()) return false;
        for (const QMetaObject *m = source->metaObject(); m; m = m->superClass())
            if (m == completion.enclosingMetaObject()) return true;
        return false;
    }
    bool expired() const { return elapsed_.isValid() && elapsed_.elapsed() >= budget_; }
    void stop(QString reason) {
        if(!gui())return;
        const reader_request_scope::Scope scope(binding_);
        if(!scope.entered())return;
        if (terminal_) return;
        if (pendingReason_.isEmpty()) pendingReason_ = reason.isEmpty() ? QStringLiteral("canceled") : reason.left(128);
        if (state_) state_->canceled.store(true, std::memory_order_release);
        timer_.stop();
        const QPointer<WorkerCallbackWitness> weak(this);
        const auto completion = completionConnection_, destroyed = destroyedConnection_;
        QObject::disconnect(completion);
        if (!weak) return;
        QObject::disconnect(destroyed);
        if (!weak) return;
        // disconnect cannot retract already queued deliveries: completed checks
        // pendingReason/terminal too. No GUI finalization on the submit stack.
        if (!submitting_) queueFinish();
    }
    void queueFinish() {
        if(!gui())return;
        const reader_request_scope::Scope scope(binding_);
        if(!scope.active())return;
        if (terminal_ || submitting_ || finishQueued_) return;
        finishQueued_ = true;
        const QPointer<WorkerCallbackWitness> weak(this);
        if (!QMetaObject::invokeMethod(this, [weak] { if (weak) weak->freeze(); }, Qt::QueuedConnection)) {
            finishQueued_ = false; // Context remains resident; timer can retire it.
        }
    }
    void freeze() {
        if(!gui())return;
        const reader_request_scope::Scope scope(binding_);
        if(!scope.active())return;
        if (terminal_ || submitting_) return;
        finishQueued_ = false;
        if (expired() && pendingReason_.isEmpty()) pendingReason_ = QStringLiteral("deadline");
        if (!source_ && pendingReason_.isEmpty()) pendingReason_ = QStringLiteral("source-destroyed");
        if (state_ && !pendingReason_.isEmpty()) state_->canceled.store(true, std::memory_order_release);
        if (state_) {
            // Acquire tail first, then the preceding entry/thread facts.
            result_.returningCount = state_->returning.load(std::memory_order_acquire);
            result_.enteredCount = state_->entered.load(std::memory_order_relaxed);
            result_.offGuiCount = state_->offGui.load(std::memory_order_relaxed);
            result_.canceledCount = state_->canceledEntries.load(std::memory_order_relaxed);
            result_.canceled = state_->canceled.load(std::memory_order_acquire);
        }
        result_.matchingCompletionCount = matchingCount_;
        result_.matchingCompletion = matchingCount_ != 0;
        if (pendingReason_.isEmpty()) {
            if (!result_.submissionReturned || !matchingCount_) pendingReason_ = QStringLiteral("completion-unobserved");
            else if (matchingCount_ != 1) pendingReason_ = QStringLiteral("completion-repeated");
            else if (result_.enteredCount != 1 || result_.returningCount != 1) pendingReason_ = QStringLiteral("callback-count");
            else if (result_.offGuiCount != 1) pendingReason_ = QStringLiteral("callback-not-off-gui");
            else if (result_.canceledCount || result_.canceled) pendingReason_ = QStringLiteral("canceled");
        }
        result_.deliveryObserved = pendingReason_.isEmpty();
        result_.refusal = pendingReason_; result_.finished = true; terminal_ = true;
        timer_.stop();
        const QPointer<WorkerCallbackWitness> weak(this);
        const auto completion = completionConnection_, destroyed = destroyedConnection_;
        QObject::disconnect(completion);
        if (!weak) return;
        QObject::disconnect(destroyed);
        if (!weak) return;
        callback_.reset(); // Native copies and any active submit local own State.
        emit finished(); // Always a queued GUI turn after submission unwinds.
    }
    const reader_request_scope::Binding binding_;
    QPointer<QObject> source_;
    QMetaObject::Connection completionConnection_, destroyedConnection_;
    QTimer timer_;
    QElapsedTimer elapsed_;
    int budget_ = 0;
    bool started_ = false, submitting_ = false, terminal_ = false, finishQueued_ = false;
    QString pendingReason_;
    QList<qulonglong> early_;
    qulonglong expected_ = 0;
    unsigned matchingCount_ = 0;
    std::shared_ptr<State> state_;
    std::shared_ptr<std::function<void()>> callback_;
    WorkerCallbackResult result_;
};
} // namespace reader_selected
