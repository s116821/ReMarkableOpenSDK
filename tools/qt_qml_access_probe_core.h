/* Development experiment: no controller methods or page operations.
 * QML singleton resolution may invoke registration and change engine ownership.
 * Supported GUI lifecycle is assumed; weak guards do not pin native lifetimes. */
#pragma once
#include <QGuiApplication>
#include <QWindow>
#include <QQmlEngine>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlError>
#include <QByteArrayView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QPointer>
#include <QTimer>
#include <QElapsedTimer>
#include <QThread>
#include <functional>

namespace qml_access {
inline constexpr char helper[] =
    "import QtQml\nimport xofm.libs.library\n"
    "QtObject { property QtObject observedController: DocumentController; "
    "readonly property bool controllerAvailable: observedController !== null }";

// English vendor-Qt diagnostics are hints, not a general parser or root cause.
// Qt owns/materializes the error list; these limits bound our inspection only.
inline const char *errorStage(const QList<QQmlError> &errors) {
    if (errors.isEmpty() || errors.size() > 8) return "component-error";
    const char *category = nullptr;
    for (const auto &error : errors) {
        const QString description = error.description();
        if (description.size() > 256) return "component-error";
        const char *current = nullptr;
        if (description.startsWith(QStringLiteral("module \"")) &&
            description.contains(QStringLiteral("\" version ")) &&
            description.endsWith(QStringLiteral(" is not installed"))) current = "module-version";
        else if (description.startsWith(QStringLiteral("module \"")) &&
                 description.endsWith(QStringLiteral("\" is not installed"))) {
            if (description == QStringLiteral("module \"QtQml\" is not installed")) current = "missing-qtqml";
            else if (description == QStringLiteral("module \"QML\" is not installed")) current = "missing-qml";
            else if (description == QStringLiteral("module \"QtQml.Models\" is not installed")) current = "missing-models";
            else if (description == QStringLiteral("module \"QtQml.WorkerScript\" is not installed")) current = "missing-worker";
            else if (description == QStringLiteral("module \"xofm.libs.library\" is not installed")) current = "missing-library";
            else current = "module-missing";
        }
        else if (description.size() > 14 && description.endsWith(QStringLiteral(" is not a type"))) current = "type-missing";
        else if (description == QStringLiteral("Invalid property assignment: unsupported type \"QObject*\"")) current = "property-type";
        else return "component-error";
        if (category && QByteArrayView(category) != QByteArrayView(current)) return "component-error";
        category = current;
    }
    return category;
}

// Private fixed-helper diagnostics only. Never serialize URLs or source text.
inline QByteArray diagnosticSnapshot(const QList<QQmlError> &errors, int attempt = 0) {
    QJsonArray retained;
    const qsizetype count = qMin<qsizetype>(errors.size(), 8);
    for (qsizetype i = 0; i < count; ++i) {
        const auto &error = errors.at(i);
        const QString description = error.description();
        QString retainedDescription = description.left(256);
        // Fixed helper has no user/document input. Keep system plugin paths
        // private, but suppress whole descriptions with personal path prefixes.
        static const QRegularExpression drive(QStringLiteral(R"([A-Za-z]:[\\/])"));
        const bool redacted = retainedDescription.contains(QStringLiteral("/home/"), Qt::CaseInsensitive) ||
            retainedDescription.contains(QStringLiteral("/root/"), Qt::CaseInsensitive) ||
            retainedDescription.contains(QStringLiteral("/Users/"), Qt::CaseInsensitive) ||
            retainedDescription.contains(QStringLiteral("\\Users\\"), Qt::CaseInsensitive) ||
            drive.match(retainedDescription).hasMatch();
        if (redacted) retainedDescription = QStringLiteral("[redacted personal path]");
        const bool truncated = description.size() > 256;
        retained.append(QJsonObject{
            {QStringLiteral("description"), retainedDescription.left(256)},
            {QStringLiteral("description_redacted"), redacted},
            {QStringLiteral("description_truncated"), truncated},
            {QStringLiteral("line"), error.line()}, {QStringLiteral("column"), error.column()}});
    }
    QJsonObject snapshot{
        {QStringLiteral("context"), QStringLiteral("last-compile-failure")},
        {QStringLiteral("failed_attempt"), attempt},
        {QStringLiteral("reported_error_count"), static_cast<qint64>(errors.size())},
        {QStringLiteral("retained_error_count"), static_cast<qint64>(count)},
        {QStringLiteral("count_truncated"), errors.size() > 8},
        {QStringLiteral("output_overflow"), false},
        {QStringLiteral("errors"), retained}};
    QByteArray bytes = QJsonDocument(snapshot).toJson(QJsonDocument::Compact);
    bytes.append('\n');
    if (bytes.size() > 8192) {
        // Reject the oversized serialization; emit a small explicit refusal.
        snapshot.insert(QStringLiteral("errors"), QJsonArray{});
        snapshot.insert(QStringLiteral("retained_error_count"), 0);
        snapshot.insert(QStringLiteral("count_truncated"), true);
        snapshot.insert(QStringLiteral("output_overflow"), true);
        bytes = QJsonDocument(snapshot).toJson(QJsonDocument::Compact) + '\n';
    }
    return bytes;
}

struct RootSummary {
    int readinessBudgetMs = 20000, accessBudgetMs = 5000, rootCount = 0;
    const char *engineKind = "unselected", *gate = "waiting", *witnessKind = "none";
    qint64 witnessAtMs = -1, firstSetDataBeginMs = -1, firstSetDataReturnMs = -1, firstErrorHandledMs = -1;
};

inline QByteArray runtimeSnapshot(const QByteArray &priorFailure, int attempts, int compiles,
                                  int events, const char *status, const char *stage, qint64 elapsedMs,
                                  const RootSummary &root = {}) {
    QJsonObject summary;
    if (priorFailure.isEmpty()) {
        summary = QJsonDocument::fromJson(diagnosticSnapshot({})).object();
        summary.insert(QStringLiteral("context"), QStringLiteral("runtime-only"));
    } else summary = QJsonDocument::fromJson(priorFailure).object();
    summary.insert(QStringLiteral("attempts"), qMin(attempts, 8));
    summary.insert(QStringLiteral("compile_attempts"), compiles);
    summary.insert(QStringLiteral("admitted_post_failure_events"), events);
    summary.insert(QStringLiteral("component_status"), QString::fromLatin1(status));
    summary.insert(QStringLiteral("terminal_stage"), QString::fromLatin1(stage));
    summary.insert(QStringLiteral("elapsed_ms"), elapsedMs);
    summary.insert(QStringLiteral("readiness_budget_ms"), root.readinessBudgetMs);
    summary.insert(QStringLiteral("access_budget_ms"), root.accessBudgetMs);
    summary.insert(QStringLiteral("gate_engine_kind"), QString::fromLatin1(root.engineKind));
    summary.insert(QStringLiteral("root_gate"), QString::fromLatin1(root.gate));
    summary.insert(QStringLiteral("root_witness_kind"), QString::fromLatin1(root.witnessKind));
    summary.insert(QStringLiteral("root_count"), root.rootCount);
    const auto timestamp = [&](const char *key, qint64 value) {
        summary.insert(QString::fromLatin1(key), value < 0 ? QJsonValue(QJsonValue::Null) : QJsonValue(value));
    };
    timestamp("root_witness_at_ms", root.witnessAtMs);
    timestamp("first_setData_begin_ms", root.firstSetDataBeginMs);
    timestamp("first_setData_return_ms", root.firstSetDataReturnMs);
    timestamp("first_error_handled_ms", root.firstErrorHandledMs);
    QByteArray bytes = QJsonDocument(summary).toJson(QJsonDocument::Compact) + '\n';
    if (bytes.size() > 8192) {
        summary.insert(QStringLiteral("errors"), QJsonArray{});
        summary.insert(QStringLiteral("retained_error_count"), 0);
        summary.insert(QStringLiteral("count_truncated"), true);
        summary.insert(QStringLiteral("output_overflow"), true);
        bytes = QJsonDocument(summary).toJson(QJsonDocument::Compact) + '\n';
    }
    return bytes;
}

class Probe final : public QObject {
public:
    using Receipt = std::function<void(const char *, bool, bool, bool, bool, const QByteArray &)>;
    Probe(QGuiApplication *app, Receipt receipt, int readinessMs = 20000, int accessMs = 5000)
        : QObject(app), app_(app), receipt_(std::move(receipt)) {
        rootSummary_.readinessBudgetMs = readinessMs;
        rootSummary_.accessBudgetMs = accessMs;
        timer_.setSingleShot(true);
        timer_.setTimerType(Qt::PreciseTimer);
        connect(&timer_, &QTimer::timeout, this, [this] { (void)expired(); });
        connect(app, &QCoreApplication::aboutToQuit, this, [this] { cancel(); });
        elapsed_.start();
        timer_.start(readinessMs);
        app->installEventFilter(this);
        queueAttempt();
    }
    ~Probe() override {
        if (app_) app_->removeEventFilter(this);
        delete owned_.data();
    }
protected:
    bool eventFilter(QObject *object, QEvent *event) override {
        if (!done_ && !component_ && (!engineSelected_ || gateConsumed_) && qobject_cast<QWindow *>(object) &&
            (event->type() == QEvent::Show || event->type() == QEvent::Expose ||
             event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut))
            queueAttempt(waitingAfterFailure_);
        return false;
    }
private:
    qint64 deadlineAtMs() const {
        return rootSummary_.witnessAtMs < 0 ? rootSummary_.readinessBudgetMs :
            rootSummary_.witnessAtMs + rootSummary_.accessBudgetMs;
    }
    bool expired() {
        if (elapsed_.elapsed() < deadlineAtMs()) return false;
        finish(rootSummary_.witnessAtMs < 0 ? "root-readiness-deadline" : "deadline");
        return true;
    }
    void disconnectRootGate() {
        QObject::disconnect(rootConnection_);
        rootConnection_ = {};
    }
    void queueRootTransition(const char *failure = nullptr) {
        if (done_ || rootTransitionQueued_) return;
        rootTransitionQueued_ = true;
        const QPointer<QQmlEngine> producer = engine_;
        const unsigned epoch = eventEpoch_;
        QMetaObject::invokeMethod(this, [this, producer, epoch, failure] {
            if (done_ || epoch != eventEpoch_) return;
            rootTransitionQueued_ = false;
            if (!producer || producer != engine_) { finish("engine-lost"); return; }
            if (expired()) return;
            if (failure) { rootSummary_.gate = "failed"; finish(failure); return; }
            // Continue the already admitted acquisition, with fresh checks.
            attempt(true);
        }, Qt::QueuedConnection);
    }
    void acceptRootWitness(QObject *object, const char *kind) {
        if (done_ || rootTransitionQueued_ || rootSummary_.witnessAtMs >= 0) return;
        // Same-thread signal handler only captures evidence; never compiles or
        // finishes inside native load(). Queuing is not a native-stack pin.
        if (!app_ || !applicationEngine_ || applicationEngine_ != engine_ ||
            applicationEngine_->thread() != app_->thread()) {
            queueRootTransition("engine-thread"); return;
        }
        if (elapsed_.elapsed() >= rootSummary_.readinessBudgetMs) {
            queueRootTransition("root-readiness-deadline"); return;
        }
        if (!object) { queueRootTransition("root-load-failed"); return; }
        const QPointer<QObject> witness = object;
        const qint64 witnessedAt = elapsed_.elapsed();
        if (witnessedAt >= rootSummary_.readinessBudgetMs) {
            queueRootTransition("root-readiness-deadline"); return;
        }
        rootWitness_ = witness;
        rootSummary_.witnessKind = kind;
        rootSummary_.witnessAtMs = witnessedAt;
        rootSummary_.gate = "accepted";
        const qint64 remaining = deadlineAtMs() - elapsed_.elapsed();
        timer_.start(static_cast<int>(qMax<qint64>(remaining, 0)));
        queueRootTransition();
    }
    void queueAttempt(bool postFailureEvent = false) {
        if (queued_ || done_) return;
        if (postFailureEvent) ++admittedPostFailureEvents_;
        queued_ = true;
        const unsigned epoch = eventEpoch_;
        QMetaObject::invokeMethod(this, [this, epoch] {
            if (epoch != eventEpoch_) return;
            queued_ = false;
            attempt();
        }, Qt::QueuedConnection);
    }
    void queueComponentReady() {
        const QPointer<QQmlComponent> current = component_;
        QMetaObject::invokeMethod(this, [this, current] {
            if (current && component_ == current) componentReady();
        }, Qt::QueuedConnection);
    }
    void attempt(bool rootContinuation = false) {
        if (done_ || component_ || expired()) return;
        appThread_ = app_ && QThread::currentThread() == app_->thread();
        if (!appThread_) { finish("application-thread"); return; }
        if (!rootContinuation && ++attempts_ > 8) { finish("readiness-cap"); return; }
        const auto windows = QGuiApplication::allWindows();
        if (windows.size() > 16) { finish("window-cap"); return; }
        QPointer<QQmlEngine> selected;
        for (QWindow *window : windows) {
            QQmlEngine *candidate = qmlEngine(window);
            if (!candidate) continue;
            if (selected && selected != candidate) { finish("multiple-engines"); return; }
            selected = candidate;
        }
        if (engineSelected_ && (!engine_ || !selected || selected != engine_)) { finish("engine-changed"); return; }
        if (!selected) return; // Only readiness events or the refusal deadline follow.
        const bool firstEngine = !engineSelected_;
        engineSelected_ = true;
        engine_ = selected;
        engineThread_ = engine_->thread() == app_->thread();
        if (!engineThread_) { finish("engine-thread"); return; }
        if (firstEngine) connect(engine_, &QObject::destroyed, this, [this] {
                QMetaObject::invokeMethod(this, [this] { finish("engine-lost"); }, Qt::QueuedConnection);
            });
        if (firstEngine) {
            applicationEngine_ = qobject_cast<QQmlApplicationEngine *>(engine_.data());
            rootSummary_.engineKind = applicationEngine_ ? "application" : "unsupported";
            if (!applicationEngine_) { rootSummary_.gate = "failed"; finish("unsupported-engine"); return; }
            const QPointer<QQmlEngine> producer = engine_;
            QThread *const expectedThread = app_->thread();
            const unsigned gateEpoch = eventEpoch_;
            rootConnection_ = connect(applicationEngine_, &QQmlApplicationEngine::objectCreated, this,
                [this, producer, expectedThread, gateEpoch](QObject *object, const QUrl &) {
                    // Never queue a raw signal pointer. Unexpected cross-thread
                    // emission queues only refusal, without touching GUI state.
                    if (QThread::currentThread() != expectedThread) {
                        QMetaObject::invokeMethod(this, [this, producer, gateEpoch] {
                            if (!done_ && gateEpoch == eventEpoch_ && producer && producer == engine_ && !gateConsumed_)
                                finish("engine-thread");
                        }, Qt::QueuedConnection);
                        return;
                    }
                    if (producer && producer == engine_) acceptRootWitness(object, "signal");
                }, Qt::DirectConnection);
            const auto roots = applicationEngine_->rootObjects();
            rootSummary_.rootCount = static_cast<int>(qMin<qsizetype>(roots.size(), 17));
            if (roots.size() > 16) { rootSummary_.gate = "failed"; finish("root-cap"); return; }
            if (!roots.isEmpty()) acceptRootWitness(roots.first(), "preexisting");
            return;
        }
        if (!gateConsumed_) {
            if (!rootContinuation || rootSummary_.witnessAtMs < 0) return;
            if (!applicationEngine_ || applicationEngine_ != engine_ ||
                applicationEngine_->thread() != app_->thread()) { finish("engine-lost"); return; }
            const auto roots = applicationEngine_->rootObjects();
            rootSummary_.rootCount = static_cast<int>(qMin<qsizetype>(roots.size(), 17));
            if (roots.size() > 16) { rootSummary_.gate = "failed"; finish("root-cap"); return; }
            if (!rootWitness_ || !roots.contains(rootWitness_.data())) {
                rootSummary_.gate = "failed"; finish("root-witness-lost"); return;
            }
            gateConsumed_ = true;
            rootSummary_.gate = "consumed";
            disconnectRootGate();
        }
        if (expired()) return; // Root-list copying is native work, not budget-free.
        component_ = new QQmlComponent(engine_, this);
        waitingAfterFailure_ = false;
        const QPointer<QQmlComponent> producer = component_;
        connect(component_, &QQmlComponent::statusChanged, this, [this, producer] {
            if (producer && component_ == producer) queueComponentReady();
        });
        if (expired()) return; // Allocation/connection do not reset the budget.
        inCall_ = true;
        if (rootSummary_.firstSetDataBeginMs < 0) rootSummary_.firstSetDataBeginMs = elapsed_.elapsed();
        ++compileAttempts_;
        component_->setData(helper, QUrl());
        if (rootSummary_.firstSetDataReturnMs < 0) rootSummary_.firstSetDataReturnMs = elapsed_.elapsed();
        inCall_ = false;
        if (settlePending()) return;
        // Never create/delete inside setData or its synchronous status callback.
        queueComponentReady();
    }
    void componentReady() {
        if (done_ || !component_ || inCall_ || expired() || created_) return;
        if (!engine_) { finish("engine-lost"); return; }
        if (component_->status() == QQmlComponent::Loading) return;
        if (component_->status() != QQmlComponent::Ready) {
            if (rootSummary_.firstErrorHandledMs < 0) rootSummary_.firstErrorHandledMs = elapsed_.elapsed();
            const auto errors = component_->errors();
            diagnostic_ = diagnosticSnapshot(errors, attempts_);
            const char *stage = errorStage(errors);
            if (QByteArrayView(stage) == QByteArrayView("missing-library")) {
                // This failed component owns no helper. Remove it before any
                // later readiness event can schedule a fresh compilation.
                QQmlComponent *failed = component_;
                // Keep the pointer nonnull during deletion to ignore reentrant
                // window events; weak callback guards invalidate on deletion.
                inCall_ = true;
                delete failed;
                component_ = nullptr;
                inCall_ = false;
                ++eventEpoch_;
                queued_ = false; // Older queued events cannot authorize retry.
                waitingAfterFailure_ = true;
                if (settlePending()) return;
                if (!engine_) { finish("engine-lost"); return; }
                (void)expired(); // Existing deadline stays fixed; no retry timer.
            } else finish(stage);
            return;
        }
        created_ = true;
        inCall_ = true;
        owned_ = component_->create();
        inCall_ = false;
        if (settlePending()) return;
        if (!engine_) { finish("engine-lost"); return; }
        if (expired()) return;
        if (!owned_ || component_->isError()) { finish("create-error"); return; }
        helperFound_ = true;
        const QVariant available = owned_->property("controllerAvailable");
        if (!engine_) { finish("engine-lost"); return; }
        if (expired()) return;
        if (available.metaType() != QMetaType::fromType<bool>()) { finish("create-error"); return; }
        controller_ = available.toBool();
        finish(controller_ ? "resolved" : "unavailable");
    }
    void cancel() {
        if (done_) return;
        disconnectRootGate();
        if (inCall_) {
            cancelPending_ = true;
            timer_.stop();
            if (app_) app_->removeEventFilter(this);
            return; // No deferred deletion can run inside a nested Qt loop.
        }
        done_ = true;
        ++eventEpoch_;
        timer_.stop();
        if (app_) app_->removeEventFilter(this);
        // App-context destruction cancels queued transitions. Only owned objects.
        delete owned_.data();
        owned_.clear();
        delete component_;
        component_ = nullptr;
        deleteLater();
    }
    void finish(const char *stage) {
        if (done_) return;
        if (inCall_) {
            pendingStage_ = stage;
            timer_.stop();
            if (app_) app_->removeEventFilter(this);
            return;
        }
        const char *status = "absent";
        if (component_) {
            switch (component_->status()) {
            case QQmlComponent::Null: status = "null"; break;
            case QQmlComponent::Loading: status = "loading"; break;
            case QQmlComponent::Ready: status = "ready"; break;
            case QQmlComponent::Error: status = "error"; break;
            }
        }
        cancel();
        // Cleanup can reenter Qt. Terminal success follows cleanup and another
        // weak guard/deadline check; neither check is a lifetime pin.
        if (controller_ && !engine_) { stage = "engine-lost"; controller_ = false; }
        const qint64 elapsedMs = elapsed_.elapsed();
        if (controller_ && elapsedMs >= deadlineAtMs()) { stage = "deadline"; controller_ = false; }
        diagnostic_ = runtimeSnapshot(diagnostic_, attempts_, compileAttempts_, admittedPostFailureEvents_, status, stage, elapsedMs, rootSummary_);
        receipt_(stage, appThread_, engineThread_, helperFound_, controller_, diagnostic_);
    }
    bool settlePending() {
        if (cancelPending_) { cancel(); return true; }
        if (pendingStage_) { const char *stage = pendingStage_; pendingStage_ = nullptr; finish(stage); return true; }
        return done_;
    }
    QPointer<QGuiApplication> app_;
    Receipt receipt_;
    QTimer timer_;
    QElapsedTimer elapsed_;
    int attempts_ = 0;
    RootSummary rootSummary_;
    bool gateConsumed_ = false, rootTransitionQueued_ = false;
    QMetaObject::Connection rootConnection_;
    QPointer<QQmlApplicationEngine> applicationEngine_;
    QPointer<QObject> rootWitness_;
    int compileAttempts_ = 0, admittedPostFailureEvents_ = 0;
    bool waitingAfterFailure_ = false;
    unsigned eventEpoch_ = 0;
    bool engineSelected_ = false;
    bool queued_ = false, done_ = false, created_ = false, inCall_ = false;
    bool cancelPending_ = false;
    const char *pendingStage_ = nullptr;
    bool appThread_ = false, engineThread_ = false;
    bool helperFound_ = false, controller_ = false;
    QPointer<QQmlEngine> engine_;
    QQmlComponent *component_ = nullptr;
    QPointer<QObject> owned_;
    QByteArray diagnostic_;
};
}
