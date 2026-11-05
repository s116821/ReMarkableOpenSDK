// Original owned scope/Qt-JS controls. No stock Task or native qualification.
#include "reader_request_scope.h"
#include "reader_retirement_scope.h"
#include "qt_qml_creation_bridge.h"
#include <QElapsedTimer>
#include <QEventLoop>
#include <QJSEngine>
#include <QJSValue>
#include <QTextStream>
#include <QTimer>
#include <stdexcept>
#include <type_traits>

namespace {
using reader_request_scope::Binding;
using reader_request_scope::Lease;
using reader_request_scope::Scope;
using namespace reader_retirement;
int checks=0,scenarios=0;
void require(bool yes,const char *name) { if(!yes)qFatal("FAIL %s",name);++checks; }
void scenario() { ++scenarios; }
QByteArray nonce(unsigned n) { return QByteArray::number(n,16).rightJustified(32,'0'); }
void nested(int ms=2) { QEventLoop loop;QTimer::singleShot(ms,&loop,&QEventLoop::quit);loop.exec(); }
void awaitRetired(Fence &f,const Token &t) {
    QElapsedTimer elapsed;elapsed.start();
    while(f.snapshot(t).phase!=Phase::Retired && f.snapshot(t).phase!=Phase::Failed && elapsed.elapsed()<500)nested(1);
    require(f.snapshot(t).phase==Phase::Retired,"retired after distinct owned queued turns");
    require(f.snapshot(t).depth==0,"retired has zero depth");
}
struct Trace { int alive=0,released=0,pumped=0; };
struct CountingLease final : Lease {
    Trace &trace;
    explicit CountingLease(Trace &t):trace(t) { ++trace.alive; }
    ~CountingLease() noexcept override { --trace.alive;++trace.released; }
};
struct FinalCapture {
    Trace &trace;
    explicit FinalCapture(Trace &t):trace(t) {}
    ~FinalCapture() noexcept {
        require(trace.alive==1,"last Hook capture destructor still holds lease");
        ++trace.pumped;nested();
        require(trace.alive==1,"lease survives destructor nested event loop");
    }
};
struct FrameLease final : Lease {
    Frame frame;
    FrameLease(Fence &f,const Token &t):frame(f.enter(t)) {}
};
struct FenceCapture {
    Fence &f;Token token;int &cleanup;int &destroyed;
    FenceCapture(Fence &owner,Token t,int &c,int &d):f(owner),token(std::move(t)),cleanup(c),destroyed(d) {}
    ~FenceCapture() noexcept {
        ++destroyed;
        require(f.snapshot(token).depth==1,"Hook destruction remains inside actual Fence Frame");
        require(f.snapshot(token).phase==Phase::Terminal,"Hook destruction after terminal remains tracked");
        nested(4);
        require(cleanup==0 && f.snapshot(token).depth==1,"cleanup blocked throughout last Hook capture unwind");
    }
};
struct AdmissionCapture {
    Fence &f;Token token;int &cleanup;int &destroyed;
    AdmissionCapture(Fence &owner,Token t,int &c,int &d):f(owner),token(std::move(t)),cleanup(c),destroyed(d) {}
    ~AdmissionCapture() noexcept {
        ++destroyed;
        require(f.snapshot(token).depth==1 && f.accepts(token),"last copied Boolean callback capture is destroyed under active Frame");
        int *const cleanupCount=&cleanup;
        require(f.requestRetire(token,[cleanupCount]{++*cleanupCount;},Effect::NoMutationAttempted),"last copied callback destructor retires original request");
        nested(4);
        require(cleanup==0 && f.snapshot(token).depth==1 && !f.accepts(token),"callback capture destructor cannot clean before admission recheck");
    }
};
bool booleanAdmission(qml_access::CreationBridge &bridge,unsigned which) {
    if(which==0)return bridge.preflightAllowed();
    if(which==1)return bridge.claimMutation();
    return bridge.admitLibraryReady();
}
struct BridgeCalls { int handlers=0,checks=0,callbacks=0; };
void installHandlers(qml_access::CreationBridge &bridge,Fence &f,const Token &token,BridgeCalls &calls) {
    const auto entered=[&f,token,&calls] {
        require(f.snapshot(token).depth>0,"actual CreationBridge handler has request Frame");
        ++calls.handlers;
    };
    bridge.check=[entered,&calls](bool){entered();++calls.checks;return true;};
    bridge.refusal=[entered](const QString &){entered();};
    bridge.returned=[entered](bool,bool){entered();};
    bridge.exception=[entered](const QString &){entered();};
    bridge.privateException=[entered](const QString &,const QString &,const QString &,const QString &){entered();};
    bridge.libraryContinuation=[entered]{entered();};
    bridge.progress=[entered](const QString &,const QString &){entered();};
    bridge.libraryAdmission=[entered]{entered();return true;};
    bridge.callback=[entered,&calls]{entered();++calls.callbacks;};
}
QJSValue jsCallback(QJSEngine &engine,qml_access::CreationBridge &bridge) {
    QJSEngine::setObjectOwnership(&bridge,QJSEngine::CppOwnership);
    const auto factory=engine.evaluate(QStringLiteral(
        "(function(b){return function(){"
        "var a=b.callbackAllowed(),p=b.preflightAllowed(),m=b.claimMutation(),l=b.admitLibraryReady();"
        "b.refuse('owned');b.observeReturn(true,false);b.observeException('owned');"
        "b.observePrivateException('owned','owned','name','message');"
        "b.queueLibraryContinuation();b.observeProgress('owned','ready');b.observeCallback();"
        "return (a?1:0)+(p?2:0)+(m?4:0)+(l?8:0);};})"));
    require(factory.isCallable(),"actual JS callback factory");
    auto callback=factory.call({engine.newQObject(&bridge)});
    require(callback.isCallable(),"actual JS callback captures actual CreationBridge");
    return callback;
}
}

