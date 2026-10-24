// Original deterministic IO models and owned pipes/processes; never input devices.
#include "reader_fixed_stroke_policy.h"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <thread>
#include <vector>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {
using namespace reader_fixed_pen;
static_assert(!std::is_copy_constructible<Recovery>::value&&!std::is_move_constructible<Recovery>::value,"one recovery holder cannot be copied or moved");
static_assert(!std::is_copy_constructible<DispatchClaim>::value&&!std::is_move_constructible<DispatchClaim>::value,"one stroke claim cannot be copied or moved");
unsigned checks=0,scenarios=0;
void require(bool value,const char *message) {++checks;if(!value)throw std::runtime_error(message);}
bool eventEqual(const Event&a,const Event&b) {return a.type==b.type&&a.code==b.code&&a.value==b.value;}
bool frameEqual(const Frame&a,const Frame&b) {
    if(a.count!=b.count||a.delayMs!=b.delayMs)return false;
    for(std::size_t i=0;i<a.count;++i)if(!eventEqual(a.events[i],b.events[i]))return false;
    return true;
}
struct IO {
    std::uint64_t time=100;bool context=true,identity=true,isReleased=true,empty=true,pause=true,confirmed=true;
    unsigned clocks=0,allows=0,identities=0,releasedCalls=0,emptyCalls=0,writes=0,pauses=0,confirms=0;
    std::uint64_t confirmationDeadline=0;
    std::function<void(const char*,unsigned)> hook;
    std::function<Write(std::size_t,unsigned)> writeResult;
    std::vector<Frame> emitted;
    void notify(const char *stage,unsigned ordinal){if(hook)hook(stage,ordinal);}
    std::uint64_t nowMs(){notify("clock",++clocks);return time;}
    bool allowed(){notify("allowed",++allows);return context;}
    bool identityValid(){notify("identity",++identities);return identity;}
    bool released(){notify("released",++releasedCalls);return isReleased;}
    bool observerEmpty(){notify("empty",++emptyCalls);return empty;}
    Write emit(const Event*events,std::size_t count) {
        ++writes;Frame f{};f.count=count;require(count<=f.events.size(),"model emitter bound");
        std::copy(events,events+count,f.events.begin());emitted.push_back(f);notify("emit",writes);
        return writeResult?writeResult(count,writes):Write{std::ptrdiff_t(count*KernelRecordBytes),0};
    }
    bool pauseMs(unsigned delay){notify("pause",++pauses);time+=delay;return pause;}
    bool confirmReleased(std::uint64_t end){confirmationDeadline=end;notify("confirm",++confirms);return confirmed;}
};
template<class T>Dispatch invokeDispatch(T &io,const Budget &budget) {DispatchClaim claim;return dispatch(io,budget,claim);}
template<class F>void scenario(const std::string&name,F fn) {
    fn();++scenarios;std::cout<<"PASS "<<name<<'\n';
}
void fixedFrames() {
    const auto fs=strokeFrames();std::size_t records=0;unsigned delay=0;
    require(fs.size()==56&&recordCount()==178,"exact frame/record count");
    for(const auto&f:fs){require(f.count>0&&f.count<=7,"all fixed frame counts bounded");require(f.events[f.count-1].type==0&&f.events[f.count-1].code==0&&f.events[f.count-1].value==0,"each fixed frame ends SYN_REPORT");records+=f.count;delay+=f.delayMs;}
    require(records==178&&delay==92,"exact fixed records/delays");
    const auto release=releaseFrame();require(release.count==5,"release five records");
    require(eventEqual(release.events[0],{3,24,0})&&eventEqual(release.events[1],{3,25,100})&&eventEqual(release.events[2],{1,330,0})&&eventEqual(release.events[3],{1,320,0})&&eventEqual(release.events[4],{0,0,0}),"exact contact-off release, no XY/eraser");
    require(frameEqual(fs[0],release)&&frameEqual(fs[55],release),"initial/final release exact");
    require(fs[1].events[0].value==11138&&fs[1].events[1].value==7371&&fs[1].count==7,"fixed source point hover");
    for(int i=1;i<=52;++i){const auto&f=fs[std::size_t(i+2)];require(f.count==3&&eventEqual(f.events[0],{3,0,11138+(-82*i)/52})&&eventEqual(f.events[1],{3,1,7371+(245*i)/52}),"all52 source-derived interpolation points");}
    require(fs[54].events[0].value==11056&&fs[54].events[1].value==7616,"fixed endpoint");
}
void dispatchControls() {
    scenario("fixed-frames",fixedFrames);
    scenario("oversize-frame",[]{const auto f=frame({{},{},{},{},{},{},{},{}},1);require(f.count==0,"oversize builder explicitly invalid");});
    scenario("budget",[]{auto b=reserve(100);require(b.valid()&&b.strokeEnd==1100&&b.recoveryEnd==3100,"original deadlines");require(!reserve(UINT64_MAX-2999).valid(),"overflow refused");require(reserve(UINT64_MAX-3000).valid(),"last nonoverflow budget valid");IO io;auto r=invokeDispatch(io,{});require(!r.attempted&&!r.unknown&&r.refusal==Refusal::Budget&&!io.writes,"invalid no dispatch");});
    scenario("complete-accepted-writes-only",[]{IO io;auto r=invokeDispatch(io,reserve(100));require(r.complete&&r.attempted&&r.contactPossible&&!r.unknown&&r.refusal==Refusal::None,"complete flags");require(r.calls==56&&r.frames==56&&r.acceptedRecords==178&&io.writes==56&&io.pauses==56,"exact nonretry counts");require(io.allows==168&&io.clocks==336&&io.time==192,"permission/clock original accounting");});
    for(unsigned at:{1u,2u,3u,28u,56u})for(int kind=0;kind<6;++kind)scenario("write-fault-"+std::to_string(at)+"-"+std::to_string(kind),[=]{
        IO io;io.writeResult=[=](std::size_t n,unsigned call){const auto full=std::ptrdiff_t(n*16);if(call!=at)return Write{full,0};switch(kind){case 0:return Write{-1,EINTR};case 1:return Write{0,0};case 2:return Write{16,0};case 3:return Write{7,0};case 4:return Write{full+16,0};default:return Write{full,EIO};}};
        auto r=invokeDispatch(io,reserve(100));require(!r.complete&&r.attempted&&r.unknown&&r.calls==at&&r.frames==at-1&&io.writes==at,"failure consumes one call and stops without retry");require(r.contactPossible==(at>=3),"contact possibility conservative");require(io.pauses==at-1,"no pause/further frame after rejected write");
        std::size_t prior=0;const auto fs=strokeFrames();for(unsigned i=0;i<at-1;++i)prior+=fs[i].count;
        require(r.acceptedRecords==prior+(kind==2?1u:0u),"only complete accepted record counts retained; partial bytes stay ambiguous");
    });
    for(unsigned at:{1u,2u,7u,8u})scenario("context-"+std::to_string(at),[=]{IO io;io.hook=[&](const char*s,unsigned n){if(std::strcmp(s,"allowed")==0&&n==at)io.context=false;};auto r=invokeDispatch(io,reserve(100));require(!r.complete&&r.refusal==Refusal::Context&&r.unknown==r.attempted,"context refusal retains ambiguity");require(io.writes==(at==1?0:at==2?1:at==7?2:3),"context prevents next frame");});
    for(const auto stage:{"allowed","emit","pause"})for(bool regress:{false,true})scenario(std::string("clock-after-")+stage+(regress?"-regress":"-expiry"),[=]{IO io;io.hook=[&](const char*s,unsigned n){if(std::strcmp(s,stage)==0&&n==1)io.time=regress?0:1100;};auto r=invokeDispatch(io,reserve(100));require(!r.complete&&r.refusal==(regress?Refusal::Clock:Refusal::Budget),"post-callout clock rejected");require(io.writes==(std::strcmp(stage,"allowed")==0?0u:1u),"no next dispatch after clock loss");});
    scenario("pause-false",[]{IO io;io.pause=false;auto r=invokeDispatch(io,reserve(100));require(r.refusal==Refusal::Pause&&r.unknown&&io.writes==1,"pause failure stops");});
    for(const auto stage:{"clock","allowed","emit","pause"})scenario(std::string("throw-")+stage,[=]{IO io;io.hook=[=](const char*s,unsigned){if(std::strcmp(s,stage)==0)throw std::runtime_error("owned");};auto r=invokeDispatch(io,reserve(100));require(r.refusal==Refusal::Exception&&!r.complete&&r.unknown==r.attempted,"exception retained");require(io.writes<=1,"exception no repeat");});
}
void claimControls() {
    scenario("stroke-claim-repeated",[]{IO io;DispatchClaim claim;auto first=dispatch(io,reserve(100),claim);const auto clocks=io.clocks;auto second=dispatch(io,reserve(100),claim);require(first.complete&&first.contactPossible&&claim.spent(),"first stroke consumed claim");require(second.refusal==Refusal::Spent&&!second.attempted&&!second.unknown&&io.writes==56&&io.clocks==clocks,"repeat no IO and original facts retained");});
    for(const auto stage:{"clock","allowed","emit","pause"})scenario(std::string("stroke-claim-reentry-")+stage,[=]{IO io;DispatchClaim claim;Dispatch nested;bool once=false;io.hook=[&](const char*s,unsigned){if(!once&&std::strcmp(s,stage)==0){once=true;nested=dispatch(io,reserve(100),claim);}};auto first=dispatch(io,reserve(100),claim);require(first.complete&&once&&nested.refusal==Refusal::Spent&&!nested.attempted&&io.writes==56,"claim consumed before every IO/reentrant entry");});
    scenario("stroke-claim-noeffect-no-renewal",[]{IO io;DispatchClaim claim;io.context=false;auto first=dispatch(io,reserve(100),claim);require(!first.attempted&&!first.unknown&&first.refusal==Refusal::Context&&claim.spent(),"even no-effect refusal consumes one attempt");io.context=true;io.time=200;const auto clocks=io.clocks;auto second=dispatch(io,reserve(200),claim);require(second.refusal==Refusal::Spent&&io.clocks==clocks&&!io.writes,"new budget does not renew old claim");});
    scenario("stroke-claim-invalidbudget",[]{IO io;DispatchClaim claim;auto first=dispatch(io,{},claim);require(first.refusal==Refusal::Budget&&claim.spent()&&!io.clocks,"bad budget is spent before IO");require(dispatch(io,reserve(100),claim).refusal==Refusal::Spent&&!io.writes,"invalid attempt never revived");});
    scenario("stroke-claim-exception",[]{IO io;DispatchClaim claim;io.hook=[](const char*,unsigned){throw std::runtime_error("owned");};auto first=dispatch(io,reserve(100),claim);io.hook={};require(first.refusal==Refusal::Exception&&!first.attempted&&dispatch(io,reserve(100),claim).refusal==Refusal::Spent&&!io.writes,"prewrite throw cannot revive claim");});
    scenario("stroke-claim-competing-owned-threads",[]{for(int run=0;run<16;++run){IO io;DispatchClaim claim;std::atomic<unsigned> ready{0};std::atomic<bool> go{false};std::array<Dispatch,2> results{};std::array<std::thread,2> threads;for(unsigned i=0;i<2;++i)threads[i]=std::thread([&,i]{ready.fetch_add(1,std::memory_order_release);while(!go.load(std::memory_order_acquire))std::this_thread::yield();results[i]=dispatch(io,reserve(100),claim);});while(ready.load(std::memory_order_acquire)!=2)std::this_thread::yield();go.store(true,std::memory_order_release);for(auto&t:threads)t.join();require(unsigned(results[0].complete)+unsigned(results[1].complete)==1,"one competing claim winner");require(unsigned(results[0].refusal==Refusal::Spent)+unsigned(results[1].refusal==Refusal::Spent)==1&&io.writes==56&&io.emitted.size()==56,"claim loser never reaches shared IO");}});
}
void recoveryControls() {
    scenario("release-not-armed-spent",[]{IO io;Recovery r(reserve(100));auto x=r.release(io);require(x.refusal==Refusal::NotArmed&&!x.attempted&&!x.observed,"unarmed refuses");require(!r.arm(io)&&r.release(io).refusal==Refusal::Spent,"cannot revive spent guard");});
    scenario("arm-one-attempt",[]{IO io;Recovery r(reserve(100));require(r.arm(io),"armed");const auto calls=io.clocks;require(!r.arm(io)&&io.clocks==calls,"arming never renewed");auto x=r.release(io);require(x.attempted&&x.written&&x.observed&&!x.unknown&&io.writes==1&&io.confirmationDeadline==3100,"one positively observed modeled release");require(r.release(io).refusal==Refusal::Spent&&io.writes==1,"exact once release");});
    for(const auto stage:{"identity","released","empty"})scenario(std::string("arm-failure-")+stage,[=]{IO io;Recovery r(reserve(100));if(std::strcmp(stage,"identity")==0)io.identity=false;else if(std::strcmp(stage,"released")==0)io.isReleased=false;else io.empty=false;require(!r.arm(io),"arm input refusal");io.identity=io.isReleased=io.empty=true;require(!r.arm(io),"failed arm not retried");});
    for(const auto stage:{"identity","released","empty"})for(bool regress:{false,true})scenario(std::string("arm-clock-")+stage+(regress?"-regress":"-expiry"),[=]{IO io;Recovery r(reserve(100));io.hook=[&](const char*s,unsigned){if(std::strcmp(s,stage)==0)io.time=regress?99:1100;};require(!r.arm(io),"arming callout cannot consume or regress deadline");require(!r.release(io).observed&&!io.writes,"failed arming gives no release proof");});
    scenario("arm-reentry-release",[]{IO io;Recovery r(reserve(100));bool inside=false,nestedArm=true;Release nested;io.hook=[&](const char*s,unsigned){if(std::strcmp(s,"identity")==0&&!inside){inside=true;nestedArm=r.arm(io);nested=r.release(io);}};require(!r.arm(io)&&!nestedArm&&nested.refusal==Refusal::NotArmed&&!io.writes,"reentrant arming/release cannot leave outer armed after spent");});
    for(unsigned at:{1u,2u})scenario("arm-clock-reentry-"+std::to_string(at),[=]{IO io;Recovery r(reserve(100));io.hook=[&](const char*s,unsigned n){if(std::strcmp(s,"clock")==0&&n==at)require(r.release(io).refusal==Refusal::NotArmed,"reentry consumes inactive recovery");};require(!r.arm(io)&&!io.writes,"clock callback cannot resurrect spent recovery");});
    scenario("release-original-budget",[]{IO io;Recovery r(reserve(100));require(r.arm(io),"arm");io.time=3100;auto x=r.release(io);require(x.refusal==Refusal::Budget&&!x.attempted&&!x.unknown&&!x.observed,"expiry means unresolved without dispatch ambiguity");require(r.release(io).refusal==Refusal::Spent,"expired release not retried");});
    scenario("release-device-loss",[]{IO io;Recovery r(reserve(100));require(r.arm(io),"arm");io.identity=false;auto x=r.release(io);require(x.refusal==Refusal::Device&&!x.attempted&&!x.observed&&!x.unknown,"identity loss cannot authorize close even before write");});
    for(int kind=0;kind<6;++kind)scenario("release-write-"+std::to_string(kind),[=]{IO io;Recovery r(reserve(100));require(r.arm(io),"arm");io.writeResult=[=](std::size_t n,unsigned){const auto full=std::ptrdiff_t(n*16);switch(kind){case 0:return Write{-1,EINTR};case 1:return Write{0,0};case 2:return Write{16,0};case 3:return Write{7,0};case 4:return Write{full+16,0};default:return Write{full,EIO};}};auto x=r.release(io);require(x.attempted&&!x.written&&!x.observed&&x.unknown&&io.confirms==0&&io.writes==1,"release error/zero/partial never retries or observes");require(r.release(io).refusal==Refusal::Spent,"failure remains spent");});
    for(const auto stage:{"emit","confirm"})scenario(std::string("release-throw-")+stage,[=]{IO io;Recovery r(reserve(100));require(r.arm(io),"arm");io.hook=[=](const char*s,unsigned){if(std::strcmp(s,stage)==0)throw std::runtime_error("owned");};auto x=r.release(io);require(x.attempted&&x.unknown&&!x.observed&&x.refusal==Refusal::Exception&&io.writes==1,"release exception unknown/spent");});
    for(const auto stage:{"emit","confirm","final-identity"})for(bool regress:{false,true})scenario(std::string("release-clock-")+stage+(regress?"-regress":"-expiry"),[=]{IO io;Recovery r(reserve(100));require(r.arm(io),"arm");io.identities=0;io.hook=[&](const char*s,unsigned n){if((std::strcmp(s,stage)==0)||(std::strcmp(stage,"final-identity")==0&&std::strcmp(s,"identity")==0&&n==2))io.time=regress?0:3100;};auto x=r.release(io);require(x.attempted&&x.written&&!x.observed&&x.unknown,"all post-callout release deadline/monotonic checks");if(std::strcmp(stage,"emit")==0)require(io.confirms==0,"expired emitter cannot start observer confirmation");});
    scenario("release-observer-false",[]{IO io;Recovery r(reserve(100));require(r.arm(io),"arm");io.confirmed=false;auto x=r.release(io);require(x.written&&!x.observed&&x.unknown&&x.refusal==Refusal::ReleaseObservation,"write accepted is not release observed");});
    scenario("release-reentry-once",[]{IO io;Recovery r(reserve(100));require(r.arm(io),"arm");Release nested;io.hook=[&](const char*s,unsigned){if(std::strcmp(s,"emit")==0)nested=r.release(io);};auto x=r.release(io);require(x.observed&&nested.refusal==Refusal::Spent&&io.writes==1,"release latch before write callout");});
}

