#pragma once
// Original one-shot GUI research helper. An invocation is not target selection.
#include "reader_insertion_post_order.h"
#include <atomic>
#include <memory>

namespace reader_insertion_target_acquisition {
using Guard=std::function<bool()>;
struct MethodInfo {
    bool admitted=false;
    int index=-1,returnTypeId=0,access=-1,kind=-1,parameterCount=-1,matches=0;
    QString signature,declaringClass,returnName,refusal;
};
struct Claim { std::atomic<bool> consumed{false}; };
struct Result {
    bool admitted=false,attempted=false,dispatched=false,unknown=false,alreadySelected=false;
    QString refusal;
    static constexpr bool nativeAuthority=false,renderAuthority=false,durableAuthority=false;
};
namespace detail {
inline thread_local bool running=false,reentered=false;
struct Gate {
    bool entered=false;
    Gate(){if(running)reentered=true;else{running=true;reentered=false;entered=true;}}
    ~Gate(){if(entered)running=false;}
};
inline MethodInfo metadata(const QMetaObject *meta,QMetaMethod *selected=nullptr) {
    MethodInfo out;
    const auto refuse=[&](const char *why){out.refusal=QString::fromLatin1(why);return out;};
    if(!meta || meta->methodCount()<0 || meta->methodCount()>512)return refuse("method-bound");
    QMetaMethod candidate;
    for(int index=0;index<meta->methodCount();++index) {
        const auto method=meta->method(index);
        if(method.name()=="moveForward"){++out.matches;candidate=method;out.index=index;}
    }
    if(out.matches!=1)return refuse(out.matches ? "method-ambiguous":"method-missing");
    const QByteArray signature=candidate.methodSignature();
    const auto *owner=candidate.enclosingMetaObject();
    const QByteArray declaring=owner ? QByteArray(owner->className()):QByteArray();
    const QMetaType returns=candidate.returnMetaType();
    const QByteArray name=returns.name() ? QByteArray(returns.name()):QByteArray();
    if(signature.isEmpty() || signature.size()>64 || declaring.isEmpty() || declaring.size()>64 ||
       name.isEmpty() || name.size()>32)return refuse("method-text-bound");
    out.signature=QString::fromUtf8(signature);out.declaringClass=QString::fromUtf8(declaring);
    out.returnName=QString::fromUtf8(name);out.returnTypeId=returns.id();
    if(out.signature.toUtf8()!=signature || out.declaringClass.toUtf8()!=declaring || out.returnName.toUtf8()!=name) {
        out.signature.clear();out.declaringClass.clear();out.returnName.clear();
        return refuse("method-text-encoding");
    }
    out.access=int(candidate.access());out.kind=int(candidate.methodType());out.parameterCount=candidate.parameterCount();
    if(candidate.access()!=QMetaMethod::Public ||
       (candidate.methodType()!=QMetaMethod::Method && candidate.methodType()!=QMetaMethod::Slot) ||
       candidate.parameterCount()!=0 || signature!="moveForward()" ||
       (returns!=QMetaType::fromType<void>() && returns!=QMetaType::fromType<QVariant>()))return refuse("method-contract");
    out.admitted=true;
    if(selected)*selected=candidate;
    return out;
}
inline bool same(const MethodInfo &a,const MethodInfo &b) {
    return a.admitted && b.admitted && a.refusal.isEmpty() && b.refusal.isEmpty() &&
        a.index==b.index && a.returnTypeId==b.returnTypeId && a.access==b.access && a.kind==b.kind &&
        a.parameterCount==b.parameterCount && a.matches==b.matches && a.signature==b.signature &&
        a.declaringClass==b.declaringClass && a.returnName==b.returnName;
}
}

// No QObject is created. Null QML associations are permitted for native/owned
// objects; a nonnull association must belong to this exact engine.
inline MethodInfo inspect(QQmlEngine *engine,QObject *reader,const Guard &guard) {
    MethodInfo result;
    detail::Gate gate;
    if(!gate.entered){result.refusal=QStringLiteral("reentered");return result;}
    const QPointer<QGuiApplication> app(qobject_cast<QGuiApplication*>(QCoreApplication::instance()));
    const QPointer<QQmlEngine> e(engine);
    const QPointer<QObject> r(reader);
    const auto refuse=[&](const char *why){result.admitted=false;result.refusal=QString::fromLatin1(why);return result;};
    if(!app || QCoreApplication::closingDown() || QThread::currentThread()!=app->thread())return refuse("gui");
    try {
        const Guard permission=guard;
        const auto alive=[&] {
            if(detail::reentered || !app || !e || !r || QCoreApplication::closingDown() ||
               QThread::currentThread()!=app->thread() || e->thread()!=app->thread() || r->thread()!=app->thread())return false;
            const auto *associated=qmlEngine(r);
            return !associated || associated==e;
        };
        const auto live=[&]{if(!alive() || !permission)return false;const bool ok=permission();return ok && alive();};
        if(!live())return refuse("guard");
        const QMetaObject *meta=r->metaObject();
        if(!live())return refuse("guard");
        if(r->metaObject()!=meta || !alive())return refuse("method-changed");
        result=detail::metadata(meta);
        if(!live())return refuse("guard");
        if(r->metaObject()!=meta || !alive())return refuse("method-changed");
        return result;
    } catch(...){return refuse("exception");}
}

// The caller supplies the fresh joined, same-object post-order and original
// epoch/deadline guard. There is deliberately no old-epoch test after the effect.
inline Result dispatch(QQmlEngine *engine,QObject *reader,QObject *document,
                       const reader_joined_creation::Request &request,
                       const reader_insertion_post_order::Result &postOrder,
                       const MethodInfo &admittedMethod,const std::shared_ptr<Claim> &claim,const Guard &guard) {
    Result result;
    detail::Gate gate;
    if(!gate.entered){result.refusal=QStringLiteral("reentered");return result;}
    const QPointer<QGuiApplication> app(qobject_cast<QGuiApplication*>(QCoreApplication::instance()));
    const QPointer<QQmlEngine> e(engine);
    const QPointer<QObject> r(reader),d(document);
    const auto refuse=[&](const char *why){result.refusal=QString::fromLatin1(why);return result;};
    if(!app || QCoreApplication::closingDown() || QThread::currentThread()!=app->thread())return refuse("gui");
    try {
        // Freeze every borrowed value before any permission callback can remove
        // the facade that owns the original references.
        const auto frozen=request;
        const auto post=postOrder;
        const auto admitted=admittedMethod;
        const auto once=claim;
        const Guard permission=guard;
        const auto alive=[&] {
            if(detail::reentered || !app || !e || !r || !d || QCoreApplication::closingDown() ||
               QThread::currentThread()!=app->thread() || e->thread()!=app->thread() ||
               r->thread()!=app->thread() || d->thread()!=app->thread())return false;
            const auto *readerEngine=qmlEngine(r),*documentEngine=qmlEngine(d);
            return (!readerEngine || readerEngine==e) && (!documentEngine || documentEngine==e);
        };
        const auto live=[&]{if(!alive() || !permission)return false;const bool ok=permission();return ok && alive();};
        if(!frozen.valid() || frozen.expectedPages.size()>16)return refuse("request");
        if(!once || once->consumed.load(std::memory_order_acquire))return refuse("claim-consumed");
        QStringList expected=frozen.expectedPages;
        expected.insert(frozen.sourceIndex+1,frozen.targetPageId);
        const bool sourceSelected=post.currentPageId==frozen.sourcePageId && post.currentPageIndex==frozen.sourceIndex;
        const bool targetSelected=post.currentPageId==frozen.targetPageId && post.currentPageIndex==frozen.sourceIndex+1;
        if(!post.observed || !post.refusal.isEmpty() || post.pageCount!=expected.size() || post.ids!=expected ||
           post.calls!=unsigned(2*expected.size()+5) ||
           (!sourceSelected && !targetSelected) || post.targetSelected!=targetSelected)return refuse("post-order");
        if(!live())return refuse("guard");
        const QMetaObject *meta=r->metaObject();
        if(!live())return refuse("guard");
        if(r->metaObject()!=meta || !alive())return refuse("method-changed");
        QMetaMethod method;
        const auto current=detail::metadata(meta,&method);
        if(!live())return refuse("guard");
        if(r->metaObject()!=meta || !alive())return refuse("method-changed");
        if(!detail::same(current,admitted))return refuse("method-changed");
        if(targetSelected){result.admitted=true;result.alreadySelected=true;return result;}
        // In the complete admitted post-order source+1 is the target, including
        // when source used to be the last (or only) page before insertion.
        if(frozen.sourceIndex+1>=post.pageCount || post.ids[frozen.sourceIndex+1]!=frozen.targetPageId)return refuse("adjacency");
        if(!live())return refuse("guard");
        if(r->metaObject()!=meta || !alive() || !detail::same(detail::metadata(meta),current))return refuse("method-changed");
        QObject *const target=r.data();
        bool expectedClaim=false;
        if(!once->consumed.compare_exchange_strong(expectedClaim,true,std::memory_order_acq_rel))return refuse("claim-consumed");
        result.admitted=true;result.attempted=true;
        // All objects and the old guard are off limits after this call starts.
        // QVariant return storage is owned and intentionally carries no claim
        // about the selected page, rendering or durable state.
        try {
            if(current.returnTypeId==QMetaType::QVariant) {
                QVariant ignored;
                result.dispatched=method.invoke(target,Qt::DirectConnection,Q_RETURN_ARG(QVariant,ignored));
            } else result.dispatched=method.invoke(target,Qt::DirectConnection);
            if(!result.dispatched){result.unknown=true;result.refusal=QStringLiteral("invoke-failed");}
        } catch(...){result.unknown=true;result.refusal=QStringLiteral("invoke-exception");}
        return result;
    } catch(...){return refuse("exception");}
}
} // namespace reader_insertion_target_acquisition
