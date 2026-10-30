// Original Qt models. No firmware implementation or native address is used.
#include "reader_insertion_target_acquisition.h"
#include <QQmlComponent>
#include <QElapsedTimer>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace reader_insertion_target_acquisition;
namespace {
unsigned checks=0,scenarios=0;
void require(bool value,const char *message){++checks;if(!value)throw std::runtime_error(message);}
void scenario(const char *name,const std::function<void()> &body) {
    ++scenarios;try{body();}catch(const std::exception &error){std::cerr<<name<<": "<<error.what()<<'\n';throw;}
}
QString id(int index){return QStringLiteral("00000000-0000-4000-8000-%1").arg(index,12,16,QLatin1Char('0'));}
reader_joined_creation::Request request(int count=3,int source=1) {
    reader_joined_creation::Request value;
    value.documentId=id(100);value.targetPageId=id(200);value.sourceIndex=source;
    for(int i=0;i<count;++i)value.expectedPages.append(id(i+1));
    value.sourcePageId=value.expectedPages[source];return value;
}
reader_insertion_post_order::Result post(const reader_joined_creation::Request &request,bool target=false) {
    reader_insertion_post_order::Result value;
    value.observed=true;value.ids=request.expectedPages;value.ids.insert(request.sourceIndex+1,request.targetPageId);
    value.pageCount=int(value.ids.size());value.currentPageIndex=request.sourceIndex+int(target);
    value.currentPageId=target?request.targetPageId:request.sourcePageId;value.targetSelected=target;
    value.calls=unsigned(2*value.pageCount+5);return value;
}
const Guard yes=[]{return true;};
void prepare(QQmlComponent &component) {
    // Fixture setup alone may run the GUI loop; neither helper does so.
    QElapsedTimer timer;timer.start();
    while(component.isLoading() && timer.elapsed()<3000){QCoreApplication::processEvents();QThread::msleep(1);}
    for(const auto &error:component.errors())std::cerr<<error.toString().toStdString()<<'\n';
    require(component.isReady(),"owned component became ready");
}
}
class Reader:public QObject {
    Q_OBJECT
public:
    int calls=0;
    std::function<void()> hook;
    Q_INVOKABLE void moveForward(){++calls;const auto action=hook;if(action)action();}
};
class VariantReader:public QObject {
    Q_OBJECT
public:
    int calls=0;
    Q_INVOKABLE QVariant moveForward(){++calls;return QStringLiteral("owned-return");}
};
class AmbiguousReader:public QObject {
    Q_OBJECT
public:
    Q_INVOKABLE void moveForward(){}
    Q_INVOKABLE void moveForward(int){}
};
class IntReader:public QObject {Q_OBJECT public:Q_INVOKABLE int moveForward(){return 1;}};
class ArityReader:public QObject {Q_OBJECT public:Q_INVOKABLE void moveForward(int){}};
class SignalReader:public QObject {Q_OBJECT signals:void moveForward();};
class PrivateReader:public QObject {Q_OBJECT private slots:void moveForward(){}};
class ProtectedReader:public QObject {Q_OBJECT protected slots:void moveForward(){}};
class SlotReader:public QObject {Q_OBJECT public slots:void moveForward(){}};
class ReaderWithAnIntentionallyOversizedDeclaringClassNameForBoundedWireContract:public QObject {
    Q_OBJECT
public:Q_INVOKABLE void moveForward(){}
};
// Only metadata is switched. A differing table must refuse before any invoke.
class SwitchReader:public Reader {
public:
    const QMetaObject *alternative=nullptr;
    const QMetaObject *metaObject()const override{return alternative?alternative:Reader::metaObject();}
};

