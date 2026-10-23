// Original owned model: no firmware function, private address or native lease.
#include "reader_worker_owner_quarantine.h"
#include <QEventLoop>
#include <QTimer>
#include <cstdio>
#include <stdexcept>
#include <vector>
using namespace reader_worker_quarantine;
static unsigned checks=0;
static void check(bool value,const char *message) {
    ++checks;if(!value) { std::fprintf(stderr,"FAIL %s\n",message);std::abort(); }
}
static void pump(int ms=8) {
    QEventLoop loop;QTimer::singleShot(ms,&loop,&QEventLoop::quit);loop.exec();
}
template<class F>static void until(F condition,int ms=1500) {
    QElapsedTimer timer;timer.start();while(!condition() && timer.elapsed()<ms)pump(2);
    check(condition(),"eventually");
}
class OwnedSource final : public QThread {
    Q_OBJECT
public:
    std::function<void()> body;
    qulonglong id=1;
    bool emitCompletion=true;
    void send(qulonglong value) { emit jobCompleted(value); }
    void run() override { if(body)body();if(emitCompletion)emit jobCompleted(id); }
    QMetaMethod completion() const { return metaObject()->method(metaObject()->indexOfSignal("jobCompleted(qulonglong)")); }
signals:
    void jobCompleted(qulonglong id);
};
struct Counts {
    std::atomic<unsigned> reads{0};
    int closes=0,destroys=0;
    bool closeResult=true,throwClose=false,throwDestroy=false;
    std::function<void()> onClose,onDestroy;
};
struct OwnedLease { std::shared_ptr<Counts> count; };
static bool closeLease(void *p) {
    auto *lease=static_cast<OwnedLease*>(p);auto count=lease->count;
    check(QThread::currentThread()==QCoreApplication::instance()->thread(),"GUI close");
    ++count->closes;if(count->onClose)count->onClose();
    if(count->throwClose)throw std::runtime_error("owned close");
    return count->closeResult;
}
static void destroyLease(void *p) {
    auto *lease=static_cast<OwnedLease*>(p);auto count=lease->count;
    check(QThread::currentThread()==QCoreApplication::instance()->thread(),"GUI destroy");
    ++count->destroys;if(count->onDestroy)count->onDestroy();
    if(count->throwDestroy)throw std::runtime_error("owned destroy");
    delete lease;
}
static void readWork(void *worker,void *context) noexcept {
    // Deliberately NO QObject or fake owner dereference: only an opaque scalar
    // pointer comparison and owned atomics model the future native read body.
    if(worker)static_cast<Counts*>(context)->reads.fetch_add(1);
}
static Token install(Pool &pool,OwnedSource &source,std::shared_ptr<Counts> count,int ms=1500) {
    return pool.install(&source,source.completion(),{new OwnedLease{count},&source,closeLease,destroyLease},{readWork,count},ms);
}
static void offGui(const std::function<void()> &cb) { std::thread thread(cb);thread.join(); }
static void positive() {
    auto *storage=ResidentStorage::create();Pool pool(storage);OwnedSource source;
    auto count=std::make_shared<Counts>();auto token=install(pool,source,count);
    check(bool(token),"installed");int released=0;bool inside=false;
    QObject::connect(&pool,&Pool::released,&pool,[&](quint64 generation) {
        check(generation==token.generation(),"generation");check(!inside,"no finish on submit stack");
        check(count->closes==1 && count->destroys==1,"signal follows cleanup");
        auto rejected=install(pool,source,std::make_shared<Counts>());
        check(!rejected,"signal reentry blocks install");
        bool nestedRan=false;
        QMetaObject::invokeMethod(&pool,[&] {
            const auto result=pool.snapshot(token);
            check(result.depth>0 && !result.settled,"nested released listener has not settled");nestedRan=true;
        },Qt::QueuedConnection);
        pump();check(nestedRan,"queued consumer can run in nested notification loop");++released;
    });
    bool settled=false;
    QObject::connect(&pool,&Pool::settled,&pool,[&](quint64 generation) {
        const auto result=pool.snapshot(token);
        check(generation==token.generation() && result.settled && result.depth==0,"settled notification is zero-depth");
        check(released==1,"released listener has unwound");
        auto *extra=new OwnedLease{count};
        check(!pool.install(&source,source.completion(),{extra,&source,closeLease,destroyLease},{},100),"settled listener blocks admission");delete extra;
        pump();settled=true;
    });
    std::function<void()> retained;
    {
        auto held=pool.hold(token);check(held && held->valid(),"heap outer Frame");inside=true;
        check(pool.submit(token,[&](const std::function<void()> &cb) {
            retained=cb;source.send(999);pump(); // Foreign early job cannot qualify.
            source.id=qulonglong(1)<<60;source.body=cb;source.start();source.wait();pump();
            return source.id;
        }),"submit returned");
        check(pool.snapshot(token).phase==Phase::Ready,"early completion buffered");pump(12);
        check(count->closes==0 && released==0,"nested loop cannot release under outer Frame");inside=false;
    }
    until([&]{return settled;});
    const auto result=pool.snapshot(token);
    check(result.phase==Phase::Released && result.closed && result.destroyed,"released success");
    check(result.jobId==QString::number(qulonglong(1)<<60),"lossless jobID");
    check(result.entered==1 && result.returning==1 && result.offGui==1 && result.completions==1,"positive facts");
    check(count->reads.load()==1,"one read");offGui(retained);check(count->reads.load()==1,"late copy cannot use raw worker");
    check(pool.snapshot(token).duplicates==1,"late duplicate fact retained");
    auto secondCount=std::make_shared<Counts>();auto second=install(pool,source,secondCount);
    check(bool(second) && second.generation()>token.generation(),"successful slot reclaimed");
    pool.cancel(token);check(pool.snapshot(second).phase==Phase::Installed,"stale token cannot cancel B");
    pool.cancel(second);pump();check(secondCount->closes==0,"canceled B pinned");
}
static void twoSuccesses() {
    struct KeepRelay final : QObject {
        bool eventFilter(QObject*,QEvent *event) override { return event->type()==QEvent::DeferredDelete; }
    } filter;
    auto *storage=ResidentStorage::create();Pool pool(storage);OwnedSource source;
    auto firstCount=std::make_shared<Counts>();auto first=install(pool,source,firstCount);
    auto *oldRelay=pool.findChild<detail::Relay*>();check(oldRelay,"per-generation receiver exists");
    oldRelay->installEventFilter(&filter); // Owned model preserves a stale queued receiver.
    pool.submit(first,[&](const std::function<void()> &cb) { offGui(cb);source.send(1);pump();return 1; });
    until([&]{return pool.snapshot(first).settled;});
    auto secondCount=std::make_shared<Counts>();auto second=install(pool,source,secondCount);
    check(bool(second),"second positive installed");
    pool.submit(second,[&](const std::function<void()> &cb) {
        QMetaObject::invokeMethod(oldRelay,"completed",Qt::QueuedConnection,Q_ARG(qulonglong,2));pump();
        check(pool.snapshot(second).completions==0,"stale receiver cannot attribute to B");
        offGui(cb);source.send(2);pump();return 2;
    });
    until([&]{return pool.snapshot(second).settled;});
    check(secondCount->closes==1 && secondCount->destroys==1 && secondCount->reads==1,"second positive released exactly once");
    check(pool.snapshot(second).completions==1,"old connection cannot duplicate completion");
}
static void negative(const char *mode) {
    auto *storage=ResidentStorage::create();auto *pool=new Pool(storage);auto *source=new OwnedSource;
    auto count=std::make_shared<Counts>();const int budget=QString::fromLatin1(mode)=="deadline"?15:1000;
    auto token=install(*pool,*source,count,budget);check(bool(token),"negative install");
    std::function<void()> retained;const QString name=QString::fromLatin1(mode);
    bool returned=pool->submit(token,[&](const std::function<void()> &cb)->qulonglong {
        retained=cb;
        if(name=="throw") { offGui(cb);throw std::runtime_error("queued unknown"); }
        if(name=="zero") { offGui(cb);source->send(0);pump();return 0; }
        if(name=="overflow") { for(int i=0;i<9;++i)source->send(100+i);pump();return 7; }
        if(name=="cancel-before") { pool->cancel(token);offGui(cb);source->send(7);pump();return 7; }
        if(name=="inline")cb();
        else if(name!="no-callback" && name!="deadline" && name!="source-deleted")offGui(cb);
        if(name=="duplicate-callback")offGui(cb);
        if(name=="reenter")check(!pool->submit(token,{}),"no second submit");
        if(name=="source-deleted") { delete source;source=nullptr;pump(); }
        else if(name!="no-completion" && name!="deadline") {
            source->send(7);if(name=="duplicate-completion")source->send(7);pump();
        }
        return 7;
    });
    if(name=="reenter") { // Attempted reentry refuses but cannot enqueue again.
        until([&]{return pool->snapshot(token).phase==Phase::Released;});
        check(count->closes==1 && count->reads==1,"refused reentry no second read");
    } else {
        if(name=="no-completion")pool->cancel(token,QStringLiteral("missing-completion"));
        if(name=="deadline")pump(30);else pump();
        check(pool->snapshot(token).phase==Phase::Quarantined,mode);
        check(count->closes==0 && count->destroys==0,"negative never releases");
        if(name=="throw" || name=="zero")check(!returned,"ambiguous mechanics false");
        if(name=="cancel-before")check(count->reads==0,"cancel checked before raw use");
        if(name=="duplicate-callback")check(count->reads==1,"duplicate claimed before raw use");
        if(name=="inline")check(count->reads==0,"GUI callback cannot read");
        offGui(retained);check(count->closes==0,"late copy does not release lease");
        if(source) {
            auto *extra=new OwnedLease{count};
            auto refused=pool->install(source,source->completion(),{extra,source,closeLease,destroyLease},{},100);
            check(!refused,"sticky capacity one");delete extra;
        }
    }
    delete pool;if(source) { source->wait();delete source; }
}
static void cancelDuring() {
    auto *storage=ResidentStorage::create();Pool pool(storage);OwnedSource source;
    auto count=std::make_shared<Counts>();
    struct Gate { std::atomic<bool> entered{false},go{false};std::atomic<unsigned> reads{0}; };
    auto gate=std::make_shared<Gate>();
    const Work work{[](void*,void *p) noexcept {
        auto *g=static_cast<Gate*>(p);g->entered=true;while(!g->go.load())std::this_thread::yield();++g->reads;
    },gate};
    auto token=pool.install(&source,source.completion(),{new OwnedLease{count},&source,closeLease,destroyLease},work,1000);
    pool.submit(token,[&](const std::function<void()> &cb) { source.id=17;source.body=cb;source.start();return 17; });
    until([&]{return gate->entered.load();});pool.cancel(token);check(count->closes==0,"running read pinned");
    gate->go=true;source.wait();pump();
    check(gate->reads==1 && pool.snapshot(token).phase==Phase::Quarantined,"cancel race may complete work but remains pinned");
    check(count->closes==0,"matching completion cannot undo cancellation");
}
static void finalization(const QString &mode) {
    auto *storage=ResidentStorage::create();auto *pool=new Pool(storage);OwnedSource source;
    auto count=std::make_shared<Counts>();auto token=install(*pool,source,count);bool closeStack=false,finish=false;
    const QPointer<Pool> weak(pool);
    QObject::connect(pool,&Pool::released,pool,[&] { check(!closeStack,"release finalizer unwound");finish=true; });
    count->closeResult=mode!="false";count->throwClose=mode=="throw-close";count->throwDestroy=mode=="throw-destroy";
    count->onClose=[&] {
        closeStack=true;
        if(mode=="delete-close") { delete pool;pool=nullptr; }
        else if(mode=="cancel-close")pool->cancel(token);
        pump();check(!finish,"nested close eventloop cannot publish");closeStack=false;
    };
    count->onDestroy=[&] {
        if(mode=="delete-destroy") { delete pool;pool=nullptr; }
        pump();check(!finish,"nested destroy eventloop cannot publish");
    };
    pool->submit(token,[&](const std::function<void()> &cb) { offGui(cb);source.send(91);pump();return 91; });
    pump(30);
    if(mode=="good")check(finish && count->closes==1 && count->destroys==1,"normal finalization");
    else {
        check(!finish && count->closes==1,"failed finalization no success/no retry");
        if(weak)check(weak->snapshot(token).phase==Phase::Quarantined,"finalization sticky");
        check(count->destroys==((mode=="throw-destroy" || mode=="delete-destroy")?1:0),"destroy only after known close");
        Pool replacement(storage);
        auto *extra=new OwnedLease{count};
        check(!replacement.install(&source,source.completion(),{extra,&source,closeLease,destroyLease},{},100),"no reclaim after facade loss/fault");delete extra;
    }
    if(pool)delete pool;
}
static void ownerDestroyed() {
    auto *storage=ResidentStorage::create();auto *pool=new Pool(storage);OwnedSource source;
    auto count=std::make_shared<Counts>();auto token=install(*pool,source,count);std::function<void()> late;
    pool->submit(token,[&](const std::function<void()> &cb) { late=cb;return 6; });
    delete pool;offGui(late);source.send(6);pump();
    check(count->reads==0 && count->closes==0 && count->destroys==0,"facade deletion leaves lease and canceled plain state");
    Pool next(storage);auto *extra=new OwnedLease{count};
    check(!next.install(&source,source.completion(),{extra,&source,closeLease,destroyLease},{},100),"new facade cannot reset sticky storage");delete extra;
}
static void submitCapture(bool throwing) {
    auto *storage=ResidentStorage::create();Pool pool(storage);OwnedSource source;
    auto count=std::make_shared<Counts>();auto token=install(pool,source,count);bool inside=true,finished=false,destroyed=false;
    struct Capture { std::function<void()> release;~Capture(){release();} };
    auto capture=std::shared_ptr<Capture>(new Capture{[&]{destroyed=true;pump();check(!finished,"Submit capture unwinds under frame");}});
    QObject::connect(&pool,&Pool::released,&pool,[&]{check(!inside,"no release inside Submit destructor");finished=true;});
    QObject::connect(&pool,&Pool::quarantined,&pool,[&]{check(!inside,"no quarantine signal inside Submit destructor");finished=true;});
    Pool::Submit submit=[&,capture](const std::function<void()> &cb)->qulonglong {
        offGui(cb);source.send(1);pump();if(throwing)throw std::runtime_error("owned unwind");return 1;
    };
    capture.reset();pool.submit(token,std::move(submit));inside=false;
    check(destroyed,"last Submit capture destroyed before return");until([&]{return finished;});
    check(count->closes==(throwing?0:1),"exceptional Submit cleanup remains quarantined");
}
static void deletedOnSubmit() {
    auto *storage=ResidentStorage::create();auto *pool=new Pool(storage);OwnedSource source;
    auto count=std::make_shared<Counts>();auto token=install(*pool,source,count);std::function<void()> late;
    const bool returned=pool->submit(token,[&](const std::function<void()> &cb) {
        late=cb;delete pool;pool=nullptr;offGui(cb);return 13;
    });
    check(returned && !pool,"submit survives facade deletion without this access");
    offGui(late);check(count->reads==0 && count->closes==0,"deleted submission keeps raw receiver pinned");
}
static void noInstallEvents() {
    struct Counter final : QObject {
        bool watching=false;unsigned all=0,children=0;
        bool eventFilter(QObject*,QEvent *event) override {
            if(watching) { ++all;if(event->type()==QEvent::ChildAdded || event->type()==QEvent::ChildRemoved)++children; }
            return false;
        }
    } counter;
    QCoreApplication::instance()->installEventFilter(&counter);
    auto *storage=ResidentStorage::create();Pool pool(storage);OwnedSource source;
    auto count=std::make_shared<Counts>();
    counter.watching=true;
    auto token=install(pool,source,count);
    check(bool(token),"event-free install transfers lease");
    check(pool.submit(token,[](const std::function<void()> &){return 57;}),"event-free deferred submit");
    counter.watching=false;
    check(counter.children==0 && counter.all==0,"install/submit create no QObject or synchronous application event");
    pool.cancel(token);pump();check(count->closes==0,"event-free uncertain request remains pinned");
    QCoreApplication::instance()->removeEventFilter(&counter);
}
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);
    positive();twoSuccesses();noInstallEvents();
    for(const char *mode: {"throw","zero","overflow","cancel-before","inline","no-callback","deadline","source-deleted","no-completion","duplicate-callback","duplicate-completion","reenter"})negative(mode);
    cancelDuring();ownerDestroyed();submitCapture(false);submitCapture(true);deletedOnSubmit();
    for(const auto &mode:{"good","false","throw-close","throw-destroy","delete-close","delete-destroy","cancel-close"})finalization(QString::fromLatin1(mode));
    std::printf("PASS %u owned quarantine checks; no native qualification\n",checks);return 0;
}
#include "reader_worker_owner_quarantine_fixture.moc"
