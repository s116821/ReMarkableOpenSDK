// Original real-QML composition and real Sample; no observation/native effect mock.
#define READER_ENTRY_OWNED_HELPERS_ONLY
#include "reader_selected_view_entry_fixture.cpp"
#define READER_RESIDENT_TESTING
#include "reader_resident_join_pair.h"
#include <QEventLoop>

static int descriptorsFor(const QString &path) {
    int count=0;
    for(const auto &name:QDir(QStringLiteral("/proc/self/fd")).entryList(QDir::AllEntries|QDir::NoDotAndDotDot))
        if(QFileInfo(QStringLiteral("/proc/self/fd/")+name).symLinkTarget()==path)++count;
    return count;
}
static void nestedPair() {QEventLoop loop;QTimer::singleShot(1,&loop,&QEventLoop::quit);loop.exec();}
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);
    qmlRegisterType<QObject>("OwnedProbe",1,0,"QtObject");
    qmlRegisterType<QQuickItem>("OwnedProbe",1,0,"Item");
    qmlRegisterType<FocusFixture>("OwnedProbe",1,0,"FocusScope");
    qmlRegisterType<SceneView>("OwnedProbe",1,0,"SceneFixture");
    qmlRegisterType<OwnershipOwner>("OwnedLegacyEntry",1,0,"Owner");
    registerCreationFixture();
    const QByteArray activation(32,'a'),first(32,'b'),second(32,'c'),identity=reader_selected::selfIdentity();
    int modes=0;
    for(const QByteArray &mode:{QByteArray("valid"),QByteArray("bad-binding"),QByteArray("unknown-host-nonce"),
            QByteArray("duplicate-result"),QByteArray("pending-result"),QByteArray("before-cancel"),QByteArray("setup-deadline"),
            QByteArray("before-b-cancel"),QByteArray("before-b-window"),QByteArray("before-b-deadline"),
            QByteArray("a-wrong-source"),QByteArray("a-unready"),QByteArray("nested-cancel"),QByteArray("nested-cancel-aba"),
            QByteArray("nested-deadline"),QByteArray("request-replaced"),QByteArray("request-aba"),
            QByteArray("postlink-cancel"),QByteArray("postlink-deadline")}) {
        ++modes;
        Fixture f;
        f.document.pages={QStringLiteral("20000000-0000-0000-0000-000000000001"),QStringLiteral("20000000-0000-0000-0000-000000000002")};
        f.reader->setProperty("currentPage",0);f.reader->setProperty("currentPageId",f.document.pages[0]);f.controller.setPageId(f.document.pages[0]);
        Library library;DocumentController creation;creation.setLibrary(&library);library.result=&f.document;
        f.engine.rootContext()->setContextProperty("externalCreation",&creation);
        xofm::bindings::qt::WindowNavigatorWrapper navigator;
        xofm::bindings::qt::WindowLayout layout;layout.activeValue=QStringLiteral("legacydevice/window/main");
        f.engine.rootContext()->setContextProperty("entryNavigator",&navigator);f.engine.rootContext()->setContextProperty("entryLayout",&layout);
        QQmlComponent component(&f.engine);
        component.setData(R"QML(import OwnedLegacyEntry 1.0
Owner { id: root; width: 100; height: 100; windowNavigator: entryNavigator; windowLayout: entryLayout; contentItem: mainView
 Owner { id: mainView; width: 100; height: 100; windowNavigator: root.windowNavigator; windowLayout: root.windowLayout }
})QML",QUrl(QStringLiteral("qrc:/qt/qml/xofm/modules/legacydevice/qml/LegacyMainWindow.qml")));
        require(component.isReady(),"owned resident composition compiles");std::unique_ptr<QObject> composition(component.create());
        auto *visual=qobject_cast<QQuickItem*>(composition.get());require(visual,"owned resident composition creates");
        visual->setParentItem(f.window.contentItem());visual->setParent(f.window.contentItem());f.reader->setVisible(false);
        navigator.onOpen=[&]{f.reader->setVisible(true);};
        reader_resident::Config config{activation,first,second,{f.document.key,0,true},f.document.pages[0],2};
        require(config.valid(),"owned fixed pair config");
        QTemporaryDir base;const QString path=base.path()+"/root";
        require(base.isValid()&&QDir().mkdir(path)&&::chmod(path.toLocal8Bit().constData(),0700)==0,"private resident root");
        putEntry(path+"/owner",activation);putEntry(path+"/attempt.identity",identity);
        if(mode=="duplicate-result")putEntry(path+"/selected-result","preserved-result");
        if(mode=="pending-result")putEntry(path+"/.pending-result","preserved-pending");
        auto pair=std::make_unique<reader_resident::Pair>(&app,path,config);
        std::unique_ptr<QQuickWindow> other;
        int beforeB=0,boundaryLookups=-1;std::array<int,2> reads{0,0};
        const QPointer<QQmlEngine> engine=&f.engine;const QPointer<QQuickWindow> window=&f.window;
        pair->beforeRequestForTest([&](int index){
            require(descriptorsFor(path)==1,"prior request descriptor closed before next admission");
            if(index==1) {
                ++beforeB;boundaryLookups=library.calls;
                if(mode=="before-b-cancel")putEntry(path+"/selected-cancel","stop");
                if(mode=="before-b-deadline")pair->advanceForTest(30001);
                if(mode=="before-b-window") {
                    other=std::make_unique<QQuickWindow>();QQmlEngine::setContextForObject(other.get(),f.engine.rootContext());
                    other->show();other->requestActivate();pumpEntry();require(QGuiApplication::focusWindow()==other.get(),"owned replacement window focused");
                }
            }
            f.current->pageRead=[&,index]{
                ++reads[index];require(descriptorsFor(path)==2,"fresh Sample owns one request descriptor");
                if(index!=0)return;
                if(mode=="nested-cancel"||mode=="nested-cancel-aba") {
                    putEntry(path+"/selected-cancel","stop");pair->checkForTest();nestedPair();
                    require(readEntry(path+"/selected-result").isEmpty(),"no publication before nested Sample unwinds");
                    if(mode=="nested-cancel-aba")require(QFile::remove(path+"/selected-cancel"),"owned cancellation ABA restored");
                }
                if(mode=="nested-deadline") {
                    pair->advanceForTest(30001);pair->checkForTest();nestedPair();
                    require(readEntry(path+"/selected-result").isEmpty(),"deadline publication waits for Sample unwind");
                }
                if(mode=="request-replaced"||mode=="request-aba") {
                    require(QFile::rename(path+"/selected-request",path+"/saved-request"),"pin original request inode");
                    putEntry(path+"/selected-request",activation+' '+identity);
                    if(mode=="request-aba") {
                        require(QFile::remove(path+"/selected-request")&&QFile::rename(path+"/saved-request",path+"/selected-request"),"restore original request inode");
                        struct timespec times[2]{{123,0},{123,0}};
                        require(::utimensat(AT_FDCWD,(path+"/selected-request").toLocal8Bit().constData(),times,0)==0,"request ABA timestamps remain detectable");
                    }
                    pair->checkForTest();nestedPair();require(readEntry(path+"/selected-result").isEmpty(),"request change cannot publish on Sample stack");
                }
            };
        });
        if(mode=="postlink-cancel")pair->postLinkForTest([&]{putEntry(path+"/selected-cancel","stop");nestedPair();});
        if(mode=="postlink-deadline")pair->postLinkForTest([&]{pair->advanceForTest(30001);nestedPair();});
        const bool started=pair->start();pumpEntry();
        if(mode=="duplicate-result"||mode=="pending-result") {
            require(!started&&pair->terminal()&&navigator.calls==0&&library.calls==0,"existing publication rejected before native getters");
            require(readEntry(path+(mode=="duplicate-result"?"/selected-result":"/.pending-result"))==(mode=="duplicate-result"?"preserved-result":"preserved-pending"),"existing output preserved");
            pair.reset();continue;
        }
        require(started&&readEntry(path+"/selected-ready")==activation+' '+identity,"identity-bound resident ready");
        require(navigator.calls==0&&library.calls==0,"no bootstrap/getter before host request");
        if(mode=="before-cancel")putEntry(path+"/selected-cancel","stop");
        if(mode=="setup-deadline")pair->advanceForTest(90001);
        if(mode=="a-wrong-source")f.controller.setPageId(f.document.pages[1]);
        if(mode=="a-unready")f.tiles.setPendingTiles(true);
        const QByteArray request=mode=="bad-binding"?activation+' '+identity+"extra":mode=="unknown-host-nonce"?first+' '+identity:activation+' '+identity;
        putEntry(path+"/selected-request",request);pair->checkForTest();pumpEntry();
        if(pair->opportunityPendingForTest())pair->opportunityForTest();
        for(int i=0;i<32&&!pair->terminal();++i)pumpEntry();
        pumpEntry();require(pair->terminal(),"owned resident pair terminal");
        const auto bytes=readEntry(path+"/selected-result");const auto json=QJsonDocument::fromJson(bytes).object();
        const bool positive=mode=="valid"||mode=="postlink-cancel"||mode=="postlink-deadline";
        if(positive) {
            if(json["stage"]!="resident-pair-observed")QTextStream(stderr)<<"resident-mode="<<mode<<" wire="<<bytes<<'\n';
            validReceipt(bytes,activation,identity,QStringLiteral("resident-pair-observed"),4096);
            require(json["diagnostic"]=="resident-readonly-pair"&&json["same_process"].toBool()&&json["same_window_engine"].toBool()&&json["first_retired_before_second"].toBool(),"explicit process/window and A retirement boundary");
            const auto requests=json["requests"].toArray();require(requests.size()==2,"exactly two fresh records");
            for(int i=0;i<2;++i) {
                const auto item=requests[i].toObject();require(item["nonce"].toString()==QString::fromLatin1(i==0?first:second)&&item["generation"].toInteger()==i+1,"fresh per-request nonce/generation");
                require(item["attempted"].toBool()&&item["observed"].toBool()&&item["creation_join_observed"].toBool()&&item["ready"].toBool()&&item["map_coherent"].toBool()&&item["retired"].toBool()&&item["cleanup_ok"].toBool(),"real fresh Sample and retirement facts");
                require(item["document_id"]==config.navigation.documentId&&item["page_id"]==config.pageId&&item["page_index"]==0&&item["page_count"]==2,"exact selected source facts");
                require(item["sample_end_ms"].toInteger()>=item["sample_begin_ms"].toInteger(),"per-request bounded ordered interval");
            }
            require(reads[0]==1&&reads[1]==1&&beforeB==1&&library.calls>=2,"two independent actual getter/creation joins");
            struct stat result{};require(::stat((path+"/selected-result").toLocal8Bit().constData(),&result)==0&&result.st_nlink==(mode=="valid"?1U:2U),"postlink uncertainty remains two-linked and consumer must reject it");
            require(engine==&f.engine&&window==&f.window,"same actual engine/window retained");
        } else {
            require(json.isEmpty()||json["stage"]!="resident-pair-observed","failure cannot claim complete pair");
            require(reads[1]==0,"no B getters after failure");
            if(boundaryLookups>=0)require(library.calls==boundaryLookups,"no B lookup after changed admission");
            require(navigator.calls<2,"bootstrap at most once even failure");
        }
        const auto saved=bytes;const int oldNavigation=navigator.calls,oldLookups=library.calls;
        pair->checkForTest();pair->opportunityForTest();pumpEntry();
        require(readEntry(path+"/selected-result")==saved&&navigator.calls==oldNavigation&&library.calls==oldLookups&&!pair->start(),"no replay/second publication");
        require(descriptorsFor(path)==1,"per-request descriptors retired before final object destruction");
        pair.reset();pumpEntry();require(descriptorsFor(path)==0,"resident owner root descriptor closed");
    }
    for(int invalid=0;invalid<5;++invalid) {
        reader_resident::Config config{activation,first,second,{QStringLiteral("10000000-0000-0000-0000-000000000001"),0,true},QStringLiteral("20000000-0000-0000-0000-000000000001"),2};
        if(invalid==0)config.first=config.second;
        if(invalid==1)config.first=config.activation;
        if(invalid==2)config.second="new unknown nonce";
        if(invalid==3)config.pageCount=0;
        if(invalid==4)config.pageId="not-page-id";
        require(!config.valid(),"invalid fixed pair identities refused");reader_resident::Pair pair(&app,QStringLiteral("/nonexistent-owned-path"),config);
        require(!pair.start()&&pair.terminal(),"invalid config never activates discovery");
    }
    QTextStream(stdout)<<"PASS resident readonly pair modes="<<modes<<"+5 invalid configs; actual QML observeCreationJoin twice, no native effect or observation mock\n";
}
