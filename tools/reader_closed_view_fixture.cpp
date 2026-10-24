// Owned Qt lifetimes/context/notifications; native object types and Loader modeled.
#include "reader_closed_view.h"
#define READER_OWNERSHIP_OWNED_CLASSES_ONLY
#include "reader_navigation_ownership_fixture.cpp"
#undef READER_OWNERSHIP_OWNED_CLASSES_ONLY
#include <stdexcept>
#include <cstdio>

class ClosedReader:public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QObject *document READ document NOTIFY documentChanged)
    Q_PROPERTY(bool documentLoaded READ documentLoaded NOTIFY documentLoadedChanged)
public:
    QPointer<QObject> documentValue;bool loaded=false;
    mutable std::function<void()> onLoaded,onDocument;
    QObject *document()const{QObject *p=documentValue;auto f=std::move(onDocument);onDocument={};if(f)f();return p;}
    bool documentLoaded()const{bool v=loaded;auto f=std::move(onLoaded);onLoaded={};if(f)f();return v;}
signals:
    void documentChanged();void documentLoadedChanged();
};
class ClosedLoader:public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool asynchronous READ asynchronous NOTIFY asynchronousChanged)
    Q_PROPERTY(QQuickItem *item READ item WRITE setItem NOTIFY itemChanged)
public:
    enum Status {Null,Ready,Loading,Error};Q_ENUM(Status)
    Status statusValue=Ready;bool asynchronousValue=false;QPointer<QQuickItem> itemValue;
    mutable std::function<void()> onStatus,onItem;
    Status status()const{auto v=statusValue;auto f=std::move(onStatus);onStatus={};if(f)f();return v;}
    bool asynchronous()const{return asynchronousValue;}
    QQuickItem *item()const{QQuickItem *p=itemValue;auto f=std::move(onItem);onItem={};if(f)f();return p;}
    void setItem(QQuickItem *p){itemValue=p;if(p){p->setParent(this);p->setParentItem(this);}emit itemChanged();}
