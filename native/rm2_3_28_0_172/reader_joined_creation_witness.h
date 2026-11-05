#pragma once
// Original ordinary-controller diagnostic; no exclusion, persistence or retry lease.
#include "reader_selected_view_probe.h"
#include "qt_qml_creation_bridge.h"
#include <QJSValue>
#include <QQmlExpression>
#include <QSet>
#include <QSizeF>
#include <QTimer>
#include <QUuid>
#include <cmath>

namespace reader_joined_creation {
inline bool canonicalId(const QString &text) {
    const QUuid id(text);
    return !id.isNull() && id.toString(QUuid::WithoutBraces) == text;
}
struct Request {
    QString documentId, sourcePageId, targetPageId;
    QStringList expectedPages;
    int sourceIndex = -1;
    QSizeF paperSize{1404, 1872};
    bool valid() const {
        if (!canonicalId(documentId) || !canonicalId(sourcePageId) || !canonicalId(targetPageId) ||
            expectedPages.isEmpty() || expectedPages.size() > 32 || sourceIndex < 0 || sourceIndex >= expectedPages.size() ||
            expectedPages[sourceIndex] != sourcePageId || expectedPages.contains(targetPageId) ||
            !std::isfinite(paperSize.width()) || !std::isfinite(paperSize.height()) ||
            paperSize.width() <= 0 || paperSize.height() <= 0 || paperSize.width() > 16384 || paperSize.height() > 16384) return false;
        QSet<QString> unique;
        for (const auto &id : expectedPages) { if (!canonicalId(id) || unique.contains(id)) return false; unique.insert(id); }
        return true;
    }
};
struct Result {
    bool finished=false, attempted=false, returnKnown=false, accepted=false, unknown=false;
    bool callbackObserved=false, duplicate=false, canceled=false;
    unsigned callbackCount=0;
    QString refusal;
};
class Witness final : public QObject {
    Q_OBJECT
public:
    explicit Witness(QObject *parent=nullptr,reader_request_scope::Binding binding={})
        : QObject(parent),binding_(std::move(binding)) {
        timer_.setParent(this); timer_.setSingleShot(true);
        connect(&timer_, &QTimer::timeout, this, [this] { cancel(QStringLiteral("deadline")); });
    }
    ~Witness() override { if (bridge_) bridge_->disarm(); }
    bool prepare(QQmlEngine *engine) {
        if(!gui())return false;
        const reader_request_scope::Scope scope(binding_);
        if(!scope.active())return false;
        if (!gui() || prepared_ || started_) return false;
        const QPointer<Witness> weak(this);
        QPointer<QQmlEngine> pin(engine);
        if (!pin || pin->thread()!=thread()) return false;
        engine_=pin;
        auto *bridge=new qml_access::CreationBridge(this,binding_);
        bridge_=bridge;
        QQmlEngine::setObjectOwnership(bridge, QQmlEngine::CppOwnership);
        bridge->callback=[weak] { if (weak) weak->callback(); };
        const QJSValue factory=pin->evaluate(QStringLiteral(
            "(function(bridge){return function(){try{if(bridge && bridge.callbackAllowed())bridge.observeCallback();}catch(e){}}})"),
            QStringLiteral("owned:joined-creation-callback"));
        if (!weak || !pin || !factory.isCallable()) return false;
        const QJSValue object=pin->newQObject(bridge);
        if (!weak || !pin || !weak->bridge_) return false;
        const QJSValue callback=factory.call({object});
        if (!weak || !pin || !callback.isCallable() || callback.isError()) return false;
        weak->callback_=callback;
        const auto connection=connect(pin, &QObject::destroyed, weak, [weak] { if(weak) weak->cancel(QStringLiteral("engine-destroyed")); });
        if (!weak) { QObject::disconnect(connection); return false; }
        weak->prepared_=true;
        return true;
    }
    // Returns invocation-return knowledge only, independent of bool acceptance/callback.
    // Every borrowed object and permission stays on this synchronous stack.
    bool submit(QObject *controller, QObject *document, const QVariant &actualId,
                const reader_selected::Observation &before, const Request &request,
                const std::function<bool()> &synchronousGuard, int remainingMs) {
        if(!gui())return false;
        const reader_request_scope::Scope scope(binding_);
        if(!scope.active())return false;
        if (!gui()) return false;
        const QPointer<Witness> weak(this);
        if (started_) { if (!terminal_) cancel(QStringLiteral("witness-reentered")); return false; }
        started_=true; elapsed_.start(); budget_=remainingMs;
        if (!prepared_ || !engine_ || !callback_.isCallable() || remainingMs<1 || remainingMs>30000) { stop(QStringLiteral("preparation-or-budget")); return false; }
        QPointer<QObject> c(controller), d(document);
        const QPointer<QQmlEngine> e=engine_;
        const auto fail=[weak](const char *reason) { if(weak) weak->stop(QString::fromLatin1(reason)); return false; };
        const auto live=[&] {
            if (!scope.active() || !weak || !c || !d || !e || !weak->gui() || weak->terminal_ || !weak->pending_.isEmpty() || weak->expired() ||
                c->thread()!=QThread::currentThread() || d->thread()!=QThread::currentThread() ||
                (qmlEngine(c) && qmlEngine(c)!=e) || (qmlEngine(d) && qmlEngine(d)!=e)) return false;
            if (!synchronousGuard || !synchronousGuard()) return false;
            return scope.active() && weak && c && d && e && weak->gui() && !weak->terminal_ && weak->pending_.isEmpty() && !weak->expired() &&
                c->thread()==QThread::currentThread() && d->thread()==QThread::currentThread();
        };
        submitting_=true; // Defer completion through all preflight/argument destructors.
        const auto perform=[&]() -> bool {
        try {
            if (!request.valid() || !before.observed || !before.creationJoinObserved || !before.mapCoherent || !before.ready ||
                before.loading || before.pageError || before.documentError || before.awaitingTiles || before.pendingTiles || before.working ||
                before.documentId!=request.documentId || before.pageId!=request.sourcePageId || before.pageIndex!=request.sourceIndex ||
                before.pageCount!=request.expectedPages.size() || !live()) return fail("source-or-permission");
            if (!nativeClass(c, reader_selected::detail::CreationControllerClass) || !nativeClass(d, "QmlDocumentWrapper")) return fail("native-class");
            const auto method=exact(c, "addPageWithTemplateAndPageSize(entry::Id,int,QString,QSizeF,QJSValue,QString)", QMetaType::fromType<bool>());
            if (!method.isValid() || method.access()!=QMetaMethod::Public ||
                method.enclosingMetaObject()->className()!=QByteArray(reader_selected::detail::CreationControllerClass) ||
                !actualId.isValid() || actualId.metaType().flags().testFlag(QMetaType::IsPointer) ||
                method.parameterMetaType(0)!=actualId.metaType() || QByteArray(actualId.metaType().name())!="entry::Id") return fail("method-or-id-type");
            if (method.parameterMetaType(1)!=QMetaType::fromType<int>() || method.parameterMetaType(2)!=QMetaType::fromType<QString>() ||
                method.parameterMetaType(3)!=QMetaType::fromType<QSizeF>() || method.parameterMetaType(4)!=QMetaType::fromType<QJSValue>() ||
                method.parameterMetaType(5)!=QMetaType::fromType<QString>()) return fail("method-types");
            const auto idProperty=d->metaObject()->property(d->metaObject()->indexOfProperty("id"));
            const auto countProperty=d->metaObject()->property(d->metaObject()->indexOfProperty("pageCount"));
            if (!idProperty.isValid() || !idProperty.isReadable() || !idProperty.isConstant() || !countProperty.isValid() || !countProperty.isReadable() ||
                countProperty.metaType()!=QMetaType::fromType<int>()) return fail("document-properties");
            if (!live()) return fail("permission");
            const QVariant currentId=idProperty.read(d);
            if (!live() || currentId.metaType()!=actualId.metaType() || currentId!=actualId) return fail("document-id-mismatch");
            QQmlExpression expression(e->rootContext(), d, QStringLiteral("String(id)"));
            const QVariant textual=expression.evaluate();
            if (!live() || expression.hasError() || textual.metaType()!=QMetaType::fromType<QString>() || textual.toString()!=request.documentId) return fail("document-id-text");
            const auto idForPage=exact(d,"idForPage(int)",QMetaType::fromType<QString>());
            const auto pageForId=exact(d,"pageForId(QString)",QMetaType::fromType<int>());
            const auto templateForPage=exact(d,"templateForPage(int)",QMetaType::fromType<QString>());
            if (!idForPage.isValid() || !pageForId.isValid() || !templateForPage.isValid()) return fail("document-methods");
            if (!live()) return fail("permission");
            const QVariant count=countProperty.read(d);
            if (!live() || count.metaType()!=QMetaType::fromType<int>() || count.toInt()!=request.expectedPages.size()) return fail("page-count");
            for (int index=0; index<request.expectedPages.size(); ++index) {
                QString id; int roundtrip=-1;
                if (!live() || !idForPage.invoke(d,Qt::DirectConnection,Q_RETURN_ARG(QString,id),Q_ARG(int,index)) || !live() || id!=request.expectedPages[index] ||
                    !pageForId.invoke(d,Qt::DirectConnection,Q_RETURN_ARG(int,roundtrip),Q_ARG(QString,id)) || !live() || roundtrip!=index) return fail("page-order");
            }
            int targetIndex=-2;
            const QString target=request.targetPageId;
            if (!live() || !pageForId.invoke(d,Qt::DirectConnection,Q_RETURN_ARG(int,targetIndex),Q_ARG(QString,target)) || !live() || targetIndex!=-1) return fail("target-present");
            QString pageTemplate; const int sourceIndex=request.sourceIndex;
            if (!templateForPage.invoke(d,Qt::DirectConnection,Q_RETURN_ARG(QString,pageTemplate),Q_ARG(int,sourceIndex)) || !live() || pageTemplate.size()>1024) return fail("page-template");
            // Local argument copies survive observer deletion and the borrowed permission
            // is never captured by persistent JS/bridge/timer state.
            const QVariant id=actualId;
            const int insertionIndex=sourceIndex+1;
            const QSizeF size=request.paperSize;
            const QJSValue callback=weak->callback_;
            if (!live()) return fail("permission");
            weak->submitting_=true; weak->result_.attempted=true;
            weak->timer_.start(qMax(1,budget_-int(elapsed_.elapsed())));
            bool accepted=false;
            const bool invoked=method.invoke(c,Qt::DirectConnection,QGenericReturnArgument("bool",&accepted),
                QGenericArgument(method.parameterTypes()[0].constData(),id.constData()),QGenericArgument("int",&insertionIndex),QGenericArgument("QString",&pageTemplate),
                QGenericArgument("QSizeF",&size),QGenericArgument("QJSValue",&callback),QGenericArgument("QString",&target));
            if (!weak) return false;
            weak->result_.returnKnown=invoked; weak->result_.accepted=invoked && accepted;
            if (!invoked) weak->stop(QStringLiteral("invocation-unknown"));
            else if (!accepted) weak->stop(QStringLiteral("controller-refused"));
            else if (!e) weak->stop(QStringLiteral("engine-destroyed"));
            else if (weak->expired()) weak->stop(QStringLiteral("deadline"));
            else if (!weak->pending_.isEmpty() || weak->result_.callbackCount) weak->queueFinish();
            return invoked;
        } catch (...) {
            if (weak) weak->stop(QStringLiteral("invocation-exception"));
            return false;
        }
        };
        const bool invoked=perform(); // All ephemeral native/JS arguments have unwound.
        if (!weak) return false;
        weak->submitting_=false;
        if (!weak->pending_.isEmpty() || weak->result_.callbackCount) weak->queueFinish();
        return invoked;
    }
    void cancel(QString reason=QStringLiteral("canceled")) {
        if(!gui())return;
        const reader_request_scope::Scope scope(binding_);
        if(!scope.entered())return;
        if (!gui() || terminal_) return;
        started_=true; result_.canceled=true;
        stop(reason.isEmpty()?QStringLiteral("canceled"):reason.left(128));
    }
    Result snapshot() const { return result_; }
signals:
    void finished();
private:
    bool gui() const {
        auto *app=qobject_cast<QGuiApplication*>(QCoreApplication::instance());
        return app && !QCoreApplication::closingDown() && thread()==app->thread() && QThread::currentThread()==app->thread();
    }
    static bool nativeClass(QObject *object,const char *name) {
        if (!object) return false;
        int count=0;
        for (auto *m=object->metaObject();m && count++<32;m=m->superClass()) if (qstrcmp(m->className(),name)==0) return true;
        return false;
    }
    static QMetaMethod exact(QObject *object,const char *signature,QMetaType result) {
        if (!object || object->metaObject()->methodCount()>512) return {};
        const int index=object->metaObject()->indexOfMethod(signature);
        if(index<0) return {};
        const auto method=object->metaObject()->method(index);
        return method.returnMetaType()==result && method.access()==QMetaMethod::Public && method.methodType()!=QMetaMethod::Signal ? method:QMetaMethod();
    }
    bool expired() const { return elapsed_.isValid() && elapsed_.elapsed()>=budget_; }
    void callback() {
        if(!gui())return;
        const reader_request_scope::Scope scope(binding_);
        if(!scope.active())return;
        if (!gui() || terminal_ || !started_ || !pending_.isEmpty()) return;
        if (expired()) { cancel(QStringLiteral("deadline")); return; }
        if (result_.callbackCount<2) ++result_.callbackCount;
        result_.callbackObserved=true; result_.duplicate=result_.callbackCount>1;
        if (!submitting_) queueFinish();
    }
    void stop(QString reason) {
        if(!gui())return;
        const reader_request_scope::Scope scope(binding_);
        if(!scope.entered())return;
        if (terminal_) return;
        if(pending_.isEmpty()) pending_=reason.left(128);
        if(bridge_) bridge_->disarm();
        timer_.stop();
        if(!submitting_) queueFinish();
    }
    void queueFinish() {
        if(!gui())return;
        const reader_request_scope::Scope scope(binding_);
        if(!scope.active())return;
        if(terminal_ || submitting_ || queued_) return;
        queued_=true;
        const QPointer<Witness> weak(this);
        QMetaObject::invokeMethod(this,[weak]{if(weak)weak->freeze();},Qt::QueuedConnection);
    }
    void freeze() {
        if(!gui())return;
        const reader_request_scope::Scope scope(binding_);
        if(!scope.active())return;
        queued_=false;
        if(terminal_ || submitting_) return;
        if(pending_.isEmpty() && expired()) pending_=QStringLiteral("deadline");
        if(pending_.isEmpty() && !engine_) pending_=QStringLiteral("engine-destroyed");
        if(pending_.isEmpty() && result_.duplicate) pending_=QStringLiteral("callback-repeated");
        if(pending_.isEmpty() && !result_.callbackObserved) pending_=QStringLiteral("callback-unobserved");
        result_.unknown=result_.attempted && (!pending_.isEmpty() || !result_.returnKnown || !result_.accepted);
        result_.refusal=pending_; result_.finished=true; terminal_=true;
        timer_.stop(); if(bridge_)bridge_->disarm();
        emit finished();
    }
    const reader_request_scope::Binding binding_;
    QPointer<QQmlEngine> engine_;
    QPointer<qml_access::CreationBridge> bridge_;
    QJSValue callback_;
    QTimer timer_;
    QElapsedTimer elapsed_;
    int budget_=0;
    bool prepared_=false,started_=false,submitting_=false,terminal_=false,queued_=false;
    QString pending_;
    Result result_;
};
} // namespace reader_joined_creation
