// Owned objects only. No native page, manual cancel or forced application quit.
#include "qt_pretoken_shutdown_entry.h"
#define main unusedOwnedFactsMain
#include "qt_page_facts_fixture.cpp"
#undef main
#include <cassert>
#include <QDir>
#include <QFile>

namespace qml_access {
struct PretokenFixtureAccess {
    static void request(FactsEntry &e){e.queueRequest();}
    static bool direct(FactsEntry &e){return e.acceptCaptureRequest();}
    static bool untouched(const FactsEntry &e){
        return !e.consumed_ && !e.captureConsumed_ && !e.focusDiscoveryStarted_ &&
            !e.focusArmed_ && !e.reading_ && !e.captureReading_ && !e.reader_ &&
            e.acceptedAt_<0 && e.captureAcceptedAt_<0 && e.sourceFactsBegin_<0;
    }
    static void normalFallback(FactsEntry &e){
        e.config_.developmentCaptureObservation=false;
        e.config_.developmentReceiverSourceFacts=false;
        e.config_.developmentReceiverSubtreeCapture=false;
    }
    static void inputFallback(FactsEntry &e){e.config_.developmentInputObservation=true;}
    static void capturePurpose(FactsEntry &e){e.config_.developmentReceiverSourceFacts=false;}
    static bool posted(const FactsEntry &e){return e.pretokenBootstrapPosted_;}
    static bool done(const FactsEntry &e){return e.done_;}
    static bool delivered(const FactsEntry &e){return !e.completed_;}
    static QString stage(const FactsEntry &e){return e.result_.stage;}
};
}
static void put(const QString &name,const QByteArray &bytes){
    QFile f(name);assert(f.open(QIODevice::WriteOnly|QIODevice::NewOnly));
    assert(f.setPermissions(QFile::ReadOwner|QFile::WriteOwner));
    assert(f.write(bytes)==bytes.size());
}
static QByteArray read(const QString &name){
    QFile f(name);assert(f.open(QIODevice::ReadOnly));return f.readAll();
}
static void drain(){for(int i=0;i<8;++i)QCoreApplication::processEvents();}
int main(int argc,char **argv){
    assert(argc==2);const QByteArray mode(argv[1]);
    const bool refusal=mode=="identity-refused" || mode=="config-refused" || mode=="stale-refused";
    constexpr auto nonce="00000000000000000000000000000000";
    auto config=pretoken_shutdown::config(nonce);
    const QString root=config.directory;
    assert(::mkdir(root.toUtf8().constData(),0700)==0);
    put(root+"/owner",nonce);
    const auto identity=QByteArray::number(::getpid())+' '+FactsEntry::processStart()+'\n';
    put(root+"/attempt.identity",identity);
    auto *recorder=shutdown_trace::Recorder::open(root.toUtf8().constData(),nonce);assert(recorder);
    QGuiApplication app(argc,argv);app.setQuitOnLastWindowClosed(false);
    recorder->record(shutdown_trace::Event::Startup);
    if(mode=="nonzero-frame")recorder->beforeRender(); // actual recorder increment, not reset
    if(mode=="identity-refused"){
        assert(QFile::remove(root+"/attempt.identity"));put(root+"/attempt.identity","1 1\n");
    }
    if(mode=="config-refused")config.facts.documentId=QStringLiteral("invalid");
    if(mode=="stale-refused")put(root+"/facts-request","unexpected-before-start");
    auto entry=pretoken_shutdown::start(&app,config,recorder);assert(entry);
    int destroyed=0;QObject::connect(entry,&QObject::destroyed,[&]{++destroyed;});
    assert(entry->pretokenInstalled()!=refusal);
    assert(PretokenFixtureAccess::posted(*entry)!=refusal);
    assert(!QFile::exists(root+"/callback.json")); // completion has not run inline
    auto trace=read(root+"/shutdown-trace.log");
    assert(trace.count(" entry-installed ")==int(!refusal));
    if(!refusal){
        const auto fields=trace.trimmed().split('\n').back().split(' ');
        assert(fields.size()==10 && fields[2]=="entry-installed");
        assert(fields[4]==QByteArray::number(::getpid()) && fields[7]=="0" && fields[8]=="0");
        assert(fields[9]==(mode=="nonzero-frame" ? "1":"0"));
    }
    std::unique_ptr<QQmlApplicationEngine> engine;
    DocumentFixture document;document.ids=config.facts.expectedOrder;
    int getterCalls=0;document.hook=[&]{++getterCalls;};
    if(!refusal){
        qmlRegisterType<SceneView>("OwnedFacts",1,0,"SceneView");
        qmlRegisterType<OwnedReceiver>("OwnedFacts",1,0,"OwnedReceiver");
        engine=std::make_unique<QQmlApplicationEngine>();
        engine->rootContext()->setContextProperty("ownedDocument",&document);
        engine->loadData(R"QML(import QtQuick
import QtQuick.Window
import OwnedFacts 1.0
Window { visible:true; width:400; height:400
 OwnedReceiver { id:receiver; objectName:"receiver"; anchors.fill:parent; focus:true
  property QtObject document:ownedDocument
  property int currentPage:0
  property string currentPageId:scene.pageId
  SceneView { id:scene; objectName:"scene"; anchors.fill:parent; focus:true
   document:receiver.document; pageId:"00000000-0000-4000-8000-000000000002"
  }
 }
})QML");
        assert(engine->rootObjects().size()==1);
        drain();assert(entry && QFile::exists(root+"/facts-waiting"));
        const int originalIds=document.idReads,originalCalls=getterCalls;
        const auto waiting=read(root+"/facts-waiting").trimmed().split(' ');
        assert(waiting.size()==9);
        const auto prefix=waiting.mid(0,5).join(' ');
        const auto suffix=" 120000 main-dev-facts-120s\n";
        if(mode=="normal-fallback")PretokenFixtureAccess::normalFallback(*entry);
        if(mode=="input-fallback")PretokenFixtureAccess::inputFallback(*entry);
        if(mode=="capture-valid")PretokenFixtureAccess::capturePurpose(*entry);
        const QByteArray sourceToken=mode=="malformed" ? QByteArray("bad\n") : prefix+" receiver-source-facts"+suffix;
        if(mode=="normal-fallback")put(root+"/facts-request",prefix+" read-facts"+suffix);
        else if(mode=="input-fallback")put(root+"/input-observation-end",prefix+" end-input-observation"+suffix);
        else if(mode=="capture-valid")put(root+"/capture-observation-request",prefix+" capture-observation"+suffix);
        else put(root+"/receiver-source-facts-request",sourceToken);
        for(int i=0;i<3;++i){PretokenFixtureAccess::request(*entry);drain();}
        assert(!PretokenFixtureAccess::direct(*entry));
        assert(PretokenFixtureAccess::untouched(*entry) && !PretokenFixtureAccess::done(*entry));
        assert(document.idReads==originalIds && getterCalls==originalCalls);
        for(const char *name:{"receiver-source-facts.json","capture-window.png","capture-observation-complete.json","diagnostics.json","callback.json"})
            assert(!QFile::exists(root+'/'+name));
        // Original engine destroyed -> cancel -> finish; completion remains queued.
        engine.reset();assert(entry && destroyed==0 && PretokenFixtureAccess::done(*entry));
        assert(!entry->pretokenInstalled() && !QFile::exists(root+"/callback.json"));
    }
    // Deliver only queued calls so the deleteLater boundary stays observable.
    const QString expected=refusal ? mode=="identity-refused" ? "facts-entry-generation-refused":mode=="config-refused" ? "facts-entry-config-refused":"facts-entry-stale-refused":"facts-entry-canceled";
    assert(PretokenFixtureAccess::stage(*entry)==expected && !PretokenFixtureAccess::delivered(*entry));
    QCoreApplication::sendPostedEvents(entry.data(),QEvent::MetaCall);
    assert(entry && destroyed==0 && PretokenFixtureAccess::delivered(*entry));
    if(mode=="config-refused")assert(!QFile::exists(root+"/callback.json")); // root was never opened
    else {
        const auto callback=QJsonDocument::fromJson(read(root+"/callback.json")).object();
        assert(callback["stage"].toString()==expected);
    }
    QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
    assert(!entry && destroyed==1);drain();assert(destroyed==1);
    assert(read(root+"/shutdown-trace.log").count(" entry-installed ")==int(!refusal));
    ::close(recorder->fd);recorder->fd=-1;
    assert(QDir(root).removeRecursively());
    std::printf("PASS pretoken %s: installation proof, no admission/getters, original deferred lifetime\n",argv[1]);
}
