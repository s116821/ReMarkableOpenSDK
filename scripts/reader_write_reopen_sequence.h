#pragma once
// Default-off research composition. An IO implementation is not native admission.
#include "reader_identity_capture.h"
#include "reader_fixed_pen_handoff.h"
#include "reader_fixed_pen_result.h"
#include "reader_closed_view.h"
#include <QTimer>
#include <array>

namespace reader_write_reopen {
using Guard=std::function<bool()>;
struct Config {
    reader_identity_capture::Expected target;
    QByteArray nonce,pid,start;
    quint64 setupEnd=0;
};
struct Capture {
    reader_identity_capture::Result facts;
    bool pairCommitted=false,retired=false;
    QByteArray scalarSha256;
};
enum class PenState {Pending,Refused,Accepted};
struct Pen {PenState state=PenState::Pending;reader_fixed_pen_result::Result facts;};
struct HolderReady {bool observed=false;reader_fixed_pen_result::Expected binding;};
struct Result {
    bool finished=false,observed=false,reconciliationRequired=false,penCancelRequested=false;
    QString refusal;
    quint64 began=0,finishedMs=0;
    std::array<Capture,3> captures;
    reader_selected::NavigationObservation opened,reopened;
    reader_fixed_pen_handoff::Result handoff;
    reader_fixed_pen_result::Result pen;
    reader_ordinary_close::Result close;
    reader_closed_view::Result closed;
    unsigned penPolls=0,closurePolls=0;
    static constexpr bool nativeAuthority=false,renderAuthority=false,durableAuthority=false,atomicAuthority=false;
};
// Implementations must bind controls/nonce/process/root/original clock and
// dedicated holder unit. All methods run on GUI thread except independent pen
// recovery. Capture closes its per-phase resources before returning retired.
// nowMs is a pure, non-reentrant platform clock. cancelPen/report must be
// bounded, noexcept and must not navigate or draw. None authenticates an IO
// implementation supplied by a caller; the native backend is qualified separately.
class IO {
public:
    virtual ~IO()=default;
    virtual quint64 nowMs()noexcept=0;
    virtual bool admitted()noexcept=0;
    virtual HolderReady holderReady()noexcept=0;
    virtual Capture capture(int,QQuickWindow *,const reader_identity_capture::Expected &,
                            const std::shared_ptr<reader_identity_capture::Claim> &,const Guard &,int)=0;
    virtual bool publishGo(const QByteArray &)=0;
    virtual Pen pen(const reader_fixed_pen_handoff::Result &)=0;
    virtual void cancelPen()noexcept=0;
    virtual void report(const Result &)noexcept=0;
};
#ifdef READER_WRITE_REOPEN_TESTING
struct TestBoundary {
    std::function<reader_selected::NavigationObservation(QQuickWindow *,const reader_selected::FixtureNavigation &,const Guard &,int)> navigate;
    std::function<reader_fixed_pen_handoff::Result(QQuickWindow *,const reader_fixed_pen_handoff::Config &,const std::shared_ptr<reader_fixed_pen_handoff::Claim> &,const Guard &,const std::function<bool(const QByteArray &)> &,int)> handoff;
    std::function<reader_ordinary_close::Result(QQuickWindow *,const reader_ordinary_close::Expected &,const std::shared_ptr<reader_ordinary_close::Claim> &,const Guard &,int)> close;
    std::function<reader_closed_view::Result(QQuickWindow *,const Guard &,int)> closed;
    int navigationWait=5000,penWait=25,closureWait=100;
};
#endif
class Sequence final:public QObject {
public:
    Sequence(QGuiApplication *app,Config config,std::shared_ptr<IO> io,QObject *parent=nullptr
#ifdef READER_WRITE_REOPEN_TESTING
             ,TestBoundary test={}
#endif
    ):QObject(parent?parent:app),app_(app),config_(std::move(config)),io_(std::move(io))
#ifdef READER_WRITE_REOPEN_TESTING
      ,test_(std::move(test))
#endif
    {}
    ~Sequence()override{if(!result_.finished&&result_.handoff.attempted&&io_)io_->cancelPen();}
    bool start(){
        if(started_)return false;
        started_=true;
        if(!app_||thread()!=app_->thread()||QThread::currentThread()!=app_->thread()||!io_||!config_.target.valid()||
           config_.nonce.size()!=32||!reader_fixed_pen_result::detail::pid(QString::fromLatin1(config_.pid))||
           !reader_fixed_pen_result::detail::decimal(QString::fromLatin1(config_.start)))return false;
        for(char c:config_.nonce)if(!((c>='a'&&c<='f')||(c>='0'&&c<='9')))return false;
        window_=qobject_cast<QQuickWindow *>(QGuiApplication::focusWindow());
        root_=window_?window_->contentItem():nullptr;engine_=window_?qmlEngine(window_):nullptr;
        const QPointer<Sequence> weak(this);const auto io=io_;
        const auto now=io->nowMs();
        if(!weak)return false;
        last_=now;result_.began=now;
        if(!now||config_.setupEnd<=now||config_.setupEnd-now>90000||!permitted()){
            if(weak)finish("admission");
            return false;
        }
        for(auto &claim:captures_)claim=std::make_shared<reader_identity_capture::Claim>();
        handoff_=std::make_shared<reader_fixed_pen_handoff::Claim>();close_=std::make_shared<reader_ordinary_close::Claim>();
        connect(app_,&QCoreApplication::aboutToQuit,this,[weak]{if(weak)weak->finish("application-closing");});
        QTimer::singleShot(int(config_.setupEnd-last_),this,[weak]{if(weak)weak->finish("setup-expired");});
        // Readiness comes from independently admitted IO, not mere file presence.
        const auto ready=io->holderReady();
        if(!weak)return false;
        holder_=ready.binding;
        const bool live=permitted();if(!weak)return false;
        if(!ready.observed||!live||!holderMatches(holder_,false)){finish("holder-not-ready");return false;}
        schedule(Step::Open,0);return true;
    }
    void cancel(){finish("canceled");}
    Result snapshot()const{return result_;}
private:
    enum class Step {Open,Blank,Handoff,Pen,Ink,Close,Closed,Reopen,Reopened};
    bool basic(bool terminalSnapshot=false)const{
        return app_&&window_&&root_&&engine_&&!QCoreApplication::closingDown()&&QThread::currentThread()==app_->thread()&&
            window_->thread()==app_->thread()&&engine_->thread()==app_->thread()&&window_->contentItem()==root_&&qmlEngine(window_)==engine_&&
            window_->isVisible()&&window_->isActive()&&QGuiApplication::focusWindow()==window_&&(terminalSnapshot||!result_.finished);
    }
    bool holderMatches(const reader_fixed_pen_result::Expected &b,bool requested)const{
        static const QRegularExpression boot(QStringLiteral("\\A[0-9a-f]{8}(-[0-9a-f]{4}){3}-[0-9a-f]{12}\\z"));
        return b.nonce==QString::fromLatin1(config_.nonce)&&b.candidatePid==QString::fromLatin1(config_.pid)&&
            b.candidateStart==QString::fromLatin1(config_.start)&&b.setupEnd==config_.setupEnd&&
            reader_fixed_pen_result::detail::pid(b.holderPid)&&reader_fixed_pen_result::detail::decimal(b.holderStart)&&
            b.holderPid!=b.candidatePid&&b.boot.size()==36&&boot.match(b.boot).hasMatch()&&
            b.holderPid==holder_.holderPid&&b.holderStart==holder_.holderStart&&b.boot==holder_.boot&&
            (requested?b.requestMs==result_.handoff.requestMs:b.requestMs==0);
    }
    bool permitted(){
        if(!basic()||!io_)return false;
        const QPointer<Sequence> weak(this);const auto io=io_;
        const auto before=io->nowMs();if(!weak)return false;
        if(!before||before<last_||before>=config_.setupEnd)return false;
        last_=before;
        const bool admitted=io->admitted();if(!weak)return false;
        const auto after=io->nowMs();if(!weak)return false;
        if(!admitted||after<last_||after>=config_.setupEnd)return false;
        last_=after;return basic();
    }
    int remaining()const{return last_<config_.setupEnd?int(qMin<quint64>(reader_selected::MaxObservationMs,config_.setupEnd-last_)):0;}
    int navigationWait()const{
#ifdef READER_WRITE_REOPEN_TESTING
        return qMax(0,test_.navigationWait);
#else
        return 5000;
#endif
    }
    int penWait()const{
#ifdef READER_WRITE_REOPEN_TESTING
        return qMax(0,test_.penWait);
#else
        return 25;
#endif
    }
    int closureWait()const{
#ifdef READER_WRITE_REOPEN_TESTING
        return qMax(0,test_.closureWait);
#else
        return 100;
#endif
    }
    void schedule(Step step,int delay){
        if(result_.finished)return;
        const quint64 ticket=++ticket_;const QPointer<Sequence> weak(this);
        QTimer::singleShot(delay,this,[weak,ticket,step]{if(weak&&!weak->result_.finished&&weak->ticket_==ticket)weak->advance(step);});
    }
    void finish(const char *why){
        if(why&&*why){stopRequested_=true;if(stopReason_.isEmpty())stopReason_=QString::fromLatin1(why);}
        if(result_.finished)return;
        result_.finished=true;result_.refusal=QString::fromLatin1(why);++ticket_;
        result_.reconciliationRequired=result_.handoff.reconciliationRequired||result_.close.effectRequiresReconciliation||result_.opened.attempted||result_.reopened.attempted;
        const auto io=io_;const QPointer<Sequence> weak(this);
        if(io){
            if(result_.handoff.attempted&&!qualifiedPen_){result_.penCancelRequested=true;io->cancelPen();if(!weak)return;}
            const auto now=io->nowMs();if(!weak)return;
            result_.finishedMs=now;
            if(result_.observed){
                const bool admitted=basic(true)&&io->admitted();if(!weak)return;
                const auto after=io->nowMs();if(!weak)return;
                result_.finishedMs=after;
                if(stopRequested_||!admitted||!basic(true)||now<last_||after<now||after>=config_.setupEnd){result_.observed=false;result_.refusal=stopReason_.isEmpty()?QStringLiteral("finish-boundary"):stopReason_;}
            }
            const Result copied=result_;io->report(copied);
        }
    }
    reader_selected::NavigationObservation navigate(const Guard &permit,int budget){
        const reader_selected::FixtureNavigation navigation{config_.target.documentId,config_.target.pageIndex,true};
#ifdef READER_WRITE_REOPEN_TESTING
        const auto test=test_.navigate;
        if(test)return test(window_,navigation,permit,budget);
#endif
        return reader_selected::openFixture(window_,navigation,permit,budget);
    }
    bool capture(int index,const Guard &permit,int budget){
        const auto io=io_;const auto claim=captures_[std::size_t(index)];const auto expected=config_.target;
        const QPointer<Sequence> weak(this);const QPointer<QQuickWindow> window=window_;
        auto evidence=io->capture(index,window,expected,claim,permit,budget);
        if(!weak)return false;
        result_.captures[std::size_t(index)]=std::move(evidence);
        const auto &e=result_.captures[std::size_t(index)];
        static const QRegularExpression hash(QStringLiteral("\\A[0-9a-f]{64}\\z"));
        return claim&&claim->consumed.load()&&e.facts.captured&&e.facts.attempted&&!e.facts.unknown&&e.facts.refusal.isEmpty()&&
            reader_identity_capture::detail::matches(e.facts.before,expected)&&reader_identity_capture::detail::matches(e.facts.after,expected)&&
            e.facts.encoded>0&&e.facts.encoded<=1048576&&e.facts.width>0&&e.facts.height>0&&
            e.pairCommitted&&e.retired&&hash.match(QString::fromLatin1(e.scalarSha256)).hasMatch()&&hash.match(QString::fromLatin1(e.facts.sha)).hasMatch();
    }
    void advance(Step step){
        if(advancing_){finish("phase-reentered");return;}
        const QPointer<Sequence> entered(this);
        const bool entryLive=permitted();if(!entered)return;
        if(!entryLive){finish("context-or-deadline");return;}
        advancing_=true;
        const QPointer<Sequence> weak(this);const auto io=io_;
        const auto reset=qScopeGuard([weak]{if(weak)weak->advancing_=false;});
        const Guard permit=[weak]{return weak&&weak->permitted();};
        const int budget=remaining();
        try{
            if(step==Step::Open||step==Step::Reopen){
                const unsigned which=step==Step::Open?0:1;
                if(navigationConsumed_[which]){finish("navigation-spent");return;}navigationConsumed_[which]=true;
                (which?result_.reopened:result_.opened).attempted=true;
                const auto r=navigate(permit,budget);if(!weak)return;
                (which?result_.reopened:result_.opened)=r;
                const bool live=permitted();if(!weak)return;
                if(!r.attempted||!r.dispatched||!r.refusal.isEmpty()||!live){finish("navigation-refused");return;}
                schedule(which?Step::Reopened:Step::Blank,navigationWait());return;
            }
            if(step==Step::Blank||step==Step::Ink||step==Step::Reopened){
                const int index=step==Step::Blank?0:(step==Step::Ink?1:2);
                if(captureConsumed_[std::size_t(index)]){finish("capture-spent");return;}captureConsumed_[std::size_t(index)]=true;
                const bool accepted=capture(index,permit,budget);if(!weak)return;
                const bool live=permitted();if(!weak)return;
                if(!accepted||!live){finish("capture-or-retirement-refused");return;}
                if(index==2){result_.observed=true;finish("");return;}
                schedule(index==0?Step::Handoff:Step::Close,0);return;
            }
            if(step==Step::Handoff){
                if(handoffConsumed_){finish("handoff-spent");return;}handoffConsumed_=true;
                const reader_fixed_pen_handoff::Config config{{config_.target.documentId,config_.target.pageId,config_.target.pageIndex,config_.target.pageCount},config_.nonce,config_.pid,config_.start,config_.setupEnd};
                const auto publish=[io](const QByteArray &bytes){return io->publishGo(bytes);};
                const auto claim=handoff_;
                reader_fixed_pen_handoff::Result r;
                result_.handoff.attempted=result_.handoff.reconciliationRequired=true;
#ifdef READER_WRITE_REOPEN_TESTING
                const auto test=test_.handoff;
                if(test)r=test(window_,config,claim,permit,publish,budget);
                else
#endif
                r=reader_fixed_pen_handoff::run(window_,config,claim,permit,publish,budget);
                if(!weak)return;
                result_.handoff=std::move(r);
                const bool live=permitted();if(!weak)return;
                const auto request=config_.nonce+' '+config_.pid+' '+config_.start+' '+QByteArray::number(result_.handoff.requestMs)+'\n';
                if(!result_.handoff.claimed||!result_.handoff.matched||!result_.handoff.attempted||!result_.handoff.published||
                   result_.handoff.originalRequest!=request||!result_.handoff.refusal.isEmpty()||!live){finish("handoff-refused");return;}
                schedule(Step::Pen,penWait());return;
            }
            if(step==Step::Pen){
                if(++result_.penPolls>128){finish("pen-poll-cap");return;}
                const auto handoff=result_.handoff;
                const auto r=io->pen(handoff);if(!weak)return;
                if(r.state==PenState::Pending){schedule(Step::Pen,penWait());return;}
                result_.pen=r.facts;
                const bool live=permitted();if(!weak)return;
                if(r.state!=PenState::Accepted||!r.facts.claimed||!r.facts.accepted||!r.facts.replay.strokeObserved||!r.facts.refusal.isEmpty()||
                   !holderMatches(r.facts.binding,true)||r.facts.completedAtMs>last_||!live){finish("pen-refused");return;}
                qualifiedPen_=true;
                schedule(Step::Ink,0);return;
            }
            if(step==Step::Close){
                const reader_ordinary_close::Expected expected{config_.target.documentId,config_.target.pageId,config_.target.pageIndex,config_.target.pageCount};
                const auto claim=close_;
                reader_ordinary_close::Result r;
                result_.close.attempted=result_.close.effectRequiresReconciliation=true;
#ifdef READER_WRITE_REOPEN_TESTING
                const auto test=test_.close;
                if(test)r=test(window_,expected,claim,permit,budget);
                else
#endif
                r=reader_ordinary_close::run(window_,expected,claim,permit,budget);
                if(!weak)return;
                result_.close=std::move(r);
                const bool live=permitted();if(!weak)return;
                if(!result_.close.claimed||!result_.close.matched||!result_.close.attempted||!result_.close.dispatched||
                   !reader_ordinary_close::matches(result_.close.before,expected)||!result_.close.refusal.isEmpty()||!live){finish("close-refused");return;}
                schedule(Step::Closed,closureWait());return;
            }
            if(step==Step::Closed){
                if(++result_.closurePolls>32){finish("closure-poll-cap");return;}
                reader_closed_view::Result r;
#ifdef READER_WRITE_REOPEN_TESTING
                const auto test=test_.closed;
                if(test)r=test(window_,permit,budget);
                else
#endif
                r=reader_closed_view::observe(window_,permit,budget);
                if(!weak)return;
                result_.closed=std::move(r);
                const bool live=permitted();if(!weak)return;
                if(!live){finish("closure-context");return;}
                if(!result_.closed.observed||!result_.closed.compositionObserved||!result_.closed.loaderReady||!result_.closed.documentNull||!result_.closed.refusal.isEmpty()){schedule(Step::Closed,closureWait());return;}
                schedule(Step::Reopen,0);return;
            }
        }catch(...){if(weak)finish("phase-exception");}
    }
    QPointer<QGuiApplication> app_;QPointer<QQuickWindow> window_;QPointer<QQuickItem> root_;QPointer<QQmlEngine> engine_;
    Config config_;std::shared_ptr<IO> io_;Result result_;
    reader_fixed_pen_result::Expected holder_;
    std::array<std::shared_ptr<reader_identity_capture::Claim>,3> captures_;
    std::shared_ptr<reader_fixed_pen_handoff::Claim> handoff_;
    std::shared_ptr<reader_ordinary_close::Claim> close_;
    std::array<bool,2> navigationConsumed_{{false,false}};std::array<bool,3> captureConsumed_{{false,false,false}};
    bool started_=false,advancing_=false,handoffConsumed_=false,stopRequested_=false,qualifiedPen_=false;
    QString stopReason_;
    quint64 last_=0,ticket_=0;
#ifdef READER_WRITE_REOPEN_TESTING
    TestBoundary test_;
#endif
};
} // namespace reader_write_reopen
