// Real owned Qt/QML methods; source identity/native joins explicitly modeled.
#define READER_ORDINARY_CLOSE_TESTING
#include "reader_ordinary_close.h"
#include <QQmlComponent>
#include <QThread>
#include <cstdio>
#include <stdexcept>
#include <thread>

class OwnedClose:public QQuickItem {
    Q_OBJECT
public:
    int calls=0;std::function<void()> action;
public Q_SLOTS:
    QVariant close(){++calls;const auto copied=action;if(copied)copied();return {};}
};
class InheritedClose:public OwnedClose {Q_OBJECT};
class WrongClose:public QQuickItem {Q_OBJECT public:Q_INVOKABLE void close(){++calls;}int calls=0;};
class DriftingClose:public OwnedClose {
public:
    bool drift=false;
    const QMetaObject *metaObject()const override{return drift?&WrongClose::staticMetaObject:&OwnedClose::staticMetaObject;}
};
namespace {
using namespace reader_ordinary_close;
unsigned checks=0,cases=0;
void require(bool b,const char *m){++checks;if(!b)throw std::runtime_error(m);}
Expected expected(){return {QStringLiteral("11111111-2222-3333-4444-555555555555"),QStringLiteral("22222222-3333-4444-5555-666666666666"),2,4};}
reader_selected::Observation facts(){auto e=expected();reader_selected::Observation s;s.documentId=e.documentId;s.pageId=e.pageId;s.pageIndex=e.pageIndex;s.pageCount=e.pageCount;s.status=s.pageStatus=1;s.loading=s.pageError=s.documentError=s.awaitingTiles=s.pendingTiles=s.working=false;s.observed=s.mapCoherent=s.creationJoinObserved=s.ready=true;return s;}
struct Seam {
    QObject *reader=nullptr;reader_selected::Observation observed=facts();Guard guard=[]{return true;};
    unsigned repeats=1,called=0;std::function<void()> before;
    TestBoundary boundary(){return {[this](QQuickWindow *,const Guard &,int,const reader_selected::ValidatedSourceAction &action){
        if(before)before();
        for(unsigned i=0;i<repeats;++i){++called;action(reader,nullptr,nullptr,{},observed,guard);}
        return observed;
    }};}
};
template<class F>void scenario(F f){++cases;f();}
}
int main(int argc,char **argv){
    QGuiApplication app(argc,argv);
    try {
        QQuickWindow window;auto e=expected();const Guard yes=[]{return true;};
        scenario([&]{OwnedClose reader;Seam s;s.reader=&reader;auto claim=std::make_shared<Claim>();auto r=run(&window,e,claim,yes,1000,s.boundary());require(r.claimed&&r.matched&&r.attempted&&r.dispatched&&r.effectRequiresReconciliation&&r.refusal.isEmpty(),"owned direct invoke");require(reader.calls==1&&r.before.pageId==e.pageId,"exact copied before and one call");auto again=run(&window,e,claim,yes,1000,s.boundary());require(again.refusal==QStringLiteral("claim-spent")&&!again.attempted&&reader.calls==1,"spent claim cannot close twice");});
        scenario([&]{qmlRegisterType<QQuickItem>("OwnedClose",1,0,"Item");QQmlEngine engine;QQmlComponent c(&engine);c.setData("import OwnedClose 1.0\nItem { property int calls: 0; function close() { calls += 1; } }",QUrl("file:///owned-close.qml"));if(!c.isReady())qWarning()<<"QML status"<<c.status()<<c.errors();require(c.isReady(),"owned QML ready");std::unique_ptr<QObject> reader(c.create());require(bool(reader),"owned QML created");const auto *meta=reader->metaObject();const auto method=meta->method(meta->indexOfMethod("close()"));require(method.methodType()==QMetaMethod::Slot&&method.access()==QMetaMethod::Public&&method.returnMetaType()==QMetaType::fromType<QVariant>()&&method.enclosingMetaObject()==meta,"actual generated QML public slot declaration");Seam s;s.reader=reader.get();auto r=run(&window,e,std::make_shared<Claim>(),yes,1000,s.boundary());require(r.dispatched&&r.refusal.isEmpty()&&reader->property("calls").toInt()==1,"actual QML close slot uses QVariant return");});
        for(int field=0;field<12;++field)scenario([&]{OwnedClose reader;Seam s;s.reader=&reader;switch(field){case 0:s.observed.documentId="wrong";break;case 1:s.observed.pageId="wrong";break;case 2:s.observed.pageIndex=1;break;case 3:s.observed.pageCount=3;break;case 4:s.observed.ready=false;break;case 5:s.observed.creationJoinObserved=false;break;case 6:s.observed.mapCoherent=false;break;case 7:s.observed.working=true;break;case 8:s.observed.loading=true;break;case 9:s.observed.nativeAuthority=true;break;case 10:s.observed.refusal="old";break;default:s.observed.pendingTiles=true;}auto r=run(&window,e,std::make_shared<Claim>(),yes,1000,s.boundary());require(!r.attempted&&reader.calls==0&&r.refusal==QStringLiteral("source-mismatch"),"wrong source fact cannot dispatch");});
        scenario([&]{QQuickItem reader;Seam s;s.reader=&reader;auto r=run(&window,e,std::make_shared<Claim>(),yes,1000,s.boundary());require(!r.attempted&&r.refusal==QStringLiteral("close-method"),"missing method refused");});
        scenario([&]{WrongClose reader;Seam s;s.reader=&reader;auto r=run(&window,e,std::make_shared<Claim>(),yes,1000,s.boundary());require(!r.attempted&&!reader.calls&&r.refusal==QStringLiteral("close-method"),"wrong return refused");});
        scenario([&]{InheritedClose reader;Seam s;s.reader=&reader;auto r=run(&window,e,std::make_shared<Claim>(),yes,1000,s.boundary());require(!r.attempted&&!reader.calls&&r.refusal==QStringLiteral("close-method"),"inherited close refused");});
        scenario([&]{OwnedClose reader;Seam s;s.reader=&reader;s.guard=[] {return false;};auto r=run(&window,e,std::make_shared<Claim>(),yes,1000,s.boundary());require(!r.attempted&&!reader.calls,"failed source guard");});
        scenario([&]{QPointer<OwnedClose> reader=new OwnedClose;Seam s;s.reader=reader;s.guard=[&]{delete reader.data();return true;};auto r=run(&window,e,std::make_shared<Claim>(),yes,1000,s.boundary());require(!reader&&!r.attempted,"guard deletes reader before method lookup");});
        scenario([&]{QPointer<OwnedClose> reader=new OwnedClose;Seam s;s.reader=reader;reader->action=[&]{delete reader.data();};auto r=run(&window,e,std::make_shared<Claim>(),yes,1000,s.boundary());require(!reader&&r.attempted&&r.dispatched&&r.effectRequiresReconciliation,"terminal action may delete receiver");});
        scenario([&]{OwnedClose reader;Seam s;s.reader=&reader;reader.action=[] {QThread::msleep(6);};auto r=run(&window,e,std::make_shared<Claim>(),yes,3,s.boundary());require(r.attempted&&r.dispatched&&r.effectRequiresReconciliation&&r.refusal==QStringLiteral("deadline-after-dispatch")&&reader.calls==1,"nonpreemptible overrun keeps possible effects");});
        scenario([&]{OwnedClose reader;Seam s;s.reader=&reader;auto claim=std::make_shared<Claim>();auto r=run(&window,e,claim,[]{return false;},1000,s.boundary());require(r.claimed&&!r.attempted&&s.called==0,"canceled before lookup");auto again=run(&window,e,claim,yes,1000,s.boundary());require(!again.attempted&&reader.calls==0,"canceled claim never renewed");});
        scenario([&]{OwnedClose reader;Seam s;s.reader=&reader;auto r=run(&window,e,std::make_shared<Claim>(),[]{QThread::msleep(5);return true;},1,s.boundary());require(!r.attempted&&s.called==0,"permission cannot spend budget then authorize");});
        scenario([&]{OwnedClose reader;Seam s;s.reader=&reader;auto claim=std::make_shared<Claim>();Result nested;bool inside=false;Guard p=[&]{if(!inside){inside=true;nested=run(&window,e,claim,yes,1000,s.boundary());}return true;};auto r=run(&window,e,claim,p,1000,s.boundary());require(r.dispatched&&reader.calls==1&&!nested.attempted&&nested.refusal==QStringLiteral("claim-spent"),"nested permission cannot duplicate operation");});
        scenario([&]{OwnedClose reader;Seam s;s.reader=&reader;s.repeats=2;auto r=run(&window,e,std::make_shared<Claim>(),yes,1000,s.boundary());require(r.attempted&&r.dispatched&&reader.calls==1&&r.refusal==QStringLiteral("source-callback-repeat"),"duplicated modeled callback does not dispatch again");});
        scenario([&]{OwnedClose reader;Seam s;s.reader=&reader;auto r=run(&window,e,std::make_shared<Claim>(),[]{throw std::runtime_error("owned");return true;},1000,s.boundary());require(r.claimed&&!r.attempted&&r.refusal==QStringLiteral("exception"),"permission exception no dispatch");});
        scenario([&]{OwnedClose reader;Seam s;s.reader=&reader;reader.action=[] {throw std::runtime_error("owned");};auto r=run(&window,e,std::make_shared<Claim>(),yes,1000,s.boundary());require(r.attempted&&r.effectRequiresReconciliation&&reader.calls==1&&r.refusal==QStringLiteral("exception"),"invoke exception retains attempted effect");});
        scenario([&]{OwnedClose reader;Seam s;s.reader=&reader;auto claim=std::make_shared<Claim>();auto invalid=e;invalid.pageId="bad";auto r=run(&window,invalid,claim,yes,1000,s.boundary());require(r.claimed&&!r.attempted&&!s.called,"invalid configuration consumes claim");require(!run(&window,e,claim,yes,1000,s.boundary()).attempted,"invalid cannot renew");});
        scenario([&]{auto claim=std::make_shared<Claim>();auto r=run(nullptr,e,claim,yes);require(r.claimed&&!r.attempted&&r.refusal==QStringLiteral("application-thread"),"null window");});
        scenario([&]{auto claim=std::make_shared<Claim>();Result r;std::thread other([&]{r=run(&window,e,claim,yes);});other.join();require(r.claimed&&!r.attempted&&r.refusal==QStringLiteral("application-thread"),"wrong actual thread");});
        scenario([&]{auto r=run(&window,e,{},yes);require(!r.claimed&&!r.attempted&&r.refusal==QStringLiteral("claim-spent"),"missing claim");});
        scenario([&]{DriftingClose reader;Seam s;s.reader=&reader;unsigned guards=0;s.guard=[&]{if(++guards==3)reader.drift=true;return true;};auto r=run(&window,e,std::make_shared<Claim>(),yes,1000,s.boundary());std::printf("meta_drift_attempted=%d calls=%d\n",int(r.attempted),reader.calls);require(!r.attempted&&!reader.calls&&r.refusal==QStringLiteral("source-changed"),"final stable-table meta drift refuses before dispatch");});
        std::printf("PASS ordinary close cases=%u checks=%u actual_qml_method=true native_join_modeled=true native_execution=false\n",cases,checks);return 0;
    }catch(const std::exception &error){std::fprintf(stderr,"FAIL ordinary close: %s cases=%u checks=%u\n",error.what(),cases,checks);return 1;}
}
#include "reader_ordinary_close_fixture.moc"
