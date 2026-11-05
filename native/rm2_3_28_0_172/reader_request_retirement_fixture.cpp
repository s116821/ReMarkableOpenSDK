// Original owned GUI/event-loop model. No stock Reader/native effect qualification.
#include "reader_request_retirement.h"
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileSystemWatcher>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTimer>
#include <stdexcept>
#include <thread>

using namespace reader_retirement;
namespace {
int checks=0;
void require(bool yes,const char *name) { if(!yes)qFatal("FAIL %s",name);++checks; }
QByteArray nonce(unsigned n) { return QByteArray::number(n,16).rightJustified(32,'0'); }
void nested(int ms=2) { QEventLoop loop;QTimer::singleShot(ms,&loop,&QEventLoop::quit);loop.exec(); }
void retired(Fence &f,const Token &t) {
    QElapsedTimer timer;timer.start();
    while(f.snapshot(t).phase!=Phase::Retired && f.snapshot(t).phase!=Phase::Failed && timer.elapsed()<200)nested(1);
    require(f.snapshot(t).phase==Phase::Retired,"retired in bounded owned loop");
}
struct MetaCalls : QObject {
    int count=0;
    bool eventFilter(QObject *,QEvent *event) override { if(event->type()==QEvent::MetaCall)++count;return false; }
};
struct CapturePump {
    std::function<void()> check;
    ~CapturePump() { if(check)check();nested();if(check)check(); }
};
struct Resources {
    static int alive;
    QFile file;
    QFileSystemWatcher watcher;
    explicit Resources(const QString &path):file(path) { ++alive;require(file.open(QIODevice::ReadWrite),"owned file open");require(watcher.addPath(path),"owned watcher add"); }
    void close() { const auto names=watcher.files();if(!names.isEmpty())require(watcher.removePaths(names).isEmpty(),"watcher remove");file.close(); }
    ~Resources() { close();--alive; }
};
int Resources::alive=0;
}
// Actual owned Qt/JS bridge; models token admission, not a native Task callback.
class OwnedJsBridge final : public QObject {
    Q_OBJECT
public:
    QPointer<Fence> fence;
    Token token;
    int calls=0,getters=0;
    OwnedJsBridge(Fence *f,Token t):fence(f),token(std::move(t)) {}
    Q_INVOKABLE void deliver() {
        ++calls;
        if(!fence)return;
        const auto frame=fence->enter(token);
        if(!frame || !fence || !fence->accepts(token))return;
        ++getters; // Explicit synthetic getter counter after admission.
    }
};
class OwnedWorkerSignal final : public QObject {
    Q_OBJECT
public:
    void deliver(quint64 generation) { emit completed(generation); }
signals:
    void completed(quint64 generation);
};
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);
    QQmlEngine engine;QQuickWindow window;
    QPointer<QQmlEngine> sameEngine=&engine;QPointer<QQuickWindow> sameWindow=&window;
    {
        Fence f;MetaCalls calls;f.installEventFilter(&calls);auto a=f.begin(nonce(1),Kind::ReadOnly);require(bool(a),"first nonce");
        require(f.snapshot(a).generation==a.generation() && f.snapshot(a).nonce==a.nonce(),"stable snapshot identity");
        require(!f.handoffReady(a),"active request cannot hand off");
        int cleanup=0,finished=0;
        const auto notification=QObject::connect(&f,&Fence::retired,&f,[&](quint64 g,QByteArray n){require(g==a.generation() && n==a.nonce(),"retired signal identity");++finished;require(!f.begin(nonce(2),Kind::ReadOnly),"no begin on retired notification stack");require(!f.handoffReady(a),"retired notification cannot hand off");nested();require(!f.handoffReady(a),"nested retired notification cannot hand off");});
        require(f.requestRetire(a,[&]{++cleanup;},Effect::NoMutationAttempted),"terminal request");
        require(!f.accepts(a) && f.snapshot(a).phase==Phase::Terminal,"terminal invalidates callback permission");
        {
            const auto frame=f.enter(a);require(frame.valid(),"terminal stack tracked");
            const int before=calls.count;nested(5);require(calls.count==before+1,"one depth-blocked check no nested reschedule spin");
            require(cleanup==0 && finished==0 && !f.begin(nonce(2),Kind::ReadOnly),"nested frame blocks cleanup and B");
        }
        retired(f,a);require(cleanup==1 && finished==1,"cleanup and retired once");require(f.handoffReady(a),"later retired request can hand off");QObject::disconnect(notification);
        auto b=f.begin(nonce(2),Kind::ReadOnly);require(bool(b) && b.generation()>a.generation() && !f.accepts(a) && f.accepts(b),"fresh B isolated");
        require(!f.begin(nonce(1),Kind::ReadOnly),"spent nonce refused");
        require(!f.handoffReady(a),"superseded token cannot hand off");
        f.requestRetire(b,{},Effect::NoMutationAttempted);retired(f,b);
    }
    for(unsigned stage=0;stage<3;++stage) {
        Fence f;auto a=f.begin(nonce(10+stage),Kind::ReadOnly);bool scopeReturned=false;int cleanup=0;
        {
            const auto frame=f.enter(a); // Sample/finish/publish modeled scope FIRST.
            require(bool(frame),"owned GUI frame");
            CapturePump local;local.check=[&]{require(f.snapshot(a).phase!=Phase::Retired && !scopeReturned,"reentrant local destructor before fence exit");};
            f.requestRetire(a,[&]{require(scopeReturned,"cleanup after original scope returns");++cleanup;},Effect::NoMutationAttempted);
            nested();require(!cleanup,"sample finish publication nested loop cannot retire");
        }
        scopeReturned=true;retired(f,a);require(cleanup==1,"owned scope cleanup once");
    }
    {
        Fence f;auto a=f.begin(nonce(20),Kind::ReadOnly);int cleanup=0,destructors=0;
        auto capture=std::make_shared<CapturePump>();capture->check=[&]{++destructors;require(f.snapshot(a).phase==Phase::Retiring && f.snapshot(a).depth>0,"cleanup capture destructor remains fenced");require(!f.begin(nonce(21),Kind::ReadOnly),"cleanup reentry cannot admit B");};
        f.requestRetire(a,[&,capture]{++cleanup;require(f.snapshot(a).phase==Phase::Retiring,"retiring during cleanup");nested();},Effect::NoMutationAttempted);capture.reset();
        retired(f,a);require(cleanup==1 && destructors==2,"last cleanup capture destroyed inside fence");
    }
    {
        Fence f;auto a=f.begin(nonce(30),Kind::ReadOnly);OwnedJsBridge bridge(&f,a);QQmlEngine::setObjectOwnership(&bridge,QQmlEngine::CppOwnership);
        const auto factory=engine.evaluate(QStringLiteral("(function(b){return function(){try{b.deliver();}catch(e){}}})"));
        const auto callback=factory.call({engine.newQObject(&bridge)});require(callback.isCallable(),"owned JS callback");callback.call();require(bridge.getters==1,"active JS accepted");
        OwnedWorkerSignal source;int oldWorkerGetters=0,newWorkerGetters=0;
        QObject::connect(&source,&OwnedWorkerSignal::completed,&f,[&f,a,&oldWorkerGetters](quint64 g){const auto frame=f.enter(a);if(frame && f.accepts(a) && g==a.generation())++oldWorkerGetters;},Qt::QueuedConnection);
        f.requestRetire(a,{},Effect::NoMutationAttempted);callback.call();require(bridge.getters==1,"terminal JS blocked before getter");retired(f,a);
        auto b=f.begin(nonce(31),Kind::ReadOnly);require(bool(b),"B after A retired");
        QObject::connect(&source,&OwnedWorkerSignal::completed,&f,[&f,b,&newWorkerGetters](quint64 g){const auto frame=f.enter(b);if(frame && f.accepts(b) && g==b.generation())++newWorkerGetters;},Qt::QueuedConnection);
        // Real cross-thread Qt queued delivery, but synthetic generation/event
        // correlation only: no native worker command, job ABI or document effect.
        std::thread oldDelivery([&]{source.deliver(a.generation());});oldDelivery.join();callback.call();nested();
        require(oldWorkerGetters==0 && newWorkerGetters==0 && bridge.getters==1,"old worker and JS cannot satisfy B");
        std::thread newDelivery([&]{source.deliver(b.generation());});newDelivery.join();nested();require(newWorkerGetters==1 && oldWorkerGetters==0,"fresh worker token accepted only by B");
        f.requestRetire(b,{},Effect::NoMutationAttempted);retired(f,b);
    }
    {
        Fence f;auto a=f.begin(nonce(40),Kind::Mutation);
        require(!f.acknowledgeReconciled(a),"no premature reconciliation");
        f.requestRetire(a,{},Effect::ReconciliationRequired);retired(f,a);
        require(!f.begin(nonce(41),Kind::Mutation),"unreconciled effect blocks new mutation");
        auto b=f.begin(nonce(41),Kind::ReadOnly);require(bool(b),"readonly reconciliation allowed");
        require(!f.acknowledgeReconciled(b),"wrong operation cannot clear latch");
        require(f.acknowledgeReconciled(a),"trusted matching reconciliation handoff");
        f.requestRetire(b,{},Effect::NoMutationAttempted);retired(f,b);
        auto c=f.begin(nonce(42),Kind::Mutation);require(bool(c),"mutation after explicit reconciliation and B retirement");f.requestRetire(c,{},Effect::NoMutationAttempted);retired(f,c);
    }
    {
        Fence f;auto a=f.begin(nonce(50),Kind::Mutation);int failed=0;
        QObject::connect(&f,&Fence::retirementFailed,&f,[&]{++failed;require(!f.begin(nonce(51),Kind::ReadOnly),"sticky failure inside notification");});
        f.requestRetire(a,[]{nested();throw std::runtime_error("owned partial cleanup");},Effect::ReconciliationRequired);nested(5);
        require(f.snapshot(a).phase==Phase::Failed && f.snapshot(a).refusal=="cleanup-exception" && failed==1,"throwing partial cleanup sticky failed");
        require(!f.begin(nonce(51),Kind::ReadOnly) && !f.acknowledgeReconciled(a),"cleanup failure never reopens admission");
    }
    {
        auto *f=new Fence;auto a=f->begin(nonce(60),Kind::ReadOnly);QPointer<Fence> weak(f);
        { const auto frame=f->enter(a);delete f;require(!weak,"owner destroyed while frame holds plain state"); }
        nested();
    }
    {
        auto *f=new Fence;auto a=f->begin(nonce(61),Kind::ReadOnly);QPointer<Fence> weak(f);
        f->requestRetire(a,[&]{delete f;},Effect::NoMutationAttempted);nested();require(!weak,"cleanup can delete owner without dangling frame");
    }
    {
        Fence f;auto a=f.begin(nonce(70),Kind::ReadOnly);bool refused=false;
        std::thread foreign([&]{
            const auto frame=f.enter(a);
            refused=!frame && !f.accepts(a) && !f.acknowledgeReconciled(a) &&
                !f.requestRetire(a,{},Effect::NoMutationAttempted) && !f.begin(nonce(71),Kind::ReadOnly) &&
                f.snapshot(a).phase==Phase::Invalid;
        });foreign.join();
        require(refused && f.accepts(a) && f.snapshot(a).depth==0,"foreign thread refuses before token state resolution");
        f.requestRetire(a,{},Effect::NoMutationAttempted);retired(f,a);
    }
    {
        QTemporaryDir temp;require(temp.isValid(),"owned resource directory");Fence f;
        const int baseline=QDir(QStringLiteral("/proc/self/fd")).entryList(QDir::AllEntries|QDir::NoDotAndDotDot).size();
        for(unsigned i=0;i<32;++i) {
            auto t=f.begin(nonce(100+i),Kind::ReadOnly);auto resources=std::make_shared<Resources>(temp.filePath(QString::number(i)));
            f.requestRetire(t,[resources]{resources->close();},Effect::NoMutationAttempted);resources.reset();retired(f,t);
            require(Resources::alive==0,"request resources destroyed before retired");
        }
        const int after=QDir(QStringLiteral("/proc/self/fd")).entryList(QDir::AllEntries|QDir::NoDotAndDotDot).size();require(after==baseline,"no request FD/watch growth");
    }
    {
        Fence f;require(!f.begin("bad",Kind::ReadOnly),"nonce grammar");
        for(unsigned i=0;i<unsigned(Fence::MaxRequests);++i) {auto t=f.begin(nonce(1000+i),Kind::ReadOnly);require(bool(t),"bounded fresh history");f.requestRetire(t,{},Effect::NoMutationAttempted);retired(f,t);}
        require(!f.begin(nonce(2000),Kind::ReadOnly),"research history exhaustion fails closed");
    }
    require(sameEngine==&engine && sameWindow==&window,"owned engine/window remain alive across closes");
    QTextStream(stdout)<<"PASS request retirement owned checks="<<checks<<"; Qt/JS and queued delivery model, no native qualification\n";
    return 0;
}
#include "reader_request_retirement_fixture.moc"