signals:
    void statusChanged();void asynchronousChanged();void itemChanged();
};
namespace {
unsigned checks=0,cases=0;
void require(bool b,const char *s){++checks;if(!b)throw std::runtime_error(s);}
void pump(){for(int n=0;n<12;++n){QCoreApplication::sendPostedEvents();QCoreApplication::processEvents();}}
struct Fixture {
    QQmlEngine engine;QQuickWindow window;
    xofm::bindings::qt::WindowNavigatorWrapper navigator;
    xofm::bindings::qt::WindowLayout layout;
    QPointer<OwnershipOwner> root,main;QPointer<ClosedLoader> loader;QPointer<ClosedReader> reader;
    Fixture(){
        QQmlEngine::setContextForObject(&window,engine.rootContext());
        engine.rootContext()->setContextProperty("ownedNavigator",&navigator);
        engine.rootContext()->setContextProperty("ownedLayout",&layout);
        layout.activeValue="legacydevice/window/main";
        QQmlComponent c(&engine);
        c.setData("import Owned.Closed 1.0\nOwner { id: root; windowNavigator: ownedNavigator; windowLayout: ownedLayout; contentItem: mainView; Main { id: mainView } }",
                  QUrl(QString::fromLatin1(reader_selected::navigation_detail::LegacyContext)));
        if(!c.isReady())qWarning()<<c.errors();
        root=qobject_cast<OwnershipOwner *>(c.create());require(root,"create owned legacy root");
        root->setParent(&window);root->setParentItem(window.contentItem());
        main=qobject_cast<OwnershipOwner *>(root->contentItem());require(main,"owned MainView");
        auto scope=qmlContext(main->childItems().first());
        for(int n=0;scope&&n<16&&scope->baseUrl()!=QUrl(QString::fromLatin1(reader_closed_view::MainContext));++n)scope=scope->parentContext();
        require(scope,"real own MainView lexical context");
        loader=qobject_cast<ClosedLoader *>(scope->objectForName("documentView"));require(loader,"real Loader scoped id");
        reader=qobject_cast<ClosedReader *>(loader->item());require(reader,"owned reader item");
        window.setGeometry(0,0,200,200);window.show();window.requestActivate();pump();
        require(window.isActive()&&QGuiApplication::focusWindow()==&window,"real focused window");
    }
    reader_closed_view::Result run(const std::function<bool()> &permit=[] {return true;},int budget=1000){return reader_closed_view::observe(&window,permit,budget);}
};
template<class F>void scenario(F f){++cases;f();}
}
int main(int argc,char **argv){
    QGuiApplication app(argc,argv);
    qmlRegisterType<QQuickItem>("Owned.Closed",1,0,"Item");
    qmlRegisterType<OwnershipOwner>("Owned.Closed",1,0,"Owner");
    qmlRegisterType<ClosedLoader>("Owned.Closed",1,0,"ClosedLoader");
    qmlRegisterType<ClosedReader>("Owned.Closed",1,0,"ClosedReader");
    qmlRegisterType(QUrl(QString::fromLatin1(reader_closed_view::MainContext)),"Owned.Closed",1,0,"Main");
    try {
        scenario([]{Fixture f;const auto r=f.run();if(!r.observed)qWarning()<<r.refusal;require(r.observed&&r.compositionObserved&&r.loaderReady&&r.documentNull&&r.refusal.isEmpty(),"fresh known closed composition");require(!f.navigator.calls,"closure never navigates");});
        for(int mode=0;mode<10;++mode)scenario([=]{Fixture f;QObject document;
            if(mode==0)f.reader->loaded=true;
            if(mode==1)f.reader->documentValue=&document;
            if(mode==2)f.loader->statusValue=ClosedLoader::Null;
            if(mode==3)f.loader->statusValue=ClosedLoader::Loading;
            if(mode==4)f.loader->statusValue=ClosedLoader::Error;
            if(mode==5)f.loader->asynchronousValue=true;
            if(mode==6)f.loader->setVisible(true);
            if(mode==7)f.reader->setParentItem(f.main);
            if(mode==8)f.loader->setItem(nullptr);
            if(mode==9)f.layout.activeValue="wrong";
            const auto r=f.run();require(!r.observed&&!r.refusal.isEmpty()&&!f.navigator.calls,"partial or unrelated state refuses");
        });
        for(int mode=0;mode<8;++mode)scenario([=]{Fixture f;QObject document;bool cancel=false;
            f.reader->onDocument=[&]{
                if(mode==0){emit f.reader->documentChanged();emit f.reader->documentChanged();}
                if(mode==1){f.loader->setVisible(true);f.loader->setVisible(false);}
                if(mode==2){f.main->setEnabled(false);f.main->setEnabled(true);}
                if(mode==3)cancel=true;
                if(mode==4)reader_closed_view::observe(&f.window,[]{return true;});
                if(mode==5)QThread::msleep(5);
                if(mode==6)delete f.reader.data();
                if(mode==7)delete f.root.data();
            };
            const auto r=f.run([&]{return !cancel;},mode==5?2:1000);
            require(!r.observed&&!r.refusal.isEmpty()&&!f.navigator.calls,"getter ABA/cancel/reentry/overrun/lifetime loss refuses");
        });
        scenario([]{Fixture f;bool returned=false;f.loader->onItem=[&]{returned=true;};const auto r=f.run([&]{if(returned)delete f.reader.data();return true;});require(!r.observed&&!f.reader&&!f.navigator.calls,"borrowed item pinned before permission callout");});
        scenario([]{Fixture f;const auto r=f.run([]{return false;});require(!r.observed&&!f.navigator.calls,"initial cancel");});
        scenario([]{Fixture f;const auto r=f.run([]()->bool{throw std::runtime_error("owned");});require(!r.observed&&!f.navigator.calls,"permission exception");});
        scenario([]{Fixture f;const auto r=reader_selected::inspectLegacyFixture(&f.window,{},[]{return true;},1000);require(!r.compositionObserved&&!r.attempted&&!f.navigator.calls,"missing read callback cannot fall through to open");});
        scenario([]{Fixture f;const auto r=reader_selected::inspectLegacyFixture(&f.window,[](QQuickItem *,QQmlContext *,const std::function<bool()> &)->bool{throw std::runtime_error("owned");},[]{return true;},1000);require(!r.compositionObserved&&!r.attempted&&!f.navigator.calls,"read callback exception");});
        scenario([]{Fixture f;bool lateProviderRead=false;const auto r=reader_selected::inspectLegacyFixture(&f.window,[&](QQuickItem *,QQmlContext *,const std::function<bool()> &guard){require(guard(),"terminal read callback final composition guard");f.layout.onRead=[&]{lateProviderRead=true;f.reader->loaded=true;};return true;},[]{return true;},1000);std::printf("post_read_provider_call=%d\n",int(lateProviderRead));require(r.compositionObserved&&!lateProviderRead,"no provider getter after callback-local final snapshot");});
        std::printf("PASS closed view cases=%u checks=%u actual_qml_context=true native_loader_modeled=true native_execution=false\n",cases,checks);return 0;
    }catch(const std::exception &e){std::fprintf(stderr,"FAIL closed view: %s cases=%u checks=%u\n",e.what(),cases,checks);return 1;}
}
#include "reader_closed_view_fixture.moc"
