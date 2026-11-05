#pragma once
// Original synchronous GUI post-order observation. No insertion or persistence.
#include "reader_joined_creation_witness.h"
#include <QQmlContext>
#include <QMetaProperty>

namespace reader_insertion_post_order {
struct Result {
    bool observed=false,targetSelected=false;
    int pageCount=-1,currentPageIndex=-1;
    QString currentPageId,refusal;
    QStringList ids;
    unsigned calls=0; // Explicit property reads, evaluations and method invokes.
    static constexpr bool nativeAuthority=false,renderAuthority=false,durableAuthority=false;
};
namespace detail {
inline thread_local bool reading=false,reentered=false;
struct Gate {
    bool entered=false;
    Gate(){if(reading)reentered=true;else{reading=true;reentered=false;entered=true;}}
    ~Gate(){if(entered)reading=false;}
};
inline bool nativeClass(const QMetaObject *meta) {
    for(unsigned depth=0;meta && depth<64;++depth,meta=meta->superClass())
        if(qstrcmp(meta->className(),"QmlDocumentWrapper")==0)return true;
    return false;
}
inline QMetaMethod method(const QMetaObject *meta,const char *signature,QMetaType returns,QMetaType argument) {
    const int index=meta->indexOfMethod(signature);
    if(index<0)return {};
    const auto m=meta->method(index);
    return m.access()==QMetaMethod::Public && m.methodType()!=QMetaMethod::Signal &&
        m.returnMetaType()==returns && m.parameterCount()==1 && m.parameterMetaType(0)==argument &&
        qstrcmp(m.enclosingMetaObject()->className(),"QmlDocumentWrapper")==0 ? m:QMetaMethod();
}
}

// The caller binds document to the fresh joined Sample and owns the original
// deadline/epoch guard. A null qmlEngine(document) is allowed for native wrappers;
// a nonnull association must be the admitted engine. No borrowed input survives.
inline Result observe(QQmlEngine *engine,QObject *document,const QVariant &actualId,
                      const reader_joined_creation::Request &request,
                      const reader_selected::Observation &joined,
                      const std::function<bool()> &guard) {
    Result result;
    detail::Gate gate;
    if(!gate.entered){result.refusal=QStringLiteral("reentered");return result;}
    const QPointer<QGuiApplication> app(qobject_cast<QGuiApplication*>(QCoreApplication::instance()));
    const QPointer<QQmlEngine> e(engine);
    const QPointer<QObject> d(document);
    const auto refuse=[&](const char *why){result.refusal=QString::fromLatin1(why);return result;};
    if(!app || QCoreApplication::closingDown() || QThread::currentThread()!=app->thread())return refuse("gui");
    try {
        // These values remain owned even when a getter/guard deletes its caller.
        const reader_joined_creation::Request frozen=request;
        const reader_selected::Observation selection=joined;
        const QVariant id=actualId;
        const std::function<bool()> permission=guard;
        const auto alive=[&] {
            if(detail::reentered || !app || !e || !d || QCoreApplication::closingDown() ||
               QThread::currentThread()!=app->thread() || e->thread()!=app->thread() || d->thread()!=app->thread())return false;
            auto *associated=qmlEngine(d);
            return !associated || associated==e;
        };
        const auto live=[&] {
            if(!alive() || !permission)return false;
            const bool allowed=permission();
            return allowed && alive();
        };
        if(!frozen.valid() || frozen.expectedPages.size()>16)return refuse("request");
        QStringList expected=frozen.expectedPages;
        expected.insert(frozen.sourceIndex+1,frozen.targetPageId);
        const bool sourceSelected=selection.pageId==frozen.sourcePageId && selection.pageIndex==frozen.sourceIndex;
        const bool targetSelected=selection.pageId==frozen.targetPageId && selection.pageIndex==frozen.sourceIndex+1;
        if(!selection.observed || !selection.creationJoinObserved || !selection.mapCoherent || !selection.ready ||
           selection.loading || selection.pageError || selection.documentError || selection.awaitingTiles ||
           selection.pendingTiles || selection.working || selection.documentId!=frozen.documentId ||
           selection.pageCount!=expected.size() || (!sourceSelected && !targetSelected))return refuse("joined-selection");
        result.targetSelected=targetSelected;result.currentPageId=selection.pageId;result.currentPageIndex=selection.pageIndex;
        if(!id.isValid() || id.metaType().flags().testFlag(QMetaType::IsPointer) ||
           !id.metaType().name() || QByteArray(id.metaType().name())!="entry::Id")return refuse("id-type");
        if(!live())return refuse("guard");
        const QMetaObject *meta=d->metaObject();
        if(!live())return refuse("guard");
        if(!meta || !detail::nativeClass(meta) || meta->methodCount()>512 || meta->propertyCount()>512)return refuse("document-meta");
        const auto idProperty=meta->property(meta->indexOfProperty("id"));
        const auto countProperty=meta->property(meta->indexOfProperty("pageCount"));
        const auto forward=detail::method(meta,"idForPage(int)",QMetaType::fromType<QString>(),QMetaType::fromType<int>());
        const auto inverse=detail::method(meta,"pageForId(QString)",QMetaType::fromType<int>(),QMetaType::fromType<QString>());
        if(!idProperty.isValid() || !idProperty.isReadable() || !idProperty.isConstant() || idProperty.metaType()!=id.metaType() ||
           !countProperty.isValid() || !countProperty.isReadable() || countProperty.metaType()!=QMetaType::fromType<int>() ||
           !forward.isValid() || !inverse.isValid())return refuse("document-contract");
        if(!live())return refuse("guard");
        ++result.calls;const QVariant currentId=idProperty.read(d);
        if(!live())return refuse("guard");
        const bool sameId=currentId.metaType()==id.metaType() && currentId==id;
        if(!live())return refuse("guard");
        if(!sameId)return refuse("document-id");
        const QPointer<QQmlContext> context=e->rootContext();
        if(!live() || !context)return refuse("guard");
        {
            QQmlExpression text(context,d,QStringLiteral("String(id)"));
            if(!live())return refuse("guard");
            ++result.calls;const QVariant value=text.evaluate();
            if(!live())return refuse("guard");
            if(text.hasError() || value.metaType()!=QMetaType::fromType<QString>() || value.toString()!=frozen.documentId)
                return refuse("document-id-text");
        }
        if(!live())return refuse("guard");
        ++result.calls;const QVariant count=countProperty.read(d);
        if(!live())return refuse("guard");
        if(count.metaType()!=QMetaType::fromType<int>())return refuse("page-count-type");
        result.pageCount=count.toInt();
        if(result.pageCount!=expected.size())return refuse("page-count");
        for(int index=0;index<expected.size();++index) {
            if(!live())return refuse("guard");
            QString page;
            ++result.calls;const bool called=forward.invoke(d,Qt::DirectConnection,Q_RETURN_ARG(QString,page),Q_ARG(int,index));
            if(!live())return refuse("guard");
            if(!called)return refuse("id-invocation");
            if(!reader_joined_creation::canonicalId(page))return refuse("page-id");
            result.ids.append(page);
            if(page!=expected[index])return refuse("page-order");
            if(!live())return refuse("guard");
            int roundtrip=-1;
            ++result.calls;const bool returned=inverse.invoke(d,Qt::DirectConnection,Q_RETURN_ARG(int,roundtrip),Q_ARG(QString,page));
            if(!live())return refuse("guard");
            if(!returned || roundtrip!=index)return refuse("page-roundtrip");
        }
        if(!live())return refuse("guard");
        ++result.calls;const QVariant finalCount=countProperty.read(d);
        if(!live())return refuse("guard");
        if(finalCount.metaType()!=QMetaType::fromType<int>() || finalCount.toInt()!=result.pageCount)return refuse("page-count-changed");
        if(!live())return refuse("guard");
        ++result.calls;const QVariant finalId=idProperty.read(d);
        if(!live())return refuse("guard");
        const bool unchanged=finalId.metaType()==id.metaType() && finalId==id;
        if(!live())return refuse("guard");
        if(!unchanged)return refuse("document-id-changed");
        result.observed=true;
        return result;
    } catch(...) {return refuse("exception");}
}
} // namespace reader_insertion_post_order
