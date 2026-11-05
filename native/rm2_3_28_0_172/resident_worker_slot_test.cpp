#define RESIDENT_WORKER_SLOT_TESTING
#include "resident_worker_slot.h"
#include <cstdio>
#include <stdexcept>
#include <thread>
struct NativeId {std::uint32_t words[3]={17,23,31};};
Q_DECLARE_METATYPE(NativeId)
namespace xofm::libs::library {class DocumentController final:public QObject {Q_OBJECT};}
class DocumentWorker final:public QThread {Q_OBJECT};
using namespace reader_worker_native;
using Result=reader_insertion_native_profile::OwnerReleaseResult;
using Kind=reader_insertion_native_profile::NativeOwnerKind;
namespace reader_worker_native::testing {
struct SlotAccess {
    static ResidentSlot *make(detail::Functions fn,Result(*release)(Kind,std::uintptr_t,std::uintptr_t)) {
        auto *slot=new ResidentSlot(fn,nullptr);slot->testRelease_=release;return slot;
    }
    static void destroy(ResidentSlot *slot){delete slot;}
    static auto retire(ResidentSlot &slot,const ResidentSlot::Token &token){return slot.retirePhysically(token);}
    static void attribution(ResidentSlot &slot,reader_insertion_native_profile::AttributionParts(*create)(QObject*,bool),
            bool(*retire)(const reader_insertion_native_profile::AttributionParts&)) {
        slot.testAttribution_=create;slot.testRetireAttribution_=retire;
    }
};
}
namespace {
unsigned checks=0;
void check(bool value){++checks;if(!value){std::fprintf(stderr,"slot check %u failed\n",checks);std::abort();}}
struct Scenario {
    xofm::libs::library::DocumentController controller;
    DocumentWorker worker,wrongWorker;
    int document=0,docControl=0,workerControl=0;
    unsigned resolves=0,acquires=0,requests=0,docReleases=0,workerReleases=0;
    bool resolveThrow=false,acquireThrow=false,requestThrow=false,docEmpty=false,workerEmpty=false,docPartial=false,wrong=false;
    qulonglong job=0x1234567800000001ULL;
    Result docResult=Result::Released,workerResult=Result::Released;
    std::function<void()> resolveHook,acquireHook,requestHook;
    unsigned attributions=0,attributionRetirements=0;
    bool attributionEmpty=false,retirementRefused=false;
    std::function<void()> attributionHook;
};
Scenario *active=nullptr;
void put(void *out,void *raw,void *control) {
    const std::array<std::uintptr_t,2> words{{reinterpret_cast<std::uintptr_t>(raw),reinterpret_cast<std::uintptr_t>(control)}};
    std::memcpy(out,words.data(),sizeof words);
}
void resolve(void *out,void *controller,const void *id) {
    auto &s=*active;++s.resolves;check(controller==&s.controller);check(static_cast<const NativeId*>(id)->words[0]==17);
    put(out,s.docEmpty||s.docPartial?nullptr:&s.document,s.docEmpty?nullptr:&s.docControl);
    if(s.resolveHook)s.resolveHook();
    if(s.resolveThrow)throw std::runtime_error("resolve");
}
void acquire(void *out,void *document,int mode) {
    auto &s=*active;++s.acquires;check(document==&s.document&&mode==1);
    put(out,s.workerEmpty?nullptr:s.wrong?&s.wrongWorker:&s.worker,s.workerEmpty?nullptr:&s.workerControl);
    if(s.acquireHook)s.acquireHook();
    if(s.acquireThrow)throw std::runtime_error("acquire");
}
qulonglong request(void *worker,const std::function<void()> &callback) {
    auto &s=*active;++s.requests;check(worker==&s.worker);
    if(s.requestHook)s.requestHook();
    if(s.requestThrow)throw std::runtime_error("request");
    callback();return s.job;
}
void forbiddenRelease(void*) {check(false);}
Result release(Kind kind,std::uintptr_t raw,std::uintptr_t control) {
    auto &s=*active;
    if(kind==Kind::Document) {
        check(raw==reinterpret_cast<std::uintptr_t>(&s.document)&&control==reinterpret_cast<std::uintptr_t>(&s.docControl));
        ++s.docReleases;return s.docResult;
    }
    check(control==reinterpret_cast<std::uintptr_t>(&s.workerControl));
    check(raw==reinterpret_cast<std::uintptr_t>(s.wrong?&s.wrongWorker:&s.worker));
    ++s.workerReleases;return s.workerResult;
}
bool attribute(void *,std::uint64_t &id)noexcept{id=17;return true;}
reader_insertion_native_profile::AttributionParts attribution(QObject *worker,bool condition) {
    auto &s=*active;++s.attributions;check(worker==&s.worker&&condition);
    if(s.attributionHook)s.attributionHook();
    if(s.attributionEmpty)return {};
    return {&s.document,reinterpret_cast<reader_insertion_job_snapshot::Snapshot*>(&s.docControl),attribute,nullptr,{}};
}
bool retireAttribution(const reader_insertion_native_profile::AttributionParts &parts) {
    auto &s=*active;++s.attributionRetirements;
    check(parts.map==&s.document&&parts.context==reinterpret_cast<reader_insertion_job_snapshot::Snapshot*>(&s.docControl));
    check(s.docReleases==0&&s.workerReleases==0); // Attribution dies before either owner reference.
    return !s.retirementRefused;
}
using Slot=std::unique_ptr<ResidentSlot,void(*)(ResidentSlot*)>;
Slot make(Scenario &s) {
    active=&s;
    return {testing::SlotAccess::make({resolve,acquire,request,forbiddenRelease,QMetaType::fromType<NativeId>().iface(),&DocumentWorker::staticMetaObject},release),testing::SlotAccess::destroy};
}
auto prepare(ResidentSlot &slot,Scenario &s,const Permit &permit=[]{return true;}) {
    return slot.prepare(&s.controller,QVariant::fromValue(NativeId{}),&s.worker,permit);
}
}
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    {
        Scenario s;auto slot=make(s);testing::SlotAccess::attribution(*slot,attribution,retireAttribution);
        auto p=prepare(*slot,s);check(bool(slot->attribution(p.token)));
        check(!slot->attribution(p.token)&&s.attributions==1);
        s.workerResult=Result::RetainedLast;
        auto r=testing::SlotAccess::retire(*slot,p.token);
        check(r.retired&&!r.reclaimed&&s.attributionRetirements==1&&s.docReleases==1&&s.workerReleases==1);
        s.workerResult=Result::Released;r=testing::SlotAccess::retire(*slot,p.token);
        check(r.reclaimed&&s.attributionRetirements==1&&s.docReleases==1&&s.workerReleases==2);
        auto replacement=prepare(*slot,s);
        check(!slot->attribution(p.token)&&s.attributions==1&&bool(slot->attribution(replacement.token)));
    }
    for(int mode=0;mode<4;++mode) {
        Scenario s;auto slot=make(s);testing::SlotAccess::attribution(*slot,attribution,retireAttribution);
        auto p=prepare(*slot,s);
        s.attributionEmpty=mode==0;s.retirementRefused=mode==1;
        if(mode==2)s.attributionHook=[&]{check(slot->cancel(p.token));};
        if(mode==3)s.attributionHook=[&]{check(!slot->attribution(p.token));};
        check(bool(slot->attribution(p.token))==(mode==1));
        if(mode!=1) {
            check(!slot->worker(p.token)&&!slot->submit(p.token,[]{},[]{return true;}).attempted&&s.requests==0);
            check(!slot->attribution(p.token)&&s.attributions==1);
        }
        auto r=testing::SlotAccess::retire(*slot,p.token);
        check(r.unknown&&!r.retired&&!r.reclaimed&&slot->occupied());
        check(s.workerReleases==0&&s.docReleases==0&&s.attributionRetirements==(mode==1?1U:0U));
        check(!testing::SlotAccess::retire(*slot,p.token).reclaimed&&s.workerReleases==0&&s.docReleases==0);
    }
    {
        Scenario s;auto slot=make(s);auto p=prepare(*slot,s);check(p.ready&&!p.unknown&&slot->occupied());
        check(slot->worker(p.token)==&s.worker);check(!prepare(*slot,s).ready&&s.resolves==1);
        unsigned callbacks=0;auto submitted=slot->submit(p.token,[&]{++callbacks;},[]{return true;});
        check(submitted.accepted&&submitted.jobId==s.job&&callbacks==1);
        check(!slot->submit(p.token,[]{},[]{return true;}).attempted&&s.requests==1);
        auto r=testing::SlotAccess::retire(*slot,p.token);check(r.matched&&r.retired&&r.reclaimed&&!slot->occupied());
        check(s.workerReleases==1&&s.docReleases==1);
        check(!testing::SlotAccess::retire(*slot,p.token).matched&&!slot->cancel(p.token));
    }
    // Releasing one owner cannot erase or retry the other owner's state.
    for(bool retainWorker:{false,true})for(auto refusal:{Result::RetainedLast,Result::RetainedBusy,Result::RefusedIdentity}) {
        Scenario s;auto slot=make(s);auto p=prepare(*slot,s);
        (retainWorker?s.workerResult:s.docResult)=refusal;
        auto r=testing::SlotAccess::retire(*slot,p.token);
        check(r.matched&&r.retired&&!r.reclaimed&&slot->occupied());
        check(r.workerRetained==retainWorker&&r.documentRetained!=retainWorker);
        check(!slot->worker(p.token)&&!slot->submit(p.token,[]{},[]{return true;}).attempted);
        check(!prepare(*slot,s).ready&&s.resolves==1);
        r=testing::SlotAccess::retire(*slot,p.token);check(!r.reclaimed);
        check(s.workerReleases==(retainWorker?2U:1U)&&s.docReleases==(retainWorker?1U:2U));
        s.workerResult=s.docResult=Result::Released;
        r=testing::SlotAccess::retire(*slot,p.token);check(r.reclaimed);
        check(s.workerReleases==(retainWorker?3U:1U)&&s.docReleases==(retainWorker?1U:3U));
    }
    for(int mode=0;mode<4;++mode) {
        Scenario s;auto slot=make(s);s.resolveThrow=mode==0;s.acquireThrow=mode==1;s.docPartial=mode==2;
        auto p=prepare(*slot,s);
        if(mode==3){s.requestThrow=true;check(slot->submit(p.token,[]{},[]{return true;}).unknown);}
        else check(!p.ready&&p.unknown);
        auto r=testing::SlotAccess::retire(*slot,p.token);
        check(r.matched&&r.unknown&&!r.retired&&!r.reclaimed&&slot->occupied());
        check(s.docReleases==0&&s.workerReleases==0&&!prepare(*slot,s).ready);
    }
    for(int mode=0;mode<3;++mode) {
        Scenario s;auto slot=make(s);s.docEmpty=mode==0;s.workerEmpty=mode==1;s.wrong=mode==2;
        auto p=prepare(*slot,s);check(!p.ready&&!p.unknown&&s.requests==0);
        check(s.docReleases==0&&s.workerReleases==0);
        auto r=testing::SlotAccess::retire(*slot,p.token);check(r.reclaimed);
        check(s.docReleases==(mode==0?0U:1U)&&s.workerReleases==(mode==2?1U:0U));
    }
    {
        Scenario s;auto slot=make(s);unsigned permits=0;
        auto p=prepare(*slot,s,[&]{return ++permits<2;});
        check(!p.ready&&!p.unknown&&s.resolves==1&&s.acquires==0&&s.docReleases==0);
        check(testing::SlotAccess::retire(*slot,p.token).reclaimed&&s.docReleases==1);
    }
    {
        Scenario s;auto slot=make(s);bool nested=false;
        auto p=prepare(*slot,s,[&]{nested=!prepare(*slot,s).ready;return true;});
        check(nested&&!p.ready&&s.resolves==0);check(testing::SlotAccess::retire(*slot,p.token).reclaimed);
    }
    {
        Scenario s;auto slot=make(s);auto p=prepare(*slot,s);
        s.requestHook=[&]{check(slot->cancel(p.token));check(!testing::SlotAccess::retire(*slot,p.token).matched);};
        auto submitted=slot->submit(p.token,[]{},[]{return true;});
        check(submitted.unknown&&!submitted.accepted);check(!testing::SlotAccess::retire(*slot,p.token).reclaimed);
        check(s.docReleases==0&&s.workerReleases==0);
    }
    {
        Scenario s;auto slot=make(s);auto p=prepare(*slot,s);s.job=0;
        check(slot->submit(p.token,[]{},[]{return true;}).unknown);
        testing::SlotAccess::attribution(*slot,attribution,retireAttribution);
        check(!slot->worker(p.token)&&!slot->attribution(p.token)&&s.attributions==0);
        check(!slot->submit(p.token,[]{},[]{return true;}).attempted&&s.requests==1);
        check(!testing::SlotAccess::retire(*slot,p.token).reclaimed&&s.docReleases==0);
    }
    {
        Scenario s;auto slot=make(s);ResidentSlot::Token stale;
        for(unsigned n=0;n<1000;++n) {
            auto p=prepare(*slot,s);check(p.ready);
            check(!slot->cancel(stale)&&!slot->worker(stale)&&!testing::SlotAccess::retire(*slot,stale).matched);
            check(!slot->submit(stale,[]{},[]{return true;}).attempted);
            check(testing::SlotAccess::retire(*slot,p.token).reclaimed);stale=p.token;
        }
        check(s.docReleases==1000&&s.workerReleases==1000&&s.requests==0);
    }
    {
        Scenario s;auto slot=make(s);auto p=prepare(*slot,s);
        bool rejected=false;std::thread other([&]{rejected=slot->occupied()&&!slot->cancel(p.token)&&!slot->worker(p.token)&&!testing::SlotAccess::retire(*slot,p.token).matched;});
        other.join();check(rejected);check(testing::SlotAccess::retire(*slot,p.token).reclaimed);
    }
    std::printf("{\"passed\":true,\"checks\":%u,\"native_ownership\":false}\n",checks);
}
#include "resident_worker_slot_test.moc"