int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);
    static_assert(!std::is_copy_constructible<Scope>::value && !std::is_move_constructible<Scope>::value,"stack-bound Scope");
    static_assert(std::has_virtual_destructor<Lease>::value,"polymorphic lease release");
    {
        scenario();Binding legacy;const Scope scope(legacy);
        require(!legacy.bound() && scope.entered() && scope.active(),"explicit unbound legacy compatibility");
    }
    {
        scenario();int acquired=0;
        Binding noAcquire({},[]{return true;});const Scope a(noAcquire);
        require(noAcquire.bound() && !a.entered() && !a.active(),"bound missing acquire refuses");
        Binding noAccept([&]() -> std::unique_ptr<Lease> {++acquired;return {};},{});const Scope b(noAccept);
        require(noAccept.bound() && !b.entered() && !b.active() && acquired==0,"missing accept refuses before acquire");
    }
    {
        scenario();int acquired=0,accepted=0;
        Binding empty([&]() -> std::unique_ptr<Lease> {++acquired;return {};},[&]{++accepted;return true;});
        const Scope a(empty);require(!a.entered() && !a.active() && acquired==1 && accepted==0,"null lease cannot call acceptance");
        Binding throwing([]() -> std::unique_ptr<Lease> {throw std::runtime_error("owned acquire");},[]{return true;});
        const Scope b(throwing);require(!b.entered() && !b.active(),"throwing acquire refuses");
    }
    {
        scenario();Trace trace;bool accepts=true;int calls=0;
        Binding binding([&]{return std::make_unique<CountingLease>(trace);},[&]{++calls;return accepts;});
        { const Scope scope(binding);require(scope.entered() && scope.active() && trace.alive==1,"bound active scope");
          accepts=false;require(scope.entered() && !scope.active() && calls==2,"entered remains distinct from fresh active"); }
        require(trace.alive==0 && trace.released==1,"lease released exactly once");
    }
    {
        scenario();Trace trace;
        Binding binding([&]{return std::make_unique<CountingLease>(trace);},[]() -> bool {throw std::runtime_error("owned accepts");});
        { const Scope scope(binding);require(scope.entered() && !scope.active(),"throwing accepts refuses without discarding lease"); }
        require(trace.released==1,"throwing accepts lease destruction");
    }
    {
        scenario();Trace trace;std::unique_ptr<Binding> original;
        original=std::make_unique<Binding>([&]() -> std::unique_ptr<Lease> {
            original.reset();return std::make_unique<CountingLease>(trace);
        },[]{return true;});
        { const Scope scope(*original);require(!original && scope.entered() && scope.active(),"snapshot survives original Binding deletion inside acquire"); }
        require(trace.released==1,"deleted original Binding does not leak lease");
    }
    {
        // Deliberately reentrant owned capture stress. Production adapter Hooks
        // remain fixed nonreentrant internals, not a generic callback promise.
        scenario();Trace trace;std::unique_ptr<Binding> original;
        auto capture=std::make_shared<FinalCapture>(trace);
        original=std::make_unique<Binding>([&,capture]() -> std::unique_ptr<Lease> {
            original.reset();return std::make_unique<CountingLease>(trace);
        },[capture]{return true;});capture.reset();
        { const Scope scope(*original);require(scope.entered() && scope.active() && trace.pumped==0,"Hooks retained after source deletion"); }
        require(trace.pumped==1 && trace.released==1 && trace.alive==0,"last Hooks capture unwinds before lease release");
    }
    {
        scenario();Fence f;auto token=f.begin(nonce(1),Kind::ReadOnly);int cleanup=0;
        const auto binding=reader_retirement::binding(&f,token);
        { const Scope outer(binding);require(outer.entered() && outer.active() && f.snapshot(token).depth==1,"adapter enters actual Frame");
          { const Scope inner(binding);require(inner.entered() && f.snapshot(token).depth==2,"nested actual Frame");
            require(f.requestRetire(token,[&]{++cleanup;},Effect::NoMutationAttempted),"request retirement while scopes live");
            require(outer.entered() && !outer.active() && inner.entered() && !inner.active(),"terminal invalidates acceptance without dropping Frames");
            nested(5);require(cleanup==0 && f.snapshot(token).phase==Phase::Terminal,"nested loop cannot clean active stack"); }
          require(f.snapshot(token).depth==1,"inner unwind preserves outer Frame"); }
        require(f.snapshot(token).depth==0 && cleanup==0,"Frame final epilogue never runs cleanup inline");
        awaitRetired(f,token);require(cleanup==1,"cleanup runs exactly once after scopes unwind");
        const Scope retired(binding);require(!retired.entered() && !retired.active(),"retired adapter refuses");
    }
    {
        scenario();Fence f;auto token=f.begin(nonce(2),Kind::ReadOnly);int cleanup=0;
        f.requestRetire(token,[&]{++cleanup;},Effect::NoMutationAttempted);
        { const Scope scope(reader_retirement::binding(&f,token));
          require(scope.entered() && !scope.active() && f.snapshot(token).depth==1,"Terminal callback tracked before rejected getter");
          nested();require(cleanup==0,"rejected callback still fences nested loop"); }
        awaitRetired(f,token);require(cleanup==1,"terminal callback eventually unwinds");
    }
    {
        // Independent last-capture ordering control using the actual Fence.
        scenario();Fence f;auto token=f.begin(nonce(3),Kind::ReadOnly);int cleanup=0,destroyed=0;
        auto capture=std::make_shared<FenceCapture>(f,token,cleanup,destroyed);
        auto original=std::make_unique<Binding>([&f,token,capture]() -> std::unique_ptr<Lease> {
            return std::make_unique<FrameLease>(f,token);
        },[&f,token,capture]{return f.accepts(token);});capture.reset();
        { const Scope scope(*original);original.reset();
          require(f.requestRetire(token,[&]{++cleanup;},Effect::NoMutationAttempted),"terminal before last Hook unwind"); }
        require(destroyed==1 && cleanup==0 && f.snapshot(token).depth==0,"last Hook destructor pumped under actual lease");
        awaitRetired(f,token);require(cleanup==1,"cleanup follows last capture and Frame epilogue");
    }
    {
        scenario();const Scope null(reader_retirement::binding(nullptr,{}));require(!null.entered() && !null.active(),"null fence refuses");
        Fence f;const Scope invalid(reader_retirement::binding(&f,{}));require(!invalid.entered() && !invalid.active(),"invalid token refuses");
        auto *owner=new Fence;auto token=owner->begin(nonce(4),Kind::ReadOnly);auto binding=reader_retirement::binding(owner,token);delete owner;
        const Scope dead(binding);require(!dead.entered() && !dead.active(),"dead fence refuses without stale access");
    }
    {
        scenario();QJSEngine engine;QObject owner;Fence f;auto a=f.begin(nonce(10),Kind::ReadOnly);BridgeCalls old;
        qml_access::CreationBridge bridge(&owner,reader_retirement::binding(&f,a));installHandlers(bridge,f,a,old);
        const auto callback=jsCallback(engine,bridge);
        require(callback.call().toInt()==15 && old.handlers==10 && old.checks==2 && old.callbacks==1,"all actual active JS bridge entries admitted");
        require(f.snapshot(a).depth==0,"all actual bridge slots unwind");
        f.requestRetire(a,{},Effect::NoMutationAttempted);
        require(callback.call().toInt()==0 && old.handlers==10,"all terminal JS slots refuse before handlers");
        awaitRetired(f,a);auto b=f.begin(nonce(11),Kind::ReadOnly);require(bool(b),"fresh B after A bridge retirement");
        BridgeCalls fresh;qml_access::CreationBridge next(&owner,reader_retirement::binding(&f,b));installHandlers(next,f,b,fresh);
        const auto nextCallback=jsCallback(engine,next);
        require(QMetaObject::invokeMethod(&owner,[callback]{callback.call();},Qt::QueuedConnection),"queue actual stale JS callback");
        nested();require(old.handlers==10 && fresh.handlers==0,"late A callback cannot read getters or satisfy B");
        require(nextCallback.call().toInt()==15 && fresh.handlers==10 && old.handlers==10,"fresh B actual bridge independently admitted");
        next.disarm();require(nextCallback.call().toInt()==0 && fresh.handlers==10,"explicit disarm blocks scoped bridge handlers");
        f.requestRetire(b,{},Effect::NoMutationAttempted);awaitRetired(f,b);
    }
    {
        scenario();QObject owner;Fence f;auto token=f.begin(nonce(12),Kind::ReadOnly);int calls=0,cleanup=0;
        qml_access::CreationBridge bridge(&owner,reader_retirement::binding(&f,token));
        bridge.callback=[&]{++calls;require(f.snapshot(token).depth==1,"actual callback slot enters before callback body");
            f.requestRetire(token,[&]{++cleanup;},Effect::NoMutationAttempted);nested(5);
            require(cleanup==0 && f.snapshot(token).depth==1,"actual bridge callback nested loop delays cleanup");};
        bridge.observeCallback();require(calls==1 && cleanup==0 && f.snapshot(token).depth==0,"bridge callback returns before cleanup");
        bridge.observeCallback();require(calls==1,"late terminal callback never enters body");
        awaitRetired(f,token);require(cleanup==1,"actual callback retirement cleanup later");
    }
    {
        scenario();QObject owner;qml_access::CreationBridge legacy(&owner);int called=0;legacy.callback=[&]{++called;};
        legacy.observeCallback();require(legacy.callbackAllowed() && called==1,"old constructor keeps explicit unbound behavior");
        legacy.disarm();legacy.observeCallback();require(!legacy.callbackAllowed() && called==1,"old explicit disarm remains effective");
    }
    for(unsigned which=0;which<3;++which) {
        scenario();QObject owner;Fence f;auto token=f.begin(nonce(20+which),Kind::ReadOnly);int calls=0,cleanup=0;
        qml_access::CreationBridge bridge(&owner,reader_retirement::binding(&f,token));
        const auto retireAndAccept=[&] {
            ++calls;require(f.snapshot(token).depth==1,"Boolean callback body owns actual request Frame");
            require(f.requestRetire(token,[&]{++cleanup;},Effect::NoMutationAttempted),"Boolean callback body retires request");
            nested(4);require(cleanup==0 && f.snapshot(token).depth==1,"Boolean callback body cannot run cleanup inside nested loop");
            return true; // Deliberately misleading: a fresh admission check wins.
        };
        if(which<2)bridge.check=[&](bool mutation){require(mutation==(which==1),"Boolean admission keeps check argument");return retireAndAccept();};
        else bridge.libraryAdmission=retireAndAccept;
        const bool accepted=booleanAdmission(bridge,which);
        require(!accepted && calls==1 && cleanup==0 && f.snapshot(token).depth==0,"callback true cannot revive terminal admission");
        require(!booleanAdmission(bridge,which) && calls==1,"terminal Boolean callback never invoked again");
        awaitRetired(f,token);require(cleanup==1,"Boolean callback cleanup only after scope unwind");
    }
    for(unsigned which=0;which<3;++which) {
        scenario();QObject owner;Fence f;auto token=f.begin(nonce(30+which),Kind::ReadOnly);int calls=0,cleanup=0,destroyed=0;
        qml_access::CreationBridge bridge(&owner,reader_retirement::binding(&f,token));
        auto capture=std::make_shared<AdmissionCapture>(f,token,cleanup,destroyed);
        if(which<2)bridge.check=[&,capture](bool mutation){
            ++calls;require(mutation==(which==1),"copied Boolean callback preserves mutation argument");
            require(f.snapshot(token).depth==1 && destroyed==0,"callback copy retains final capture during body");
            bridge.check={};return true;
        };
        else bridge.libraryAdmission=[&,capture]{
            ++calls;require(f.snapshot(token).depth==1 && destroyed==0,"library callback copy retains final capture during body");
            bridge.libraryAdmission={};return true;
        };
        capture.reset();
        const bool accepted=booleanAdmission(bridge,which);
        require(!accepted && calls==1 && destroyed==1 && cleanup==0 && f.snapshot(token).depth==0,"last callback capture retirement invalidates returned true");
        require(!booleanAdmission(bridge,which) && calls==1 && destroyed==1,"destructor-triggered terminal cannot invoke callback again");
        awaitRetired(f,token);require(cleanup==1,"callback capture and Scope unwind before deferred cleanup");
    }
    QTextStream(stdout)<<"PASS request scope owned scenarios="<<scenarios<<" checks="<<checks
        <<"; actual Fence and Qt/JS CreationBridge, no native qualification\n";
    return 0;
}
