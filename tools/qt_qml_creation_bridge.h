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
    std::function<void()> exception, callback;
    void disarm() { armed_ = false; }
public slots:
    bool callbackAllowed() const { return allowed(); }
    bool preflightAllowed() { return allowed() && check && check(false); }
    bool claimMutation() { return allowed() && check && check(true); }
    void refuse(const QString &stage) { if (allowed() && refusal) refusal(stage); }
    void observeReturn(bool known, bool value) { if (allowed() && returned) returned(known, value); }
    void observeException() { if (allowed() && exception) exception(); }
    void observeCallback() { if (allowed() && callback) callback(); }
private:
    bool allowed() const { return QThread::currentThread() == guiThread_ && armed_; }
    QThread *const guiThread_;
    bool armed_ = true;
};
}
