// Full owned Coordinator -> executor pipeline. Native hook outcomes and owner
// callouts are modeled. Real Qt witnesses, worker delivery, Slot and Fence run.
#include "resident_mutation.h"
#define READER_JOINED_CREATION_OWNED_CLASSES_ONLY
#include "reader_joined_creation_witness_fixture.cpp"
#include <QSemaphore>
#include <deque>
#include <mutex>
#include <cstdio>
struct OwnedNativeId {std::uint32_t words[3]={1,2,3};};
Q_DECLARE_METATYPE(OwnedNativeId)
class DocumentWorker final:public QThread {
    Q_OBJECT
public:
    struct Job {qulonglong id;std::function<void()> call;int added=-1;};
    std::mutex mutex;std::deque<Job> jobs;QSemaphore ready;bool stop=false;
    DocumentWorker(){start();}
    ~DocumentWorker()override{{std::lock_guard<std::mutex> lock(mutex);stop=true;}ready.release();wait();}
    void queue(Job job){{std::lock_guard<std::mutex> lock(mutex);jobs.push_back(std::move(job));}ready.release();}
    void run()override{for(;;){ready.acquire();Job j;{std::lock_guard<std::mutex> lock(mutex);if(stop)return;j=std::move(jobs.front());jobs.pop_front();}j.call();if(j.added>=0)emit pageAdded(j.id,j.added);emit jobCompleted(j.id);}}
signals:void jobCompleted(qulonglong);void pageAdded(qulonglong,int);
};
class SceneController final:public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* worker READ worker CONSTANT)
public:QObject *value=nullptr;QObject *worker()const{return value;}
};
class Reader final:public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QObject* sceneController READ sceneController CONSTANT)
public:QObject *value=nullptr;QObject *sceneController()const{return value;}
};
using Mutation=reader_worker_native::ResidentMutation;
using Entry=reader_insertion_entry::Coordinator;
using Slot=reader_worker_native::ResidentSlot;
using Outcome=reader_insertion_hooks::Outcome;
using Release=reader_insertion_native_profile::OwnerReleaseResult;
using Kind=reader_insertion_native_profile::NativeOwnerKind;
namespace {
unsigned checks=0;
void check(bool v,const char *why){++checks;if(!v)qFatal("pipeline %u: %s",checks,why);}
void pump(int ms=5){QElapsedTimer t;t.start();do{QCoreApplication::processEvents(QEventLoop::AllEvents,1);QThread::msleep(1);}while(t.elapsed()<ms);}
bool words(reader_insertion_predicate::Address,reader_insertion_predicate::Words&){return false;}
reader_insertion_predicate::Address search(reader_insertion_predicate::Address,const QString&){return 0;}
bool equal(const QString&a,const QString&b){return a==b;}
bool conditionWords(reader_insertion_condition::Address,reader_insertion_condition::Words&){return false;}
bool tree(reader_insertion_condition::Address,reader_insertion_condition::Tree&){return false;}
reader_insertion_condition::Address find(reader_insertion_condition::Address,const QString&){return 0;}
bool attribute(void*,std::uint64_t &v)noexcept{v=12;return true;}
struct Model {
    DocumentWorker *worker=nullptr;QObject *controller=nullptr,*document=nullptr;
    int docControl=0,workerControl=0;unsigned registrations=0,coreRetires=0,releases=0,attributions=0,clears=0,canceled=0;
    QPointer<Entry> entry;Mutation *executor=nullptr;
    std::mutex mutex;Outcome outcome;
    std::unique_ptr<reader_insertion_hooks::Registration> registration;
    Outcome snapshot(){std::lock_guard<std::mutex> l(mutex);return outcome;}
    void complete(){std::lock_guard<std::mutex> l(mutex);auto &o=outcome;
        o.terminal=o.witness=o.attributed=o.forwardCommitted=o.innerReturned=o.outerReturned=o.condition.matched=true;
        o.jobId=12;o.outerCalls=o.innerCalls=o.trackedForwards=1;
    }
};
Model *current=nullptr;
void pair(void*out,void*raw,void*control){std::array<std::uintptr_t,2> w{{reinterpret_cast<std::uintptr_t>(raw),reinterpret_cast<std::uintptr_t>(control)}};std::memcpy(out,w.data(),sizeof w);}
void resolve(void*out,void*c,const void*){check(c==current->controller,"actual controller");pair(out,current->document,&current->docControl);}
void acquire(void*out,void*d,int mode){check(d==current->document&&mode==1,"actual retained document");pair(out,current->worker,&current->workerControl);}
qulonglong submit(void*w,const std::function<void()>&cb){check(w==current->worker,"actual worker");current->worker->queue({11,cb,-1});return 11;}
void forbidden(void*){qFatal("ordinary native owner release");}
Release release(Kind,std::uintptr_t,std::uintptr_t){
    check(current->coreRetires==1&&!current->entry&&current->clears==1,"Core and facade and attribution retired before owners");
    ++current->releases;return Release::Released;
}
reader_insertion_native_profile::AttributionParts attribution(QObject*w,bool condition){
    check(w==current->worker&&condition,"attribution identity");++current->attributions;
    return {current->document,reinterpret_cast<reader_insertion_job_snapshot::Snapshot*>(current),attribute,nullptr,{conditionWords,search,equal,tree,find}};
}
bool clearAttribution(const reader_insertion_native_profile::AttributionParts&p){
    check(p.context==reinterpret_cast<reader_insertion_job_snapshot::Snapshot*>(current)&&current->coreRetires==1&&!current->entry&&!current->releases,"attribution exact cleanup order");
    ++current->clears;return true;
}
}
namespace reader_worker_native::testing {
struct SlotAccess {
    static Slot *make(){
#if defined(__arm__)
        const auto *id=QMetaType::fromType<entry::Id>().iface();
#else
        const auto *id=QMetaType::fromType<OwnedNativeId>().iface();
#endif
        auto *s=new Slot({resolve,acquire,submit,forbidden,id,&DocumentWorker::staticMetaObject},nullptr);
        s->testRelease_=release;s->testAttribution_=attribution;s->testRetireAttribution_=clearAttribution;return s;
    }
    static void destroy(Slot*s){delete s;}
};
struct MutationAccess {
    static Mutation *make(reader_insertion_entry::Runtime r){return new Mutation(std::move(r));}
    static void configure(Mutation&m,std::function<void(Entry&)> entry,std::shared_ptr<Model> model,bool late){
        m.testEntry_=std::move(entry);
        m.testCoreSnapshot_=[model,late]{auto o=model->snapshot();if(late)o.interference=true;return o;};
        m.testCoreRetire_=[model]{check(!model->entry&&model->canceled==0,"normal Coordinator disarmed before Core retirement");++model->coreRetires;model->registration.reset();return true;};
    }
    static void destroy(Mutation*m){delete m;}
};
}
int main(int argc,char**argv){
    QGuiApplication app(argc,argv);qRegisterMetaType<entry::Id>("entry::Id");
    QMetaType::registerConverter<entry::Id,QString>([](const entry::Id&id){return id.text;});
    using SA=reader_worker_native::testing::SlotAccess;using MA=reader_worker_native::testing::MutationAccess;
    for(const QByteArray mode:{"normal","nested","preflight","cancel","late-core"}){
        QQmlEngine engine;QQuickWindow window;QQmlEngine::setContextForObject(&window,engine.rootContext());window.show();window.requestActivate();pump();
        DocumentWorker worker;SceneController scene;scene.value=&worker;Reader reader;reader.value=&scene;
        QmlDocumentWrapper doc;xofm::libs::library::DocumentController controller;
        auto model=std::make_shared<Model>();current=model.get();model->worker=&worker;model->controller=&controller;model->document=&doc;
        auto *slot=SA::make();reader_insertion_entry::Runtime runtime;runtime.owners=slot;
        runtime.calls={words,search,equal};runtime.admission={true,true,true,true,true,true};runtime.conditionAdmission={true,true,true,true,true,true};
        runtime.outerReturnPC=0x1000;runtime.innerReturnPC=0x2000;
        runtime.makeAttribution=[slot](QObject*,const Slot::Token&t){const auto a=slot->attribution(t);return reader_insertion_entry::Attribution{a.map,a.context,a.read64,a.postRejectProbe,a.conditionCalls};};
        auto *executor=MA::make(runtime);model->executor=executor;Mutation::Token token,stale;QJSValue oldCallback;
        const unsigned rounds=(mode=="normal"||mode=="nested"||mode=="preflight")?3:1;
        for(unsigned round=0;round<rounds;++round){
            model->registrations=model->coreRetires=model->releases=model->attributions=model->clears=model->canceled=0;
            {std::lock_guard<std::mutex> l(model->mutex);model->outcome={};}
            reader_insertion_entry::Config config;config.wantConditionalForward=true;config.budgetMs=2000;
            config.request.documentId=doc.key.text;config.request.expectedPages=doc.pages;config.request.sourceIndex=round==1?doc.pages.size()-1:0;
            config.request.sourcePageId=doc.pages[config.request.sourceIndex];config.request.targetPageId=QStringLiteral("20000000-0000-0000-0000-%1").arg(round+1,12,10,QLatin1Char('0'));
            check(config.valid(),"valid request");unsigned samples=0,delivered=0;
            reader_insertion_entry::Result result;
            MA::configure(*executor,[&](Entry&e){
                model->entry=&e;Entry::TestBoundary test;
                test.sample=[&](QQuickWindow*,const reader_insertion_entry::Permit&guard,int,const reader_selected::ValidatedSourceAction&action){
                    ++samples;reader_selected::Observation o;o.documentId=doc.key.text;o.pageId=config.request.sourcePageId;o.pageIndex=config.request.sourceIndex;o.pageCount=doc.pages.size();
                    o.observed=o.creationJoinObserved=o.mapCoherent=o.ready=true;
                    o.loading=o.pageError=o.documentError=o.awaitingTiles=o.pendingTiles=o.working=false;
                    if(guard())action(&reader,&doc,&controller,QVariant::fromValue(doc.key),o,guard);
                    return o;
                };
#if !defined(__arm__)
                // Host QString has a different size; the slot receives an owned
                // 12-byte ABI model. ARM runs the actual typed Coordinator path.
                test.acquire=[&e,slot](QObject*c,const QVariant&,QObject*w,const reader_insertion_entry::Permit&permit){auto p=slot->prepare(c,QVariant::fromValue(OwnedNativeId{}),w,permit);e.testingOwnerToken(p.token);return p.ready;};
#endif
                test.registerOnce=[model](reader_insertion_hooks::Registration&&r){++model->registrations;model->registration=std::make_unique<reader_insertion_hooks::Registration>(std::move(r));return true;};
                test.snapshot=[model]{return model->snapshot();};test.cancel=[model]{++model->canceled;};e.testing(std::move(test));
                auto *child=new QObject(&e);QObject::connect(child,&QObject::destroyed,&window,[&]{check(executor->busy()&&!model->coreRetires&&!model->releases,"destruction before physical retirement");if(mode=="nested")pump(15);check(!model->releases,"nested destruction cannot retire owners");});
                if(mode=="nested")QObject::connect(e.findChild<reader_joined_creation::Witness*>(),&reader_joined_creation::Witness::finished,&window,[&]{pump(30);check(model->entry&&!model->coreRetires&&!model->releases,"child producer stack blocks cleanup");});
            },model,mode=="late-core");
            controller.onCall=[&]{
                check(model->registrations==1,"registration precedes ordinary invoke");doc.pages.insert(config.request.sourceIndex+1,config.request.targetPageId);
                worker.queue({12,[model]{model->complete();},config.request.sourceIndex+1});
                if(mode=="cancel")check(executor->cancel(token),"cancel original token");
            };
            check(!executor->cancel(stale),"stale request cannot cancel replacement");
            const bool preflight=mode=="preflight"&&round==0;
            token=executor->start(preflight?nullptr:&window,config,QByteArray(32,'a'+round),[]{return true;},[&](const auto&r){++delivered;result=r;check(executor->busy(),"busy through publication");});
            if(oldCallback.isCallable())oldCallback.call(); // Old disarmed bridge during replacement A's active lifetime.
            check(bool(token),"request consumed");QElapsedTimer deadline;deadline.start();while(!delivered&&deadline.elapsed()<3000)pump();
            check(delivered==1&&!model->entry,"published after facade cleanup");
            if(preflight){check(!result.observed&&!result.unknown&&!executor->busy()&&!slot->occupied(),"known preflight refusal reusable");check(!model->registrations&&!model->coreRetires&&!model->attributions&&!model->releases,"preflight never enters native path");}
            else if(rounds>1){check(result.observed&&!result.unknown&&!executor->busy()&&!slot->occupied(),"normal complete reusable path");check(model->coreRetires==1&&model->releases==2&&model->clears==1&&samples>=3,"exact lifecycle and post-order");}
            else {check(!result.observed&&executor->busy()&&slot->occupied()&&!model->coreRetires&&!model->releases,"uncertain path quarantined");if(mode=="late-core")check(result.core.interference&&result.unknown,"late evidence published");}
            if(controller.retained.isCallable()){controller.retained.call();oldCallback=controller.retained;}
            pump();check(delivered==1,"late JS callback disarmed");stale=token;
        }
        MA::destroy(executor);SA::destroy(slot);current=nullptr;
    }
    std::printf("resident mutation full owned pipeline: %u checks PASS\n",checks);
}
#include "resident_mutation_pipeline_test.moc"
