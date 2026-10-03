#include "qt_page_facts_entry.h"
#define main standaloneFactsFixtureMain
#include "qt_page_facts_fixture.cpp"
#undef main
#include <QFile>
#include <QDir>
#include <QTemporaryDir>
#ifdef OWNED_FACTS_PUBLISHER
#include "facts-request-publisher.h"
#include <sys/mman.h>
#endif
static bool put(const QString &path,const QByteArray &value) {
    QFile file(path); if (!file.open(QIODevice::WriteOnly|QIODevice::NewOnly)) return false;
    if (!file.setPermissions(QFile::ReadOwner|QFile::WriteOwner)) return false;
    return file.write(value)==value.size();
}
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv); app.setQuitOnLastWindowClosed(false);
    if (argc!=2) return 2;
    const QByteArray mode(argv[1]);
    qmlRegisterType<SceneView>("OwnedFacts",1,0,"SceneView");
    qmlRegisterType<OwnedReceiver>("OwnedFacts",1,0,"OwnedReceiver");
    QTemporaryDir parent;
    const QString nonce=QStringLiteral("0123456789abcdef0123456789abcdef");
    const QString root=mode=="integration" ? QString::fromLocal8Bit(qgetenv("FACTS_OWNED_ROOT")) : parent.path()+"/rmb-qt-probe-"+nonce;
    if (root.isEmpty()) return 8;
    QDir().mkdir(root); ::chmod(root.toUtf8().constData(),0700);
    if (!put(root+"/owner",nonce.toLatin1()) || !put(root+"/attempt.identity",QByteArray::number(::getpid())+' '+FactsEntry::processStart()+'\n')) return 3;
    DocumentFixture document;
    FactsEntryConfig config{nonce,root,{document.id(),{},6,5000},20000};
    for (int i=2;i<=7;++i) config.facts.expectedOrder.append(QStringLiteral("00000000-0000-4000-8000-%1").arg(i,12,16,QLatin1Char('0')));
    document.ids=config.facts.expectedOrder;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("ownedDocument",&document);
    const QByteArray goodQml=R"QML(import QtQuick
