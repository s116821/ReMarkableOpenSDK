#include "qt_page_facts_entry.h"
#define main standaloneCaptureFactsFixtureMain
#include "qt_page_facts_fixture.cpp"
#undef main
#include <QFile>
#include <QDir>
#include <QTemporaryDir>
#ifdef OWNED_CAPTURE_PUBLISHER
#define CAPTURE_PUBLISHER_FIXTURE
#include "capture-observation-publisher.h"
#include <sys/mman.h>
#endif
static bool put(const QString &path,const QByteArray &value) {
    QFile file(path);if(!file.open(QIODevice::WriteOnly|QIODevice::NewOnly))return false;
    return file.setPermissions(QFile::ReadOwner|QFile::WriteOwner) && file.write(value)==value.size();
}
namespace qml_access {
struct CaptureObservationFixtureAccess {
    static void grab(FactsEntry &entry,std::function<QImage()> callback){entry.captureGrabForFixture_=std::move(callback);}
    static void temporary(FactsEntry &entry,std::function<void()> callback){entry.captureTemporaryForFixture_=std::move(callback);}
};
}
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);app.setQuitOnLastWindowClosed(false);
    if(argc!=2)return 2;
    const QByteArray mode(argv[1]);
    qmlRegisterType<SceneView>("OwnedFacts",1,0,"SceneView");qmlRegisterType<OwnedReceiver>("OwnedFacts",1,0,"OwnedReceiver");
    QTemporaryDir parent;const QString nonce=QStringLiteral("0123456789abcdef0123456789abcdef"),
        root=mode=="publisher-interleave" ? QString::fromLocal8Bit(qgetenv("FACTS_OWNED_ROOT")):parent.path()+"/rmb-qt-probe-"+nonce;
    if(root.isEmpty())return 3;
    QDir().mkdir(root);::chmod(root.toUtf8().constData(),0700);
    if(!put(root+"/owner",nonce.toLatin1()) || !put(root+"/attempt.identity",QByteArray::number(::getpid())+' '+FactsEntry::processStart()+'\n'))return 3;
    DocumentFixture document;FactsEntryConfig config;config.nonce=nonce;config.directory=root;
    config.facts={document.id(),{QStringLiteral("00000000-0000-4000-8000-000000000002")},6,5000};document.ids=config.facts.expectedOrder;
    config.setupBudgetMs=120000;config.developmentSetup120=true;config.developmentCaptureObservation=true;config.setupSelection=QStringLiteral("main-dev-facts-120s");
    QQmlApplicationEngine engine;engine.rootContext()->setContextProperty("ownedDocument",&document);
    engine.loadData(R"QML(import QtQuick
import QtQuick.Window
import OwnedFacts 1.0
Window { visible:true; width:400; height:400
 OwnedReceiver { id:receiver; objectName:"receiver"; anchors.fill:parent; focus:true
  property QtObject document:ownedDocument
  property int currentPage:0
  readonly property string currentPageId:scene.pageId
  SceneView {id:scene;objectName:"scene";anchors.fill:parent;focus:true;document:receiver.document;pageId:"00000000-0000-4000-8000-000000000002"}
 }
})QML");
    if(engine.rootObjects().size()!=1)return 4;
    auto *window=qobject_cast<QQuickWindow *>(engine.rootObjects()[0]);auto *receiver=window->findChild<OwnedReceiver *>("receiver");auto *scene=window->findChild<SceneView *>("scene");
    receiver->evidence=[&]{return scene->hasActiveFocus();};window->requestActivate();scene->forceActiveFocus();QCoreApplication::processEvents();
    qint64 time=0;int callbacks=0,grabs=0,mappings=0;bool inGrab=false,held=true,acquired=false;FactsEntryResult result;
    QByteArray external(400*400*4,char(0xff));
    receiver->evidence=[&]{if(acquired && mode=="external-buffer")external.fill(char(0));return scene->hasActiveFocus();};
    FactsEntry entry(&app,config,[&](FactsEntryResult value){++callbacks;held=held && !inGrab;result=std::move(value);},[&]{
        if(mode=="completion-late" && QFileInfo::exists(root+"/capture-observation-complete.json"))time=5000;
        return time;
    });
    document.hook=[&]{++mappings;};
    if(mode=="temporary-unlink")CaptureObservationFixtureAccess::temporary(entry,[&]{QFile::remove(root+"/capture-observation-request.tmp");});
    CaptureObservationFixtureAccess::grab(entry,[&]{
        ++grabs;inGrab=true;
        if(mode=="grab-epoch")emit scene->viewportChanged();
        if(mode=="grab-late")time=5000;
        if(mode=="grab-cancel"){entry.cancel();QCoreApplication::processEvents();}
        if(mode=="nested-early" || mode=="nested-purpose") {
            put(root+(mode=="nested-early" ? "/facts-request.tmp":"/input-observation-end.tmp"),"wrong");
            for(int i=0;i<10;++i)QCoreApplication::processEvents();
        }
        if(mode=="completion-epoch")QMetaObject::invokeMethod(&app,[&]{emit scene->viewportChanged();},Qt::QueuedConnection);
        QImage image;
        if(mode!="empty") {image=QImage(400,400,QImage::Format_ARGB32);image.fill(Qt::white);}
        if(mode=="external-buffer")image=QImage(reinterpret_cast<uchar *>(external.data()),400,400,QImage::Format_ARGB32);
        if(mode=="dimension-mismatch")image=QImage(200,400,QImage::Format_ARGB32);
        acquired=true;
        inGrab=false;return image;
    });
    const auto drain=[] {for(int i=0;i<25;++i)QCoreApplication::processEvents();};
    entry.start();drain();
    QFile waiting(root+"/facts-waiting");if(!waiting.open(QIODevice::ReadOnly))return 5;
    const auto fields=waiting.readAll().trimmed().split(' ');
    auto token=fields.mid(0,5).join(' ')+" capture-observation 120000 main-dev-facts-120s\n";
    if(mode=="wrong-token")token[0]='f';
    if(mode=="early-facts")put(root+"/facts-request.tmp","early");
    if(mode=="cross-purpose")put(root+"/input-observation-end.tmp","wrong");
    if(mode=="setup-near")time=119999;
    if(mode=="publisher-interleave") {
#ifdef OWNED_CAPTURE_PUBLISHER
        using namespace capture_observation;
        Descriptor process(::open((QStringLiteral("/proc/")+QString::number(::getpid())).toUtf8().constData(),O_RDONLY|O_DIRECTORY|O_CLOEXEC));
        Descriptor executable(::openat(process.value,"exe",O_RDONLY|O_CLOEXEC)),payload(::open((root+"/payload.so").toUtf8().constData(),O_RDONLY|O_CLOEXEC));
        struct stat st{};if(payload.value<0 || ::fstat(payload.value,&st)!=0)return 6;
        const auto exeHash=digest(executable.value,64*1024*1024),payloadHash=digest(payload.value,16*1024*1024);
        void *mapped=::mmap(nullptr,size_t(st.st_size),PROT_READ,MAP_PRIVATE,payload.value,0);if(mapped==MAP_FAILED)return 6;
        capturePublishedFixtureHook=[&](int){drain();};
        const int published=publishCapture(root,nonce,exeHash,payloadHash);
        ::munmap(mapped,size_t(st.st_size));
        if(published!=0 || !QFileInfo::exists(root+"/capture-observation-complete.json") || QFileInfo::exists(root+"/capture-observation-request.tmp"))return 6;
#else
        return 6;
#endif
    }else put(root+"/capture-observation-request",token);
    if((mode=="publication-alias" || mode=="temporary-unlink") && ::link((root+"/capture-observation-request").toUtf8().constData(),(root+"/capture-observation-request.tmp").toUtf8().constData())!=0)return 6;
    if(mode=="foreign-temporary")put(root+"/capture-observation-request.tmp",token);
    drain();
    const bool captureExpected=mode=="good" || mode=="duplicate" || mode=="visual-epoch" || mode=="token-replaced" || mode=="png-replaced" || mode=="setup-expiry" || mode=="setup-near" || mode=="external-buffer" || mode=="visual-double-click" || mode=="publication-alias" || mode=="publisher-interleave" || mode=="temporary-reappears" || mode=="temporary-unlink";
    QFile complete(root+"/capture-observation-complete.json");const bool hasComplete=complete.open(QIODevice::ReadOnly);
    if(hasComplete!=captureExpected || mappings!=0 || grabs>1 || !held)return 6;
    if(captureExpected) {
        const auto object=QJsonDocument::fromJson(complete.readAll()).object();
        QFile image(root+"/capture-window.png");if(!image.open(QIODevice::ReadOnly))return 7;
        const auto png=image.readAll();
        if(object.size()!=36 || object.value("native_authority").toBool(true) || object.value("render_authority").toBool(true) || object.value("observed_order").toBool(true) ||
           object.value("png_sha256").toString().toLatin1()!=QCryptographicHash::hash(png,QCryptographicHash::Sha256).toHex() || callbacks!=0)return 8;
        if(mode=="external-buffer" && QImage::fromData(png,"png").pixelColor(0,0)!=QColor(Qt::white))return 8;
        if(mode=="duplicate") { if(!put(root+"/unrelated","notify"))return 7;drain(); }
        if(mode=="visual-epoch")emit scene->workerChanged();
        if(mode=="publication-alias")QFile::remove(root+"/capture-observation-request.tmp");
        if(mode=="temporary-reappears" && ::link((root+"/capture-observation-request").toUtf8().constData(),(root+"/capture-observation-request.tmp").toUtf8().constData())!=0)return 7;
        if(mode=="visual-double-click") {
            QMouseEvent event(QEvent::MouseButtonDblClick,QPointF(1,1),QPointF(1,1),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
            QCoreApplication::sendEvent(window,&event);
        }
        if(mode=="token-replaced") {QFile::remove(root+"/capture-observation-request");put(root+"/capture-observation-request",token);}
        if(mode=="png-replaced") {QFile::remove(root+"/capture-window.png");put(root+"/capture-window.png","wrong");}
        if(mode=="setup-expiry") {time=120000;entry.checkDeadline();drain();}
        else {put(root+"/facts-request",fields.mid(0,5).join(' ')+" read-facts 120000 main-dev-facts-120s\n");drain();}
    }
    const bool factsExpected=mode=="good" || mode=="duplicate" || mode=="setup-near" || mode=="external-buffer" || mode=="publication-alias" || mode=="publisher-interleave" || mode=="temporary-unlink";
    if(callbacks!=1 || result.observed!=factsExpected || !held || grabs!=(mode=="wrong-token" || mode=="early-facts" || mode=="cross-purpose" || mode=="foreign-temporary" ? 0:1))return 9;
    std::printf("PASS capture %s stage=%s grabs=%d mappings=%d native_authority=false\n",argv[1],result.stage.toUtf8().constData(),grabs,mappings);
    return 0;
}
