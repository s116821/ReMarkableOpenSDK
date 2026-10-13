#pragma once
#include <QObject>
#include <QThread>
#include <functional>

namespace qml_access {
// Owned GUI-only bridge. Native QJSValue captures the owned QML helper, never
// Probe or a native pointer. Every entry checks thread before GUI state.
class CreationBridge final : public QObject {
    Q_OBJECT
public:
    explicit CreationBridge(QObject *parent) : QObject(parent), guiThread_(parent->thread()) {}
    std::function<bool(bool)> check;
    std::function<void(const QString &)> refusal;
    std::function<void(bool, bool)> returned;
    std::function<void(const QString &)> exception;
    std::function<void(const QString &, const QString &, const QString &, const QString &)> privateException;
    std::function<void()> libraryContinuation;
    std::function<void(const QString &, const QString &)> progress;
    std::function<bool()> libraryAdmission;
    std::function<void()> callback;
    void disarm() { armed_ = false; }
public slots:
    bool callbackAllowed() const { return allowed(); }
    bool preflightAllowed() { return allowed() && check && check(false); }
    bool claimMutation() { return allowed() && check && check(true); }
    void refuse(const QString &stage) { if (allowed() && refusal) refusal(stage); }
    void observeReturn(bool known, bool value) { if (allowed() && returned) returned(known, value); }
    void observeException(const QString &operation) { if (allowed() && exception) exception(operation); }
    void observePrivateException(const QString &operation, const QString &category, const QString &name, const QString &message) {
        if (allowed() && privateException) privateException(operation, category, boundedError(name), boundedError(message));
    }
    void queueLibraryContinuation() { if (allowed() && libraryContinuation) libraryContinuation(); }
    void observeProgress(const QString &stage, const QString &readiness) { if (allowed() && progress) progress(stage, readiness); }
    bool admitLibraryReady() { return allowed() && libraryAdmission && libraryAdmission(); }
    void observeCallback() { if (allowed() && callback) callback(); }
private:
    static QString boundedError(const QString &text) {
        QString bounded = text.left(256);
        if (!bounded.isEmpty() && bounded.back().isHighSurrogate()) bounded.chop(1);
        return bounded;
    }
    bool allowed() const { return QThread::currentThread() == guiThread_ && armed_; }
    QThread *const guiThread_;
    bool armed_ = true;
};
}
