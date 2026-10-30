// Original owned Entry composition. Native acquisition/Core and Sample identity
// are explicit models; Qt callbacks, worker copies, files and order reads run.
#define READER_INSERTION_GUARD_TRANSPORT_TESTING 1
#define READER_INSERTION_GUARD_ENTRY_TESTING 1
#define main includedCoordinatorMain
#include "reader_insertion_guard_entry_fixture.cpp"
#undef main
#include "reader_insertion_guard_transport.h"
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <cerrno>

namespace reader_insertion_transport {
struct TestAccess {
    static void configure(Entry &e,std::function<void(reader_insertion_entry::Coordinator*)> fn) { e.configureCoordinator_=std::move(fn);e.opportunityDelay_=0; }
    static void bootstrap(Entry &e,decltype(Entry::bootstrap_) fn) { e.bootstrap_=std::move(fn); }
    static void readback(Entry &e,decltype(Entry::readback_) fn) { e.readback_=std::move(fn); }
    static void cleanup(Entry &e,std::function<void()> fn) { e.beforeCleanup_=std::move(fn); }
    static void beforeReadback(Entry &e,std::function<void()> fn) { e.beforeReadback_=std::move(fn); }
    static void close(Entry &e,std::function<int(int)> fn) { e.closeRequest_=std::move(fn); }
    static void publication(Entry &e,std::function<void()> before,std::function<void()> after) { e.preLink_=std::move(before);e.postLink_=std::move(after); }
    static void tick(Entry &e) { e.tick(); }
    static void finish(Entry &e) { e.finish(); }
    static void expire(Entry &e) { e.deadline_=Entry::Clock::now()-std::chrono::milliseconds(1); }
    static void expireSetup(Entry &e) { e.setupDeadline_=Entry::Clock::now()-std::chrono::milliseconds(1); }
    static auto deadline(const Entry &e) { return e.deadline_; }
    static reader_retirement::Fence *fence(Entry &e) { return e.fence_.get(); }
    static reader_retirement::Token mutation(const Entry &e) { return e.mutationToken_; }
    static bool mutationClean(const Entry &e) { return e.mutationCleanup_&&e.mutationObjectsGone_&&e.mutationFdClosed_&&!e.coordinator_; }
    static bool readStarted(const Entry &e) { return e.readbackStarted_; }
    static reader_insertion_entry::Coordinator *coordinator(const Entry &e) { return e.coordinator_; }
};
}
namespace resident_transport_owned {
using Transport=reader_insertion_transport::Entry;
using Access=reader_insertion_transport::TestAccess;
using Coordinator=reader_insertion_entry::Coordinator;
int assertions=0,cases=0,maxWire=0;
void verify(bool yes,const char *name) { if(!yes)qFatal("FAIL resident transport %s",name);++assertions; }
void events(int ms=3) { QElapsedTimer t;t.start();do {QCoreApplication::processEvents(QEventLoop::AllEvents,1);QThread::msleep(1);}while(t.elapsed()<ms); }
void writeFile(const QString &path,const QByteArray &bytes) {
    const auto encoded=path.toLocal8Bit();
    const int fd=::open(encoded.constData(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
    verify(fd>=0,"exclusive local file");
    verify(::fchmod(fd,0600)==0 && ::write(fd,bytes.constData(),size_t(bytes.size()))==bytes.size(),"exact private local bytes");
    verify(::close(fd)==0,"local writer close");
}
QByteArray readFile(const QString &path) { QFile file(path);return file.open(QIODevice::ReadOnly)?file.readAll():QByteArray(); }
bool committed(const QString &root) {
    struct stat st{};
    return ::lstat((root+"/selected-result").toLocal8Bit().constData(),&st)==0 && S_ISREG(st.st_mode) &&
        st.st_nlink==1 && !QFile::exists(root+"/.pending-result");
}
void exportCase(const QByteArray &mode,const QByteArray &wire,const QJsonObject &facts) {
    const QString output=qEnvironmentVariable("RESIDENT_MUTATION_WIRE_OUTPUT");
    if(output.isEmpty())return;
    verify(QDir().mkpath(output),"wire export directory");
    if(!wire.isEmpty())writeFile(output+'/'+QString::fromLatin1(mode)+".wire",wire);
    writeFile(output+'/'+QString::fromLatin1(mode)+".facts.json",QJsonDocument(facts).toJson(QJsonDocument::Compact)+'\n');
}
void runCase(const QByteArray &mode,QGuiApplication &app) {
    ++cases;
    QTemporaryDir temporary;
    verify(temporary.isValid(),"temporary local directory");
    const QString root=temporary.path()+"/root";
    verify(QDir().mkdir(root) && ::chmod(root.toLocal8Bit().constData(),0700)==0,"private request directory");
    QQmlEngine engine;
    auto window=std::make_unique<QQuickWindow>();
    QQmlEngine::setContextForObject(window.get(),engine.rootContext());
    window->show();window->requestActivate();events();
    QPointer<QQuickWindow> originalWindow=window.get();
    QPointer<QQmlEngine> originalEngine=&engine;
    std::unique_ptr<QQuickWindow> boundaryWindow;
    DocumentWorker worker;
    SceneController scene;scene.value=&worker;
    Reader reader;reader.value=&scene;
    QmlDocumentWrapper document;
    Controller controller;controller.mode=Controller::Async;
    if(mode=="n16") {
        document.pages.clear();
        for(int i=1;i<=16;++i)document.pages.append(QStringLiteral("10000000-0000-0000-0000-%1").arg(i,12,10,QLatin1Char('0')));
    }
    reader_insertion_transport::Config config;
    config.nonce=QByteArray(32,'a');config.mutationNonce=QByteArray(32,'b');config.readbackNonce=QByteArray(32,'c');config.residentFollowup=true;
    config.coordinator.request=requestFor(document);
    if(mode=="n16") {config.coordinator.request.sourceIndex=14;config.coordinator.request.sourcePageId=document.pages[14];}
    config.coordinator.wrongSourceIndex=-1;config.coordinator.wantConditionalForward=true;config.coordinator.wantTargetAcquisition=true;
    config.coordinator.budgetMs=4000;
    config.navigation={document.key.text,config.coordinator.request.sourceIndex,true};
    verify(config.valid(),"selected resident configuration valid");
    const auto request=config.coordinator.request;
    const auto initial=beforeFor(request);
    const QByteArray identity=reader_selected::selfIdentity();
    verify(!identity.isEmpty(),"actual owned process identity");
    writeFile(root+"/owner",config.nonce);writeFile(root+"/attempt.identity",identity);
    auto model=std::make_shared<Model>();
    reader_insertion_entry::Runtime runtime;
    runtime.calls={words,search,equal};runtime.admission={true,true,true,true,true,true};
    runtime.conditionAdmission={true,true,true,true,true,true};runtime.outerReturnPC=0x1000;runtime.innerReturnPC=0x2000;
    runtime.makeAttribution=[model](QObject *) {
        reader_insertion_entry::Attribution at{reinterpret_cast<void*>(0x30c8),model.get(),attribute,nullptr};
        at.conditionCalls={conditionWords,search,equal,tree,find};return at;
    };
    // These are closed serializer model inputs, not observed pre-main facts.
    const QJsonObject startup{{"profile_qualified",true},{"condition_qualified",true},{"application_absent_at_install",true},
        {"tasks_at_install",4096},{"bind_now",true},{"native_authority",false}};
    auto holder=std::make_unique<Transport>(&app,root,config,runtime,startup);
    QPointer<Transport> weak=holder.get();
    QPointer<Coordinator> facade;
    int bootstraps=0,cleanups=0,beforeReads=0,readbacks=0,closed=0,childDestroyed=0,finished=0;
    bool selected=false,nestedBlocked=false,boundaryTriggered=false;
    std::chrono::steady_clock::time_point originalDeadline{};
    const auto cancel=[&] { if(!QFile::exists(root+"/selected-cancel"))writeFile(root+"/selected-cancel","stop"); };
    const auto quit=[&] {verify(QMetaObject::invokeMethod(&app,"aboutToQuit",Qt::DirectConnection),"direct owned quit signal");};
    Access::bootstrap(*holder,[&](QQuickWindow *w,const reader_selected::FixtureNavigation &navigation,const std::function<bool()> &permit,int budget) {
        ++bootstraps;verify(w==originalWindow && navigation.documentId==request.documentId && permit() && budget>0,"modeled bootstrap uses actual binding");
        originalDeadline=Access::deadline(*holder);
        reader_selected::NavigationObservation result;
        if(mode=="bootstrap-refuse")result.refusal=QStringLiteral("owned-bootstrap-refusal");
        else result.attempted=result.dispatched=true;
        return result;
    });
    Access::configure(*holder,[&](Coordinator *coordinator) {
        facade=coordinator;
        auto *fence=Access::fence(*holder);const auto token=Access::mutation(*holder);
        verify(fence && fence->snapshot(token).depth>0,"coordinator configured under full construction frame");
        auto *child=new QObject(coordinator);
        QObject::connect(child,&QObject::destroyed,&app,[&] {
            ++childDestroyed;
            if(mode=="nested-child") {
                verify(weak && Access::fence(*weak)->snapshot(Access::mutation(*weak)).depth>0,"child destruction remains framed");
                events(40);verify(readbacks==0,"no B from nested child destruction");nestedBlocked=true;
            }
        });
        if(mode=="nested-retired")QObject::connect(fence,&reader_retirement::Fence::retired,&app,[&,fence,token](quint64 generation,QByteArray) {
            if(generation!=token.generation())return;
            verify(!fence->handoffReady(token),"retired signal is not yet a handoff");
            events(60);verify(readbacks==0 && !fence->handoffReady(token),"heartbeat cannot admit B inside retired listener");nestedBlocked=true;
        });
        Coordinator::TestBoundary test;
        test.sample=[&](QQuickWindow *w,const reader_insertion_entry::Permit &permit,int budget,const reader_selected::ValidatedSourceAction &action) {
            ++model->samples;verify(w==originalWindow && budget>0 && budget<=config.coordinator.budgetMs,"modeled A Sample budget/window");
            auto observed=initial;
            if(model->samples>=3) {
                observed.pageCount=document.pages.size();
                observed.pageId=selected?request.targetPageId:request.sourcePageId;
                observed.pageIndex=request.sourceIndex+(selected?1:0);
            }
            if(permit())action(&reader,&document,&controller,QVariant::fromValue(document.key),observed,permit);
            return observed;
        };
        test.acquire=[model](QObject *c,const QVariant &id,QObject *w,const reader_insertion_entry::Permit &permit) {
            ++model->acquisitions;return c && id.isValid() && w && permit();
        };
        test.submitProbe=[&,model](const std::function<void()> &callback,const reader_insertion_entry::Permit &permit) {
            verify(permit(),"synchronous modeled enqueue permission");++model->probes;
            reader_worker_native::Submission submission;submission.attempted=submission.accepted=true;submission.jobId=UINT64_MAX-3;
            worker.queue({submission.jobId,callback,true,false,false});return submission;
        };
        test.registerOnce=[model](reader_insertion_hooks::Registration &&registration) {
            ++model->registrations;model->registration=std::make_unique<reader_insertion_hooks::Registration>(std::move(registration));return true;
        };
        test.snapshot=[model]{return model->snapshot();};
        test.cancel=[model]{++model->canceled;};
        coordinator->testing(std::move(test));
        if(mode=="a-boundary-expire" || mode=="a-boundary-window") {
            // Preserve the actual finished->Entry finish route, inserting a
            // source-bound outer-admission change before its existing receiver.
            verify(QObject::disconnect(coordinator,SIGNAL(finished()),holder.get(),nullptr),"replace only the actual outer finish receiver order");
            QObject::connect(coordinator,&Coordinator::finished,&app,[&] {
                verify(facade && facade->snapshot().observed,"inner Coordinator is already positive before outer boundary changes");
                boundaryTriggered=true;
                if(mode=="a-boundary-expire")Access::expire(*holder);
                else {
                    boundaryWindow=std::make_unique<QQuickWindow>();
                    QQmlEngine::setContextForObject(boundaryWindow.get(),engine.rootContext());
                    boundaryWindow->show();boundaryWindow->requestActivate();events(4);
                    verify(QGuiApplication::focusWindow()==boundaryWindow.get(),"actual focused window changed before outer A admission");
                }
                verify(facade && facade->snapshot().observed && !readbacks,"outer revocation cannot rewrite inner positive or start B");
            });
            QObject::connect(coordinator,&Coordinator::finished,holder.get(),[&] {if(weak)Access::finish(*weak);});
        }
        QObject::connect(coordinator,&Coordinator::finished,&app,[&] {
            ++finished;
            if(mode=="nested-finished") {events(45);verify(!cleanups && !readbacks,"finished producer blocks nested retirement");nestedBlocked=true;}
        });
        if(mode=="unexpected-facade")QTimer::singleShot(0,&app,[&] {if(facade)delete facade;});
    });
    controller.onCall=[&] {
        verify(model->registrations==1 && controller.calls==1 && controller.lastId==document.key &&
            controller.index==request.sourceIndex+1 && controller.target==request.targetPageId,"one actual ordinary Qt invocation with modeled effect");
        document.pages.insert(request.sourceIndex+1,request.targetPageId);selected=true;
        worker.queue({UINT64_MAX-2,[model,request] {
            model->complete("conditional-middle");
            std::lock_guard<std::mutex> lock(model->mutex);
            auto &c=model->outcome.condition;
            c.wordReads=c.treeReads=2;c.searches=2;c.equalities=c.finds=1;c.count=request.expectedPages.size();c.treeCount=c.count;
            c.sourcePosition=request.sourceIndex;c.rightPosition=request.sourceIndex+1;
            c.sourceAtIndex=c.rightAdjacent=c.targetMatches=c.targetAbsent=c.spanUnchanged=c.treeUnchanged=true;
        },true,false,true,request.sourceIndex+1});
    };
    Access::cleanup(*holder,[&] {
        ++cleanups;verify(!readbacks,"A cleanup before all B reads");
        if(mode=="cleanup-cancel")cancel();
        if(mode=="cleanup-expire")Access::expire(*holder);
        if(mode=="cleanup-throw")throw std::runtime_error("owned cleanup failure");
        if(mode=="cleanup-delete")holder.reset();
    });
    Access::close(*holder,[&](int fd) {
        ++closed;verify(fd>=0 && ::fcntl(fd,F_GETFD)>=0,"actual dedicated request descriptor live");
        const bool refuse=(mode=="close-a" && closed==1)||(mode=="close-b" && closed==2);
        const int result=::close(fd);verify(result==0,"owned actual descriptor closed once");
        errno=0;verify(::fcntl(fd,F_GETFD)==-1 && errno==EBADF,"closed descriptor invalid before any reuse");
        if(refuse){errno=EIO;return -1;}return 0;
    });
    Access::beforeReadback(*holder,[&] {
        ++beforeReads;verify(!facade && childDestroyed==1 && Access::mutationClean(*holder),"all A QObjects and FD retired before B phase");
        verify(Access::deadline(*holder)==originalDeadline,"B cannot renew original operation clock");
        if(mode=="before-b-cancel")cancel();
        if(mode=="before-b-expire")Access::expire(*holder);
        if(mode=="setup-expire")Access::expireSetup(*holder);
        if(mode=="window-loss")window.reset();
    });
    Access::readback(*holder,[&](QQuickWindow *w,const std::function<bool()> &permit,int budget,const reader_selected::ValidatedSourceAction &action) {
        ++readbacks;
        verify(weak && !facade && Access::mutationClean(*weak) && beforeReads==1,"fresh B after exact A cleanup");
        verify(w==originalWindow && qmlEngine(w)==originalEngine && reader_selected::selfIdentity()==identity &&
            Access::deadline(*weak)==originalDeadline && budget>0 && budget<=20000,"B same process/window/engine and shared original deadline");
        auto joined=initial;joined.pageCount=document.pages.size();joined.pageId=request.targetPageId;joined.pageIndex=request.sourceIndex+1;
        if(mode=="b-delete"){holder.reset();return reader_selected::Observation{};}
        if(mode=="b-throw")throw std::runtime_error("owned readback throw");
        if(mode=="b-unready"){joined={};joined.refusal=QStringLiteral("worker-source-unready");return joined;}
        if(mode=="b-wrong-order")document.pages.swapItemsAt(0,1);
        if(mode=="b-source"){joined.pageId=request.sourcePageId;joined.pageIndex=request.sourceIndex;}
        if(mode!="b-no-action" && permit())action(&reader,&document,&controller,QVariant::fromValue(document.key),joined,permit);
        return joined;
    });
    const auto publicationChange=[&] {
        if(mode.endsWith("quit"))quit();
        if(mode.endsWith("delete"))holder.reset();
    };
    if(mode.startsWith("pre-"))Access::publication(*holder,publicationChange,{});
    if(mode.startsWith("post-"))Access::publication(*holder,{},publicationChange);
    verify(holder->start(),"actual resident Entry starts once");
    writeFile(root+"/selected-request",config.nonce+' '+identity);
    Access::tick(*holder);
    QElapsedTimer elapsed;elapsed.start();
    while(weak && !weak->terminal() && elapsed.elapsed()<6000)events();
    verify(!weak || weak->terminal(),"bounded actual Entry terminal or explicitly deleted");
    events(4);
    const QByteArray wire=readFile(root+"/selected-result");
    const bool commit=committed(root);
    const auto parsed=wire.isEmpty()?QJsonObject():QJsonDocument::fromJson(wire).object();
    const bool positive=parsed.value("stage")==QStringLiteral("resident-mutation-observed");
    const bool expected=mode=="positive" || mode=="n16" || mode.startsWith("nested-") || mode=="late-js";
    verify((commit && positive)==expected,"combined positive only for admitted complete lifecycle");
    if(expected) {
        const auto r=parsed.value("resident").toObject();
        verify(parsed.value("schema").toInt()==4 && parsed.value("diagnostic")=="insertion-guard-resident" && wire.size()<=8192,"actual closed schema4 within selected cap");
        verify(r.value("mutation_retired").toBool() && r.value("mutation_cleanup").toBool() && r.value("mutation_fd_closed").toBool() &&
            r.value("mutation_objects_gone").toBool() && r.value("readback_retired").toBool() && r.value("readback_fd_closed").toBool() &&
            r.value("unresolved_mutation").toBool() && r.value("further_mutation_refused").toBool(),"actual retirement and unresolved-latch conjunction");
        verify(r.value("mutation_generation")=="1" && r.value("readback_generation")=="2" &&
            r.value("mutation_nonce")==QString::fromLatin1(config.mutationNonce) && r.value("readback_nonce")==QString::fromLatin1(config.readbackNonce),"fresh correlated fixed A/B identities");
        const auto order=r.value("readback_order").toObject();
        verify(order.value("observed").toBool() && order.value("target_selected").toBool() &&
            order.value("page_count").toInt()==document.pages.size() && order.value("ids").toArray().size()==document.pages.size() &&
            order.value("current_page_id")==request.targetPageId && order.value("current_page_index").toInt()==request.sourceIndex+1,"fresh B real full-order method reads");
        verify(closed==2 && cleanups==1 && childDestroyed==1 && !facade && readbacks==1 && finished==1,"one A cleanup and one B without capacity reuse");
        verify(model->canceled==0 && model->samples>=3 && controller.calls==1 && model->probes==1 && model->registrations==1,"controlled disarm and no effect retry");
        if(mode.startsWith("nested-"))verify(nestedBlocked,"selected nested boundary actually reached");
        if(mode=="n16")verify(document.pages.size()==17 && wire.size()>4096,"actual max-page wire exercises distinct schema4 cap");
        if(mode=="late-js") {
            const auto original=wire;const int b=readbacks;controller.retained.call();events(35);
            verify(readbacks==b && readFile(root+"/selected-result")==original && controller.calls==1,"late original JS callback cannot revive A or repeat B");
        }
    } else {
        verify(readbacks<=1 && model->probes<=1 && model->registrations<=1 && controller.calls<=1,"failure never retries readback or effects");
        if(mode.startsWith("cleanup-") || mode=="close-a" || mode=="unexpected-facade" || mode=="bootstrap-refuse")verify(readbacks==0,"failed A cannot start B");
        if(mode=="before-b-cancel" || mode=="before-b-expire" || mode=="setup-expire" || mode=="window-loss")verify(beforeReads==1 && readbacks==0,"revoked pre-B permission prevents getters");
        if(mode=="pre-quit" || mode=="pre-delete")verify(wire.isEmpty(),"pre-link suppression preserves pending only");
        if(mode=="post-quit" || mode=="post-delete")verify(!commit && QFile::exists(root+"/.pending-result"),"post-link uncertainty remains forensic");
        if(mode=="a-boundary-expire" || mode=="a-boundary-window") {
            const auto resident=parsed.value("resident").toObject();
            verify(boundaryTriggered && commit && readbacks==0 && controller.calls==1 && finished==1,
                "positive inner A plus expired or changed outer admission never starts B");
            verify(parsed.value("coordinator_observed").toBool() && parsed.value("stage")=="resident-mutation-incomplete" &&
                parsed.value("mutation_stage")=="insertion-target-incomplete" && !parsed.value("mutation_refusal").toString().isEmpty(),
                "wire retains inner positive while classifying outer A failure");
            verify(resident.value("mutation_same_process").toBool() &&
                resident.value("mutation_same_window_engine").toBool()==(mode=="a-boundary-expire") &&
                !resident.value("readback_retired").toBool(),"frozen outer A context and no B completion are explicit");
        }
    }
    if(!wire.isEmpty()) {
        verify(!parsed.value("native_authority").toBool() && !parsed.value("render_authority").toBool() && !parsed.value("durable_authority").toBool(),"models grant no authority");
        maxWire=qMax(maxWire,int(wire.size()));
    }
    exportCase(mode,wire,QJsonObject{{"mode",QString::fromLatin1(mode)},{"committed",commit},{"positive_bytes",positive},
        {"expected_positive",expected},{"wire_bytes",int(wire.size())},{"b_reads",readbacks},{"cleanups",cleanups},{"descriptor_closes",closed},
        {"native_modeled",true},{"sample_identity_modeled",true},{"native_execution",false}});
    holder.reset();
    verify(!facade,"no remaining request facade after owned Entry deletion");
    controller.onCall={};document.onRead={};
    QTextStream(stdout)<<"PASS resident-transport-"<<mode<<'\n';
}
}
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);
    qRegisterMetaType<entry::Id>("entry::Id");
    QMetaType::registerConverter<entry::Id,QString>([](const entry::Id &id){return id.text;});
    for(const QByteArray &mode : {QByteArray("positive"),QByteArray("n16"),QByteArray("nested-finished"),QByteArray("nested-child"),QByteArray("nested-retired"),
            QByteArray("late-js"),QByteArray("cleanup-cancel"),QByteArray("cleanup-expire"),QByteArray("cleanup-throw"),QByteArray("cleanup-delete"),
            QByteArray("close-a"),QByteArray("close-b"),QByteArray("before-b-cancel"),QByteArray("before-b-expire"),QByteArray("setup-expire"),
            QByteArray("window-loss"),QByteArray("b-wrong-order"),QByteArray("b-source"),QByteArray("b-unready"),QByteArray("b-no-action"),
            QByteArray("b-throw"),QByteArray("b-delete"),QByteArray("pre-quit"),QByteArray("post-quit"),QByteArray("pre-delete"),QByteArray("post-delete"),
            QByteArray("unexpected-facade"),QByteArray("bootstrap-refuse"),QByteArray("a-boundary-expire"),QByteArray("a-boundary-window")})resident_transport_owned::runCase(mode,app);
    QTextStream(stdout)<<"PASS resident mutation transport scenarios="<<resident_transport_owned::cases<<" checks="<<resident_transport_owned::assertions
        <<" max_wire="<<resident_transport_owned::maxWire<<"; native and Sample identity explicitly modeled\n";
    return 0;
}
