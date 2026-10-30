// Actual owned FIFO/child/pipes/FDs; device snapshots, identity, faults modeled.
#define READER_FIXED_PEN_HOLDER_TESTING
#include <QJsonDocument> // Deliberately Qt-first: published IO.emit remains usable.
#include "reader_fixed_pen_holder_wire.h"
#ifndef emit
#error Holder must restore caller Qt emit macro.
#endif
#include <QCryptographicHash>
#include <QFile>
#include <QDir>
#include <sys/stat.h>
#include <time.h>
#include <stdexcept>
#include <vector>

namespace {
using namespace reader_fixed_pen_holder;
unsigned cases=0,checks=0;
void require(bool b,const char*m){++checks;if(!b)throw std::runtime_error(m);}
void competingReaper(int)noexcept{int status=0;while(::waitpid(-1,&status,WNOHANG)>0){}}
std::uint64_t now()noexcept{timespec t{};if(::clock_gettime(CLOCK_BOOTTIME,&t))return 0;return std::uint64_t(t.tv_sec)*1000+std::uint64_t(t.tv_nsec)/1000000;}
struct Raw {std::uint32_t seconds=0,micros=0;std::uint16_t type=0,code=0;std::int32_t value=0;};
static_assert(sizeof(Raw)==16&&alignof(Raw)==4,"original owned carrier, not native input_event proof");
enum class Fault {None,InitialHeld,InitialMismatch,InitialQueue,CancelBefore,CancelContact,ChildExitContact,ChildShort,ChildPartial,
    ChildEintr,ChildCancel,ChildPause,ChildNoResult,ReadMalformed,ReadDropped,TraceShort,IdentityLost,SnapshotMismatch,FinishLate,FinishContext,CloseFailure,NoFence,UnknownWait};
struct IO {
    Fault fault;int observer=-1,writer=-1,recovery=-1,trace=-1;std::string folder,path,tracePath;
    Snapshot state{},initialState{};std::uint64_t initialClock=0,lastRead=0,offset=0;unsigned writes=0,releases=0,snapshots=0;
    bool contactSeen=false,rawGood=true,quiet=false,matched=false,identity=true,devicesClosed=false,tracedClosed=false,parentWriterClosed=false,inConfirm=false;
    std::size_t bytes=0;QByteArray stored,traceHash;
    explicit IO(Fault f):fault(f) {
        char name[]="/tmp/reader-holder-owned-XXXXXX";char*made=::mkdtemp(name);require(made,"owned directory created");folder=made;path=folder+"/model.fifo";tracePath=folder+"/trace.raw";
        require(::mkfifo(path.c_str(),0600)==0,"owned FIFO created");observer=::open(path.c_str(),O_RDONLY|O_NONBLOCK|O_CLOEXEC);writer=::open(path.c_str(),O_WRONLY|O_NONBLOCK|O_CLOEXEC);recovery=::open(path.c_str(),O_WRONLY|O_NONBLOCK|O_CLOEXEC);
        trace=::open(tracePath.c_str(),O_RDWR|O_CREAT|O_EXCL|O_CLOEXEC,0600);require(observer>=0&&writer>=0&&recovery>=0&&trace>=0,"independently opened owned endpoints");
        state.x=initialState.x=11056;state.y=initialState.y=7616;initialClock=now();
        if(fault==Fault::InitialQueue){Raw event{};event.type=0;const auto n=::write(recovery,&event,sizeof event);require(n==sizeof event,"preexisting modeled queue record");}
    }
    ~IO(){for(int fd:{observer,writer,recovery,trace})if(fd>=0)::close(fd);::unlink(path.c_str());::unlink(tracePath.c_str());::rmdir(folder.c_str());}
    static bool closeFd(int&fd)noexcept{if(fd<0)return true;int n=fd;fd=-1;return ::close(n)==0;}
    std::uint64_t nowMs()noexcept {return now()+offset;}
    bool context()noexcept {return fault!=Fault::CancelBefore&&!(fault==Fault::CancelContact&&contactSeen)&&!(fault==Fault::FinishContext&&devicesClosed);}
    bool snapshot(Snapshot&s)noexcept {s=state;++snapshots;if(fault==Fault::InitialHeld)s.touch=true;if(fault==Fault::InitialMismatch&&snapshots==2)--s.x;if(fault==Fault::SnapshotMismatch&&inConfirm)--s.x;return true;}
    bool identityValid()noexcept {
        if(fault==Fault::IdentityLost&&contactSeen)return false;
        struct stat a{},b{};identity=observer>=0&&recovery>=0&&!::fstat(observer,&a)&&!::fstat(recovery,&b)&&S_ISFIFO(a.st_mode)&&a.st_dev==b.st_dev&&a.st_ino==b.st_ino;
        return identity;
    }
    bool releasedState()noexcept{return reader_fixed_pen_stream::released(state)&&reader_fixed_pen_stream::same(state,initialState);}
    bool observerEmpty()noexcept{pollfd p{observer,POLLIN,0};return ::poll(&p,1,0)==0;}
    bool afterForkChild()noexcept {bool ok=closeFd(observer);ok=closeFd(recovery)&&ok;return closeFd(trace)&&ok;}
    bool afterForkParent()noexcept {parentWriterClosed=closeFd(writer);return parentWriterClosed;}
    bool childAllowed()noexcept {return fault!=Fault::ChildCancel||writes<3;}
    reader_fixed_pen::Write emitWriter(const Event*events,std::size_t count)noexcept {
        ++writes;std::array<Raw,7> raw{};for(std::size_t i=0;i<count;++i){raw[i].type=events[i].type;raw[i].code=events[i].code;raw[i].value=events[i].value;}
        if(writes==3&&fault==Fault::ChildEintr)return {-1,EINTR};
        std::size_t amount=count*sizeof(Raw);if(writes==3&&fault==Fault::ChildShort)amount=sizeof(Raw);if(writes==3&&fault==Fault::ChildPartial)amount=sizeof(Raw)+1;
        const auto n=::write(writer,raw.data(),amount);
        if(writes==3&&fault==Fault::ChildExitContact)::_exit(33);
        if(writes==3&&fault==Fault::ChildPause){pollfd none{};const int paused=::poll(&none,0,2000);(void)paused;}
        if(writes==3&&fault==Fault::ReadMalformed){const char bad='x';const auto extra=::write(writer,&bad,1);(void)extra;}
        if(writes==3&&fault==Fault::ReadDropped){Raw dropped{};dropped.type=0;dropped.code=3;const auto extra=::write(writer,&dropped,sizeof dropped);(void)extra;}
        if(writes==56&&fault==Fault::ChildNoResult)::_exit(36);
        return {n,n<0?errno:0};
    }
    reader_fixed_pen::Write emitRecovery(const Event*events,std::size_t count)noexcept {
        ++releases;std::array<Raw,7> raw{};for(std::size_t i=0;i<count;++i){raw[i].type=events[i].type;raw[i].code=events[i].code;raw[i].value=events[i].value;}
        const auto n=::write(recovery,raw.data(),count*sizeof(Raw));return {n,n<0?errno:0};
    }
    bool observe(Stream&stream)noexcept {
        std::array<Raw,256> raw{};const auto n=::read(observer,raw.data(),sizeof raw);
        if(n<0)return errno==EAGAIN||errno==EWOULDBLOCK;
        if(n==0)return rawGood=false;
        lastRead=nowMs();
        std::size_t amount=std::size_t(n);if(fault==Fault::TraceShort&&contactSeen&&amount) --amount;
        const auto wrote=::write(trace,raw.data(),amount);if(wrote!=n)rawGood=false;
        if(wrote>0)bytes+=std::size_t(wrote);
        if(n%ssize_t(sizeof(Raw)))return rawGood=false;
        for(ssize_t i=0;i<n/ssize_t(sizeof(Raw));++i) {
            const Event e{raw[std::size_t(i)].type,raw[std::size_t(i)].code,raw[std::size_t(i)].value};
            if(e.type==1&&e.code==330&&e.value)contactSeen=true;
            if(e.type==1){if(e.code==320)state.pen=e.value;else if(e.code==330)state.touch=e.value;}
            else if(e.type==3)switch(e.code){case 0:state.x=e.value;break;case 1:state.y=e.value;break;case 24:state.pressure=e.value;break;case 25:state.distance=e.value;break;default:break;}
            if(!stream.accept(e))rawGood=false;
        }
        return rawGood;
    }
    bool waitParent(unsigned ms)noexcept {pollfd p{observer,POLLIN,0};return ::poll(&p,1,int(ms))>=0&&!(p.revents&(POLLERR|POLLHUP|POLLNVAL));}
    pid_t testWaitChild(pid_t child,int*status,int options)noexcept {if(fault==Fault::NoFence){offset=4000;return 0;}if(fault==Fault::UnknownWait){errno=ECHILD;return -1;}return ::waitpid(child,status,options);}
    bool confirmReleased(Stream&stream,std::uint64_t end)noexcept {
        if(!rawGood)return false;
        inConfirm=true;lastRead=nowMs();auto last=lastRead;
        while(true){const auto at=nowMs();if(at<last||at>=end)return false;last=at;if(!observe(stream))return false;const auto after=nowMs();
            if(after>=end||after<at)return false;
            if(after>=lastRead&&after-lastRead>=20){quiet=true;Snapshot a{},b{};if(!snapshot(a)||!snapshot(b)||!reader_fixed_pen_stream::same(a,b)||!observerEmpty()||!identityValid())return false;matched=reader_fixed_pen_stream::same(stream.report().state,b);return matched&&stream.releasedMatches(b);}if(!waitParent(2))return false;}
    }
    Snapshot finalSnapshot()const noexcept{return state;}
    bool quietConfirmed()const noexcept{return quiet;}
    bool snapshotMatched()const noexcept{return matched;}
    bool finish() {
        struct stat st{};bool good=trace>=0&&!::fstat(trace,&st)&&st.st_size>=0&&st.st_size<=131072;
        stored.resize(good?int(st.st_size):0);if(good&&::pread(trace,stored.data(),std::size_t(stored.size()),0)!=stored.size())good=false;
        traceHash=QCryptographicHash::hash(stored,QCryptographicHash::Sha256).toHex();tracedClosed=closeFd(trace);
        bool closed=closeFd(observer);closed=closeFd(writer)&&closed;closed=closeFd(recovery)&&closed;devicesClosed=closed;
        if(fault==Fault::FinishLate)offset=4000;
        if(fault==Fault::CloseFailure)devicesClosed=false;
        return good&&closed&&tracedClosed&&fault!=Fault::CloseFailure;
    }
    bool deviceFdsClosed()const noexcept{return devicesClosed;}
    bool traceClosed()const noexcept{return tracedClosed;}
    Evidence evidence()const{return {std::uint64_t(stored.size()),traceHash,rawGood,quiet,matched,identity};}
};
template<class F>void scenario(F f){++cases;f();}
void exportWire(const char*name,const QByteArray&wire,const QByteArray&raw,const QByteArray&expected) {
    const char*out=::getenv("READER_HOLDER_EXPORT");if(!out)return;QDir dir(QString::fromLocal8Bit(out));require(dir.mkpath("."),"owned wire export directory");
    QFile scalar(dir.filePath(QString::fromLatin1(name)+".json"));require(scalar.open(QIODevice::WriteOnly|QIODevice::NewOnly)&&scalar.write(wire)==wire.size(),"original C++ wire export");scalar.close();
    QFile trace(dir.filePath(QString::fromLatin1(name)+".raw"));require(trace.open(QIODevice::WriteOnly|QIODevice::NewOnly)&&trace.write(raw)==raw.size(),"original owned raw model export");trace.close();
    QFile facts(dir.filePath(QString::fromLatin1(name)+".expected.json"));require(facts.open(QIODevice::WriteOnly|QIODevice::NewOnly)&&facts.write(expected)==expected.size(),"separate owned binding expectation export");facts.close();
}
Binding binding(const reader_fixed_pen::Budget&budget){
    Binding b;b.nonce="11111111222233334444555555555555";b.boot="11111111-2222-3333-4444-555555555555";
    // Live owned parent is the modeled UI context; this is never xochitl admission.
    b.candidatePid=std::uint64_t(::getppid());b.candidateStart=detail::processStart(::getppid());
    b.holderPid=std::uint64_t(::getpid());b.holderStart=detail::processStart(::getpid());
    require(b.candidatePid&&b.candidateStart&&b.holderStart&&b.candidatePid!=b.holderPid,"independently live owned parent binding");
    b.setupEnd=budget.began+90000;b.requestMs=b.beganMs=budget.began;b.goClaimed=true;return b;
}
QByteArray expectations(const char*name,const Binding&b,bool accepted){
    // Prepared from independently sampled Binding, never parsed/rebuilt from wire.
    const QJsonObject facts{{"schema","owned-holder-consumer-expectation/1"},{"case",name},{"expected_accepted",accepted},
        {"candidate_identity_model","live-owned-parent"},{"external_completion_modeled",true},
        {"nonce",QString::fromLatin1(b.nonce)},{"boot_id",QString::fromLatin1(b.boot)},
        {"candidate_pid",QString::number(b.candidatePid)},{"candidate_start",QString::number(b.candidateStart)},
        {"holder_pid",QString::number(b.holderPid)},{"holder_start",QString::number(b.holderStart)},
        {"setup_end_ms",QString::number(b.setupEnd)},{"request_ms",QString::number(b.requestMs)},
        {"completion_observed_ms",QString::number(b.beganMs+5000)},{"completion_now_ms",QString::number(b.beganMs+5000)}};
    return QJsonDocument(facts).toJson(QJsonDocument::Compact)+'\n';
}
}
int main(){
    struct sigaction ignore{};ignore.sa_handler=SIG_IGN;::sigemptyset(&ignore.sa_mask);if(::sigaction(SIGPIPE,&ignore,nullptr))return 2;
    try {
        scenario([&]{IO io(Fault::None);const auto budget=reader_fixed_pen::reserve(now());const auto r=run(io,budget);
            require(r.accepted&&r.childStarted&&r.childGone&&r.childStatusKnown&&r.childExit==0&&!r.childSignal&&r.childResultKnown,"full owned child dispatch and parent release accepted");
            require(r.armed&&io.releases==1&&r.recovery.observed&&r.stream.strokeObserved&&r.stream.contactIntervals==1,"one independent recovery and matching stream");
            require(r.dispatch.calls==56&&r.dispatch.frames==56&&r.dispatch.acceptedRecords==178&&r.childStart>0,"full fixed dispatch counters and actual child identity");
            require(r.deviceFdsClosed&&r.ipcFdsClosed&&r.traceClosed&&io.parentWriterClosed,"device/IPC/trace closures before result");
            require(reader_fixed_pen_stream::same(r.initialSnapshot,io.initialState),"initial snapshot preserved for independent replay");
            require(::kill(r.childPid,0)<0&&errno==ESRCH,"own writer child actually gone");const auto bound=binding(budget);const auto expected=expectations("positive",bound,true);const auto wire=normalWire(bound,r,io.evidence());
            require(!wire.isEmpty()&&wire.size()<=4096&&QJsonDocument::fromJson(wire).object().value("accepted").toBool(),"actual parent compact JSON fits4096");exportWire("positive",wire,io.stored,expected);
        });
        for(int disposition=0;disposition<3;++disposition)scenario([&]{
            const pid_t own=::fork();require(own>=0,"owned inherited-reaper subprocess fork");
            if(!own){
                struct sigaction action{};action.sa_handler=disposition==0?SIG_IGN:(disposition==1?SIG_DFL:competingReaper);action.sa_flags=disposition==1?SA_NOCLDWAIT:0;::sigemptyset(&action.sa_mask);
                if(::sigaction(SIGCHLD,&action,nullptr))::_exit(40);
                bool denied=false;
                {IO io(Fault::None);const auto r=run(io,reader_fixed_pen::reserve(now()));denied=!r.childStarted&&r.childPid==0&&!r.armed&&io.releases==0;}
                ::_exit(denied?0:41);
            }
            int status=0;require(::waitpid(own,&status,0)==own&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"auto-reaping SIGCHLD denied before child creation");
        });
        scenario([&]{IO io(Fault::UnknownWait);const auto r=run(io,reader_fixed_pen::reserve(now()));
            require(r.diagnostic&&!r.childGone&&!r.accepted&&io.releases==0&&!r.recovery.attempted,"unknown wait ownership no release/normal result");
            pollfd none{};const int pause=::poll(&none,0,20);require(pause==0,"owned wait ownership grace");
            int status=0;const pid_t waited=::waitpid(r.childPid,&status,WNOHANG);
            require(waited==0||(waited==r.childPid&&WIFEXITED(status)),"unknown wait ownership only closes cancellation pipe, no SIGKILL");
            if(!waited)require(::kill(r.childPid,SIGKILL)==0&&::waitpid(r.childPid,&status,0)==r.childPid,"owned external test cleanup fences remaining child independently");
            else require(WEXITSTATUS(status)==0||(WEXITSTATUS(status)>=109&&WEXITSTATUS(status)<=115),"closed IPC permits only fixed child exit/result-refusal codes");
            io.finish();
        });
        for(Fault f:{Fault::InitialHeld,Fault::InitialMismatch,Fault::InitialQueue,Fault::CancelBefore})scenario([&]{IO io(f);const auto r=run(io,reader_fixed_pen::reserve(now()));require(!r.accepted&&!r.childStarted&&!r.armed&&io.releases==0,"prestart failure no writer/recovery write");});
        for(Fault f:{Fault::CancelContact,Fault::ChildExitContact,Fault::ChildShort,Fault::ChildPartial,Fault::ChildEintr,Fault::ChildCancel,Fault::ChildPause,Fault::ChildNoResult,Fault::ReadMalformed,Fault::ReadDropped,Fault::TraceShort,Fault::SnapshotMismatch,Fault::FinishLate,Fault::FinishContext,Fault::CloseFailure})scenario([&]{
            IO io(f);const auto budget=reader_fixed_pen::reserve(now());const auto r=run(io,budget);require(!r.accepted&&r.childGone&&r.childStatusKnown&&r.childStarted,"failure retains actual child/gone facts without acceptance");
            require(io.releases==1&&r.recovery.attempted,"after child gone failure does not revoke independent release attempt");
            require(r.ipcFdsClosed,"all parent child IPC closed");require(::kill(r.childPid,0)<0&&errno==ESRCH,"failure child actually reaped");
            if(f==Fault::ReadDropped||f==Fault::ReadMalformed||f==Fault::TraceShort||f==Fault::ChildPartial)require(!r.recovery.observed&&!r.observationGood,"poisoned observation cannot be reset into release proof");
            if(f==Fault::ChildExitContact||f==Fault::ChildNoResult)require(!r.childResultKnown&&r.dispatch.unknown&&r.dispatch.contactPossible,"missing child pipe cannot erase possible contact");
            if(f==Fault::ChildPause)require(r.childSignal==SIGKILL&&!r.childResultKnown,"original stroke deadline actually fences paused child");
            const auto bound=binding(budget);const auto expected=expectations("canceled",bound,false);const auto wire=normalWire(bound,r,io.evidence());require(!wire.isEmpty()&&!QJsonDocument::fromJson(wire).object().value("accepted").toBool(),"actual failure wire retains false acceptance");
            if(f==Fault::CancelContact)exportWire("canceled",wire,io.stored,expected);
        });
        scenario([&]{IO io(Fault::IdentityLost);const auto r=run(io,reader_fixed_pen::reserve(now()));require(!r.accepted&&r.childGone&&io.releases==0&&!r.recovery.attempted&&!r.recovery.observed,"device-identity loss cannot authorize release write");});
        scenario([&]{IO io(Fault::NoFence);const auto budget=reader_fixed_pen::reserve(now());const auto r=run(io,budget);require(r.diagnostic&&!r.childGone&&!r.accepted&&io.releases==0&&!r.recovery.attempted&&!io.deviceFdsClosed(),"modeled unavailable reap status never authorizes recovery or close");
            const auto bound=binding(budget);const auto expected=expectations("unfenced-diagnostic",bound,false);const auto wire=diagnosticWire(bound,r);require(!wire.isEmpty()&&normalWire(binding(budget),r,{}).isEmpty(),"unfenced child has diagnostic-only schema");exportWire("unfenced-diagnostic",wire,{},expected);
            int status=0;require(::waitpid(r.childPid,&status,0)==r.childPid,"actual owned child reaped separately after modeled status uncertainty");io.finish();
        });
        scenario([&]{IO io(Fault::None);const auto budget=reader_fixed_pen::reserve(now()-4000);const auto r=run(io,budget);require(!r.accepted&&!r.childStarted&&!r.armed&&io.releases==0,"original expired budget cannot be renewed");});
        std::printf("PASS fixed pen holder cases=%u checks=%u owned_fifo_child=true device_identity_snapshot_clock_models=true native_io=false\n",cases,checks);return 0;
    }catch(const std::exception&e){std::fprintf(stderr,"FAIL fixed pen holder %s cases=%u checks=%u\n",e.what(),cases,checks);return 1;}
}
