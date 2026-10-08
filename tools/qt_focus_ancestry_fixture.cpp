#include "qt_page_facts_entry.h"
#define main standaloneFocusFactsFixtureMain
#define SceneView OriginalSceneView
#define qt_meta_tag_ZN9SceneViewE_t qt_meta_tag_originalSceneView_t
#include "qt_page_facts_fixture.cpp"
#undef main
#undef SceneView
#undef qt_meta_tag_ZN9SceneViewE_t
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
    static bool serializer(FactsEntry &entry){
        CaptureOwnerFailure d;d.branch="owner-discovery";d.owner.visited=8;
        d.sceneFunnel={1,2,1,1,2,1};entry.captureOwnerFailure_=d;
        const char *names[]={"scene_rejected_engine","scene_rejected_class","scene_rejected_page_id","scene_rejected_page_id_changed","scene_rejected_document_wrapper_changed","scene_passed"};
        const int expected[]={1,2,1,1,2,1};
        const auto sample=[&]{return QJsonDocument::fromJson(entry.captureOwnerFailureBytes()).object();};
        auto value=sample();if(value.size()!=43 || value["version"]!=4)return false;
        for(int i=0;i<6;++i)if(value[names[i]]!=expected[i])return false;
        for(const char *branch:{"initial-progress","observer-install","owner-revalidation"}){
            entry.captureOwnerFailure_.branch=branch;value=sample();
            for(const char *name:names)if(!value[name].isNull())return false;
        }
        entry.captureOwnerFailure_.branch="owner-discovery";entry.captureOwnerFailure_.owner.visited=-1;value=sample();
        for(const char *name:names)if(!value[name].isNull())return false;
        entry.config_.developmentFocusAncestry=false;value=sample();
        if(value.size()!=33 || value["version"]!=2)return false;
        for(const char *name:names)if(value.contains(name))return false;
        return true;
    }
    static int items(const FactsEntry &entry){return entry.focusGuard_.chain.size();}
    static void generation(FactsEntry &entry){++entry.retainedGeneration_;}
    static bool secondFactory(FactsEntry &entry){return bool(entry.makeRetainedReader([](PageFactsResult){}));}
    static bool sessionAllowed(FactsEntry &entry){return entry.reader_ && entry.reader_->allowed();}
    static bool hasSession(const FactsEntry &entry){return bool(entry.reader_);}
    static bool hasTicket(const FactsEntry &entry){return bool(entry.pendingRetainedTicket_) || bool(entry.retainedRecord_);}
    static PageOwner captureOwner(const FactsEntry &entry){return entry.captureOwner_;}
    static void request(FactsEntry &entry){entry.queueRequest();}
    static bool invalid(const FactsEntry &entry){return entry.captureInvalid_;}
    static void suppress(FactsEntry &entry){entry.focusGuard_.disconnect();entry.app_->removeEventFilter(&entry);}
    static bool observed(const FactsEntry &entry){return entry.result_.observed;}
    static bool callbackWritten(const FactsEntry &entry){return entry.exists("callback.json");}
    static bool bindings(FactsEntry &entry){return entry.captureBindingsCurrent();}
};
}
class SceneView : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QString pageId MEMBER pageId NOTIFY pageIdChanged)
public: QString pageId;
signals: void pageIdChanged(); void documentWrapperChanged();
};
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
    const bool receiverMode=mode.startsWith("receiver-");
    const bool receiver512=mode.startsWith("receiver-512-");
    const QString receiverScope=receiver512 ? "receiver-subtree-capture-unqualified-v2":"receiver-subtree-capture-unqualified-v1";
    qmlRegisterType<OriginalSceneView>("OwnedFacts",1,0,"SceneView");
    qmlRegisterType<SceneView>("OwnedFacts",1,0,"NegativeScene");
    qmlRegisterType<OwnedReceiver>("OwnedFacts",1,0,"OwnedReceiver");
    QTemporaryDir tmp;const QString nonce="0123456789abcdef0123456789abcdef",root=tmp.path()+"/rmb-qt-probe-"+nonce;
    QDir().mkdir(root);::chmod(root.toUtf8().constData(),0700);
    if(!put(root+"/owner",nonce.toLatin1()) || !put(root+"/attempt.identity",QByteArray::number(::getpid())+' '+FactsEntry::processStart()+'\n'))return 3;
    DocumentFixture ordinaryDocument;IncompatibleDocument incompatibleDocument;
    DocumentFixture &document=(mode.startsWith("metadata") ? static_cast<DocumentFixture &>(incompatibleDocument):ordinaryDocument);document.ids={"00000000-0000-4000-8000-000000000002"};
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
    if(receiverMode){
        qml.replace("objectName:\"scene\"; focus:true","objectName:\"scene\"; focus:false");
        qml.insert(qml.indexOf("  SceneView")," Item { objectName:\"captureAnchor\"; focus:true }\n");
        if(mode=="receiver-two" || mode=="receiver-other-page"){
            const QByteArray page=mode=="receiver-two" ? "00000000-0000-4000-8000-000000000002":"00000000-0000-4000-8000-000000000003";
            qml.insert(qml.indexOf("  SceneView")," SceneView { focus:false; document:receiver.document; pageId:\""+page+"\" }\n");
        }
    }
    if(mode=="scene9-negative"){
        const int at=qml.indexOf("  SceneView");
        qml.insert(at,QByteArray(" NegativeScene { focus:true\n").repeated(9));
        qml.insert(qml.lastIndexOf('}'),QByteArray("}\n").repeated(9));
    }
    engine.loadData(qml,QUrl("qrc:/FocusOwned.qml"));
    if(engine.rootObjects().size()!=1)return 4;
    auto *window=qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    auto *receiver=window->findChild<OwnedReceiver *>("receiver");
    auto *scene=window->findChild<OriginalSceneView *>("scene");
    if(!window || !receiver || !scene)return 4;
    if(mode.startsWith("endpoint"))scene->setFlag(QQuickItem::ItemIsFocusScope);
    int ownerReads=0;
    receiver->evidence=[&] {++ownerReads;return true;};
    if(auto *outer=window->findChild<OwnedReceiver *>("outer"))outer->evidence=[] {return true;};
    if(mode=="edge24" || mode=="edge25"){
        QQuickItem *parent=window->contentItem();
        for(int i=0;i<(mode=="edge24" ? 22:23);++i){auto *item=new QQuickItem(parent);parent=item;}
        receiver->setParentItem(parent);
    }
    if(mode=="large")for(int i=0;i<4200;++i)new QQuickItem(window->contentItem());
    window->requestActivate();
    if(receiverMode)window->findChild<QQuickItem *>("captureAnchor")->forceActiveFocus();else scene->forceActiveFocus();
    const auto drain=[] {for(int i=0;i<30;++i)QCoreApplication::processEvents();};drain();
    FactsEntryConfig config;config.nonce=nonce;config.directory=root;
    config.facts={document.id(),document.ids,6,5000};config.setupBudgetMs=120000;
    config.developmentSetup120=config.developmentCaptureObservation=config.developmentFocusAncestry=true;
    if(receiverMode){config.developmentFocusAncestry=false;config.developmentReceiverSubtreeCapture=true;config.developmentReceiverSubtreeCapture512=receiver512;}
    config.setupSelection="main-dev-facts-120s";
    qint64 time=0;int callbacks=0,grabs=0,mappings=0,finalChecks=0;
    std::function<void()> clockHook;FactsEntryResult result;
    FactsEntry entry(&app,config,[&](FactsEntryResult value){++callbacks;result=std::move(value);},[&]{if(clockHook)clockHook();return time;});
    if(mode=="funnel-serializer"){
        if(!FocusAncestryFixtureAccess::serializer(entry))return 10;
        std::puts("PASS focus-owned funnel-serializer numeric/null/unselected native_authority=false");return 0;
    }
    auto *subscene=new QQuickItem(scene);
    if(receiver512){
        QList<QQuickItem *> items{receiver};
        for(qsizetype i=0;i<items.size();++i)items.append(items[i]->childItems());
        const int target=mode=="receiver-512-overflow" ? 513:512;
        if(items.size()>target)return 4;
        for(qsizetype i=items.size();i<target;++i)new QQuickItem(receiver);
        items={receiver};for(qsizetype i=0;i<items.size();++i)items.append(items[i]->childItems());
        if(items.size()!=target)return 4;
    }
    if(mode=="receiver-cap")for(int i=0;i<257;++i)new QQuickItem(receiver);
    if(mode=="receiver-depth"){
        QQuickItem *parent=receiver;
        for(int i=0;i<9;++i)parent=new QQuickItem(parent);
    }
    bool getterChanged=false;
    if(mode=="receiver-getter-loss")receiver->evidence=[&]{++ownerReads;if(!getterChanged){getterChanged=true;new QQuickItem(receiver);}return true;};
    const auto mismatch=[&]{FocusAncestryFixtureAccess::suppress(entry);subscene->forceActiveFocus();if(!scene->hasActiveFocus() || window->activeFocusItem()!=subscene)std::abort();};
    CaptureObservationFixtureAccess::grab(entry,[&]{++grabs;if(mode=="endpoint-capture")mismatch();QImage image(400,400,QImage::Format_ARGB32);image.fill(Qt::white);return image;});
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
    if(mode=="endpoint-visual")receiver->evidence=[&]{++ownerReads;if(grabs && !FocusAncestryFixtureAccess::invalid(entry))mismatch();return true;};
    drain();
    if(receiverMode){
        if(FocusAncestryFixtureAccess::hasTicket(entry) || FocusAncestryFixtureAccess::hasSession(entry) || mappings || scene->hasActiveFocus())return 11;
        if(mode=="receiver-two" || mode=="receiver-cap" || mode=="receiver-512-overflow" || mode=="receiver-depth" || mode=="receiver-getter-loss"){
            QFile file(root+"/capture-owner-refusal.json");if(!file.open(QIODevice::ReadOnly))return 12;
            const auto value=QJsonDocument::fromJson(file.readAll()).object();
            if(value.size()!=34 || value["version"]!=5 || value["discovery_scope"]!=receiverScope || grabs || callbacks!=1 || result.observed || QFile::exists(root+"/capture-window.png"))return 12;
            if(mode=="receiver-two" && (value["matched_pairs"]!=2 || value["discovery_result"]!="open-owner-ambiguous"))return 12;
            if(mode=="receiver-cap" && (value["discovery_result"]!="open-capture-subtree-bound" || value["topology_limit"]!="subtree-items"))return 12;
            if(mode=="receiver-512-overflow" && (value["discovery_result"]!="open-capture-subtree-bound" || value["topology_limit"]!="subtree-items" || ownerReads || !value["matched_pairs"].isNull() || value["topology_queue_size"]!=512 || value["topology_child_count"]!=1))return 12;
            if(mode=="receiver-depth" && (value["discovery_result"]!="open-capture-subtree-bound" || value["topology_limit"]!="subtree-depth" || value["topology_depth"]!=8))return 12;
            if(mode=="receiver-getter-loss" && (!getterChanged || value["predicate"]!="invalidated"))return 12;
        }else{
            QFile file(root+"/capture-observation-complete.json");if(!file.open(QIODevice::ReadOnly))return 13;
            const auto value=QJsonDocument::fromJson(file.readAll()).object();
            if(value.size()!=37 || value["version"]!=2 || value["discovery_scope"]!=receiverScope || grabs!=1 || callbacks || value["page_index"]!=0)return 13;
            if(activeOwner(FocusAncestryFixtureAccess::captureOwner(entry),&engine))return 13;
            if(mode=="receiver-facts-refusal"){
                if(!put(root+"/facts-request",fields.mid(0,5).join(' ')+" read-facts 120000 main-dev-facts-120s\n"))return 14;
                FocusAncestryFixtureAccess::request(entry);drain();
                if(callbacks!=1 || result.stage!="capture-observation-purpose-refused" || result.observed || mappings || FocusAncestryFixtureAccess::hasSession(entry) || FocusAncestryFixtureAccess::hasTicket(entry))return 14;
            }
        }
        std::printf("PASS receiver-subtree %s capture-only unqualified native_authority=false\n",argv[1]);return 0;
    }
    if(mode=="endpoint-capture" || mode=="endpoint-visual" || mode=="scene9-negative" || mode=="edge25" || mode=="ambiguous" || mode=="focus-event" || mode=="initial-invalidated" || mode=="aba-out" || mode=="aba-in"){
        if(mode.startsWith("endpoint")){
            if(callbacks!=1 || result.observed || !FocusAncestryFixtureAccess::invalid(entry) || mappings)return 6;
            std::printf("PASS focus-owned %s endpoint-refused native_authority=false\n",argv[1]);return 0;
        }
        QFile file(root+"/capture-owner-refusal.json");if(!file.open(QIODevice::ReadOnly))return 6;
        const auto value=QJsonDocument::fromJson(file.readAll()).object();
        if(value.size()!=43 || value["version"]!=4 || value["discovery_scope"]!="window-focus-ancestry-v1" ||
            !value["topology_limit"].isNull() || callbacks!=1 || result.observed || grabs || mappings)return 6;
        const bool classified=value["branch"]=="owner-discovery" && !value["visited_items"].isNull();
        int total=0;
        for(const char *name:{"scene_rejected_engine","scene_rejected_class","scene_rejected_page_id","scene_rejected_page_id_changed","scene_rejected_document_wrapper_changed","scene_passed"}){
            if(classified){if(!value[name].isDouble() || value[name].toInt(-1)<0 || value[name].toInt()>25)return 6;total+=value[name].toInt();}
            else if(!value[name].isNull())return 6;
        }
        if(classified && (total!=value["visited_items"].toInt() || value["scene_passed"]!=value["scene_candidates"]))return 6;
        if(mode=="scene9-negative" && (value["scene_candidates"]!=9 || value["discovery_result"]!="open-candidate-bound" || !value["matched_pairs"].isNull()))return 6;
        if(mode=="edge25" && (value["chain_items"]!=25 || value["chain_complete"]!=false || value["chain_failure"]!="depth-bound"))return 6;
        if(mode=="ambiguous" && (value["chain_complete"]!=true || value["matched_pairs"]!=2 || value["discovery_result"]!="open-owner-ambiguous"))return 6;
        if((mode=="focus-event" || mode=="initial-invalidated" || mode=="aba-out" || mode=="aba-in") && (value["branch"]!="initial-progress" || !value["chain_items"].isNull() || value["predicate"]!="invalidated"))return 6;
    }else{
        QFile complete(root+"/capture-observation-complete.json");if(!complete.open(QIODevice::ReadOnly) || grabs!=1 || mappings || callbacks)return 7;
        if(QJsonDocument::fromJson(complete.readAll()).object().size()!=36)return 7;
        const int count=FocusAncestryFixtureAccess::items(entry);
        if(count!=(mode=="edge24" ? 25:3))return 7;
        if(mode=="endpoint-bindings"){mismatch();if(FocusAncestryFixtureAccess::bindings(entry) || !FocusAncestryFixtureAccess::invalid(entry))return 8;entry.cancel();drain();std::printf("PASS focus-owned endpoint-bindings native_authority=false\n");return 0;}
        if(mode=="endpoint-final1" || mode=="endpoint-final2")clockHook=[&]{if(FocusAncestryFixtureAccess::observed(entry) && (mode=="endpoint-final1" ? ++finalChecks==1:FocusAncestryFixtureAccess::callbackWritten(entry)))mismatch();};
        if(mode=="metadata-ticket-loss")receiver->evidence=[&,reads=0]()mutable{++ownerReads;if(FocusAncestryFixtureAccess::hasSession(entry) && ++reads==2)FocusAncestryFixtureAccess::generation(entry);return true;};
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
        const bool success=mode!="generation" && mode!="ticket-reentrant" && mode!="delivery-generation" && !mode.startsWith("metadata") && !mode.startsWith("endpoint");
        if(callbacks!=1 || result.observed!=success || FocusAncestryFixtureAccess::items(entry)!=count)return 8;
        if(mode.startsWith("endpoint")){if(!FocusAncestryFixtureAccess::invalid(entry) || result.observed || result.stage!=(mode=="endpoint-final1" ? "facts-entry-delivery-refused":"facts-entry-output-unknown"))return 8;}
        else if(!success){
            QFile file(root+"/refusal.json");if(!file.open(QIODevice::ReadOnly))return 8;
            const auto value=QJsonDocument::fromJson(file.readAll()).object();
            if(value.size()!=27 || value["reader_stage"]!=(mode=="metadata-only" ? "facts-metadata-or-context-refused":"facts-retained-owner-refused") || value["reader_result_had_facts"]!=false || value["entry_stage"]!="facts-entry-read-refused" || value["refusal_path"]!="reader-result")return 8;
        }
    }
    std::printf("PASS focus-owned %s stage=%s grabs=%d mappings=%d native_authority=false\n",argv[1],result.stage.toUtf8().constData(),grabs,mappings);
    return 0;
}

#include "qt_focus_ancestry_fixture.moc"
