#pragma once
// SDK adaptation of the measured public-metaobject post-order reader. Historical
// facts only; no revision/input epoch, mutation exclusion or writing authority.
#include "reader_insertion_post_order.h"
#include "../resident/wire.h"

namespace remarkable::rm2_3_28_0_172 {
struct OrderObservation {
    bool observed=false;
    resident::Facts facts;
    QString refusal;
};
// Called synchronously inside observeCreationJoin's ValidatedSourceAction. The
// borrowed wrapper, QVariant and Sample guard must never escape that callback.
inline OrderObservation observeOrder(QQmlEngine *engine,QObject *document,const QVariant &actualId,
                                    const reader_selected::Observation &joined,
                                    const std::function<bool()> &guard) {
    OrderObservation result;
    reader_insertion_post_order::detail::Gate gate;
    const auto fail=[&](const char *why){result.observed=false;result.facts={};result.refusal=QString::fromLatin1(why);return result;};
    if(!gate.entered)return fail("order-reentered");
    const QPointer<QGuiApplication> app(qobject_cast<QGuiApplication*>(QCoreApplication::instance()));
    if(!app||QCoreApplication::closingDown()||QThread::currentThread()!=app->thread())return fail("order-gui");
    const QPointer<QQmlEngine> e(engine);const QPointer<QObject> d(document);
    try {
        const auto selection=joined;const auto id=actualId;const auto permission=guard;
        const auto alive=[&]{
            if(reader_insertion_post_order::detail::reentered||!app||!e||!d||QCoreApplication::closingDown()||
               e->thread()!=app->thread()||d->thread()!=app->thread())return false;
            auto *associated=qmlEngine(d);return !associated||associated==e;
        };
        const auto live=[&]{return alive()&&permission&&permission()&&alive();};
        // Retain the handoff's bounded 16-original-plus-one-page observation
        // envelope. Widening this is a separate native qualification decision.
        if(!selection.observed||!selection.creationJoinObserved||!selection.mapCoherent||!selection.ready||
           selection.loading||selection.pageError||selection.documentError||selection.awaitingTiles||
           selection.pendingTiles||selection.working||selection.pageCount<1||selection.pageCount>17||
           selection.pageIndex<0||selection.pageIndex>=selection.pageCount||
           !reader_joined_creation::canonicalId(selection.documentId)||!reader_joined_creation::canonicalId(selection.pageId))return fail("order-selection");
        if(!id.isValid()||id.metaType().flags().testFlag(QMetaType::IsPointer)||!id.metaType().name()||
           QByteArray(id.metaType().name())!="entry::Id")return fail("order-id-type");
        if(!live())return fail("order-guard");
        const auto *meta=d->metaObject();if(!live())return fail("order-guard");
        if(!meta||!reader_insertion_post_order::detail::nativeClass(meta)||meta->methodCount()>512||meta->propertyCount()>512)return fail("order-meta");
        const auto idProperty=meta->property(meta->indexOfProperty("id"));
        const auto countProperty=meta->property(meta->indexOfProperty("pageCount"));
        const auto forward=reader_insertion_post_order::detail::method(meta,"idForPage(int)",QMetaType::fromType<QString>(),QMetaType::fromType<int>());
        const auto inverse=reader_insertion_post_order::detail::method(meta,"pageForId(QString)",QMetaType::fromType<int>(),QMetaType::fromType<QString>());
        if(!idProperty.isValid()||!idProperty.isReadable()||!idProperty.isConstant()||idProperty.metaType()!=id.metaType()||
           !countProperty.isValid()||!countProperty.isReadable()||countProperty.metaType()!=QMetaType::fromType<int>()||
           !forward.isValid()||!inverse.isValid())return fail("order-contract");
        const auto sameId=[&]{
            if(!live())return false;
            const auto value=idProperty.read(d);if(!live())return false;
            const bool equal=value.metaType()==id.metaType()&&value==id;return live()&&equal;
        };
        const auto sameCount=[&]{
            if(!live())return false;
            const auto value=countProperty.read(d);if(!live())return false;
            return value.metaType()==QMetaType::fromType<int>()&&value.toInt()==selection.pageCount;
        };
        if(!sameId()||!sameCount())return fail("order-identity");
        const QPointer<QQmlContext> context=e->rootContext();if(!live()||!context)return fail("order-guard");
        {
            QQmlExpression expression(context,d,QStringLiteral("String(id)"));if(!live())return fail("order-guard");
            const auto value=expression.evaluate();if(!live())return fail("order-guard");
            if(expression.hasError()||value.metaType()!=QMetaType::fromType<QString>()||value.toString()!=selection.documentId)return fail("order-document");
        }
        QStringList ids;
        for(int pass=0;pass<2;++pass) {
            for(int index=0;index<selection.pageCount;++index) {
                if(!live())return fail("order-guard");
                QString page;
                const bool called=forward.invoke(d,Qt::DirectConnection,Q_RETURN_ARG(QString,page),Q_ARG(int,index));
                if(!live())return fail("order-guard");
                if(!called||!reader_joined_creation::canonicalId(page))return fail("order-page");
                if(pass==0){if(ids.contains(page))return fail("order-duplicate");ids.append(page);}
                else if(page!=ids[index])return fail("order-changed");
                if(!live())return fail("order-guard");
                int back=-1;
                const bool returned=inverse.invoke(d,Qt::DirectConnection,Q_RETURN_ARG(int,back),Q_ARG(QString,page));
                if(!live())return fail("order-guard");
                if(!returned||back!=index)return fail("order-inverse");
            }
            if(!sameCount()||!sameId())return fail("order-changed");
        }
        if(ids[selection.pageIndex]!=selection.pageId||!live())return fail("order-selected-page");
        result.facts.document=QUuid(selection.documentId).toRfc4122();result.facts.page=QUuid(selection.pageId).toRfc4122();
        for(const auto &page:ids)result.facts.order.push_back(QUuid(page).toRfc4122());
        if(!resident::validFacts(result.facts))return fail("order-wire-facts");
        result.observed=true;return result;
    }catch(...){return fail("order-exception");}
}
}
