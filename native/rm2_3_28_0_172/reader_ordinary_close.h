#pragma once
// Research-only terminal QML action; successful invoke is not save completion.
#include "reader_selected_view_probe.h"
#include <atomic>
#include <memory>

namespace reader_ordinary_close {
using Guard=std::function<bool()>;
struct Expected {
    QString documentId,pageId;int pageIndex=-1,pageCount=-1;
    bool valid() const {
        static const QRegularExpression id(QStringLiteral("^[0-9a-f]{8}(-[0-9a-f]{4}){3}-[0-9a-f]{12}$"));
        const auto canonical=[&](const QString &s){return s.size()==36&&id.match(s).hasMatch()&&s!=QStringLiteral("00000000-0000-0000-0000-000000000000");};
        return canonical(documentId)&&canonical(pageId)&&pageCount>0&&pageCount<=16&&pageIndex>=0&&pageIndex<pageCount;
    }
};
struct Claim {std::atomic<bool> consumed{false};};
struct Result {
    bool claimed=false,matched=false,attempted=false,dispatched=false,effectRequiresReconciliation=false;
    reader_selected::Observation before;QString refusal;
    static constexpr bool nativeAuthority=false,durableAuthority=false;
};
#ifdef READER_ORDINARY_CLOSE_TESTING
struct TestBoundary {
    std::function<reader_selected::Observation(QQuickWindow *,const Guard &,int,
        const reader_selected::ValidatedSourceAction &)> sample;
};
#endif
inline bool matches(const reader_selected::Observation &s,const Expected &e) {
    return s.observed&&s.mapCoherent&&s.creationJoinObserved&&s.ready&&s.refusal.isEmpty()&&
        s.documentId==e.documentId&&s.pageId==e.pageId&&s.pageIndex==e.pageIndex&&s.pageCount==e.pageCount&&s.status>=0&&s.pageStatus>=0&&
        !s.loading&&!s.pageError&&!s.documentError&&!s.awaitingTiles&&!s.pendingTiles&&!s.working&&
        !s.nativeAuthority&&!s.renderAuthority&&!s.durableAuthority;
}
inline Result run(QQuickWindow *inputWindow,const Expected &inputExpected,
                  const std::shared_ptr<Claim> &inputClaim,const Guard &inputPermit,int budgetMs=1000
#ifdef READER_ORDINARY_CLOSE_TESTING
                  ,const TestBoundary &inputTest={}
#endif
) {
    Result result;
    // Pin the supplied claim before any copied user callable or source lookup.
    const auto claim=inputClaim;
    if(!claim||claim->consumed.exchange(true)){result.refusal=QStringLiteral("claim-spent");return result;}
    result.claimed=true;
    const auto refuse=[&](const char *why){result.refusal=QString::fromLatin1(why);return result;};
    const QPointer<QGuiApplication> app(qobject_cast<QGuiApplication *>(QCoreApplication::instance()));
    const QPointer<QQuickWindow> window(inputWindow);
    if(!app||QThread::currentThread()!=app->thread()||!window||window->thread()!=app->thread()||QCoreApplication::closingDown())return refuse("application-thread");
    if(budgetMs<1||budgetMs>reader_selected::MaxObservationMs)return refuse("budget-invalid");
    QElapsedTimer elapsed;elapsed.start();
    try {
        const Expected expected=inputExpected;
        const Guard permit=inputPermit;
        if(!expected.valid()||!permit)return refuse("configuration");
        const Guard allowed=[&]{
            if(!app||!window||QCoreApplication::closingDown()||elapsed.elapsed()>=budgetMs)return false;
            if(!permit())return false;
            return app&&window&&!QCoreApplication::closingDown()&&elapsed.elapsed()<budgetMs;
        };
        if(!allowed())return refuse("permission");
        unsigned callbacks=0;
        const reader_selected::ValidatedSourceAction action=[&](QObject *reader,QObject *,QObject *,const QVariant &,
                const reader_selected::Observation &observed,const Guard &sourceGuard){
            if(++callbacks!=1||result.attempted){result.refusal=QStringLiteral("source-callback-repeat");return;}
            const QPointer<QObject> pin(reader);
            if(!app||!window||!pin||pin->thread()!=app->thread()||!matches(observed,expected)||!sourceGuard||!sourceGuard()||!pin||!allowed()) {
                result.refusal=QStringLiteral("source-mismatch");return;
            }
            const QMetaObject *meta=pin->metaObject();
            const int index=meta->indexOfMethod("close()");
            const QMetaMethod method=index>=0?meta->method(index):QMetaMethod{};
            if(!method.isValid()||method.name()!="close"||method.parameterCount()!=0||
               method.access()!=QMetaMethod::Public||method.methodType()!=QMetaMethod::Slot||
               method.returnMetaType()!=QMetaType::fromType<QVariant>()||method.enclosingMetaObject()!=meta) {
                result.refusal=QStringLiteral("close-method");return;
            }
            if(!sourceGuard()||!pin||!allowed()) {result.refusal=QStringLiteral("source-changed");return;}
            // allowed() can call user code. Revalidate the original source once
            // more after it; only its production guard is used at this boundary.
            if(!sourceGuard()||!pin||elapsed.elapsed()>=budgetMs||pin->metaObject()!=meta) {result.refusal=QStringLiteral("source-changed");return;}
            result.before=observed;result.matched=true;
            result.attempted=true;result.effectRequiresReconciliation=true;
            QVariant returned;
            result.dispatched=method.invoke(pin,Qt::DirectConnection,Q_RETURN_ARG(QVariant,returned));
            // Reader/guard may be invalid now. No post-action pointer reads.
            if(!result.dispatched)result.refusal=QStringLiteral("invoke-refused");
            else if(elapsed.elapsed()>=budgetMs)result.refusal=QStringLiteral("deadline-after-dispatch");
        };
        reader_selected::Observation observed;
#ifdef READER_ORDINARY_CLOSE_TESTING
        if(inputTest.sample)observed=inputTest.sample(window,allowed,budgetMs,action);
        else
#endif
        observed=reader_selected::observeCreationJoin(window,allowed,budgetMs,{},action);
        if(!result.attempted&&result.refusal.isEmpty())
            result.refusal=observed.refusal.isEmpty()?QStringLiteral("source-action-unreached"):observed.refusal;
        return result;
    }catch(...){return refuse("exception");}
}
} // namespace reader_ordinary_close
