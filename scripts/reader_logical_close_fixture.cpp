// Synthetic owned-QML experiment. Never loads xochitl or native page code.
#include "qt_pretoken_shutdown_entry.h"
#define main unusedOwnedFactsMain
#include "qt_page_facts_fixture.cpp"
#undef main
#include <QDir>
#include <QFile>
#include <QQuickWindow>
#include <cassert>

namespace qml_access {
struct PretokenFixtureAccess {
    static bool done(const FactsEntry &e) { return e.done_; }
    static bool untouched(const FactsEntry &e) {
        return !e.consumed_ && !e.captureConsumed_ && !e.focusDiscoveryStarted_ &&
            !e.focusArmed_ && !e.reading_ && !e.captureReading_ && !e.reader_ &&
            e.acceptedAt_<0 && e.captureAcceptedAt_<0 && e.sourceFactsBegin_<0;
    }
};
}
static void put(const QString &name,const QByteArray &bytes) {
    QFile f(name);assert(f.open(QIODevice::WriteOnly|QIODevice::NewOnly));
    assert(f.setPermissions(QFile::ReadOwner|QFile::WriteOwner));
    assert(f.write(bytes)==bytes.size());
}
static QByteArray read(const QString &name) {
    QFile f(name);assert(f.open(QIODevice::ReadOnly));return f.readAll();
}
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);app.setQuitOnLastWindowClosed(false);
    const auto originalPid=::getpid();const auto originalStart=FactsEntry::processStart();
    qmlRegisterType<SceneView>("OwnedFacts",1,0,"SceneView");
    qmlRegisterType<OwnedReceiver>("OwnedFacts",1,0,"OwnedReceiver");
    QQmlApplicationEngine engine;
    DocumentFixture document;
    document.ids=pretoken_shutdown::config("00000000000000000000000000000000").facts.expectedOrder;
    int getterCalls=0;document.hook=[&]{++getterCalls;};
    engine.rootContext()->setContextProperty("ownedDocument",&document);
    engine.loadData(R"QML(import QtQuick
import QtQuick.Window
import OwnedFacts 1.0
Window { visible:true; width:400; height:400
 property int heartbeat:0
 OwnedReceiver { id:receiver; objectName:"receiver"; anchors.fill:parent; focus:true
  property QtObject document:ownedDocument
  property int currentPage:0
  property string currentPageId:scene.pageId
  SceneView { id:scene; objectName:"scene"; anchors.fill:parent; focus:true
   document:receiver.document; pageId:"00000000-0000-4000-8000-000000000002"
  }
 }
})QML");
    assert(engine.rootObjects().size()==1);
    QPointer<QObject> window=engine.rootObjects().front();
    const auto originalWindow=window.data();
    int engineDestroyed=0,windowDestroyed=0,quitSeen=0;
    const auto engineConnection=QObject::connect(&engine,&QObject::destroyed,[&]{++engineDestroyed;});
    const auto windowConnection=QObject::connect(window,&QObject::destroyed,[&]{++windowDestroyed;});
    const auto quitConnection=QObject::connect(&app,&QCoreApplication::aboutToQuit,[&]{++quitSeen;});
    for(int session=0;session<2;++session) {
        const QByteArray nonce=session ? QByteArray(32,'1'):QByteArray(32,'0');
        auto config=pretoken_shutdown::config(nonce.constData());const QString root=config.directory;
        assert(::mkdir(root.toUtf8().constData(),0700)==0);
        put(root+"/owner",nonce);
        put(root+"/attempt.identity",QByteArray::number(::getpid())+' '+originalStart+'\n');
        auto *recorder=shutdown_trace::Recorder::open(root.toUtf8().constData(),nonce.constData());assert(recorder);
        auto entry=pretoken_shutdown::start(&app,config,recorder);assert(entry);
        int entryDestroyed=0;QObject::connect(entry,&QObject::destroyed,[&]{++entryDestroyed;});
        QCoreApplication::sendPostedEvents(entry.data(),QEvent::MetaCall);
        assert(entry && entry->pretokenInstalled());
        assert(read(root+"/shutdown-trace.log").contains(" entry-engine-ready "));
        const auto beforeIds=document.idReads;const auto beforeCalls=getterCalls;
        assert(PretokenFixtureAccess::untouched(*entry));
        // Deliberate new synthetic discriminator: close only the entry while
        // retaining the engine and visible window. No Qt quit or process stop.
        entry->cancel();
        assert(entry && PretokenFixtureAccess::done(*entry) && entryDestroyed==0);
        assert(!QFile::exists(root+"/callback.json"));
        QCoreApplication::sendPostedEvents(entry.data(),QEvent::MetaCall);
        assert(entry && entryDestroyed==0);
        const auto result=QJsonDocument::fromJson(read(root+"/callback.json")).object();
        assert(result["stage"].toString()==QStringLiteral("facts-entry-canceled"));
        QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
        assert(!entry && entryDestroyed==1);
        int heartbeat=0;
        QTimer::singleShot(0,&engine,[&]{++heartbeat;window->setProperty("heartbeat",session+1);});
        for(int i=0;i<8;++i)QCoreApplication::processEvents();
        assert(heartbeat==1 && window && window.data()==originalWindow);
        assert(window->property("heartbeat").toInt()==session+1);
        assert(engine.rootObjects().size()==1 && engine.rootObjects().front()==originalWindow);
        assert(engineDestroyed==0 && windowDestroyed==0 && quitSeen==0);
        assert(::getpid()==originalPid && FactsEntry::processStart()==originalStart);
        assert(document.idReads==beforeIds && getterCalls==beforeCalls);
        assert(entryDestroyed==1);
        ::close(recorder->fd);recorder->fd=-1;
        assert(QDir(root).removeRecursively());
        std::printf("PASS session%d: logical entry close/destruction; engine/window/process retained; queued heartbeat; no getter admission\n",session+1);
    }
    // The owned test program itself may now destroy its engine normally. These
    // counters described the still-live application during each logical close.
    QObject::disconnect(engineConnection);
    QObject::disconnect(windowConnection);
    QObject::disconnect(quitConnection);
    std::printf("PASS two serial owned logical sessions; native execution/lifecycle/unload remain unqualified\n");
}
