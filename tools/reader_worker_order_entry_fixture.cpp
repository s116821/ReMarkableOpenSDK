// Owned QML source + QThread worker; no private native calls or tablet access.
#define READER_ENTRY_OWNED_HELPERS_ONLY
#include "reader_selected_view_entry_fixture.cpp"
#define READER_WORKER_ORDER_TESTING
#define READER_WORKER_ORDER_ENTRY_TESTING
#include "reader_worker_order_entry.h"
#include <QSemaphore>
#include <stdexcept>

class DocumentWorker final:public QThread {
    Q_OBJECT
public:
    std::function<void()> callback;std::array<QString,3> pages;
    qulonglong job=18446744073709551614ULL;
    bool hold=false,invoke=true,inlineGui=false,empty=false,badInverse=false;
    unsigned reads=0;QSemaphore gate;
    ~DocumentWorker()override{gate.release();wait();}
    qulonglong submit(const std::function<void()>&value){callback=value;if(inlineGui){callback();emit jobCompleted(job);}else start();return job;}
    void run()override{if(hold)gate.acquire();if(invoke)callback();emit jobCompleted(job);}
    static QString idAt(QObject *object,int index){auto *w=static_cast<DocumentWorker*>(object);++w->reads;return w->empty?QString():w->pages.at(size_t(index));}
    static int indexFor(QObject *object,const QString&id){auto *w=static_cast<DocumentWorker*>(object);++w->reads;if(w->badInverse)return -1;for(int i=0;i<3;++i)if(w->pages[size_t(i)]==id)return i;return -1;}
signals:
    void jobCompleted(qulonglong);
};
struct OwnedLease {
    bool *closed,*destroyed;
    static bool close(void *p){auto *s=static_cast<OwnedLease*>(p);*s->closed=true;return true;}
    static void destroy(void *p){auto *s=static_cast<OwnedLease*>(p);*s->destroyed=true;delete s;}
};
int main(int argc,char**argv){
    QGuiApplication app(argc,argv);
    qmlRegisterType<QObject>("OwnedProbe",1,0,"QtObject");qmlRegisterType<QQuickItem>("OwnedProbe",1,0,"Item");
    qmlRegisterType<FocusFixture>("OwnedProbe",1,0,"FocusScope");qmlRegisterType<SceneView>("OwnedProbe",1,0,"SceneFixture");
    qmlRegisterType<OwnershipOwner>("OwnedLegacyEntry",1,0,"Owner");registerCreationFixture();
    const QByteArray nonce(32,'d'),identity=reader_selected::selfIdentity();int modes=0;
    for(const QByteArray&mode:{QByteArray("valid"),QByteArray("empty"),QByteArray("inverse"),QByteArray("inline"),QByteArray("skipped"),QByteArray("zero"),
            QByteArray("throw-submit"),QByteArray("cancel-submit"),QByteArray("deadline-submit"),QByteArray("replace-submit"),QByteArray("before-cancel"),
            QByteArray("wrong-source"),QByteArray("unready"),QByteArray("postlink-cancel"),QByteArray("postlink-deadline"),QByteArray("duplicate-result"),QByteArray("nested-release")}){
        ++modes;Fixture f;
        f.document.pages={QStringLiteral("20000000-0000-0000-0000-000000000001"),QStringLiteral("20000000-0000-0000-0000-000000000002"),QStringLiteral("20000000-0000-0000-0000-000000000003")};
        f.reader->setProperty("currentPage",1);f.reader->setProperty("currentPageId",f.document.pages[1]);f.controller.setPageId(f.document.pages[1]);
        Library library;DocumentController creation;creation.setLibrary(&library);library.result=&f.document;
        f.engine.rootContext()->setContextProperty("externalCreation",&creation);
        DocumentWorker worker;for(int i=0;i<3;++i)worker.pages[size_t(i)]=f.document.pages[i];f.controller.workerValue=&worker;
        worker.empty=mode=="empty";worker.badInverse=mode=="inverse";worker.inlineGui=mode=="inline";worker.invoke=mode!="skipped";
        worker.hold=mode.endsWith("submit");if(mode=="zero")worker.job=0;
        xofm::bindings::qt::WindowNavigatorWrapper navigator;xofm::bindings::qt::WindowLayout layout;layout.activeValue=QStringLiteral("legacydevice/window/main");
        f.engine.rootContext()->setContextProperty("entryNavigator",&navigator);f.engine.rootContext()->setContextProperty("entryLayout",&layout);
        QQmlComponent component(&f.engine);component.setData(R"QML(import OwnedLegacyEntry 1.0
Owner {id:root;width:100;height:100;windowNavigator:entryNavigator;windowLayout:entryLayout;contentItem:mainView
Owner {id:mainView;width:100;height:100;windowNavigator:root.windowNavigator;windowLayout:root.windowLayout}}
)QML",QUrl(QStringLiteral("qrc:/qt/qml/xofm/modules/legacydevice/qml/LegacyMainWindow.qml")));
        require(component.isReady(),"worker tuple owned composition compiles");std::unique_ptr<QObject>composition(component.create());
        auto *visual=qobject_cast<QQuickItem*>(composition.get());require(visual,"worker tuple owned composition creates");
        visual->setParentItem(f.window.contentItem());visual->setParent(f.window.contentItem());f.reader->setVisible(false);navigator.onOpen=[&]{f.reader->setVisible(true);};
        QTemporaryDir base;const QString path=base.path()+"/root";require(base.isValid()&&QDir().mkdir(path)&&::chmod(path.toLocal8Bit().constData(),0700)==0,"private tuple root");
        putEntry(path+"/owner",nonce);putEntry(path+"/attempt.identity",identity);
        if(mode=="duplicate-result")putEntry(path+"/selected-result","preserved");
        reader_worker_order_entry::Config config{nonce,{f.document.key,1,true},worker.pages,3};
        bool closed=false,destroyed=false;int submissions=0;
        reader_worker_order_entry::Entry entry(&app,path,config);
        int nestedRelease=0;
        if(mode=="nested-release"){
            auto *pool=entry.findChild<reader_worker_quarantine::Pool*>();require(pool,"owned registration has resident pool");
            QObject::connect(pool,&reader_worker_quarantine::Pool::released,&entry,[&](quint64){
                QMetaObject::invokeMethod(&entry,[&]{++nestedRelease;require(readEntry(path+"/selected-result").isEmpty(),"nested release turn cannot publish before notification Frame unwinds");},Qt::QueuedConnection);
                pumpEntry();
                require(readEntry(path+"/selected-result").isEmpty(),"release notification itself is not settled publication");
            });
        }
        entry.adaptersForTest(&DocumentWorker::idAt,&DocumentWorker::indexFor,[&](QObject*source){
            require(source==&worker&&entry.samplingForTest(),"tuple lease prepares only qualified source stack");
            return reader_worker_quarantine::Lease{new OwnedLease{&closed,&destroyed},source,&OwnedLease::close,&OwnedLease::destroy};
        },[&](const std::function<void()>&callback){
            ++submissions;require(entry.samplingForTest(),"submit remains under source Sample");const auto job=worker.submit(callback);
            if(mode=="throw-submit")throw std::runtime_error("owned uncertain submission");
            if(mode=="cancel-submit")putEntry(path+"/selected-cancel","stop");
            if(mode=="deadline-submit")entry.advanceForTest(30001);
            if(mode=="replace-submit"){require(QFile::rename(path+"/selected-request",path+"/saved-request"),"preserve request inode");putEntry(path+"/selected-request",nonce+' '+identity);}
            if(mode.endsWith("submit")){entry.checkForTest();pumpEntry();require(!closed&&!destroyed,"nested cancel/expiry does not release raw worker");}
            require(readEntry(path+"/selected-result").isEmpty(),"no tuple result on submit stack");return job;
        });
        if(mode=="postlink-cancel")entry.postLinkForTest([&]{putEntry(path+"/selected-cancel","stop");pumpEntry();});
        if(mode=="postlink-deadline")entry.postLinkForTest([&]{entry.advanceForTest(30001);pumpEntry();});
        const bool started=entry.start();pumpEntry();
        if(mode=="duplicate-result"){require(!started&&readEntry(path+"/selected-result")=="preserved"&&submissions==0,"prior output untouched");continue;}
        require(started&&readEntry(path+"/selected-ready")==nonce+' '+identity,"bound tuple readiness");
        if(mode=="before-cancel")putEntry(path+"/selected-cancel","stop");
        putEntry(path+"/selected-request",nonce+' '+identity);entry.checkForTest();pumpEntry();
        if(mode=="wrong-source")f.controller.setPageId(f.document.pages[0]);
        if(mode=="unready")f.current->setStatus(SceneView::Inactive);
        if(entry.opportunityPendingForTest())entry.opportunityForTest();
        pumpEntry();
        worker.gate.release();worker.wait();for(int n=0;n<30;++n)pumpEntry();
        const auto bytes=readEntry(path+"/selected-result");
        QTextStream(stdout)<<"owned mode "<<mode<<" "<<bytes<<'\n';
        if(mode=="valid"||mode=="postlink-cancel"||mode=="postlink-deadline"||mode=="nested-release"){
            validReceipt(bytes,nonce,identity,"worker-order-observed",4096);const auto json=QJsonDocument::fromJson(bytes).object();const auto w=json["worker"].toObject();
            require(json["tuple_observed"].toBool()&&json["tuple_calls"].toInt()==6&&worker.reads==6&&submissions==1&&closed&&destroyed,"one completed tuple closes exact owned lease");
            require(w["job_id"].toString()==QString::number(worker.job)&&w["owners_closed"].toBool()&&w["owners_destroyed"].toBool(),"lossless matching job and separate ownership facts");
            require(w["settled"].toBool()&&w["depth"].toInt()==0,"publication follows settled zero-depth release");
            if(mode=="nested-release")require(nestedRelease==1,"nested queued release regression exercised once");
            require(QFile::exists(path+"/.pending-result")==mode.startsWith("postlink"),"post-link uncertainty preserves pending name");
        }else if(mode=="before-cancel"||mode=="cancel-submit"||mode=="replace-submit")require(bytes.isEmpty(),"disarmed root cannot publish");
        else{validReceipt(bytes,nonce,identity,"worker-order-refused",4096);}
        if(mode.endsWith("submit"))require(!closed&&!destroyed,"uncertain/canceled raw-worker lease remains quarantined after late closure");
        if(mode=="inline"||mode=="skipped")require(worker.reads==0&&!closed,"wrong-thread/skipped callback cannot read or close native receiver");
        if(mode=="wrong-source"||mode=="unready"||mode=="before-cancel")require(submissions==0&&worker.reads==0,"invalid pre-submit source performs no worker action");
        if(mode=="empty"||mode=="inverse")require(closed&&destroyed&&worker.reads<6,"negative passive tuple can finish native ownership independently");
        // Unknown leases are deliberately retained in resident storage. Their
        // close callbacks are never run at fixture exit; no destructor cleanup.
    }
    QTextStream(stdout)<<"PASS worker tuple Entry "<<modes<<" owned modes; no native private calls\n";
    return 0;
}
#include "reader_worker_order_entry_fixture.moc"