import QtQuick.Window
import OwnedFacts 1.0
Window { visible:true; width:400; height:400
 OwnedReceiver { id:receiver; objectName:"receiver"; anchors.fill:parent; focus:true
  property QtObject document:ownedDocument
  property int currentPage:0
  readonly property string currentPageId:scene ? scene.pageId : ""
  SceneView { id:scene; objectName:"scene"; anchors.fill:parent; focus:true
   document:receiver.document; pageId:document ? document.idForPage(receiver.currentPage) : ""
  }
 }
})QML";
    const bool myfiles=mode=="myfiles" || mode=="no-owner-then-ready";
    engine.loadData(myfiles ? QByteArray("import QtQuick\nimport QtQuick.Window\nWindow {visible:true; width:400; height:400}") : goodQml);
    if (engine.rootObjects().size()!=1) return 4;
    auto *window=qobject_cast<QQuickWindow *>(engine.rootObjects()[0]);
    auto *receiver=window->findChild<OwnedReceiver *>("receiver");
    auto *scene=window->findChild<SceneView *>("scene");
    if (receiver) receiver->evidence=[&]{return scene && scene->hasActiveFocus();};
    window->requestActivate(); if (scene) scene->forceActiveFocus(); QCoreApplication::processEvents();
    if (mode=="wrong-document") config.facts.documentId=QStringLiteral("00000000-0000-4000-8000-000000000099");
    if (mode=="directory-mode") ::chmod(root.toUtf8().constData(),0755);
    if (mode=="owner-mode") ::chmod((root+"/owner").toUtf8().constData(),0644);
    if (mode=="stale") put(root+"/facts-request","stale");
    qint64 fakeTime=0;
    int requestClockCalls=0,privateClockCalls=0,callbacks=0;
    bool getterActive=false,held=true;
    FactsEntryResult result;
    std::unique_ptr<FactsEntry> entry;
    entry=std::make_unique<FactsEntry>(&app,config,[&](FactsEntryResult value){
        if (getterActive) held=false;
        ++callbacks; result=std::move(value);
        if (mode=="release") entry.reset();
    },[&]{
        if (QFileInfo::exists(root+"/facts-request") && ++requestClockCalls==4 && mode=="delayed-dispatch") fakeTime=5000;
        if (QFileInfo::exists(root+"/diagnostics.json") && ++privateClockCalls==2 && mode=="late-delivery") fakeTime=5000;
        return fakeTime;
    });
    entry->start();
    const auto drain=[&]{for (int i=0;i<20;++i) {QCoreApplication::processEvents(); QThread::msleep(2);}};
    drain();
    if (mode=="setup-deadline" || mode=="myfiles") { fakeTime=20000; entry->checkDeadline(); drain(); }
    else if (callbacks==0) {
        QFile waiting(root+"/facts-waiting"); if (!waiting.open(QIODevice::ReadOnly)) return 5;
        const auto fields=waiting.readAll().trimmed().split(' ');
        if (fields.size()!=8 || fields[5]!="waiting-facts") return 6;
        QByteArray token=fields.mid(0,5).join(' ')+" read-facts\n";
        if (mode=="wrong-token") token[0]='f';
        if (mode=="oversize") token.append(QByteArray(129,'x'));
        if (mode=="restoring-before") put(root+"/restore.claim","closed");
        if (mode=="symlink-token") { put(root+"/target",token); ::symlink("target",(root+"/facts-request").toUtf8().constData()); }
        else if (mode=="fifo-token") ::mkfifo((root+"/facts-request").toUtf8().constData(),0600);
        else if (mode=="integration") {
#ifdef OWNED_FACTS_PUBLISHER
            facts_request::Descriptor proc(::open((QStringLiteral("/proc/")+QString::number(::getpid())).toUtf8().constData(),O_RDONLY|O_DIRECTORY|O_CLOEXEC));
            facts_request::Descriptor executable(::openat(proc.value,"exe",O_RDONLY|O_CLOEXEC));
            facts_request::Descriptor payload(::open((root+"/payload.so").toUtf8().constData(),O_RDONLY|O_CLOEXEC));
            struct stat st{}; if (payload.value<0 || ::fstat(payload.value,&st)!=0) return 9;
            const auto exeHash=facts_request::digest(executable.value,64*1024*1024);
            const auto payloadHash=facts_request::digest(payload.value,16*1024*1024);
            void *mapped=::mmap(nullptr,size_t(st.st_size),PROT_READ,MAP_PRIVATE,payload.value,0);
            if (mapped==MAP_FAILED || facts_request::publish(root,nonce,exeHash,payloadHash)!=0) return 10;
            ::munmap(mapped,size_t(st.st_size));
#else
            return 11;
#endif
        }
        else if (!put(root+"/facts-request",token)) return 7;
        if (mode=="token-mode") ::chmod((root+"/facts-request").toUtf8().constData(),0644);
        document.hook=[&]{
            if (getterActive) return;
            if (mode=="nested-cancel" || mode=="nested-deadline" || mode=="restoring-getter" || mode=="directory-replaced") {
                getterActive=true;
                if (mode=="nested-cancel") entry->cancel();
                if (mode=="nested-deadline") fakeTime=5000;
                if (mode=="restoring-getter") put(root+"/entry.closed","closed");
                if (mode=="directory-replaced") { QDir().rename(root,root+".held"); QDir().mkdir(root); ::chmod(root.toUtf8().constData(),0700); }
                QCoreApplication::processEvents(); if (callbacks) held=false;
                getterActive=false;
            }
        };
        drain();
    }
    if (mode=="no-owner-then-ready") { engine.loadData(goodQml); drain(); }
    if (mode=="duplicate-after") { QFile::remove(root+"/facts-request"); put(root+"/facts-request","again"); drain(); }
    const bool expected=mode=="good" || mode=="release" || mode=="duplicate-after" || mode=="integration";
    bool boundary=true;
    if (mode=="delayed-dispatch") boundary=result.stage=="facts-entry-dispatch-refused" && document.idReads==1;
    if (mode=="late-delivery") boundary=result.stage=="facts-entry-delivery-refused";
    if (result.observed) {
        QFile privateFile(root+"/diagnostics.json"); QFile callbackFile(root+"/callback.json");
        if (!privateFile.open(QIODevice::ReadOnly) || !callbackFile.open(QIODevice::ReadOnly)) boundary=false;
        else {
            const auto facts=QJsonDocument::fromJson(privateFile.readAll()).object();
            const auto callback=QJsonDocument::fromJson(callbackFile.readAll()).object();
            boundary=boundary && facts.value("nonce").toString()==nonce &&
                facts.value("attempt_pid").toString()==QString::number(::getpid()) &&
                facts.value("document_id").toString()==config.facts.documentId &&
                facts.value("required_metadata_validated").toBool() && facts.value("required_connections_installed").toBool() &&
                !facts.value("native_authority").toBool(true) && !facts.value("atomic_snapshot").toBool(true) &&
                callback.value("stage").toString()==result.stage;
        }
    }
    const bool passed=callbacks==1 && result.observed==expected && held && boundary;
    std::printf("entry-%s: %s (%s; callback=%d; held=%d; boundary=%d; output=%d)\n",argv[1],passed ? "PASS" : "FAIL",qPrintable(result.stage),callbacks,held,boundary,result.outputPublished);
    return passed ? 0 : 1;
}
