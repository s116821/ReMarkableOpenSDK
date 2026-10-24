#pragma once
// Research composition: real child/pipe lifecycle, caller-supplied device IO.
// Single-threaded/fork-safe caller with SIGPIPE ignored required. IO methods do
// not throw/reenter. No Qt, heap or logging in child. SIGCHLD must be SIG_DFL,
// and no signal handler/thread/other caller may reap this holder-owned child.
#pragma push_macro("emit")
#undef emit
#include "reader_fixed_pen_stream.h"
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace reader_fixed_pen_holder {
using Snapshot=reader_fixed_pen_stream::Snapshot;
using Stream=reader_fixed_pen_stream::Stream;
using Event=reader_fixed_pen::Event;
struct Result {
    bool accepted=false,diagnostic=false,childStarted=false,childGone=false,childStatusKnown=false,childResultKnown=false;
    bool armed=false,initialSnapshotsMatch=false,initialQueueEmpty=false,observationGood=true,contextLive=false;
    bool deviceFdsClosed=false,ipcFdsClosed=false,traceClosed=false;
    pid_t childPid=0;std::uint64_t childStart=0,finishedMs=0;
    int childExit=-1,childSignal=0;const char*refusal="unstarted";
    reader_fixed_pen::Dispatch dispatch{};reader_fixed_pen::Release recovery{};
    reader_fixed_pen_stream::Report stream{};Snapshot initialSnapshot{},snapshot{};
};
namespace detail {
struct Fd {
    int value=-1;bool closed=true;
    Fd()=default;explicit Fd(int fd):value(fd),closed(fd<0){}
    ~Fd(){if(value>=0)::close(value);}
    Fd(const Fd&)=delete;Fd&operator=(const Fd&)=delete;
    bool close()noexcept {if(value<0)return closed;const int fd=value;value=-1;closed=::close(fd)==0;return closed;}
};
inline std::uint64_t processStart(pid_t pid)noexcept {
    char path[64];const int count=std::snprintf(path,sizeof path,"/proc/%ld/stat",static_cast<long>(pid));
    if(count<1||std::size_t(count)>=sizeof path)return 0;
    Fd fd(::open(path,O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC));if(fd.value<0)return 0;
    char bytes[1024];const ssize_t n=::read(fd.value,bytes,sizeof bytes-1);
    if(n<=0||n>=ssize_t(sizeof bytes-1)||!fd.close())return 0;
    bytes[n]=0;char*end=std::strrchr(bytes,')');if(!end||end[1]!=' ')return 0;
    char*save=nullptr;char*token=::strtok_r(end+2," ",&save);
    for(unsigned field=1;field<20&&token;++field)token=::strtok_r(nullptr," ",&save);
    if(!token||!*token)return 0;
    for(const char*p=token;*p;++p)if(*p<'0'||*p>'9')return 0;
    errno=0;char*tail=nullptr;const auto start=std::strtoull(token,&tail,10);
    return !errno&&tail&&!*tail?start:0;
}
struct ChildRecord {
    std::uint32_t magic=0x50454e31,complete=0,attempted=0,contact=0,unknown=0,calls=0,frames=0,records=0,refusal=0;
};
static_assert(sizeof(ChildRecord)<=128,"one bounded atomic pipe record");
template<class IO>struct ChildIO {
    IO&io;int cancel;pid_t parent;
    std::uint64_t nowMs()noexcept{return io.nowMs();}
    bool allowed()noexcept {
        char byte;const ssize_t n=::read(cancel,&byte,1);
        return n<0&&(errno==EAGAIN||errno==EWOULDBLOCK)&&::getppid()==parent&&io.childAllowed();
    }
    reader_fixed_pen::Write emit(const Event*events,std::size_t count)noexcept{return io.emitWriter(events,count);}
    bool pauseMs(unsigned ms)noexcept {pollfd p{cancel,POLLIN|POLLHUP,0};return ::poll(&p,1,int(ms))==0;}
};
template<class IO>[[noreturn]]void child(IO&io,int start,int cancel,int output,pid_t parent,const reader_fixed_pen::Budget&budget)noexcept {
    if(!io.afterForkChild())::_exit(110);
    char command=0;
    for(;;) {
        const auto now=io.nowMs();if(now<budget.began||now>=budget.strokeEnd)::_exit(111);
        const ssize_t n=::read(start,&command,1);
        if(n==1)break;
        if(n==0||!(errno==EAGAIN||errno==EWOULDBLOCK))::_exit(112);
        pollfd p{start,POLLIN|POLLHUP,0};if(::poll(&p,1,2)<0)::_exit(112);
    }
    if(command!='G'||::close(start))::_exit(113);
    reader_fixed_pen::DispatchClaim claim;ChildIO<IO> writer{io,cancel,parent};
    const auto result=reader_fixed_pen::dispatch(writer,budget,claim);
    ChildRecord record;record.complete=result.complete;record.attempted=result.attempted;
    record.contact=result.contactPossible;record.unknown=result.unknown;record.calls=std::uint32_t(result.calls);
    record.frames=std::uint32_t(result.frames);record.records=std::uint32_t(result.acceptedRecords);record.refusal=std::uint32_t(result.refusal);
    if(::write(output,&record,sizeof record)!=ssize_t(sizeof record))::_exit(114);
    ::_exit(result.complete?0:115); // Never run Qt/heap/destructor unwinding after fork.
}
template<class IO>struct RecoveryIO {
    IO&io;Stream&stream;
    std::uint64_t nowMs()noexcept{return io.nowMs();}
    bool identityValid()noexcept{return io.identityValid();}
    bool released()noexcept{return io.releasedState();}
    bool observerEmpty()noexcept{return io.observerEmpty();}
    reader_fixed_pen::Write emit(const Event*events,std::size_t count)noexcept{return io.emitRecovery(events,count);}
    bool confirmReleased(std::uint64_t end)noexcept {
        if(!stream.report().valid)return false;
        return io.confirmReleased(stream,end)&&stream.report().valid&&stream.report().finalized&&stream.report().finalReleased;
    }
};
}

// IO owns separately opened device/trace descriptors. Methods are plain bounded
// syscalls or explicit owned models, not arbitrary application callbacks.
template<class IO>Result run(IO&io,const reader_fixed_pen::Budget&budget)noexcept {
    Result r;
    const auto finish=[&](const char*why) {
        r.refusal=why;const bool closed=io.finish();
        r.deviceFdsClosed=io.deviceFdsClosed();r.traceClosed=io.traceClosed();
        const auto before=io.nowMs();r.contextLive=io.context();r.finishedMs=io.nowMs();
        if(r.finishedMs<before)r.accepted=false;
        if(!closed||!r.deviceFdsClosed||!r.traceClosed)r.accepted=false;
        return r;
    };
    struct sigaction childDisposition{};
    if(::sigaction(SIGCHLD,nullptr,&childDisposition)||childDisposition.sa_handler!=SIG_DFL||
       (childDisposition.sa_flags&SA_NOCLDWAIT))return finish("child-reaper");
    if(!budget.valid())return finish("budget");
    Snapshot first{},second{};
    if(!io.snapshot(first)||!io.snapshot(second)||!reader_fixed_pen_stream::same(first,second)||
       !reader_fixed_pen_stream::released(first)||first.tiltX||first.tiltY)return finish("initial-snapshot");
    r.initialSnapshotsMatch=true;r.initialSnapshot=first;r.snapshot=first;
    if(!io.observerEmpty())return finish("initial-queue");
    r.initialQueueEmpty=true;
    Stream stream(first);reader_fixed_pen::Recovery recovery(budget);detail::RecoveryIO<IO> recoveryIO{io,stream};
    if(!io.context()||!recovery.arm(recoveryIO))return finish("recovery-arm");
    r.armed=true;
    int sp[2],cp[2],rp[2];
    if(::pipe2(sp,O_CLOEXEC|O_NONBLOCK))return finish("start-pipe");
    detail::Fd startRead(sp[0]),startWrite(sp[1]);
    if(::pipe2(cp,O_CLOEXEC|O_NONBLOCK))return finish("cancel-pipe");
    detail::Fd cancelRead(cp[0]),cancelWrite(cp[1]);
    if(::pipe2(rp,O_CLOEXEC|O_NONBLOCK))return finish("result-pipe");
    detail::Fd resultRead(rp[0]),resultWrite(rp[1]);
    const pid_t parent=::getpid();r.childPid=::fork();
    if(r.childPid<0){r.childPid=0;return finish("fork");}
    if(!r.childPid) {
        if(!startWrite.close()||!cancelWrite.close()||!resultRead.close())::_exit(109);
        detail::child(io,startRead.value,cancelRead.value,resultWrite.value,parent,budget);
    }
    bool ipcGood=startRead.close();ipcGood=cancelRead.close()&&ipcGood;ipcGood=resultWrite.close()&&ipcGood;
    const bool parentReady=io.afterForkParent();r.childStart=detail::processStart(r.childPid);
    bool killSent=false;bool childOwnership=true;bool contextGood=true;std::uint64_t last=budget.began;
    const auto stop=[&] {
        ipcGood=cancelWrite.close()&&ipcGood;
        if(!killSent){killSent=true;if(::kill(r.childPid,SIGKILL)&&errno!=ESRCH)r.observationGood=false;}
    };
    const auto now=io.nowMs();
    if(ipcGood&&parentReady&&r.childStart&&io.context()&&now>=budget.began&&now<budget.strokeEnd) {
        const char go='G';r.childStarted=::write(startWrite.value,&go,1)==1;
    }
    ipcGood=startWrite.close()&&ipcGood;
    if(!r.childStarted)stop();
    while(!r.childGone) {
        if(!io.observe(stream)){r.observationGood=false;stop();}
        int status=0;
#ifdef READER_FIXED_PEN_HOLDER_TESTING
        const pid_t waited=io.testWaitChild(r.childPid,&status,WNOHANG);
#else
        const pid_t waited=::waitpid(r.childPid,&status,WNOHANG);
#endif
        if(waited==r.childPid){r.childGone=true;r.childStatusKnown=true;if(WIFEXITED(status))r.childExit=WEXITSTATUS(status);else if(WIFSIGNALED(status))r.childSignal=WTERMSIG(status);break;}
        if(waited<0){r.observationGood=false;childOwnership=false;break;}
        const auto at=io.nowMs();
        if(at<last){r.observationGood=false;stop();break;}last=at;
        if(!io.context()){contextGood=false;stop();}
        if(at>=budget.strokeEnd)stop();
        if(at>=budget.recoveryEnd)break;
        if(!io.waitParent(2)){r.observationGood=false;stop();}
    }
    if(!r.childGone) {
        // An unknown wait result loses kill authority: never signal a possibly
        // reaped/reused raw PID. Closing cancel remains a passive EOF signal.
        if(childOwnership)stop();
        startRead.close();startWrite.close();cancelRead.close();cancelWrite.close();resultRead.close();resultWrite.close();
        r.diagnostic=true;r.refusal="child-not-fenced";r.finishedMs=io.nowMs();r.stream=stream.report();
        // No recovery write, close or positive result. Caller retains device
        // holders, publishes diagnostic only and may reap under outer setup end.
        return r;
    }
    if(!io.observe(stream))r.observationGood=false;
    detail::ChildRecord record{};std::array<unsigned char,sizeof record+1> bytes{};
    const ssize_t received=::read(resultRead.value,bytes.data(),bytes.size());
    if(received==ssize_t(sizeof record)) {
        std::memcpy(&record,bytes.data(),sizeof record);
        r.childResultKnown=record.magic==0x50454e31&&record.complete<=1&&record.attempted<=1&&record.contact<=1&&record.unknown<=1&&
            record.calls<=56&&record.frames<=record.calls&&record.records<=178&&record.refusal<=std::uint32_t(reader_fixed_pen::Refusal::InvalidFrame);
    }
    if(r.childResultKnown) {
        r.dispatch.complete=record.complete;r.dispatch.attempted=record.attempted;r.dispatch.contactPossible=record.contact;
        r.dispatch.unknown=record.unknown;r.dispatch.calls=record.calls;r.dispatch.frames=record.frames;r.dispatch.acceptedRecords=record.records;
        r.dispatch.refusal=reader_fixed_pen::Refusal(record.refusal);
    }else {r.dispatch.unknown=r.childStarted;r.dispatch.contactPossible=r.childStarted;}
    // Stream failure/cancellation cannot revoke independent original-budget release.
    const auto releaseAt=io.nowMs();
    if(releaseAt<last||releaseAt>=budget.recoveryEnd) {
        r.observationGood=false;r.recovery.unknown=r.childStarted;r.recovery.refusal=reader_fixed_pen::Refusal::Budget;
    }else r.recovery=recovery.release(recoveryIO);
    r.snapshot=io.finalSnapshot();r.stream=stream.report();
    ipcGood=startRead.close()&&ipcGood;ipcGood=startWrite.close()&&ipcGood;
    ipcGood=cancelRead.close()&&ipcGood;ipcGood=cancelWrite.close()&&ipcGood;
    ipcGood=resultRead.close()&&ipcGood;ipcGood=resultWrite.close()&&ipcGood;r.ipcFdsClosed=ipcGood;
    const auto ended=io.nowMs();
    r.accepted=contextGood&&io.context()&&r.childStarted&&r.childResultKnown&&r.childExit==0&&r.childSignal==0&&
        r.dispatch.complete&&!r.dispatch.unknown&&r.dispatch.calls==56&&r.dispatch.frames==56&&r.dispatch.acceptedRecords==178&&
        r.observationGood&&r.recovery.observed&&r.stream.strokeObserved&&io.quietConfirmed()&&io.snapshotMatched()&&ipcGood&&
        ended>=budget.began&&ended<budget.recoveryEnd;
    r=finish(r.accepted?"":"not-accepted");
    // Post-close/context callouts cannot spend or revive the original budget.
    if(r.finishedMs<ended||r.finishedMs>=budget.recoveryEnd||!r.contextLive)r.accepted=false;
    if(!r.accepted&&!*r.refusal)r.refusal="finish-boundary";
    return r;
}
} // namespace reader_fixed_pen_holder
#pragma pop_macro("emit")
