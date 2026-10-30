#include "carriers.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <dobby.h>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <thread>

static unsigned checks=0;
static void check(bool value,const char *message) {
    ++checks;
    if(!value){std::fprintf(stderr,"FAILED %s\n",message);std::abort();}
}
static std::atomic<int> liveTemps{0};
struct Temporary {
    QString text{QStringLiteral("original owned temporary \u03bb")};
    Temporary(){++liveTemps;}
    ~Temporary(){--liveTemps;}
};
enum class Mode { Accepted, PrivateReject, UnrelatedThrow, TransparentWideInner };
struct OwnedMap {
    std::mutex mutex;
    bool held=false;
    std::int32_t index=-1,fifth=0,sixth=0;
    int entries=0;
    QString target,pageTemplate;
    const QString *templateAddress=nullptr;
    Mode mode=Mode::Accepted;
};
struct OwnedWorker {
    OwnedMap map;
    const OwnedOptionalQString *optional=nullptr;
    const QUuid *uuid=nullptr;
    std::int32_t index=-1,source=-1;
    std::uint8_t sizePresent=0,inherit=0;
    QSizeF size;
    bool refused=false,cleanAtCatch=false,mutexFreeAtCatch=false;
    int outerEntries=0;
};
struct GuardRejected {};
struct OriginalFailure {};
static OuterCarrier originalOuter=nullptr;
static InnerCarrier originalInner=nullptr;
static thread_local OwnedWorker *eligible=nullptr;
extern "C" int owned_probe_registers(void (*)(void *),void *);
extern "C" __attribute__((noinline,noclone,noipa)) void owned_inner_body(OwnedMap *map,
    std::int32_t index,const QString &target,const QString &pageTemplate,
    std::int32_t fifth,std::int32_t sixth) {
    Temporary temporary;
    if(!map->held) throw OriginalFailure{};
    ++map->entries;
    map->index=index;map->fifth=fifth;map->sixth=sixth;
    map->target=target;map->pageTemplate=pageTemplate;map->templateAddress=&pageTemplate;
    if(map->mode==Mode::UnrelatedThrow) throw OriginalFailure{};
}
extern "C" __attribute__((noinline,noclone,noipa)) void owned_outer_body(OwnedWorker *worker,
    std::int32_t index,const OwnedOptionalQString &optional,std::uint8_t sizePresent,
    QSizeF size,std::int32_t source,std::uint8_t inherit,const QUuid &uuid) {
    Temporary temporary;
    ++worker->outerEntries;
    worker->optional=&optional;worker->uuid=&uuid;worker->index=index;
    worker->sizePresent=sizePresent;worker->size=size;worker->source=source;worker->inherit=inherit;
    std::unique_lock<std::mutex> lock(worker->map.mutex);
    struct Held {
        OwnedMap *map;
        explicit Held(OwnedMap *m):map(m){map->held=true;}
        ~Held(){map->held=false;}
    } held(&worker->map);
    Temporary innerTemporary;
    const QString target=uuid.toString(QUuid::WithoutBraces);
    owned_inner_entry(&worker->map,index,target,optional.value,-1,0);
}
extern "C" __attribute__((noinline,noclone,noipa)) void outer_hook(OwnedWorker *worker,
    std::int32_t index,const OwnedOptionalQString &optional,std::uint8_t sizePresent,
    QSizeF size,std::int32_t source,std::uint8_t inherit,const QUuid &uuid) {
    try { originalOuter(worker,index,optional,sizePresent,size,source,inherit,uuid); }
    catch(const GuardRejected &){
        worker->refused=true;
        worker->cleanAtCatch=liveTemps.load()==0 && !worker->map.held;
        worker->mutexFreeAtCatch=worker->map.mutex.try_lock();
        if(worker->mutexFreeAtCatch)worker->map.mutex.unlock();
    }
}
extern "C" __attribute__((noinline,noclone,noipa)) void inner_hook(OwnedMap *map,
    std::int32_t index,const QString &target,const QString &pageTemplate,
    std::int32_t fifth,std::int32_t sixth) {
    if(eligible && map==&eligible->map && map->mode==Mode::PrivateReject) throw GuardRejected{};
    originalInner(map,index,target,pageTemplate,fifth,sixth);
}
struct Context {
    OwnedWorker worker;
    OwnedOptionalQString optional{QStringLiteral("owned template \u03bb"),0x7b};
    const QUuid uuid{QStringLiteral("11111111-2222-4333-8444-555555555555")};
    bool unrelatedCaught=false,unexpected=false,offGui=false;
    std::thread::id gui;
    int vfpMask=0;
};
static void outsideOwnedCatch(void *opaque) {
    auto &context=*static_cast<Context *>(opaque);
    context.offGui=std::this_thread::get_id()!=context.gui;
    try {
        if(context.worker.map.mode==Mode::TransparentWideInner){
            std::unique_lock<std::mutex> lock(context.worker.map.mutex);
            context.worker.map.held=true;
            const QString target=QStringLiteral("wide-owned-target");
            owned_inner_entry(&context.worker.map,19,target,context.optional.value,-123456789,0x7f123456);
            context.worker.map.held=false;
        } else owned_outer_entry(&context.worker,0x13579,context.optional,0x7f,
            QSizeF(1404.25,1872.5),0x2468,0x5b,context.uuid);
    } catch(const OriginalFailure &){context.unrelatedCaught=true;}
    catch(...){context.unexpected=true;}
}
static void run(Mode mode,const char *label) {
    std::printf("CASE %s\n",label);std::fflush(stdout);
    Context context;context.gui=std::this_thread::get_id();context.worker.map.mode=mode;
    QThread *thread=QThread::create([&]{
        eligible=mode==Mode::TransparentWideInner?nullptr:&context.worker;
        context.vfpMask=owned_probe_registers(&outsideOwnedCatch,&context);
        eligible=nullptr;
    });
    thread->start();check(thread->wait(10000),"bounded owned worker completion");delete thread;
    check(context.vfpMask==3,"D8 and D9 sentinel bits preserved through hooked call/unwind");
    check(context.offGui && !context.unexpected,"actual owned worker and expected exception types");
    check(liveTemps==0 && !context.worker.map.held,"original C++ RAII cleanup");
    check(context.worker.map.mutex.try_lock(),"mutex available after veneer and C++ unwind");context.worker.map.mutex.unlock();
    if(mode!=Mode::TransparentWideInner){
        check(context.worker.outerEntries==1,"outer relocated original called exactly once");
        check(context.worker.optional==&context.optional && context.worker.uuid==&context.uuid,"optional/UUID reference identity");
        check(context.worker.optional->engaged==0x7b,"owned optional engagement offset/value");
        check(context.worker.index==0x13579 && context.worker.source==0x2468,"outer integer core/stack values");
        check(context.worker.sizePresent==0x7f && context.worker.inherit==0x5b,"outer byte flags preserved without bool assumption");
        check(context.worker.size==QSizeF(1404.25,1872.5),"D0/D1 hard-float fractional size values");
    }
    if(mode==Mode::PrivateReject){
        check(context.worker.refused && context.worker.cleanAtCatch && context.worker.mutexFreeAtCatch,"private exception caught only after original cleanup");
        check(context.worker.map.entries==0 && !context.unrelatedCaught,"private rejection before inner original");
    } else {
        check(!context.worker.refused && context.worker.map.entries==1,"accepted/foreign/unrelated paths not false refusal");
        check(context.unrelatedCaught==(mode==Mode::UnrelatedThrow),"unrelated exception reached outside owned catch");
        check(context.worker.map.templateAddress==&context.optional.value && context.worker.map.pageTemplate==context.optional.value,"inner QString reference/value forwarding");
        if(mode==Mode::TransparentWideInner)
            check(context.worker.map.fifth==-123456789 && context.worker.map.sixth==0x7f123456,"both inner stack arguments retain all32 bits");
        else check(context.worker.map.fifth==-1 && context.worker.map.sixth==0,"selected inner signed-word values");
    }
}
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    const auto bytes=[](const void *p){return QString::fromLatin1(QByteArray(static_cast<const char *>(p),16).toHex());};
    const QString outerBefore=bytes(reinterpret_cast<void *>(&owned_outer_entry));
    const QString innerBefore=bytes(reinterpret_cast<void *>(&owned_inner_entry));
    check(outerBefore.startsWith("f04f2de90040a0e1"),"owned outer prefix push9 plus movr4,r0");
    check(innerBefore.startsWith("f04f2de90350a0e1"),"owned inner prefix push9 plus movr5,r3");
    void *outer=nullptr,*inner=nullptr;
    check(DobbyHook(reinterpret_cast<void *>(&owned_outer_entry),reinterpret_cast<void *>(&outer_hook),&outer)==0,"install outer owned entry hook");
    originalOuter=reinterpret_cast<OuterCarrier>(outer);
    check(DobbyHook(reinterpret_cast<void *>(&owned_inner_entry),reinterpret_cast<void *>(&inner_hook),&inner)==0,"install inner owned entry hook");
    originalInner=reinterpret_cast<InnerCarrier>(inner);
    run(Mode::Accepted,"accepted-carriers");
    run(Mode::PrivateReject,"private-unwind");
    run(Mode::UnrelatedThrow,"unrelated-unwind-both-veneers");
    run(Mode::TransparentWideInner,"transparent-full32-inner");
    QJsonObject result{{"passed",true},{"checks",int(checks)},{"cases",4},{"native_qualification",false},
        {"owned_outer_before16",outerBefore},{"owned_inner_before16",innerBefore},
        {"owned_outer_trampoline16",bytes(outer)},{"owned_inner_trampoline16",bytes(inner)},
        {"D8_D9_preserved",true},{"runtime_unhook_attempted",false},{"owned_extra_veneer_frame",true}};
    std::puts(QJsonDocument(result).toJson(QJsonDocument::Compact).constData());
}
