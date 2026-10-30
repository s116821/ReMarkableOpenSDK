#pragma once
// One terminal handoff from fresh native source identity. No evdev IO here.
#include "reader_ordinary_close.h"
#include <ctime>
#include <limits>

namespace reader_fixed_pen_handoff {
using Guard=reader_ordinary_close::Guard;
using Expected=reader_ordinary_close::Expected;
struct Config {Expected target;QByteArray nonce,pid,start;quint64 setupEnd=0;};
struct Claim {std::atomic<bool> consumed{false};};
struct Result {
    bool claimed=false,matched=false,attempted=false,published=false,reconciliationRequired=false;
    quint64 requestMs=0;QByteArray originalRequest;
    reader_selected::Observation before;QString refusal;
    static constexpr bool nativeAuthority=false,atomicAuthority=false;
};
inline quint64 bootMs(){struct timespec t{};if(::clock_gettime(CLOCK_BOOTTIME,&t)||t.tv_sec<0||t.tv_nsec<0||t.tv_nsec>=1000000000)return 0;
    const auto seconds=quint64(t.tv_sec);return seconds>(std::numeric_limits<quint64>::max()-999)/1000?0:seconds*1000+quint64(t.tv_nsec)/1000000;}
#ifdef READER_FIXED_PEN_HANDOFF_TESTING
struct TestBoundary {
    std::function<reader_selected::Observation(QQuickWindow *,const Guard &,int,const reader_selected::ValidatedSourceAction &)> sample;
    std::function<quint64()> clock;
};
#endif
inline Result run(QQuickWindow *inputWindow,const Config &inputConfig,const std::shared_ptr<Claim> &inputClaim,
                  const Guard &inputPermit,const std::function<bool(const QByteArray &)> &inputPublish,int budgetMs=1000
#ifdef READER_FIXED_PEN_HANDOFF_TESTING
                  ,const TestBoundary &inputTest={}
#endif
){
    Result r;const auto claim=inputClaim;
    if(!claim||claim->consumed.exchange(true)){r.refusal=QStringLiteral("claim-spent");return r;}r.claimed=true;
    const auto no=[&](const char *why){r.refusal=QString::fromLatin1(why);return r;};
    const QPointer<QGuiApplication> app(qobject_cast<QGuiApplication *>(QCoreApplication::instance()));
    const QPointer<QQuickWindow> window(inputWindow);
    if(!app||QThread::currentThread()!=app->thread()||!window||window->thread()!=app->thread()||QCoreApplication::closingDown())return no("application-thread");
    if(budgetMs<1||budgetMs>reader_selected::MaxObservationMs)return no("budget-invalid");
    QElapsedTimer elapsed;elapsed.start();
    try {
        const auto config=inputConfig;const auto permit=inputPermit;const auto publish=inputPublish;
        const auto decimal=[](const QByteArray &s){if(s.isEmpty()||s.size()>20||s[0]=='0')return false;for(char c:s)if(c<'0'||c>'9')return false;bool ok=false;s.toULongLong(&ok);return ok;};
        if(!config.target.valid()||config.nonce.size()!=32||!decimal(config.pid)||config.pid.toULongLong()>2147483647||!decimal(config.start)||!permit||!publish||!config.setupEnd)return no("configuration");
        for(char c:config.nonce)if(!((c>='a'&&c<='f')||(c>='0'&&c<='9')))return no("configuration");
        const auto clock=[&]()->quint64{
#ifdef READER_FIXED_PEN_HANDOFF_TESTING
            if(inputTest.clock)return inputTest.clock();
#endif
            return bootMs();
        };
        const quint64 began=clock();quint64 last=began;
        const auto current=[&]{const auto t=clock();const bool ok=t&&t>=last&&t<config.setupEnd&&elapsed.elapsed()<budgetMs;last=t;return ok;};
        const Guard allowed=[&]{
            if(!app||!window||QCoreApplication::closingDown()||!current())return false;
            if(!permit())return false;
            return app&&window&&!QCoreApplication::closingDown()&&current();
        };
        if(!began||!allowed())return no("permission");
        unsigned callbacks=0;
        const reader_selected::ValidatedSourceAction action=[&](QObject *reader,QObject *,QObject *,const QVariant &,
                const reader_selected::Observation &observed,const Guard &sourceGuard){
            if(++callbacks!=1||r.attempted){r.refusal=QStringLiteral("source-callback-repeat");return;}
            const QPointer<QObject> pin(reader);
            if(!app||!window||!pin||pin->thread()!=app->thread()||!reader_ordinary_close::matches(observed,config.target)||!sourceGuard||!sourceGuard()||!pin||!allowed()){
                r.refusal=QStringLiteral("source-mismatch");return;
            }
            r.before=observed;r.matched=true;
            // The production guard runs last while every Sample watcher lives.
            if(!sourceGuard()||!pin||!app||!window||QCoreApplication::closingDown()||!current()){r.refusal=QStringLiteral("source-changed");return;}
            r.requestMs=last;
            if(config.setupEnd-r.requestMs<=3250){r.refusal=QStringLiteral("recovery-reserve");return;}
            r.originalRequest=config.nonce+' '+config.pid+' '+config.start+' '+QByteArray::number(r.requestMs)+'\n';
            r.attempted=r.reconciliationRequired=true;
            r.published=publish(r.originalRequest);
            // The terminal file operation may invalidate Reader. Never consult
            // its old guard or borrowed objects after possible publication.
            if(!r.published)r.refusal=QStringLiteral("publication-unknown");
            else if(!current())r.refusal=QStringLiteral("deadline-after-publication");
        };
        reader_selected::Observation observed;
#ifdef READER_FIXED_PEN_HANDOFF_TESTING
        if(inputTest.sample)observed=inputTest.sample(window,allowed,budgetMs,action);
        else
#endif
        observed=reader_selected::observeCreationJoin(window,allowed,budgetMs,{},action);
        if(!r.attempted&&r.refusal.isEmpty())r.refusal=observed.refusal.isEmpty()?QStringLiteral("source-action-unreached"):observed.refusal;
        return r;
    }catch(...){return no("exception");}
}
} // namespace reader_fixed_pen_handoff
