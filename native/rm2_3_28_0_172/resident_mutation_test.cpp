// Owned executor lifecycle model. Injects settled operation evidence and owner
// records; real Fence, executor publication and Slot retirement execute. This
// does not exercise native submission, hooks or the Coordinator join pipeline.
#include "resident_mutation.h"
#include <cstdio>
#include <thread>
using Release=reader_insertion_native_profile::OwnerReleaseResult;
using Kind=reader_insertion_native_profile::NativeOwnerKind;
using Result=reader_insertion_entry::Result;
namespace {
unsigned checks=0,releases=0,coreRetires=0;
Release releaseResult=Release::Released;
void check(bool value){++checks;if(!value){std::fprintf(stderr,"mutation check %u failed\n",checks);std::abort();}}
Release release(Kind,std::uintptr_t,std::uintptr_t){check(coreRetires==1);++releases;return releaseResult;}
void pump(int ms=8){QElapsedTimer t;t.start();do{QCoreApplication::processEvents(QEventLoop::AllEvents,1);QThread::msleep(1);}while(t.elapsed()<ms);}
Result success(){
    Result r;r.finished=r.observed=r.ownersRetained=r.registrationAccepted=true;r.probeJob=11;r.job=12;
    r.matchingCompletions=r.pageAdded=1;r.postOrder.observed=true;
    auto &p=r.probe;p.finished=p.attempted=p.submissionReturned=p.deliveryObserved=p.matchingCompletion=true;
    p.matchingCompletionCount=p.enteredCount=p.returningCount=p.offGuiCount=1;p.jobId=QStringLiteral("11");
    auto &c=r.creation;c.finished=c.attempted=c.returnKnown=c.accepted=c.callbackObserved=true;c.callbackCount=1;
    auto &o=r.core;o.terminal=o.witness=o.attributed=o.forwardCommitted=o.innerReturned=o.outerReturned=o.condition.matched=true;
    o.jobId=12;o.outerCalls=o.innerCalls=o.trackedForwards=1;return r;
}
}
namespace reader_worker_native::testing {
struct SlotAccess {
    static ResidentSlot *make(){auto *s=new ResidentSlot({},nullptr);s->testRelease_=release;return s;}
    static ResidentSlot::Token occupy(ResidentSlot &s,QObject &worker){
        check(!s.current_);auto v=std::make_shared<ResidentSlot::State>();
        v->preparing=false;v->ready=v->consumed=true;v->controller=&worker;v->publicWorker=&worker;
        v->worker.words={1,2};v->worker.ownership=ResidentSlot::Ownership::Owned;
        v->document.words={3,4};v->document.ownership=ResidentSlot::Ownership::Owned;
        s.current_=v;return ResidentSlot::Token(v);
    }
    static void destroy(ResidentSlot *s){delete s;}
    static void uncertain(ResidentSlot &s){s.current_->unknown=true;}
};
struct MutationAccess {
    using Gate=remarkable::rm2_3_28_0_172::ResidentEffectGate;
    using State=ResidentMutation::Active;
    static ResidentMutation *make(ResidentSlot *s){reader_insertion_entry::Runtime r;r.owners=s;return new ResidentMutation(std::move(r),*new remarkable::rm2_3_28_0_172::ResidentEffectGate);}
    static void destroy(ResidentMutation *m){auto *gate=&m->effects_;delete m;delete gate;}
    static Gate &gate(ResidentMutation &m){return m.effects_;}
    static ResidentMutation *sharing(ResidentSlot *s,Gate &gate){reader_insertion_entry::Runtime r;r.owners=s;return new ResidentMutation(std::move(r),gate);}
    static void destroyOnly(ResidentMutation *m){delete m;}
    static Gate *newGate(){return new Gate;}
    static void deleteGate(Gate *gate){delete gate;}
    static auto acquire(Gate &gate){return gate.acquire();}
    static bool release(Gate &gate,const Gate::Token &token){return gate.release(token);}
    static auto begin(ResidentMutation &m,ResidentSlot &s,QObject &worker,Result r,ResidentMutation::Completion completion){
        check(!m.busy());releases=coreRetires=0;releaseResult=Release::Released;
        auto a=std::make_shared<State>();a->effect=m.effects_.acquire();check(bool(a->effect));a->fence=std::make_unique<reader_retirement::Fence>(static_cast<QObject*>(&m));
        a->fenceToken=a->fence->begin(QByteArray(32,'a'),reader_retirement::Kind::Mutation);check(bool(a->fenceToken));
        a->binding=reader_retirement::binding(a->fence.get(),a->fenceToken);a->result=std::move(r);
        a->ownerToken=SlotAccess::occupy(s,worker);a->worker=&worker;a->completion=std::move(completion);m.active_=a;
        m.testCoreSnapshot_=[a]{return a->result.core;};m.testCoreRetire_=[] {++coreRetires;return true;};return a;
    }
    static void settle(ResidentMutation &m,const std::shared_ptr<State> &a){
        a->terminal=true;m.classify(a,false);
        check(a->fence->requestRetire(a->fenceToken,[a]{a->cleaned=true;},reader_retirement::Effect::ReconciliationRequired));
    }
    static void step(ResidentMutation &m){m.maintain();m.maintenance_.stop();}
    static auto token(const std::shared_ptr<State>&a){return ResidentMutation::Token(a);}
    static void late(ResidentMutation &m,reader_insertion_hooks::Outcome o){m.testCoreSnapshot_=[o]{return o;};}
    static bool normal(const Result&r){return ResidentMutation::normal(r);}
    static void releaseCapture(ResidentMutation&m){m.testCoreSnapshot_={};}
};
}
int main(int argc,char **argv){
    QGuiApplication app(argc,argv);QObject worker;
    using namespace reader_worker_native;using A=testing::MutationAccess;using S=testing::SlotAccess;
    check(A::normal(success()));
    // More than the individual research Fence's 256-entry history, all through
    // the same process executor and owner slot. No real native calls occur.
    auto *slot=S::make();auto *executor=A::make(slot);ResidentMutation::Token old;
    for(unsigned i=0;i<300;++i){
        unsigned delivered=0,destroyed=0;
        auto capture=std::shared_ptr<int>(new int(0),[&](int *p){
            ++destroyed;check(executor->busy());pump(1);check(executor->busy());delete p;
        });
        auto a=A::begin(*executor,*slot,worker,success(),[&,capture](const Result&r){
            ++delivered;check(r.observed&&!r.unknown);check(executor->busy());
            check(!executor->start(nullptr,{},QByteArray(32,'b'),[]{return true;},[](const Result&){}));
            pump(1);check(executor->busy());
        });
        capture.reset();
        check(!executor->cancel(old));old=A::token(a);
        {
            const reader_request_scope::Scope frame(a->binding);
            A::settle(*executor,a);pump();A::step(*executor);
            check(!a->cleaned&&delivered==0&&coreRetires==0&&releases==0);
        }
        pump();A::step(*executor);
        check(delivered==1&&destroyed==1&&coreRetires==1&&releases==2&&!executor->busy()&&!slot->occupied());
        A::step(*executor);check(delivered==1);A::releaseCapture(*executor);a.reset();check(!old);
    }
    A::destroy(executor);S::destroy(slot);
    // Correlation failures at terminal classification suppress stale success.
    for(unsigned mode=0;mode<5;++mode){
        slot=S::make();executor=A::make(slot);auto r=success();
        if(mode==0){r.probeJob=r.job;r.probe.jobId=QString::number(r.job);}
        if(mode==1)r.probe.matchingCompletionCount=2;
        if(mode==2)r.creation.duplicate=true;
        if(mode==3)r.postOrder.observed=false;
        if(mode==4)r.core.interference=true;
        unsigned delivered=0;auto a=A::begin(*executor,*slot,worker,r,[&](const Result&out){
            ++delivered;check(!out.observed&&out.unknown&&out.reconciliationRequired);
            check(out.refusal==QStringLiteral("terminal-proof-refused"));
        });
        A::settle(*executor,a);pump();A::step(*executor);
        check(delivered==1&&coreRetires==0&&releases==0&&executor->busy()&&slot->occupied());
        A::step(*executor);check(delivered==1);A::releaseCapture(*executor);a.reset();A::destroy(executor);S::destroy(slot);
    }
    // A late Core snapshot is authoritative over the earlier settled result.
    for(unsigned mode=0;mode<3;++mode){
        slot=S::make();executor=A::make(slot);unsigned delivered=0;
        auto a=A::begin(*executor,*slot,worker,success(),[&](const Result&r){
            ++delivered;check(!r.observed&&r.unknown&&r.reconciliationRequired);
            check(r.refusal==QStringLiteral("retirement-core-changed"));
            check(mode==0?r.core.interference:mode==1?r.core.canceled:r.core.jobId==99);
        });
        A::settle(*executor,a);auto late=a->result.core;
        if(mode==0)late.interference=true;
        if(mode==1)late.canceled=true;
        if(mode==2)late.jobId=99;
        A::late(*executor,late);pump();A::step(*executor);
        check(delivered==1&&coreRetires==0&&releases==0&&executor->busy()&&slot->occupied());
        a.reset();A::destroy(executor);S::destroy(slot);
    }
    // Last-owner retention does not erase known operation evidence; it prevents
    // reuse and bounds attempts. No ordinary native destructor fallback exists.
    slot=S::make();executor=A::make(slot);unsigned delivered=0;
    auto a=A::begin(*executor,*slot,worker,success(),[&](const Result&r){++delivered;check(r.observed&&!r.unknown);});
    A::settle(*executor,a);releaseResult=Release::RetainedLast;pump();
    for(unsigned i=0;i<8;++i)A::step(*executor);
    check(delivered==1&&coreRetires==1&&releases==16&&executor->busy()&&slot->occupied());
    A::releaseCapture(*executor);a.reset();A::destroy(executor);S::destroy(slot);
    slot=S::make();executor=A::make(slot);delivered=0;
    a=A::begin(*executor,*slot,worker,success(),[&](const Result&r){
        ++delivered;check(!r.observed&&r.unknown&&r.reconciliationRequired);
        check(r.refusal==QStringLiteral("retirement-owner-refused"));
    });
    A::settle(*executor,a);S::uncertain(*slot);pump();A::step(*executor);
    check(delivered==1&&coreRetires==1&&releases==0&&executor->busy());
    A::releaseCapture(*executor);a.reset();A::destroy(executor);S::destroy(slot);
    {
        auto *gate=A::newGate(),*foreign=A::newGate();auto original=A::acquire(*gate);
        check(bool(original)&&gate->busy()&&!A::acquire(*gate));
        check(!A::release(*foreign,original)&&gate->busy());
        bool wrongRelease=true;std::thread other([&]{check(gate->busy());wrongRelease=A::release(*gate,original);});other.join();
        check(!wrongRelease&&gate->busy());check(A::release(*gate,original)&&!gate->busy());
        auto next=A::acquire(*gate);check(!A::release(*gate,original)&&gate->busy());
        next={};check(gate->busy()); // Dropping every caller token does not clear uncertainty.
        A::deleteGate(gate);A::deleteGate(foreign); // Owned fixture teardown only.
    }
    for(bool uncertain : {false,true}) {
        auto *firstSlot=S::make(),*secondSlot=S::make();auto *first=A::make(firstSlot);
        auto *gate=&A::gate(*first);auto *second=A::sharing(secondSlot,*gate);
        auto result=success();if(uncertain)result.core.interference=true;
        bool captureDestroyed=false;
        auto capture=std::shared_ptr<int>(new int(0),[&](int *value){
            captureDestroyed=true;check(gate->busy()&&second->busy());pump(1);check(gate->busy()&&second->busy());delete value;
        });
        auto state=A::begin(*first,*firstSlot,worker,result,[&,capture](const Result&){
            check(gate->busy()&&second->busy());pump(1);check(gate->busy()&&second->busy());
        });
        capture.reset();
        check(second->busy()&&!second->start(nullptr,{},QByteArray(32,'c'),[]{return true;},[](const Result&){}));
        A::settle(*first,state);pump();A::step(*first);
        check(captureDestroyed&&gate->busy()==uncertain&&second->busy()==uncertain);
        A::releaseCapture(*first);state.reset();A::destroyOnly(first);
        check(gate->busy()==uncertain&&second->busy()==uncertain); // Facade/owner destruction cannot clear another gate.
        A::destroyOnly(second);A::deleteGate(gate);S::destroy(firstSlot);S::destroy(secondSlot);
    }
    std::printf("resident mutation owned lifecycle: %u checks PASS\n",checks);
}