struct Raw16 {std::array<unsigned char,8> timestamp{};std::uint16_t type=0,code=0;std::int32_t value=0;};
static_assert(sizeof(Raw16)==16&&alignof(Raw16)==4,"owned record carrier only, not vendor ABI proof");
struct PipeIO:IO {
    int writer=-1,reader=-1;bool dieAfterDown=false;std::size_t expectedRecords=0;std::vector<Raw16> observed;
    Write emit(const Event*events,std::size_t count) {
        const auto model=IO::emit(events,count);if(model.error||model.bytes<0)return model;
        std::array<Raw16,7> records{};for(std::size_t i=0;i<count;++i){records[i].type=events[i].type;records[i].code=events[i].code;records[i].value=events[i].value;}
        const auto n=::write(writer,records.data(),count*sizeof(Raw16));const int error=n<0?errno:0;
        if(dieAfterDown&&writes==3)::_exit(23);
        return {n,error};
    }
    bool confirmReleased(std::uint64_t end) {
        confirmationDeadline=end;++confirms;std::array<Raw16,256> records{};
        const auto n=::read(reader,records.data(),sizeof records);
        if(n<0||n%16)return false;
        observed.assign(records.begin(),records.begin()+n/16);
        if(observed.size()!=expectedRecords||observed.size()<5)return false;
        const auto release=releaseFrame();for(std::size_t i=0;i<5;++i){const auto&r=observed[observed.size()-5+i];if(r.type!=release.events[i].type||r.code!=release.events[i].code||r.value!=release.events[i].value)return false;}
        return true;
    }
};
void pipes() {
    scenario("owned-child-exit-retained-parent-fd",[]{int fds[2];require(::pipe(fds)==0,"pipe");require(::fcntl(fds[0],F_SETFL,O_NONBLOCK)==0,"bounded nonblocking observer");PipeIO io;io.reader=fds[0];io.writer=fds[1];io.expectedRecords=22;Recovery recovery(reserve(100));require(recovery.arm(io),"parent release prearmed");struct stat before{},after{};require(::fstat(io.writer,&before)==0,"parent descriptor identity");const auto child=::fork();require(child>=0,"owned fork");if(child==0){io.dieAfterDown=true;const auto r=invokeDispatch(io,reserve(100));::_exit(r.complete?0:24);}int status=0;require(::waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==23,"child stopped after accepted down frame");require(::fstat(io.writer,&after)==0&&before.st_dev==after.st_dev&&before.st_ino==after.st_ino,"parent writer remains bound after child exit");auto r=recovery.release(io);require(r.observed&&r.written&&io.writes==1&&io.observed.size()==22,"one parent release after child death, owned stream model");require(recovery.release(io).refusal==Refusal::Spent&&io.writes==1,"no second release");require(::close(fds[0])==0&&::close(fds[1])==0,"owned descriptors closed");});
    scenario("owned-writer-fd-loss-independent-release",[]{int fds[2];require(::pipe(fds)==0,"pipe");require(::fcntl(fds[0],F_SETFL,O_NONBLOCK)==0,"nonblocking");const int writer=::dup(fds[1]);require(writer>=0&&::close(writer)==0,"separate child writer close");PipeIO stroke;stroke.writer=writer;auto d=invokeDispatch(stroke,reserve(100));require(!d.complete&&d.attempted&&d.unknown&&stroke.writes==1,"real EBADF stops one write");PipeIO parent;parent.writer=fds[1];parent.reader=fds[0];parent.expectedRecords=5;Recovery recovery(reserve(100));require(recovery.arm(parent),"independent parent arm");require(recovery.release(parent).observed&&parent.writes==1,"surviving descriptor release in owned stream only");require(::close(fds[0])==0&&::close(fds[1])==0,"closed");});
}
int historicalRed() {
    IO io;Recovery r(reserve(100));bool inside=false,nested=false;Release release;
    io.hook=[&](const char*s,unsigned){if(std::strcmp(s,"identity")==0&&!inside){inside=true;nested=r.arm(io);release=r.release(io);}};
    const bool outer=r.arm(io);require(outer&&nested&&release.observed,"old arm reentry counterexample");
    IO second;Recovery q(reserve(100));require(q.arm(second),"old release arm");second.identities=0;second.hook=[&](const char*s,unsigned n){if(std::strcmp(s,"identity")==0&&n==2)second.time=3100;};const auto result=q.release(second);
    require(result.observed,"old final identity deadline counterexample");
    std::cout<<"REPRODUCED old_arm_after_spent=true old_release_after_deadline_observed=true; models_only=true\n";return 0;
}
}
int main(int argc,char**argv) {
    try{if(argc==2&&std::string(argv[1])=="--historical-red")return historicalRed();dispatchControls();claimControls();recoveryControls();pipes();std::cout<<"PASS fixed stroke policy scenarios="<<scenarios<<" checks="<<checks<<" frames=56 records=178; modeled_io=true owned_pipe_child=true native_execution=false\n";return 0;}
    catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<" after scenarios="<<scenarios<<" checks="<<checks<<'\n';return 1;}
}
