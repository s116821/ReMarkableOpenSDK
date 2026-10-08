#include "qt_page_facts_entry.h"
#define main standaloneFocusFactsFixtureMain
#include "qt_page_facts_fixture.cpp"
#undef main
#include <QFile>
#include <QDir>
#include <QTemporaryDir>
static bool put(const QString &path,const QByteArray &value){
    QFile file(path);if(!file.open(QIODevice::WriteOnly|QIODevice::NewOnly))return false;
    return file.setPermissions(QFile::ReadOwner|QFile::WriteOwner) && file.write(value)==value.size();
}
namespace qml_access {
struct CaptureObservationFixtureAccess {
    static void accept(FactsEntry &entry){(void)entry.acceptCaptureRequest();}
    static void grab(FactsEntry &entry,std::function<QImage()> callback){entry.captureGrabForFixture_=std::move(callback);}
};
struct FocusAncestryFixtureAccess {
    static int items(const FactsEntry &entry){return entry.focusGuard_.chain.size();}
    static void generation(FactsEntry &entry){++entry.retainedGeneration_;}
    static bool secondFactory(FactsEntry &entry){return bool(entry.makeRetainedReader([](PageFactsResult){}));}
    static bool sessionAllowed(FactsEntry &entry){return entry.reader_ && entry.reader_->allowed();}
    static bool hasSession(const FactsEntry &entry){return bool(entry.reader_);}
    static bool invalid(const FactsEntry &entry){return entry.captureInvalid_;}
};
}
class FocusReentryFilter : public QObject {
public:
    QEvent::Type type=QEvent::FocusOut;
    std::function<void()> hook;
    bool seen=false;
    bool eventFilter(QObject *,QEvent *event) override {
        if(!seen && event->type()==type){seen=true;hook();}
        return false;
    }
};
int main(int argc,char **argv){
    QGuiApplication app(argc,argv);app.setQuitOnLastWindowClosed(false);
    if(argc!=2)return 2;
    const QByteArray mode=argv[1];
    qmlRegisterType<SceneView>("OwnedFacts",1,0,"SceneView");
    qmlRegisterType<OwnedReceiver>("OwnedFacts",1,0,"OwnedReceiver");
    QTemporaryDir tmp;const QString nonce="0123456789abcdef0123456789abcdef",root=tmp.path()+"/rmb-qt-probe-"+nonce;
    QDir().mkdir(root);::chmod(root.toUtf8().constData(),0700);
    if(!put(root+"/owner",nonce.toLatin1()) || !put(root+"/attempt.identity",QByteArray::number(::getpid())+' '+FactsEntry::processStart()+'\n'))return 3;
    DocumentFixture document;document.ids={"00000000-0000-4000-8000-000000000002"};
    QQmlApplicationEngine engine;engine.rootContext()->setContextProperty("ownedDocument",&document);
    QByteArray qml=R"QML(import QtQuick
import QtQuick.Window
import OwnedFacts 1.0
Window { visible:true; width:400; height:400
 OwnedReceiver { id:receiver; objectName:"receiver"; focus:true
  property QtObject document:ownedDocument
  property int currentPage:0
  property string currentPageId:scene.pageId
  SceneView { id:scene; objectName:"scene"; focus:true; document:receiver.document; pageId:"00000000-0000-4000-8000-000000000002" }
 }
})QML";
    if(mode=="ambiguous"){
        qml.replace(" OwnedReceiver { id:receiver"," OwnedReceiver { objectName:\"outer\"; focus:true; property QtObject document:ownedDocument; property int currentPage:0; property string currentPageId:\"owned\"\n OwnedReceiver { id:receiver");
        qml.insert(qml.lastIndexOf('}'),'}');
    }
    engine.loadData(qml,QUrl("qrc:/FocusOwned.qml"));
    if(engine.rootObjects().size()!=1)return 4;
    auto *window=qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    auto *receiver=window->findChild<OwnedReceiver *>("receiver");
    auto *scene=window->findChild<SceneView *>("scene");
    if(!window || !receiver || !scene)return 4;
    int ownerReads=0;
    receiver->evidence=[&] {++ownerReads;return true;};
    if(auto *outer=window->findChild<OwnedReceiver *>("outer"))outer->evidence=[] {return true;};
    if(mode=="edge24" || mode=="edge25"){
        QQuickItem *parent=window->contentItem();
        for(int i=0;i<(mode=="edge24" ? 22:23);++i){auto *item=new QQuickItem(parent);parent=item;}
        receiver->setParentItem(parent);
    }
    if(mode=="large")for(int i=0;i<4200;++i)new QQuickItem(window->contentItem());
    window->requestActivate();scene->forceActiveFocus();
    const auto drain=[] {for(int i=0;i<30;++i)QCoreApplication::processEvents();};drain();
    FactsEntryConfig config;config.nonce=nonce;config.directory=root;
    config.facts={document.id(),document.ids,6,5000};config.setupBudgetMs=120000;
    config.developmentSetup120=config.developmentCaptureObservation=config.developmentFocusAncestry=true;
    config.setupSelection="main-dev-facts-120s";
    qint64 time=0;int callbacks=0,grabs=0,mappings=0;FactsEntryResult result;
    FactsEntry entry(&app,config,[&](FactsEntryResult value){++callbacks;result=std::move(value);},[&]{return time;});
    CaptureObservationFixtureAccess::grab(entry,[&]{++grabs;QImage image(400,400,QImage::Format_ARGB32);image.fill(Qt::white);return image;});
    document.hook=[&]{++mappings;};entry.start();drain();
    auto *other=new QQuickItem(window->contentItem());
    FocusReentryFilter reentry;
    if(mode=="aba-out" || mode=="aba-in"){
        reentry.type=mode=="aba-out" ? QEvent::FocusOut:QEvent::FocusIn;
        (mode=="aba-out" ? static_cast<QObject *>(scene):static_cast<QObject *>(other))->installEventFilter(&reentry);
        reentry.hook=[&]{if(!FocusAncestryFixtureAccess::invalid(entry))std::abort();scene->forceActiveFocus();};
    }
    QFile waiting(root+"/facts-waiting");if(!waiting.open(QIODevice::ReadOnly))return 5;
    const auto fields=waiting.readAll().trimmed().split(' ');
    if(!put(root+"/capture-observation-request",fields.mid(0,5).join(' ')+" capture-observation 120000 main-dev-facts-120s\n"))return 5;
    if(mode=="focus-event" || mode=="initial-invalidated" || mode=="aba-out" || mode=="aba-in"){
        CaptureObservationFixtureAccess::accept(entry);
        if(mode=="focus-event") {QFocusEvent event(QEvent::FocusOut);QCoreApplication::sendEvent(scene,&event);}
        else if(mode=="initial-invalidated")entry.invalidateCapture();
        else {other->forceActiveFocus();if(!reentry.seen || window->activeFocusItem()!=scene) return 9;}
    }
    drain();
    if(mode=="edge25" || mode=="ambiguous" || mode=="focus-event" || mode=="initial-invalidated" || mode=="aba-out" || mode=="aba-in"){
        QFile file(root+"/capture-owner-refusal.json");if(!file.open(QIODevice::ReadOnly))return 6;
        const auto value=QJsonDocument::fromJson(file.readAll()).object();
        if(value.size()!=37 || value["version"]!=3 || value["discovery_scope"]!="window-focus-ancestry-v1" ||
            !value["topology_limit"].isNull() || callbacks!=1 || result.observed || grabs || mappings)return 6;
        if(mode=="edge25" && (value["chain_items"]!=25 || value["chain_complete"]!=false || value["chain_failure"]!="depth-bound"))return 6;
        if(mode=="ambiguous" && (value["chain_complete"]!=true || value["matched_pairs"]!=2 || value["discovery_result"]!="open-owner-ambiguous"))return 6;
        if((mode=="focus-event" || mode=="initial-invalidated" || mode=="aba-out" || mode=="aba-in") && (value["branch"]!="initial-progress" || !value["chain_items"].isNull() || value["predicate"]!="invalidated"))return 6;
    }else{
        QFile complete(root+"/capture-observation-complete.json");if(!complete.open(QIODevice::ReadOnly) || grabs!=1 || mappings || callbacks)return 7;
        if(QJsonDocument::fromJson(complete.readAll()).object().size()!=36)return 7;
        const int count=FocusAncestryFixtureAccess::items(entry);
        if(count!=(mode=="edge24" ? 25:3))return 7;
        if(mode=="generation")FocusAncestryFixtureAccess::generation(entry);
        if(mode=="ticket-reentrant")receiver->evidence=[&]{
            if(FocusAncestryFixtureAccess::hasSession(entry)){(void)FocusAncestryFixtureAccess::sessionAllowed(entry);}
            return true;
        };
        if(mode=="second-factory")document.hook=[&]{++mappings;if(FocusAncestryFixtureAccess::secondFactory(entry))std::abort();};
        if(mode=="getter-counts")document.hook=[&]{++mappings;const int before=ownerReads;if(!FocusAncestryFixtureAccess::sessionAllowed(entry) || ownerReads-before!=3)std::abort();};
        if(mode=="delivery-generation")document.hook=[&]{++mappings;QMetaObject::invokeMethod(&app,[&]{FocusAncestryFixtureAccess::generation(entry);},Qt::QueuedConnection);};
        if(!put(root+"/facts-request",fields.mid(0,5).join(' ')+" read-facts 120000 main-dev-facts-120s\n"))return 7;
        drain();
        const bool success=mode!="generation" && mode!="ticket-reentrant" && mode!="delivery-generation";
        if(callbacks!=1 || result.observed!=success || FocusAncestryFixtureAccess::items(entry)!=count)return 8;
        if(!success){
            QFile file(root+"/refusal.json");if(!file.open(QIODevice::ReadOnly))return 8;
            const auto value=QJsonDocument::fromJson(file.readAll()).object();
            if(value.size()!=27 || value["reader_stage"]!="facts-retained-owner-refused" || value["reader_result_had_facts"]!=false || value["entry_stage"]!="facts-entry-read-refused" || value["refusal_path"]!="reader-result")return 8;
        }
    }
    std::printf("PASS focus-owned %s stage=%s grabs=%d mappings=%d native_authority=false\n",argv[1],result.stage.toUtf8().constData(),grabs,mappings);
    return 0;
}
