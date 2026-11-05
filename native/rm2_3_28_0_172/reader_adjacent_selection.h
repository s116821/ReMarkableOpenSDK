#pragma once
#include "order_observation.h"
#include "reader_insertion_target_acquisition.h"

namespace remarkable::rm2_3_28_0_172 {
// Synchronous inner step only. The executor must supply a fresh joined order,
// hold the process effect gate and rejoin after an attempted transition. Neither
// a successful invoke nor this historical result is target-selection authority.
inline reader_insertion_target_acquisition::Result dispatchAdjacent(
        QQmlEngine *engine,QObject *reader,QObject *document,
        const resident::Facts &expected,const resident::Id &targetId,
        const OrderObservation &observed,
        const reader_insertion_target_acquisition::MethodInfo &admittedMethod,
        const std::shared_ptr<reader_insertion_target_acquisition::Claim> &claim,
        const std::function<bool()> &guard) {
    namespace nav=reader_insertion_target_acquisition;
    nav::Result result;nav::detail::Gate gate;
    const auto refuse=[&](const char *reason){result.refusal=QString::fromLatin1(reason);return result;};
    if(!gate.entered)return refuse("reentered");
    const QPointer<QGuiApplication> app(qobject_cast<QGuiApplication*>(QCoreApplication::instance()));
    if(!app||QCoreApplication::closingDown()||QThread::currentThread()!=app->thread())return refuse("gui");
    const QPointer<QQmlEngine> e(engine);const QPointer<QObject> r(reader),d(document);
    try {
        // Freeze caller-owned inputs before any permission callback can destroy
        // their owner. The Sample permission remains synchronous and borrowed.
        const auto source=expected;const auto target=targetId;const auto order=observed;
        const auto admitted=admittedMethod;const auto once=claim;const auto permission=guard;
        const auto alive=[&]{
            if(nav::detail::reentered||!app||!e||!r||!d||QCoreApplication::closingDown()||
               QThread::currentThread()!=app->thread()||e->thread()!=app->thread()||
               r->thread()!=app->thread()||d->thread()!=app->thread())return false;
            const auto *re=qmlEngine(r),*de=qmlEngine(d);
            return (!re||re==e)&&(!de||de==e);
        };
        const auto live=[&]{return alive()&&permission&&permission()&&alive();};
        if(!resident::validSelection(source,target)||source.order.size()>17||!order.observed||!order.refusal.isEmpty()||
           !resident::validFacts(order.facts)||order.facts.document!=source.document||
           order.facts.page!=source.page||order.facts.order!=source.order)return refuse("fresh-order");
        if(!once||once->consumed.load(std::memory_order_acquire))return refuse("claim-consumed");
        if(!live())return refuse("guard");
        if(target==source.page){result.admitted=true;result.alreadySelected=true;return result;}
        const QMetaObject *meta=r->metaObject();
        if(!live())return refuse("guard");
        if(r->metaObject()!=meta||!alive())return refuse("method-changed");
        QMetaMethod method;const auto current=nav::detail::metadata(meta,&method);
        if(!live())return refuse("guard");
        if(r->metaObject()!=meta||!alive()||!nav::detail::same(current,admitted))return refuse("method-changed");
        if(!live())return refuse("guard");
        if(r->metaObject()!=meta||!alive()||!nav::detail::same(nav::detail::metadata(meta),current))return refuse("method-changed");
        QObject *const receiver=r.data();bool available=false;
        if(!once->consumed.compare_exchange_strong(available,true,std::memory_order_acq_rel))return refuse("claim-consumed");
        result.admitted=true;result.attempted=true;
        // Old objects and the Sample guard are never inspected after this point.
        try {
            if(current.returnTypeId==QMetaType::QVariant) {
                QVariant ignored;
                result.dispatched=method.invoke(receiver,Qt::DirectConnection,Q_RETURN_ARG(QVariant,ignored));
            }else result.dispatched=method.invoke(receiver,Qt::DirectConnection);
            if(!result.dispatched){result.unknown=true;result.refusal=QStringLiteral("invoke-failed");}
        }catch(...){result.unknown=true;result.refusal=QStringLiteral("invoke-exception");}
        return result;
    }catch(...){return refuse("exception");}
}
}