int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);
    qmlRegisterType<QObject>("OwnedTarget",1,0,"Root");
    try {
        scenario("void",[]{
            QQmlEngine engine;Reader reader;QObject document;auto req=request();auto order=post(req);
            const auto metadata=inspect(&engine,&reader,yes);
            require(metadata.admitted && metadata.matches==1,"public method admitted");
            require(metadata.signature==QStringLiteral("moveForward()") && metadata.declaringClass==QStringLiteral("Reader"),"exact descriptor");
            require(metadata.returnTypeId==QMetaType::Void && metadata.returnName==QStringLiteral("void") && metadata.parameterCount==0,"void descriptor");
            require(metadata.access==int(QMetaMethod::Public) && metadata.kind==int(QMetaMethod::Method),"access and kind");
            auto once=std::make_shared<Claim>();const auto result=dispatch(&engine,&reader,&document,req,order,metadata,once,yes);
            require(result.admitted && result.attempted && result.dispatched && !result.unknown && !result.alreadySelected,"dispatch conjunction");
            require(reader.calls==1 && once->consumed.load(),"one consumed effect");
            const auto again=dispatch(&engine,&reader,&document,req,order,metadata,once,yes);
            require(!again.attempted && again.refusal==QStringLiteral("claim-consumed") && reader.calls==1,"no second effect");
            const auto selected=dispatch(&engine,&reader,&document,req,post(req,true),metadata,once,yes);
            require(!selected.admitted && selected.refusal==QStringLiteral("claim-consumed"),"used claim also rejects no-op reuse");
        });
        scenario("variant",[]{
            QQmlEngine e;VariantReader r;QObject d;auto q=request();auto m=inspect(&e,&r,yes);
            require(m.admitted && m.returnTypeId==QMetaType::QVariant && m.returnName==QStringLiteral("QVariant"),"variant contract");
            const auto out=dispatch(&e,&r,&d,q,post(q),m,std::make_shared<Claim>(),yes);
            require(out.dispatched && !out.unknown && r.calls==1,"variant invoke");
        });
        scenario("qml-method",[]{
            QQmlEngine e;QQmlComponent component(&e);
            component.setData("import OwnedTarget 1.0\nRoot { property int calls: 0; function moveForward() { calls += 1; } }",QUrl("qrc:/OwnedReader.qml"));
            prepare(component);
            std::unique_ptr<QObject> r(component.create());QObject d;
            require(bool(r),"owned QML object created");require(qmlEngine(r.get())==&e,"actual engine association");
            const auto m=inspect(&e,r.get(),yes);auto q=request();
            require(m.admitted && m.returnTypeId==QMetaType::QVariant,"actual QML method metadata");
            const auto out=dispatch(&e,r.get(),&d,q,post(q),m,std::make_shared<Claim>(),yes);
            require(out.dispatched && r->property("calls").toInt()==1,"actual QML method invoked once");
        });
        const auto rejectMeta=[](QObject *r,const char *reason){QQmlEngine e;const auto m=inspect(&e,r,yes);require(!m.admitted && m.refusal==QLatin1String(reason),"metadata refused");};
        scenario("missing",[&]{QObject r;rejectMeta(&r,"method-missing");});
        scenario("ambiguous",[&]{AmbiguousReader r;rejectMeta(&r,"method-ambiguous");});
        scenario("return",[&]{IntReader r;rejectMeta(&r,"method-contract");});
        scenario("arity",[&]{ArityReader r;rejectMeta(&r,"method-contract");});
        scenario("signal",[&]{SignalReader r;rejectMeta(&r,"method-contract");});
        scenario("private",[&]{PrivateReader r;rejectMeta(&r,"method-contract");});
        scenario("protected",[&]{ProtectedReader r;rejectMeta(&r,"method-contract");});
        scenario("public-slot",[]{QQmlEngine e;SlotReader r;QObject d;auto q=request();auto m=inspect(&e,&r,yes);require(m.admitted && m.kind==int(QMetaMethod::Slot),"public slot admitted");auto out=dispatch(&e,&r,&d,q,post(q),m,std::make_shared<Claim>(),yes);require(out.dispatched && !out.unknown,"slot invoked");});
        scenario("class-byte-bound",[&]{ReaderWithAnIntentionallyOversizedDeclaringClassNameForBoundedWireContract r;QQmlEngine e;auto m=inspect(&e,&r,yes);require(!m.admitted && m.refusal==QStringLiteral("method-text-bound") && m.declaringClass.isEmpty(),"oversized names not retained");});
        scenario("method-count-bound",[]{
            QQmlEngine e;QQmlComponent component(&e);QByteArray text="import OwnedTarget 1.0\nRoot { function moveForward() {}\n";
            for(int i=0;i<513;++i)text+="function f"+QByteArray::number(i)+"() {}\n";
            text+="}";component.setData(text,QUrl("qrc:/OwnedWide.qml"));prepare(component);std::unique_ptr<QObject> r(component.create());
            require(bool(r),"wide QML created");require(r->metaObject()->methodCount()>512,"actual meta count exceeds cap");
            auto m=inspect(&e,r.get(),yes);require(!m.admitted && m.refusal==QStringLiteral("method-bound"),"method cap");
        });
        scenario("already-selected",[]{QQmlEngine e;Reader r;QObject d;auto q=request();auto c=std::make_shared<Claim>();auto m=inspect(&e,&r,yes);auto out=dispatch(&e,&r,&d,q,post(q,true),m,c,yes);require(out.admitted && out.alreadySelected && !out.attempted && !out.dispatched && !out.unknown,"target no-op");require(!c->consumed.load() && r.calls==0,"no invoke claim for no-op");});
        for(const int count:{1,2,16})scenario("last-or-single",[count]{QQmlEngine e;Reader r;QObject d;auto q=request(count,count-1);auto m=inspect(&e,&r,yes);auto out=dispatch(&e,&r,&d,q,post(q),m,std::make_shared<Claim>(),yes);require(out.dispatched && r.calls==1,"post insertion neighbor exists");});
        const std::vector<std::function<void(reader_joined_creation::Request&,reader_insertion_post_order::Result&)>> badOrders={
            [](auto&,auto&p){p.observed=false;},[](auto&,auto&p){p.refusal=QStringLiteral("failed");},
            [](auto&,auto&p){++p.pageCount;},[](auto&,auto&p){p.ids.swapItemsAt(0,1);},
            [](auto&,auto&p){p.ids[2]=id(901);},[](auto&,auto&p){p.currentPageIndex=0;p.currentPageId=id(1);},
            [](auto&,auto&p){p.targetSelected=true;},[](auto&,auto&p){--p.calls;},
            [](auto&q,auto&){q.targetPageId=q.sourcePageId;},[](auto&q,auto&){q.sourceIndex=-1;},
            [](auto&q,auto&){q.documentId=QStringLiteral("bad");}
        };
        for(const auto &mutate:badOrders)scenario("invalid-order",[&]{QQmlEngine e;Reader r;QObject d;auto q=request();auto p=post(q);auto m=inspect(&e,&r,yes);mutate(q,p);auto c=std::make_shared<Claim>();auto out=dispatch(&e,&r,&d,q,p,m,c,yes);require(!out.admitted && !out.attempted && !out.unknown && r.calls==0 && !c->consumed.load(),"inconsistent order cannot invoke");});
        scenario("request-cap",[]{QQmlEngine e;Reader r;QObject d;auto q=request(17,1);auto out=dispatch(&e,&r,&d,q,post(q),inspect(&e,&r,yes),std::make_shared<Claim>(),yes);require(out.refusal==QStringLiteral("request") && !out.attempted,"N bound");});
        const std::vector<std::function<void(MethodInfo&)>> drift={
            [](auto&m){m.admitted=false;},[](auto&m){++m.index;},[](auto&m){++m.returnTypeId;},
            [](auto&m){++m.access;},[](auto&m){++m.kind;},[](auto&m){++m.parameterCount;},
            [](auto&m){++m.matches;},[](auto&m){m.signature+=QLatin1Char('x');},
            [](auto&m){m.declaringClass+=QLatin1Char('x');},[](auto&m){m.returnName+=QLatin1Char('x');},
            [](auto&m){m.refusal=QStringLiteral("bad");}
        };
        for(const auto &mutate:drift)scenario("descriptor-drift",[&]{QQmlEngine e;Reader r;QObject d;auto q=request();auto m=inspect(&e,&r,yes);mutate(m);auto c=std::make_shared<Claim>();auto out=dispatch(&e,&r,&d,q,post(q),m,c,yes);require(out.refusal==QStringLiteral("method-changed") && !out.attempted && !c->consumed.load(),"all descriptor fields bound");});
        for(int at:{1,2,3})scenario("inspect-guard-loss",[at]{QQmlEngine e;Reader r;int calls=0;auto out=inspect(&e,&r,[&]{return ++calls!=at;});require(!out.admitted && out.refusal==QStringLiteral("guard"),"inspect guard checked");});
        for(int at:{1,2,3,4})scenario("dispatch-guard-loss",[at]{QQmlEngine e;Reader r;QObject d;auto q=request();auto m=inspect(&e,&r,yes);int calls=0;auto c=std::make_shared<Claim>();auto out=dispatch(&e,&r,&d,q,post(q),m,c,[&]{return ++calls!=at;});require(!out.attempted && !out.admitted && !out.unknown && !c->consumed.load(),"guard loss before effect");});
        scenario("guard-exception",[]{QQmlEngine e;Reader r;QObject d;auto q=request();auto m=inspect(&e,&r,yes);auto out=dispatch(&e,&r,&d,q,post(q),m,std::make_shared<Claim>(),[]{throw std::runtime_error("owned");return true;});require(!out.attempted && out.refusal==QStringLiteral("exception") && !out.unknown,"pre-effect throw");auto meta=inspect(&e,&r,[]{throw std::runtime_error("owned");return true;});require(!meta.admitted && meta.refusal==QStringLiteral("exception"),"inspect throw");});
        scenario("inspect-reentry",[]{QQmlEngine e;Reader r;bool nested=false;auto m=inspect(&e,&r,[&]{if(!nested){nested=true;auto n=inspect(&e,&r,yes);require(n.refusal==QStringLiteral("reentered"),"nested refused");}return true;});require(!m.admitted && m.refusal==QStringLiteral("guard"),"nested inspect poisons outer");});
        scenario("dispatch-reentry",[]{QQmlEngine e;Reader r;QObject d;auto q=request();auto p=post(q);auto m=inspect(&e,&r,yes);auto c=std::make_shared<Claim>();bool nested=false;auto out=dispatch(&e,&r,&d,q,p,m,c,[&]{if(!nested){nested=true;auto n=dispatch(&e,&r,&d,q,p,m,c,yes);require(!n.attempted && n.refusal==QStringLiteral("reentered"),"nested effect refused");}return true;});require(!out.attempted && !c->consumed.load(),"outer poisoned");});
        scenario("effect-reentry",[]{QQmlEngine e;Reader r;QObject d;auto q=request();auto p=post(q);auto m=inspect(&e,&r,yes);auto c=std::make_shared<Claim>();r.hook=[&]{auto n=dispatch(&e,&r,&d,q,p,m,c,yes);require(!n.attempted && n.refusal==QStringLiteral("reentered"),"effect nested refused");};auto out=dispatch(&e,&r,&d,q,p,m,c,yes);require(out.dispatched && !out.unknown && r.calls==1,"no old epoch check after effect");});
        for(int at:{1,2,3})scenario("inspect-meta-pointer-drift",[at]{QQmlEngine e;SwitchReader r;int calls=0;auto m=inspect(&e,&r,[&]{if(++calls==at)r.alternative=&VariantReader::staticMetaObject;return true;});if(at==1)require(m.admitted && m.returnTypeId==QMetaType::QVariant,"initial metadata sampled");else require(!m.admitted && m.refusal==QStringLiteral("method-changed"),"metadata pointer drift rejected");});
        for(int at:{2,3,4})scenario("dispatch-meta-pointer-drift",[at]{QQmlEngine e;SwitchReader r;QObject d;auto q=request();auto m=inspect(&e,&r,yes);int calls=0;auto out=dispatch(&e,&r,&d,q,post(q),m,std::make_shared<Claim>(),[&]{if(++calls==at)r.alternative=&VariantReader::staticMetaObject;return true;});require(!out.attempted && out.refusal==QStringLiteral("method-changed"),"no stale meta invoke");});
        scenario("intentional-old-epoch-loss",[]{QQmlEngine e;Reader r;QObject d;auto q=request();auto m=inspect(&e,&r,yes);bool live=true;int calls=0,atEffect=0;r.hook=[&]{atEffect=calls;live=false;};auto out=dispatch(&e,&r,&d,q,post(q),m,std::make_shared<Claim>(),[&]{++calls;return live;});require(out.dispatched && !out.unknown && !live && calls==atEffect,"old guard unused after intentional change");});
        scenario("invoke-throw-once",[]{QQmlEngine e;Reader r;QObject d;auto q=request();auto m=inspect(&e,&r,yes);auto c=std::make_shared<Claim>();r.hook=[]{throw std::runtime_error("owned-invoke");};auto out=dispatch(&e,&r,&d,q,post(q),m,c,yes);require(out.admitted && out.attempted && !out.dispatched && out.unknown && out.refusal==QStringLiteral("invoke-exception"),"throw is consumed unknown");auto again=dispatch(&e,&r,&d,q,post(q),m,c,yes);require(!again.attempted && r.calls==1 && c->consumed.load(),"throw cannot retry");});
        scenario("null-claim",[]{QQmlEngine e;Reader r;QObject d;auto q=request();auto out=dispatch(&e,&r,&d,q,post(q),inspect(&e,&r,yes),{},yes);require(!out.attempted && out.refusal==QStringLiteral("claim-consumed"),"claim required");});
        for(int which:{0,1})scenario("foreign-engine",[which]{QQmlEngine e,foreign;Reader r;QObject d;QQmlEngine::setContextForObject(which?&d:static_cast<QObject*>(&r),foreign.rootContext());require(qmlEngine(which?&d:static_cast<QObject*>(&r))==&foreign,"foreign association actual");auto m=inspect(&e,&r,yes);auto q=request();auto out=dispatch(&e,&r,&d,q,post(q),m,std::make_shared<Claim>(),yes);require(!out.attempted && out.refusal==QStringLiteral("guard"),"foreign association refused");if(!which)require(!m.admitted,"inspect foreign reader refused");});
        for(int which:{0,1,2})scenario("null-object",[which]{QQmlEngine e;Reader r;QObject d;auto q=request();auto m=inspect(&e,&r,yes);auto out=dispatch(which==0?nullptr:&e,which==1?nullptr:&r,which==2?nullptr:&d,q,post(q),m,std::make_shared<Claim>(),yes);require(!out.attempted && out.refusal==QStringLiteral("guard"),"null object refused");});
        for(int which:{0,1,2})for(int at:{1,2,3,4})scenario("deletion-at-guard",[which,at]{auto e=std::make_unique<QQmlEngine>();auto r=std::make_unique<Reader>();auto d=std::make_unique<QObject>();auto q=request();auto m=inspect(e.get(),r.get(),yes);int calls=0;auto out=dispatch(e.get(),r.get(),d.get(),q,post(q),m,std::make_shared<Claim>(),[&]{if(++calls==at){if(which==0)e.reset();else if(which==1)r.reset();else d.reset();}return true;});require(!out.attempted && out.refusal==QStringLiteral("guard"),"guard-deleted object pinned");});
        scenario("inspect-reader-deletion",[]{QQmlEngine e;auto r=std::make_unique<Reader>();auto m=inspect(&e,r.get(),[&]{r.reset();return true;});require(!m.admitted && m.refusal==QStringLiteral("guard"),"inspect deletion safe");});
        scenario("delete-during-effect",[]{QQmlEngine e;auto r=std::make_unique<Reader>();auto d=std::make_unique<QObject>();auto q=request();auto m=inspect(&e,r.get(),yes);r->hook=[&]{d.reset();r.reset();};auto out=dispatch(&e,r.get(),d.get(),q,post(q),m,std::make_shared<Claim>(),yes);require(out.dispatched && !out.unknown && !r && !d,"no borrowed object read after invoke");});
        scenario("facade-inputs-deletion",[]{
            struct Facade{reader_joined_creation::Request q=request();reader_insertion_post_order::Result p=post(q);MethodInfo m;std::shared_ptr<Claim> c=std::make_shared<Claim>();Guard guard;};
            QQmlEngine e;Reader r;QObject d;auto f=std::make_unique<Facade>();f->m=inspect(&e,&r,yes);std::weak_ptr<Claim> weak=f->c;
            bool heldAtEffect=false;r.hook=[&]{auto c=weak.lock();heldAtEffect=c && c->consumed.load();};f->guard=[&]{f.reset();return true;};
            auto out=dispatch(&e,&r,&d,f->q,f->p,f->m,f->c,f->guard);
            require(out.dispatched && heldAtEffect && !f && weak.expired(),"owned inputs and claim survive destroyed facade");
        });
        scenario("off-gui-call",[]{QQmlEngine e;Reader r;QObject d;auto q=request();auto m=inspect(&e,&r,yes);Result out;MethodInfo info;std::thread worker([&]{info=inspect(&e,&r,yes);out=dispatch(&e,&r,&d,q,post(q),m,std::make_shared<Claim>(),yes);});worker.join();require(!info.admitted && info.refusal==QStringLiteral("gui") && !out.attempted && out.refusal==QStringLiteral("gui"),"GUI execution only");});
        scenario("foreign-reader-thread",[]{
            QQmlEngine e;std::promise<Reader*> ready;std::promise<void> release;auto future=release.get_future();
            std::thread worker([&]{Reader r;ready.set_value(&r);future.wait();});Reader *r=ready.get_future().get();
            auto m=inspect(&e,r,yes);require(!m.admitted && m.refusal==QStringLiteral("guard"),"same GUI ownership required");release.set_value();worker.join();
        });
        require(!Result::nativeAuthority && !Result::renderAuthority && !Result::durableAuthority,"all authority false");
        std::cout<<"{\"scenarios\":"<<scenarios<<",\"checks\":"<<checks<<",\"native_authority\":false,\"mode\":\"owned-qt-models\"}\n";
        return 0;
    } catch(...){return 1;}
}
#include "reader_insertion_target_acquisition_fixture.moc"
