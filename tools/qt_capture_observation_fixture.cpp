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
    static void accept(FactsEntry &entry){(void)entry.acceptCaptureRequest();}
    static void run(FactsEntry &entry){entry.captureOwnerWindow();}
    static void invalidate(FactsEntry &entry){entry.captureInvalid_=true;}
    static CaptureOwnerFailure latched(const FactsEntry &entry){return entry.captureOwnerFailure_;}
    static QByteArray visitedScalar(FactsEntry &entry){
        auto &d=entry.captureOwnerFailure_;d={};d.branch="owner-discovery";d.discovery="open-topology-bound";
        d.accepted=0;d.failure=0;d.effectiveDeadline=5000;d.owner.visited=4097;d.owner.receivers=0;d.owner.scenes=0;d.owner.topologyLimit="visited";
        return entry.captureOwnerFailureBytes();
    }
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
    IncompleteDocument incomplete;
    QQmlApplicationEngine engine;engine.rootContext()->setContextProperty("ownedDocument",mode=="owner-observer" ? static_cast<QObject *>(&incomplete):static_cast<QObject *>(&document));
    QByteArray qml=R"QML(import QtQuick
import QtQuick.Window
import OwnedFacts 1.0
Window { visible:true; width:400; height:400
 OwnedReceiver { id:receiver; objectName:"receiver"; anchors.fill:parent; focus:true
  property QtObject document:ownedDocument
  property int currentPage:0
  readonly property string currentPageId:scene.pageId
  SceneView {id:scene;objectName:"scene";anchors.fill:parent;focus:true;document:receiver.document;pageId:"00000000-0000-4000-8000-000000000002"}
 }
})QML";
    if(mode=="owner-zero")qml.replace("property int currentPage:0","");
    if(mode=="owner-ambiguous") {
        qml.replace(" OwnedReceiver { id:receiver", " OwnedReceiver { objectName:\"outer\"; anchors.fill:parent; focus:true; property QtObject document:ownedDocument; property int currentPage:0; property string currentPageId:\"owned\"\n OwnedReceiver { id:receiver");
        qml.insert(qml.lastIndexOf('}'),'}');
    }
    if(mode=="owner-cap") {
        QByteArray extras;
        for(int i=0;i<8;++i)extras+="OwnedReceiver {property QtObject document:ownedDocument; property int currentPage:0; property string currentPageId:\"owned\"}\n";
        qml.insert(qml.lastIndexOf('}'),extras);
    }
    engine.loadData(qml);
    if(engine.rootObjects().size()!=1)return 4;
    auto *window=qobject_cast<QQuickWindow *>(engine.rootObjects()[0]);auto *receiver=window->findChild<OwnedReceiver *>("receiver");auto *scene=window->findChild<SceneView *>("scene");
    receiver->evidence=[&]{return scene->hasActiveFocus();};window->requestActivate();scene->forceActiveFocus();QCoreApplication::processEvents();
    if(mode=="owner-ambiguous")window->findChild<OwnedReceiver *>("outer")->evidence=[] {return true;};
    if(mode=="owner-topology" || mode=="topology-boundary-depth") {QQuickItem *node=window->contentItem();for(int i=0;i<(mode=="owner-topology" ? 25:24);++i)node=new QQuickItem(node);}
    if(mode=="owner-topology-queue" || mode=="topology-boundary-queue")for(int i=0;i<(mode=="owner-topology-queue" ? 4094:4093);++i)new QQuickItem(window->contentItem());
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
    if(mode=="topology-visited-scalar"){
        const auto value=QJsonDocument::fromJson(CaptureObservationFixtureAccess::visitedScalar(entry)).object();
        if(value.size()!=33 || value["version"].toInt()!=2 || value["visited_items"].toInt()!=4097 || value["topology_limit"]!="visited" ||
            !value["topology_depth"].isNull() || !value["topology_queue_size"].isNull() || !value["topology_child_count"].isNull() ||
            !value["matched_pairs"].isNull() || callbacks || grabs || mappings)return 30;
        std::printf("PASS topology visited scalar seam only; no bounded traversal reproduction\n");return 0;
    }
    if(mode=="owner-topology" || mode=="owner-topology-queue" || mode=="topology-boundary-depth" || mode=="topology-boundary-queue"){
        int focusReads=0,progressReads=0;receiver->evidence=[&]{++focusReads;return scene->hasActiveFocus();};
        PageOwner owner;const auto progress=[&]{++progressReads;return true;};
        const QByteArray ordinary=findPageOwner(&engine,progress,owner);const int ordinaryFocus=focusReads,ordinaryProgress=progressReads;
        focusReads=0;progressReads=0;PageOwnerDiagnostics diagnostics;
        const QByteArray observed=findPageOwner(&engine,progress,owner,&diagnostics);
        if(ordinary!=observed || focusReads!=ordinaryFocus || progressReads!=ordinaryProgress)return 30;
        const bool boundary=mode.startsWith("topology-boundary-");
        const int expectedVisited=mode=="owner-topology" ? 28:mode=="owner-topology-queue" ? 2:mode=="topology-boundary-depth" ? 27:4096;
        if(diagnostics.visited!=expectedVisited || progressReads!=expectedVisited+(boundary ? 2:0) || focusReads!=(boundary ? 2:0) ||
            ordinary!=(boundary ? "open-owner-observed":"open-topology-bound"))return 30;
        if(boundary){
            if(diagnostics.topologyLimit || diagnostics.topologyDepth!=-1 || diagnostics.topologyQueueSize!=-1 || diagnostics.topologyChildCount!=-1)return 30;
            std::printf("PASS topology boundary %s sink/no-sink visits=%d progress=%d getters=%d\n",argv[1],diagnostics.visited,progressReads,focusReads);return 0;
        }
        receiver->evidence=[&]{return scene->hasActiveFocus();};
    }
    QFile waiting(root+"/facts-waiting");if(!waiting.open(QIODevice::ReadOnly))return 5;
    const auto fields=waiting.readAll().trimmed().split(' ');
    auto token=fields.mid(0,5).join(' ')+" capture-observation 120000 main-dev-facts-120s\n";
    if(mode=="wrong-token")token[0]='f';
    if(mode=="early-facts")put(root+"/facts-request.tmp","early");
    if(mode=="cross-purpose")put(root+"/input-observation-end.tmp","wrong");
    if(mode=="setup-near")time=119999;
    if(mode.startsWith("owner-")) {
        int focusReads=0;
        if(mode=="owner-discovery") {
            receiver->evidence=[&]{++focusReads;return false;};PageOwner owner;int progressReads=0;
            const auto progress=[&]{++progressReads;return true;};
            const QByteArray ordinary=findPageOwner(&engine,progress,owner);const int ordinaryFocus=focusReads,ordinaryProgress=progressReads;
            focusReads=0;progressReads=0;PageOwnerDiagnostics diagnostics;
            const QByteArray observed=findPageOwner(&engine,progress,owner,&diagnostics);
            if(ordinary!=observed || focusReads!=ordinaryFocus || progressReads!=ordinaryProgress || diagnostics.matches!=0 || diagnostics.firstReceiver!=0 || diagnostics.firstScene!=0)return 20;
        }
        focusReads=0;
        receiver->evidence=[&]{++focusReads;return mode=="owner-revalidation" ? focusReads<3 : mode!="owner-discovery";};
        if(mode=="owner-final-active")receiver->evidence=[&]{++focusReads;return focusReads<2;};
        if(mode=="owner-progress")receiver->evidence=[&]{++focusReads;CaptureObservationFixtureAccess::invalidate(entry);return true;};
        bool nestedRefused=false,callbackHeld=true;
        if(mode=="owner-reentrant" || mode=="owner-reentrant-noevents")receiver->evidence=[&]{++focusReads;if(focusReads==3){nestedRefused=!entry.captureAllowed();if(mode=="owner-reentrant")QCoreApplication::processEvents();callbackHeld=callbacks==0;}return true;};
        if(!put(root+"/capture-observation-request",token))return 21;
        CaptureObservationFixtureAccess::accept(entry);
        if(mode=="owner-initial")CaptureObservationFixtureAccess::invalidate(entry);
        if(mode=="owner-deadline")time=5000;
        if(mode=="owner-window")window->hide();
        if(mode=="owner-foreign" || mode=="owner-closure" || mode=="owner-generation")receiver->evidence=[&]{++focusReads;return false;};
        CaptureObservationFixtureAccess::run(entry);
        const auto first=CaptureObservationFixtureAccess::latched(entry);const int originalFocus=focusReads;
        receiver->evidence=[&]{++focusReads;return true;};time=9999;
        if(mode=="owner-foreign")put(root+"/capture-owner-refusal.json","foreign");
        if(mode=="owner-closure")put(root+"/entry.closed","closed");
        if(mode=="owner-generation"){QFile::remove(root+"/attempt.identity");put(root+"/attempt.identity","foreign\n");}
        drain();
        if(callbacks!=1 || result.stage!="capture-observation-owner-refused" || result.observed || focusReads!=originalFocus || grabs || mappings)return 22;
        QFile diagnostic(root+"/capture-owner-refusal.json");
        if(mode=="owner-closure" || mode=="owner-generation"){if(diagnostic.exists())return 23;}
        else if(mode=="owner-foreign"){if(!diagnostic.open(QIODevice::ReadOnly) || diagnostic.readAll()!="foreign")return 23;}
        else {
            if(!diagnostic.open(QIODevice::ReadOnly))return 23;
            const auto bytes=diagnostic.readAll();const auto value=QJsonDocument::fromJson(bytes).object();
            if(bytes.size()>8192 || value.size()!=33 || value["failure_ms"].toInteger()!=first.failure || value["failure_ms"].toInteger()==9999 ||
                value["native_authority"].toBool(true) || value["render_authority"].toBool(true) || value["ui_acknowledged"].toBool(true))return 24;
            const QString branch=value["branch"].toString();
            QJsonObject expected{{"kind","development-capture-owner-refusal"},{"version",2},{"nonce",nonce},
                {"attempt_pid",QString::number(::getpid())},{"attempt_start",QString::fromLatin1(FactsEntry::processStart())},
                {"root_device",QString::fromLatin1(fields[3])},{"root_inode",QString::fromLatin1(fields[4])},
                {"setup_profile","main-dev-facts-120s"},{"capture_accepted_ms",0},{"failure_ms",mode=="owner-deadline" ? 5000:0},
                {"effective_deadline_ms",5000},{"native_authority",false},{"render_authority",false},{"ui_acknowledged",false}};
            for(const char *name:{"deadline_check_ms","predicate","discovery_result","visited_items","receiver_candidates","scene_candidates","matched_pairs","first_pair_receiver","first_pair_scene","first_pair_rejection","active_owner_rejection","observer_role","observer_member","observer_failure","topology_limit","topology_depth","topology_queue_size","topology_child_count"})expected[name]=QJsonValue::Null;
            expected["branch"]="owner-discovery";
            expected["discovery_result"]="open-owner-unavailable";
            expected["visited_items"]=3;expected["receiver_candidates"]=1;expected["scene_candidates"]=1;expected["matched_pairs"]=0;
            if(mode=="owner-initial" || mode=="owner-deadline" || mode=="owner-reentrant") {
                expected["branch"]="initial-progress";expected["predicate"]=mode=="owner-deadline" ? "deadline":"invalidated";
                for(const char *name:{"discovery_result","visited_items","receiver_candidates","scene_candidates","matched_pairs"})expected[name]=QJsonValue::Null;
                if(mode=="owner-deadline")expected["deadline_check_ms"]=5000;
            }else if(mode=="owner-window"){
                expected["discovery_result"]="open-current-window-unavailable";
                for(const char *name:{"visited_items","receiver_candidates","scene_candidates","matched_pairs"})expected[name]=QJsonValue::Null;
            }else if(mode=="owner-zero")expected["receiver_candidates"]=0;
            else if(mode=="owner-discovery"){expected["first_pair_receiver"]=0;expected["first_pair_scene"]=0;expected["first_pair_rejection"]="drawing-area-focused";}
            else if(mode=="owner-progress" || mode=="owner-final-active"){
                expected["discovery_result"]="open-context-lost";expected["matched_pairs"]=1;
                if(mode=="owner-progress")expected["predicate"]="invalidated";
                else expected["active_owner_rejection"]="drawing-area-focused";
                if(originalFocus!=(mode=="owner-progress" ? 1:2))return 29;
            }else if(mode=="owner-cap"){expected["discovery_result"]="open-candidate-bound";expected["visited_items"]=10;expected["receiver_candidates"]=9;expected["scene_candidates"]=0;expected["matched_pairs"]=QJsonValue::Null;}
            else if(mode=="owner-topology"){expected["discovery_result"]="open-topology-bound";expected["visited_items"]=28;expected["matched_pairs"]=QJsonValue::Null;expected["topology_limit"]="depth";expected["topology_depth"]=25;}
            else if(mode=="owner-topology-queue"){
                expected["discovery_result"]="open-topology-bound";expected["visited_items"]=2;expected["receiver_candidates"]=1;expected["scene_candidates"]=0;expected["matched_pairs"]=QJsonValue::Null;
                expected["topology_limit"]="queue-cap";expected["topology_depth"]=1;expected["topology_queue_size"]=4096;expected["topology_child_count"]=1;
            }
            else if(mode=="owner-ambiguous"){expected["discovery_result"]="open-owner-ambiguous";expected["visited_items"]=4;expected["receiver_candidates"]=2;expected["matched_pairs"]=2;}
            else if(mode=="owner-observer" || mode=="owner-revalidation" || mode=="owner-reentrant-noevents"){
                expected["discovery_result"]="open-owner-observed";
                for(const char *name:{"visited_items","receiver_candidates","scene_candidates","matched_pairs"})expected[name]=QJsonValue::Null;
                if(mode=="owner-observer"){expected["branch"]="observer-install";expected["observer_role"]="document";expected["observer_member"]="pageCountChanged(int,int)";expected["observer_failure"]="signal-missing";}
                else {expected["branch"]="owner-revalidation";expected["predicate"]=mode=="owner-reentrant-noevents" ? "invalidated-after":"active-owner";if(mode=="owner-revalidation")expected["active_owner_rejection"]="drawing-area-focused";}
            }
            if((mode=="owner-reentrant" || mode=="owner-reentrant-noevents") && (!nestedRefused || !callbackHeld || originalFocus!=3))return 29;
            if(value!=expected){qWarning()<<"diagnostic matrix mismatch"<<mode<<value<<expected;return 29;}
            if(mode=="owner-initial" && (branch!="initial-progress" || value["predicate"]!="invalidated" || !value["discovery_result"].isNull()))return 25;
            if(mode=="owner-deadline" && (branch!="initial-progress" || value["predicate"]!="deadline" || value["deadline_check_ms"].toInteger()!=5000))return 25;
            if((mode=="owner-discovery" || mode=="owner-zero" || mode=="owner-window") && branch!="owner-discovery")return 25;
            if(mode=="owner-discovery" && (value["first_pair_rejection"]!="drawing-area-focused" || originalFocus!=1))return 25;
            if(mode=="owner-zero" && (value["receiver_candidates"].toInt()!=0 || !value["first_pair_receiver"].isNull()))return 25;
            if(mode=="owner-observer" && (branch!="observer-install" || value["observer_role"]!="document" || value["observer_member"]!="pageCountChanged(int,int)" || value["observer_failure"]!="signal-missing"))return 25;
            if(mode=="owner-revalidation" && (branch!="owner-revalidation" || value["predicate"]!="active-owner" || value["active_owner_rejection"]!="drawing-area-focused" || originalFocus!=3))return 25;
            struct stat st{};if(::stat((root+"/capture-owner-refusal.json").toUtf8().constData(),&st)!=0 || (st.st_mode&0777)!=0600)return 26;
        }
        QFile callback(root+"/callback.json");if(!callback.open(QIODevice::ReadOnly) || QJsonDocument::fromJson(callback.readAll()).object().size()!=4)return 27;
        if(QFileInfo::exists(root+"/capture-window.png") || QFileInfo::exists(root+"/capture-observation-complete.json"))return 28;
        std::printf("PASS owner diagnostic %s original-focus-reads=%d immutable=true native_authority=false\n",argv[1],originalFocus);
        return 0;
    }
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
