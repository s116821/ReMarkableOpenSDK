// Original owned ARM/QThread controls. No firmware bodies or native resolution.
#include "reader_insertion_guard_hooks.h"
#include <QCoreApplication>
#include <QThread>
#include <dobby.h>
#include <condition_variable>
#include <cstdio>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string>
#include <limits>
#include <climits>
#include <sys/mman.h>
#include <unistd.h>

using namespace reader_insertion_hooks;
extern "C" void owned_outer_entry(void*,int,const void*,Word,QSizeF,int,Word,const QUuid&);
extern "C" void owned_inner_entry(void*,int,const QString&,const QString&,std::int32_t,std::int32_t);
extern "C" void invoke_outer(void*,int,const void*,Word,QSizeF,int,Word,const QUuid&);
extern "C" void invoke_inner(void*,int,const QString&,const QString&,std::int32_t,std::int32_t);
extern "C" char outer_return[],inner_return[];
// Original generic instruction families and public carrier forwarding, not native code.
asm(R"(
.syntax unified
.arch armv7-a
.fpu neon-vfpv4
.arm
.text
.align 2
.global owned_outer_entry
.type owned_outer_entry,%function
owned_outer_entry:
.fnstart
 push {r4-r11,lr}
.save {r4-r11,lr}
 mov r4,r0
 vpush {d8-d9}
.vsave {d8-d9}
 sub sp,sp,#12
.pad #12
 ldr r12,[sp,#64]
 str r12,[sp,#0]
 ldr r12,[sp,#68]
 str r12,[sp,#4]
 ldr r12,[sp,#72]
 str r12,[sp,#8]
 bl owned_outer_body
 add sp,sp,#12
 vpop {d8-d9}
 pop {r4-r11,pc}
.fnend
.size owned_outer_entry,.-owned_outer_entry
.align 2
.global owned_inner_entry
.type owned_inner_entry,%function
owned_inner_entry:
.fnstart
 push {r4-r11,lr}
.save {r4-r11,lr}
 mov r5,r3
 vpush {d8-d9}
.vsave {d8-d9}
 sub sp,sp,#12
.pad #12
 ldr r12,[sp,#64]
 str r12,[sp,#0]
 ldr r12,[sp,#68]
 str r12,[sp,#4]
 bl owned_inner_body
 add sp,sp,#12
 vpop {d8-d9}
 pop {r4-r11,pc}
.fnend
.size owned_inner_entry,.-owned_inner_entry
.align 2
.global invoke_outer
.type invoke_outer,%function
invoke_outer:
.fnstart
 push {r4,lr}
.save {r4,lr}
 sub sp,sp,#16
.pad #16
 ldr r12,[sp,#24]
 str r12,[sp,#0]
 ldr r12,[sp,#28]
 str r12,[sp,#4]
 ldr r12,[sp,#32]
 str r12,[sp,#8]
 bl owned_outer_entry
.global outer_return
outer_return:
 add sp,sp,#16
 pop {r4,pc}
.fnend
.size invoke_outer,.-invoke_outer
.align 2
.global invoke_inner
.type invoke_inner,%function
invoke_inner:
.fnstart
 push {r4,lr}
.save {r4,lr}
 sub sp,sp,#8
.pad #8
 ldr r12,[sp,#16]
 str r12,[sp,#0]
 ldr r12,[sp,#20]
 str r12,[sp,#4]
 bl owned_inner_entry
.global inner_return
inner_return:
 add sp,sp,#8
 pop {r4,pc}
.fnend
.size invoke_inner,.-invoke_inner
.text
)");
static unsigned checks=0;
static void check(bool value,const char *what) { ++checks; if(!value){std::fprintf(stderr,"FAILED %s\n",what);std::abort();} }
static std::string scenario;
static std::string testName;
static bool postRequested=false;
static unsigned postTries=0,postUnlocks=0;
static Core *core=nullptr;
static unsigned originalOuterCalls=0,originalInnerCalls=0,cleanup=0,hookCalls=0,submissions=0,attributions=0;
static void *firstTrampoline=nullptr;
static bool outsideThrow=false,lockFree=false,forwarding=true,nested=false;
alignas(8) static unsigned char workerStorage[512];
static void *worker=workerStorage,*map=workerStorage+200;
static int optional=7;
static const void *borrowedOptional=nullptr;
static int borrowedSource=0;
static QSizeF borrowedSize;
static bool borrowedScenario() { return scenario.rfind("borrowed-",0)==0; }
static bool badRegistration() { return scenario.rfind("borrowed-reg-",0)==0; }
static bool borrowedPositive() {
    return scenario=="borrowed-stack-a" || scenario=="borrowed-stack-b" ||
        scenario=="borrowed-stack-c" || scenario=="borrowed-boundary";
}
static const QString real=QStringLiteral("11111111-1111-4111-8111-111111111111");
static const QString right=QStringLiteral("22222222-2222-4222-8222-222222222222");
static const QString wrong=QStringLiteral("33333333-3333-4333-8333-333333333333");
static const QString target=QStringLiteral("44444444-4444-4444-8444-444444444444");
static const QUuid targetId(target),foreignId(QStringLiteral("55555555-5555-4555-8555-555555555555"));
static std::mutex mapMutex;
extern "C" void owned_inner_body(void *m,int i,const QString &u,const QString &t,std::int32_t a,std::int32_t b) {
    ++originalInnerCalls;
    forwarding=forwarding && m==map && i==2 && !u.isEmpty() && t==real && a==-123456789 && b==0x7f123456;
    if(scenario=="unrelated-inner") throw std::runtime_error("owned unrelated inner");
}
extern "C" void owned_outer_body(void *w,int i,const void *o,Word p,QSizeF z,int s,Word h,const QUuid &u) {
    ++originalOuterCalls;
    if(borrowedScenario())
        forwarding=forwarding && w==worker && i==2 && o==borrowedOptional && p==1 && h==0 &&
            std::memcmp(&s,&borrowedSource,sizeof s)==0 && std::memcmp(&z,&borrowedSize,sizeof z)==0;
    else
        forwarding=forwarding && w==worker && i==2 && o==&optional && p==0x7f123401 && z==QSizeF(1404.25,1872.5) && s==1 && h==0x7e123400;
    if(scenario=="unrelated-outer") throw std::runtime_error("owned unrelated outer");
    if(scenario=="nested" && !nested) { nested=true; invoke_outer(w,i,o,p,z,s,h,foreignId); nested=false; }
    std::unique_lock<std::mutex> lock(mapMutex);
    QString temporary=QStringLiteral("owned temporary");
    struct Cleanup { ~Cleanup(){++cleanup;} } counted;
    if(testName=="post-normal")return;
    if(testName=="post-busy")lock.release(); // Owned defect control: original fails to unlock.
    if(scenario=="cancel-inner") core->cancel();
    const QString effective=scenario=="transformed-target" ? right : u.toString(QUuid::WithoutBraces);
    const int effectiveIndex=scenario=="transformed-index" ? i+1 : i;
    if(scenario=="wrong-callsite") owned_inner_entry(map,effectiveIndex,effective,real,-123456789,0x7f123456);
    else if(scenario=="duplicate-inner") {
        try {invoke_inner(map,effectiveIndex,effective,real,-123456789,0x7f123456);}catch(...){}
        invoke_inner(map,effectiveIndex,effective,real,-123456789,0x7f123456);
    } else invoke_inner(map,effectiveIndex,effective,real,-123456789,0x7f123456);
    check(!temporary.isEmpty(),"owned temporary alive on normal path");
}
static bool query(void*,std::uintptr_t at,Mapping &m) {
    if(scenario=="bad-protection") return false;
    FILE *file=std::fopen("/proc/self/maps","r"); if(!file) return false;
    char line[512],perms[8]; unsigned long begin=0,end=0; bool found=false;
    while(std::fgets(line,sizeof line,file)) if(std::sscanf(line,"%lx-%lx %7s",&begin,&end,perms)==3 && at>=begin && at<end) {
        m={begin,end,perms[0]=='r',perms[1]=='w',perms[2]=='x'}; found=true; break;
    }
    std::fclose(file); return found;
}
static bool readBytes(void*,std::uintptr_t at,unsigned char *out,std::size_t n) {
    std::memcpy(out,reinterpret_cast<void*>(at),n);
    if((scenario=="bad-entry" && hookCalls && at==reinterpret_cast<std::uintptr_t>(&owned_outer_entry)) ||
       (scenario=="bad-trampoline" && n==16)) out[0]^=1;
    return true;
}
static int hook(void*,void *at,void *replacement,void **original) {
    ++hookCalls;
    if(scenario=="throw-installer" && hookCalls==2) throw std::runtime_error("owned installer");
    if(scenario=="second-install-fails" && hookCalls==2) return -1;
    if(scenario=="early-entry") Core::outerEntry(worker,2,&optional,0,QSizeF(),1,0,targetId);
    const int result=DobbyHook(at,replacement,original);
    if(hookCalls==1)firstTrampoline=*original;
    if(hookCalls==2 && (scenario=="second-corrupts-entry"||scenario=="second-corrupts-trampoline")) {
        // Corrupt an owned branch literal, not an instruction to be executed.
        // Checked fixture-only memory writes avoid using Dobby's unchecked patch
        // API as the oracle. No victim executes again; no native proof is claimed.
        auto *victim=static_cast<unsigned char*>(scenario=="second-corrupts-entry" ? reinterpret_cast<void*>(&owned_outer_entry):firstTrampoline);
        victim+=scenario=="second-corrupts-entry"?4:12;
        const long pageSize=::sysconf(_SC_PAGESIZE);check(pageSize>0,"owned page size");
        const auto width=std::uintptr_t(pageSize);check((width&(width-1))==0,"power-of-two owned pages");
        const auto page=reinterpret_cast<std::uintptr_t>(victim)&~(width-1);
        Mapping previous;
        check(query(nullptr,reinterpret_cast<std::uintptr_t>(victim),previous) && previous.readable && !previous.writable && previous.executable,"actual owned RX mapping before corruption");
        std::fprintf(stderr,"MAPPING %lx-%lx RX before corruption\n",static_cast<unsigned long>(previous.begin),static_cast<unsigned long>(previous.end));
        unsigned char corrupted=0;std::memcpy(&corrupted,victim,1);corrupted^=1;
        check(::mprotect(reinterpret_cast<void*>(page),std::size_t(pageSize),PROT_READ|PROT_WRITE|PROT_EXEC)==0,"owned write permission");
        *static_cast<volatile unsigned char*>(victim)=corrupted;
        check(::mprotect(reinterpret_cast<void*>(page),std::size_t(pageSize),PROT_READ|PROT_EXEC)==0,"owned RX restored");
        __builtin___clear_cache(reinterpret_cast<char*>(victim),reinterpret_cast<char*>(victim)+1);
        unsigned char actual=0;std::memcpy(&actual,victim,1);check(actual==corrupted,"actual first-hook bytes corrupted");
        std::fprintf(stderr,"CORRUPTED %s after valid second install\n",scenario.c_str());
    }
    if(scenario=="null-original") *original=nullptr;
    return result;
}
static void stopped(void*,const char *why) {
    std::fprintf(stderr,"FAILSTOP %s submissions=%u outer=%u inner=%u hooks=%u\n",why,submissions,originalOuterCalls,originalInnerCalls,hookCalls);
    if(scenario=="throw-failstop") throw std::runtime_error("owned fail-stop callback");
}
static bool readWords(reader_insertion_predicate::Address,reader_insertion_predicate::Words &v) { v={0x1000,0x1040};return true; }
static reader_insertion_predicate::Address search(reader_insertion_predicate::Address,const QString &id) {
    if(scenario=="cancel-predicate")core->cancel();
    if(id==real) return 0x1008;
    if(id==right) return 0x1010;
    if(id==wrong) return 0x1028;
    return 0x1040;
}
static bool equal(const QString &a,const QString &b) { return a==b; }
static int postTry(pthread_mutex_t *mutex) {
    ++postTries;
    check(cleanup==1 && originalOuterCalls==1 && originalInnerCalls==0,"probe runs after original owned cleanup");
    return ::pthread_mutex_trylock(mutex);
}
static int postUnlock(pthread_mutex_t *mutex) {
    ++postUnlocks;
    const int result=::pthread_mutex_unlock(mutex);
    if(testName=="post-after-cancel")core->cancel();
    if(testName=="post-after-deadline")std::this_thread::sleep_for(std::chrono::milliseconds(350));
    return testName=="post-unlock-failure" ? EPERM : result;
}
static bool attribute(void*,std::uint64_t &job) noexcept {
    ++attributions;
    check(originalOuterCalls==0,"attribution before handler");
    check(mapMutex.try_lock(),"attribution outside map lock");mapMutex.unlock();
    const auto pending=core->snapshot();check(!pending.terminal&&pending.jobId==0,"no partial job publication");
    job=scenario=="job-zero" ? 0 : UINT64_C(0xfedcba9876543210);
    if(scenario=="job-cancel")core->cancel();
    return scenario!="job-false";
}
class Worker final:public QThread { public:std::function<void()> body; void run()override{body();} };
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv); check(argc==2,"one scenario"); scenario=argv[1];testName=scenario;
    postRequested=scenario.rfind("post-",0)==0;
    if(postRequested) {
        scenario=scenario.substr(5);
        if(scenario=="valid"||scenario=="busy"||scenario=="unlock-failure"||scenario=="after-cancel"||scenario=="after-deadline"||scenario=="normal"||scenario=="wrong-thread")scenario="mismatch";
        if(scenario=="unrelated")scenario="unrelated-outer";
    }
    core=Core::create({nullptr,hook,readBytes,query,stopped});
    Install plan; plan.outer=reinterpret_cast<void*>(&owned_outer_entry);plan.inner=reinterpret_cast<void*>(&owned_inner_entry);
    if(scenario=="overlap")plan.inner=reinterpret_cast<unsigned char*>(plan.outer)+4;
    std::memcpy(plan.outerBytes.data(),plan.outer,8);std::memcpy(plan.innerBytes.data(),plan.inner,8);
    if(scenario=="bad-original"||scenario=="throw-failstop") plan.innerBytes[0]^=1;
    if(scenario=="misaligned") plan.outer=workerStorage+1;
    core->install(plan);
    std::mutex mutex;std::condition_variable cv;bool ready=false,go=false,allowSubmission=true;std::thread::id tid;
    Worker thread;
    thread.body=[&] {
        {std::unique_lock<std::mutex> lock(mutex);tid=std::this_thread::get_id();ready=true;cv.notify_one();cv.wait(lock,[&]{return go;});}
        if(!allowSubmission)return;
        try {
            const QUuid id=!postRequested && (scenario=="foreign"||scenario=="unrelated-outer"||scenario=="unrelated-inner") ? foreignId:targetId;
            int index=scenario=="binding"?3:2;
            alignas(4) unsigned char dynamic[32]{};
            const void *actualOptional=&optional;Word actualPresent=0x7f123401,actualInherit=0x7e123400;
            int actualSource=1;QSizeF actualSize(1404.25,1872.5);void *actualWorker=worker;
            if(borrowedScenario()) {
                actualOptional=dynamic+(scenario=="borrowed-stack-b"?8:scenario=="borrowed-stack-c"?16:0);
                actualPresent=1;actualInherit=0;
                const Word sourceBits=scenario=="borrowed-stack-b"?0x80000000U:scenario=="borrowed-stack-c"?0x7fffffffU:0x87654321U;
                static_assert(sizeof actualSource==sizeof sourceBits);
                std::memcpy(&actualSource,&sourceBits,sizeof actualSource);
                if(scenario=="borrowed-boundary")actualSize=QSizeF(16384,16384);
                if(scenario=="borrowed-null")actualOptional=nullptr;
                if(scenario=="borrowed-misaligned")actualOptional=dynamic+1;
                if(scenario=="borrowed-present")actualPresent=0;
                if(scenario=="borrowed-present-wide")actualPresent=0x1000001;
                if(scenario=="borrowed-inherit")actualInherit=1;
                if(scenario=="borrowed-inherit-wide")actualInherit=0x1000000;
                if(scenario=="borrowed-size")actualSize=QSizeF(1404.125,1872.5);
                if(scenario=="borrowed-worker")actualWorker=workerStorage+4;
                if(scenario=="borrowed-index")index=3;
                borrowedOptional=actualOptional;borrowedSource=actualSource;borrowedSize=actualSize;
            }
            if(scenario=="owned-pointer")actualOptional=dynamic;
            if(scenario=="owned-source")actualSource=INT_MIN;
            ++submissions;
            invoke_outer(actualWorker,index,actualOptional,actualPresent,actualSize,actualSource,actualInherit,id);
            if(scenario=="duplicate") invoke_outer(worker,2,&optional,0x7f123401,QSizeF(1404.25,1872.5),1,0x7e123400,targetId);
        } catch(const std::runtime_error &) {outsideThrow=true;}
        lockFree=mapMutex.try_lock();if(lockFree)mapMutex.unlock();
    };
    thread.start();{std::unique_lock<std::mutex> lock(mutex);cv.wait(lock,[&]{return ready;});}
    reader_insertion_predicate::Config config{1,5,8,real,right,wrong,target};
    auto frozen=reader_insertion_predicate::freeze(config);check(bool(frozen.frozen),"frozen predicate");
    Registration r;r.target=targetId;r.frozen=std::move(frozen.frozen);r.calls={readWords,search,equal};
    if(scenario.rfind("job-",0)==0 || scenario=="foreign" || scenario=="binding" || scenario=="duplicate")r.attribute=attribute;
    r.admission={true,true,true,true,true,true};r.residentNativeOwners=workerStorage;
    r.deadline=std::chrono::steady_clock::now()+std::chrono::seconds(scenario=="deadline"?-1:30);
    r.binding={worker,map,tid,reinterpret_cast<std::uintptr_t>(outer_return),reinterpret_cast<std::uintptr_t>(inner_return),2,1,&optional,0x7f123401,0x7e123400,QSizeF(1404.25,1872.5)};
    if(postRequested) {
        auto *mutex=mapMutex.native_handle();const auto at=reinterpret_cast<std::uintptr_t>(mutex);
        r.postRejectProbe=reader_insertion_mutex_probe::Probe::create({mutex,{at,at+sizeof(*mutex)},postTry,postUnlock});
        check(r.postRejectProbe!=nullptr,"resident owned post-rejection probe");
        if(testName=="post-wrong-thread")r.binding.thread=std::this_thread::get_id();
        if(testName=="post-after-deadline")r.deadline=std::chrono::steady_clock::now()+std::chrono::milliseconds(250);
    }
    if(borrowedScenario()) {
        r.binding.policy=CallerPolicy::BorrowedExplicitSize;r.binding.optional=nullptr;
        r.binding.sizePresent=1;r.binding.inherit=0;r.binding.sourceIndex=42;
        if(scenario=="borrowed-boundary")r.binding.size=QSizeF(16384,16384);
        if(scenario=="borrowed-caller")r.binding.outerReturnPC+=4;
        if(scenario=="borrowed-thread")r.binding.thread=std::this_thread::get_id();
        if(badRegistration()) {
            const Binding valid=r.binding;
            if(scenario=="borrowed-reg-pointer")r.binding.optional=&optional;
            if(scenario=="borrowed-reg-present")r.binding.sizePresent=0;
            if(scenario=="borrowed-reg-present-wide")r.binding.sizePresent=0x1000001;
            if(scenario=="borrowed-reg-inherit")r.binding.inherit=1;
            if(scenario=="borrowed-reg-inherit-wide")r.binding.inherit=0x1000000;
            if(scenario=="borrowed-reg-zero-width")r.binding.size.setWidth(0);
            if(scenario=="borrowed-reg-zero-height")r.binding.size.setHeight(0);
            if(scenario=="borrowed-reg-negative")r.binding.size.setWidth(-1);
            if(scenario=="borrowed-reg-negative-zero")r.binding.size.setHeight(-0.0);
            if(scenario=="borrowed-reg-infinity")r.binding.size.setWidth(std::numeric_limits<double>::infinity());
            if(scenario=="borrowed-reg-nan")r.binding.size.setHeight(std::numeric_limits<double>::quiet_NaN());
            if(scenario=="borrowed-reg-cap-width")r.binding.size.setWidth(16385);
            if(scenario=="borrowed-reg-cap-height")r.binding.size.setHeight(16385);
            if(scenario=="borrowed-reg-unknown")r.binding.policy=static_cast<CallerPolicy>(255);
            check(!core->registerOnce(std::move(r)),"malformed borrowed registration refused");
            check(!core->snapshot().terminal && submissions==0 && originalOuterCalls==0 && originalInnerCalls==0,"malformed policy has no dispatch/publication");
            r.binding=valid;
            check(core->registerOnce(std::move(r)),"invalid policy did not consume registration");
            allowSubmission=false;
            {std::lock_guard<std::mutex> lock(mutex);go=true;cv.notify_one();}thread.wait();
            check(submissions==0 && originalOuterCalls==0 && originalInnerCalls==0 && attributions==0,"corrected owned registration still submits nothing");
            std::printf("{\"passed\":true,\"scenario\":\"%s\",\"checks\":%u,\"tracked_forward\":0,\"native_qualification\":false}\n",scenario.c_str(),checks);
            return 0;
        }
    }
    check(core->registerOnce(std::move(r)),"registration after verified pair");
    Registration empty;check(!core->registerOnce(std::move(empty)),"no second registration");
    if(scenario=="cancel")core->cancel();
    {std::lock_guard<std::mutex> lock(mutex);go=true;cv.notify_one();}thread.wait();
    const auto result=core->snapshot();check(lockFree,"original mutex released");
    if(postRequested) {
        const bool tried=testName=="post-valid"||testName=="post-after-cancel"||testName=="post-after-deadline";
        check(postTries==unsigned(tried)&&postUnlocks==unsigned(tried),"probe only after qualified successful private unwind");
        check(reader_insertion_mutex_probe::successful(result.postReject)==tried,"post-rejection facts exact");
        check(result.witness==(testName=="post-valid"),"only uncanceled unexpired full post-rejection witness");
        check(result.trackedForwards==0&&originalInnerCalls==0,"post-rejection remains negative only");
        if(testName=="post-unrelated")check(outsideThrow&&!result.privateCaught,"unrelated tracked original exception propagates without probe");
        if(testName=="post-normal")check(!result.privateCaught&&result.unknown&&result.refusal==Refusal::MissingInner,"normal return never probes");
        if(testName=="post-after-cancel")check(result.canceled&&result.refusal==Refusal::Canceled,"post-unlock cancellation suppresses witness");
        if(testName=="post-after-deadline")check(result.refusal==Refusal::Deadline,"post-unlock original deadline suppresses witness");
        std::printf("{\"passed\":true,\"scenario\":\"%s\",\"checks\":%u,\"tracked_forward\":0,\"native_qualification\":false}\n",testName.c_str(),checks);
        return 0;
    }
    if(scenario=="foreign") {check(!result.terminal,"unrelated no tracked result");check(originalInnerCalls==1&&forwarding,"transparent full carriers");}
    else if(scenario=="unrelated-outer"||scenario=="unrelated-inner") check(outsideThrow,"unrelated exception propagated");
    else {
        check(result.terminal,"tracked terminal");check(originalInnerCalls==(scenario=="nested"?1U:0U),"no tracked insertion forward");
        check(result.trackedForwards==0,"unconditional zero-forward fact");
        if(scenario=="binding"||scenario=="cancel"||scenario=="deadline"||scenario=="job-zero"||scenario=="job-false"||scenario=="job-cancel" ||
           (borrowedScenario()&&!borrowedPositive()) || scenario=="owned-pointer" || scenario=="owned-source")
            check(!result.privateCaught&&originalOuterCalls==0,"early refusal no outer call");
        else {check(result.privateCaught&&cleanup==(scenario=="nested"?2U:1U),"private caught after original cleanup");
            check(result.observedMismatch==(scenario!="transformed-target"&&scenario!="transformed-index"&&scenario!="cancel-inner"),"predicate outcome exact");}
        if(scenario=="duplicate"||scenario=="nested")check(result.interference,"interference observable");
        check(result.witness==(scenario=="mismatch"||scenario=="job-valid"||borrowedPositive()),"only uncontested mismatch is passing witness");
        if(scenario=="cancel-predicate")check(result.canceled&&result.refusal==Refusal::Canceled,"cancel during passive read remains distinct");
    }
    const unsigned expectedAttributions=(scenario.rfind("job-",0)==0||scenario=="duplicate")?1U:0U;
    check(attributions==expectedAttributions,"attribution exactly once only eligible tracked call");
    if(scenario=="job-valid"||scenario=="duplicate"||scenario=="job-cancel")check(result.attributed&&result.jobId==UINT64_C(0xfedcba9876543210),"full64 job attribution");
    if(scenario=="job-false"||scenario=="job-zero")check(!result.attributed&&result.jobId==0&&result.refusal==Refusal::Attribution,"failed attribution unpublished");
    if(borrowedPositive()) {
        check(forwarding && borrowedOptional!=&optional && borrowedOptional,"dynamic opaque optional and source/size bits preserved through original");
        check(result.privateCaught && result.observedMismatch && cleanup==1 && result.trackedForwards==0,"borrowed negative barrier and native-like cleanup unchanged");
    } else if(borrowedScenario() || scenario=="owned-pointer" || scenario=="owned-source")
        check(result.refusal==Refusal::Binding && !result.observedMismatch && originalOuterCalls==0 && attributions==0,"binding mismatch never reaches original or attribution");
    std::printf("{\"passed\":true,\"scenario\":\"%s\",\"checks\":%u,\"tracked_forward\":%u,\"native_qualification\":false}\n",scenario.c_str(),checks,result.trackedForwards);
}
