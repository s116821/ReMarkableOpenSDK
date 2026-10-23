#pragma once
#include "reader_request_scope.h"
#include <QObject>
#include <QPointer>
#include <QThread>
#include <functional>

namespace qml_access {
// Owned GUI-only bridge. Native QJSValue captures the owned QML helper, never
// Probe or a native pointer. Every entry checks thread before GUI state.
class CreationBridge final : public QObject {
    Q_OBJECT
public:
    explicit CreationBridge(QObject *parent,reader_request_scope::Binding binding={})
        : QObject(parent),guiThread_(parent->thread()),binding_(std::move(binding)) {}
    std::function<bool(bool)> check;
    std::function<void(const QString &)> refusal;
    std::function<void(bool, bool)> returned;
    std::function<void(const QString &)> exception;
    std::function<void(const QString &, const QString &, const QString &, const QString &)> privateException;
    std::function<void()> libraryContinuation;
    std::function<void(const QString &, const QString &)> progress;
    std::function<bool()> libraryAdmission;
    std::function<void()> callback;
    void disarm() {
        if(QThread::currentThread()!=guiThread_)return;
        const reader_request_scope::Scope scope(binding_);
        if(!scope.entered())return;
        armed_ = false;
    }
public slots:
    bool callbackAllowed() const {
        if(QThread::currentThread()!=guiThread_)return false;
        const reader_request_scope::Scope scope(binding_);
        return scope.active() && allowed();
    }
    bool preflightAllowed() {
        if(QThread::currentThread()!=guiThread_)return false;
        const reader_request_scope::Scope scope(binding_);
        if(!scope.active() || !allowed())return false;
        const QPointer<CreationBridge> weak(this);
        auto fn=check;const bool yes=fn && fn(false);fn={};
        return scope.active() && weak && yes && weak->allowed();
    }
    bool claimMutation() {
        if(QThread::currentThread()!=guiThread_)return false;
        const reader_request_scope::Scope scope(binding_);
        if(!scope.active() || !allowed())return false;
        const QPointer<CreationBridge> weak(this);
        auto fn=check;const bool yes=fn && fn(true);fn={};
        return scope.active() && weak && yes && weak->allowed();
    }
    void refuse(const QString &stage) {
        if(QThread::currentThread()!=guiThread_)return;
        const reader_request_scope::Scope scope(binding_);
        if(!scope.active() || !allowed())return;
        const auto fn=refusal;if(fn)fn(stage);
    }
    void observeReturn(bool known,bool value) {
        if(QThread::currentThread()!=guiThread_)return;
        const reader_request_scope::Scope scope(binding_);
        if(!scope.active() || !allowed())return;
        const auto fn=returned;if(fn)fn(known,value);
    }
    void observeException(const QString &operation) {
        if(QThread::currentThread()!=guiThread_)return;
        const reader_request_scope::Scope scope(binding_);
        if(!scope.active() || !allowed())return;
        const auto fn=exception;if(fn)fn(operation);
    }
    void observePrivateException(const QString &operation, const QString &category, const QString &name, const QString &message) {
        if(QThread::currentThread()!=guiThread_)return;
        const reader_request_scope::Scope scope(binding_);
        if(!scope.active() || !allowed())return;
        const auto fn=privateException;if(fn)fn(operation,category,boundedError(name),boundedError(message));
    }
    void queueLibraryContinuation() {
        if(QThread::currentThread()!=guiThread_)return;
        const reader_request_scope::Scope scope(binding_);
        if(!scope.active() || !allowed())return;
        const auto fn=libraryContinuation;if(fn)fn();
    }
    void observeProgress(const QString &stage,const QString &readiness) {
        if(QThread::currentThread()!=guiThread_)return;
        const reader_request_scope::Scope scope(binding_);
        if(!scope.active() || !allowed())return;
        const auto fn=progress;if(fn)fn(stage,readiness);
    }
    bool admitLibraryReady() {
        if(QThread::currentThread()!=guiThread_)return false;
        const reader_request_scope::Scope scope(binding_);
        if(!scope.active() || !allowed())return false;
        const QPointer<CreationBridge> weak(this);
        auto fn=libraryAdmission;const bool yes=fn && fn();fn={};
        return scope.active() && weak && yes && weak->allowed();
    }
    void observeCallback() {
        if(QThread::currentThread()!=guiThread_)return;
        const reader_request_scope::Scope scope(binding_);
        if(!scope.active() || !allowed())return;
        const auto fn=callback;if(fn)fn();
    }
private:
    static QString boundedError(const QString &text) {
        QString bounded = text.left(256);
        if (!bounded.isEmpty() && bounded.back().isHighSurrogate()) bounded.chop(1);
        return bounded;
    }
    bool allowed() const { return QThread::currentThread() == guiThread_ && armed_; }
    QThread *const guiThread_;
    bool armed_ = true;
    const reader_request_scope::Binding binding_;
};
}
