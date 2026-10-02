/* Development experiment: no controller methods or page operations.
 * QML singleton resolution may invoke registration and change engine ownership.
 * Supported GUI lifecycle is assumed; weak guards do not pin native lifetimes. */
#pragma once
#include <QGuiApplication>
#include <QWindow>
#include <QQmlEngine>
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

inline QByteArray runtimeSnapshot(const QByteArray &priorFailure, int attempts, int compiles,
                                  int events, const char *status, const char *stage, qint64 elapsedMs) {
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
    Probe(QGuiApplication *app, Receipt receipt, int deadlineMs = 5000)
        : QObject(app), app_(app), receipt_(std::move(receipt)), deadlineMs_(deadlineMs) {
        timer_.setSingleShot(true);
        timer_.setTimerType(Qt::PreciseTimer);
        connect(&timer_, &QTimer::timeout, this, [this] { finish("deadline"); });
        connect(app, &QCoreApplication::aboutToQuit, this, [this] { cancel(); });
        elapsed_.start();
        timer_.start(deadlineMs_);
        app->installEventFilter(this);
        queueAttempt();
    }
    ~Probe() override {
        if (app_) app_->removeEventFilter(this);
        delete owned_.data();
    }
protected:
    bool eventFilter(QObject *object, QEvent *event) override {
        if (!done_ && !component_ && qobject_cast<QWindow *>(object) &&
            (event->type() == QEvent::Show || event->type() == QEvent::Expose ||
             event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut))
            queueAttempt(waitingAfterFailure_);
        return false;
    }
private:
    bool expired() { if (elapsed_.elapsed() < deadlineMs_) return false; finish("deadline"); return true; }
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
    void attempt() {
        if (done_ || component_ || expired()) return;
        appThread_ = app_ && QThread::currentThread() == app_->thread();
        if (!appThread_) { finish("application-thread"); return; }
        if (++attempts_ > 8) { finish("readiness-cap"); return; }
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
        component_ = new QQmlComponent(engine_, this);
        waitingAfterFailure_ = false;
        const QPointer<QQmlComponent> producer = component_;
        connect(component_, &QQmlComponent::statusChanged, this, [this, producer] {
            if (producer && component_ == producer) queueComponentReady();
        });
        inCall_ = true;
        ++compileAttempts_;
        component_->setData(helper, QUrl());
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
        if (inCall_) {
            cancelPending_ = true;
            timer_.stop();
            if (app_) app_->removeEventFilter(this);
            return; // No deferred deletion can run inside a nested Qt loop.
        }
        done_ = true;
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
        if (controller_ && elapsedMs >= deadlineMs_) { stage = "deadline"; controller_ = false; }
        diagnostic_ = runtimeSnapshot(diagnostic_, attempts_, compileAttempts_, admittedPostFailureEvents_, status, stage, elapsedMs);
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
    int deadlineMs_, attempts_ = 0;
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
