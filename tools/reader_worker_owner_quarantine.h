#pragma once
// Original bounded research ownership fence. No native cancel, drain or unload.
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QMetaMethod>
#include <QPointer>
#include <QThread>
#include <QTimer>
#include <array>
#include <atomic>
#include <functional>
#include <memory>
#include <thread>

namespace reader_worker_quarantine {
enum class Phase { Invalid, Installed, Submitted, Ready, Finalizing, Finalized, Released, Quarantined };
struct Lease {
    void *object=nullptr,*worker=nullptr;
    bool (*closeOnGui)(void*)=nullptr;
    void (*destroyClosedOnGui)(void*)=nullptr;
};
struct Work {
    void (*run)(void *rawWorker,void *plainContext) noexcept=nullptr;
    // Owned plain/value data only. Its destructor must not use native owners,
    // raw native pointers, QObject, GUI, filesystem or request observer state.
    std::shared_ptr<void> plainContext;
};
class Pool;
namespace detail {
struct Plain {
    explicit Plain(Work value):work(std::move(value)),guiThread(std::this_thread::get_id()) {}
    Work work;
    const std::thread::id guiThread;
    std::atomic<bool> claimed{false},canceled{false};
    std::atomic<unsigned> entered{0},returning{0},offGui{0},duplicates{0},skipped{0};
};
struct Slot;
class Relay;
}
// This storage has no destructor and must remain resident through process exit.
// Never put a native lease in a static/atexit cleanup path. Capacity is ONE;
// successful histories retain small records, capped at256 for this experiment.
class ResidentStorage final {
    friend class Pool;
    friend struct detail::Slot;
    ResidentStorage()=default;
    ~ResidentStorage()=default;
    detail::Slot *current=nullptr;
    quint64 generation=0;
    bool facadePresent=false,notifying=false;
public:
    ResidentStorage(const ResidentStorage&)=delete;
    ResidentStorage&operator=(const ResidentStorage&)=delete;
    static ResidentStorage *create() { return new ResidentStorage; }
};
namespace detail {
struct Slot {
    ResidentStorage *storage=nullptr;
    QPointer<Pool> facade;
    QPointer<QThread> source;
    QMetaObject::Connection completion,destroyed;
    QPointer<Relay> relay;
    Lease lease;
    std::shared_ptr<Plain> plain;
    quint64 generation=0,jobId=0;
    unsigned depth=0,completionCount=0,earlyCount=0;
    std::array<qulonglong,8> early{};
    QElapsedTimer elapsed;
    int budget=0;
    Phase phase=Phase::Installed;
    bool consumed=false,submitting=false,queued=false,notified=false,settled=false;
    bool returned=false,closeAttempted=false,closeSucceeded=false,destroyAttempted=false,destroySucceeded=false;
    QString refusal;
};
}
class Token {
    friend class Pool;
    detail::Slot *slot_=nullptr;
    quint64 generation_=0;
public:
    quint64 generation() const { return generation_; }
    explicit operator bool() const { return slot_ && generation_; }
};
struct Snapshot {
    Phase phase=Phase::Invalid;
    quint64 generation=0;
    QString jobId,refusal;
    unsigned depth=0,entered=0,returning=0,offGui=0,duplicates=0,skipped=0,completions=0;
    bool attempted=false,submissionReturned=false,closeAttempted=false,closed=false,destroyAttempted=false,destroyed=false,settled=false;
};
// FIRST local / LAST destruction, including reentrant capture and parameter
// destruction. hold() supports a surrounding Sample whose Token is installed
// later: declare its unique_ptr FIRST, assign immediately after install.
class Frame final {
    friend class Pool;
    friend class detail::Relay;
    explicit Frame(detail::Slot *slot):slot_(slot) { if(slot_)++slot_->depth; }
    detail::Slot *slot_=nullptr;
public:
    Frame(const Frame&)=delete;Frame&operator=(const Frame&)=delete;
    Frame(Frame&&)=delete;Frame&operator=(Frame&&)=delete;
    ~Frame();
    bool valid() const { return slot_!=nullptr; }
    explicit operator bool() const { return valid(); }
};
namespace detail {
// Per-slot receiver binds queued deliveries to the ORIGINAL generation. A
// stale queued event must never be interpreted using a later slot's job ID.
class Relay final : public QObject {
    Q_OBJECT
public:
    Relay(Slot *slot,QObject *parent):QObject(parent),slot_(slot) {}
    bool bind(Slot *slot) { if(slot_ || !slot)return false;slot_=slot;return true; }
public slots:
    void completed(qulonglong id);
private:
    Slot *slot_;
};
}
class Pool final : public QObject {
    Q_OBJECT
public:
    using Submit=std::function<qulonglong(const std::function<void()>&)>;
    explicit Pool(ResidentStorage *storage,QObject *parent=nullptr):QObject(parent),storage_(storage) {
        if(gui() && storage_ && !storage_->facadePresent) {
            storage_->facadePresent=true;bound_=true;
            const QPointer<Pool> weak(this);
            timer_.setParent(this);if(!weak)return;
            timer_.setSingleShot(true);
            connect(&timer_,&QTimer::timeout,this,[this] {
                auto *s=storage_->current;
                if(s && s->generation==timerGeneration_ && !s->settled)cancelToken(s,QStringLiteral("deadline"));
            });
            auto *first=new detail::Relay(nullptr,this); // BEFORE guarded Sample.
            if(!weak)return;
            reserve_=first;
        }
    }
    ~Pool() override {
        if(!bound_)return;
        auto *s=storage_->current;
        if(s && s->facade==this) {
            s->facade=nullptr;
            if(s->phase!=Phase::Released || !s->settled)quarantine(s,QStringLiteral("owner-destroyed"));
            QObject::disconnect(s->completion);QObject::disconnect(s->destroyed);
        }
        storage_->facadePresent=false;
        // No lease/context finalizer here, including application shutdown.
    }
    // A valid returned Token ALWAYS means lease ownership transferred, even
    // if connection reentry immediately quarantined it. Invalid => caller owns.
    Token install(QObject *source,QMetaMethod completion,Lease lease,Work work,int remainingMs) {
        Token token;
        if(!gui() || !bound_ || storage_->current || storage_->notifying ||
           storage_->generation>=256 || !reserve_ || !lease.object || !lease.worker ||
           !lease.closeOnGui || !lease.destroyClosedOnGui || remainingMs<1 || remainingMs>30000)return token;
        const QPointer<Pool> weak(this);
        const QPointer<QObject> sourceGuard(source);
        auto *threadSource=qobject_cast<QThread*>(source);
        if(!weak || !sourceGuard || !threadSource || source!=lease.worker || source->thread()!=QThread::currentThread() ||
           !validCompletion(source,completion))return token;
        auto *s=new detail::Slot; // Intentionally resident; no automatic native cleanup.
        s->storage=storage_;s->facade=this;s->source=threadSource;s->lease=lease;
        s->plain=std::make_shared<detail::Plain>(std::move(work));
        s->generation=++storage_->generation;s->budget=remainingMs;s->elapsed.start();
        storage_->current=s;token.slot_=s;token.generation_=s->generation;
        const Frame frame(s);
        auto *relay=reserve_.data();reserve_=nullptr;s->relay=relay;
        if(!relay->bind(s)) { quarantine(s,QStringLiteral("relay-unavailable"));return token; }
        const int index=relay->metaObject()->indexOfSlot("completed(qulonglong)");
        const auto connection=QObject::connect(source,completion,relay,relay->metaObject()->method(index),Qt::QueuedConnection);
        s->completion=connection;
        if(!weak || !s->source || s->phase==Phase::Quarantined) { quarantine(s,QStringLiteral("source-or-owner-destroyed"));return token; }
        s->destroyed=QObject::connect(source,&QObject::destroyed,this,[weak,s] {
            if(weak)weak->cancelToken(s,QStringLiteral("source-destroyed"));
        },Qt::QueuedConnection);
        if(!weak || !s->source || !s->completion || !s->destroyed) { quarantine(s,QStringLiteral("completion-connect"));return token; }
        if(expired(s))quarantine(s,QStringLiteral("deadline"));
        else { timerGeneration_=s->generation;timer_.start(qMax(1,remainingMs-int(s->elapsed.elapsed()))); }
        return token;
    }
    Frame enter(const Token &token) {
        if(!gui())return Frame(nullptr);
        auto *s=resolve(token);return Frame(s && s->phase!=Phase::Released?s:nullptr);
    }
    std::unique_ptr<Frame> hold(const Token &token) {
        if(!gui())return {};
        auto *s=resolve(token);
        return s && s->phase!=Phase::Released?std::unique_ptr<Frame>(new Frame(s)):nullptr;
    }
    bool submit(const Token &token,Submit submitter) {
        if(!gui()) { submitter={};return false; }
        auto *s=resolve(token);
        const Frame frame(s); // FIRST live frame; captures reset inside it.
        if(!s || s->phase!=Phase::Installed || s->consumed) { submitter={};return false; }
        const QPointer<Pool> weak(this);
        s->consumed=true;s->submitting=true;s->phase=Phase::Submitted;
        if(!submitter || !s->source || expired(s)) {
            submitter={};s->submitting=false;quarantine(s,QStringLiteral("submission-refused"));return false;
        }
        const auto plain=s->plain;
        const auto callback=std::make_shared<std::function<void()>>([plain,raw=s->lease.worker] {
            // Claim BEFORE any raw pointer access. Copied/late invocations only
            // touch resident atomics, even after successful lease destruction.
            if(plain->claimed.exchange(true,std::memory_order_acq_rel)) {
                plain->duplicates.fetch_add(1,std::memory_order_relaxed);return;
            }
            plain->entered.fetch_add(1,std::memory_order_relaxed);
            const bool off=std::this_thread::get_id()!=plain->guiThread;
            if(off)plain->offGui.fetch_add(1,std::memory_order_relaxed);
            if(off && !plain->canceled.load(std::memory_order_acquire)) {
                if(plain->work.run)plain->work.run(raw,plain->work.plainContext.get());
            } else plain->skipped.fetch_add(1,std::memory_order_relaxed);
            plain->returning.fetch_add(1,std::memory_order_release);
        });
        qulonglong id=0;bool threw=false;
        try { id=submitter(*callback); } catch(...) { threw=true; }
        submitter={}; // Destructor reentry remains under submitting_ and frame.
        s->submitting=false;
        if(threw)quarantine(s,QStringLiteral("submission-exception"));
        else {
            s->returned=true;s->jobId=id;
            if(!id)quarantine(s,QStringLiteral("submission-id-invalid"));
            else if(!s->source || !weak)quarantine(s,QStringLiteral("source-or-owner-destroyed"));
            else if(expired(s))quarantine(s,QStringLiteral("deadline"));
            else if(s->phase!=Phase::Quarantined) {
                for(unsigned i=0;i<s->earlyCount;++i)if(s->early[i]==id)++s->completionCount;
                s->earlyCount=0;evaluate(s);
            }
        }
        return !threw && id!=0; // Mechanics only, never release/delivery success.
    }
    void cancel(const Token &token,QString reason=QStringLiteral("canceled")) {
        if(!gui())return;
        auto *s=resolve(token);if(s)cancelToken(s,reason.left(128));
    }
    Snapshot snapshot(const Token &token) const {
        Snapshot r;if(!gui()) { r.refusal=QStringLiteral("application-thread");return r; }
        auto *s=resolveHistory(token);if(!s)return r;
        r.phase=s->phase;r.generation=s->generation;r.jobId=QString::number(s->jobId);r.refusal=s->refusal;r.depth=s->depth;
        r.entered=s->plain->entered.load();r.returning=s->plain->returning.load(std::memory_order_acquire);
        r.offGui=s->plain->offGui.load();r.duplicates=s->plain->duplicates.load();r.skipped=s->plain->skipped.load();
        r.completions=s->completionCount;r.attempted=s->consumed;r.submissionReturned=s->returned;
        r.closeAttempted=s->closeAttempted;r.closed=s->closeSucceeded;r.destroyAttempted=s->destroyAttempted;r.destroyed=s->destroySucceeded;r.settled=s->settled;return r;
    }
signals:
    void released(quint64 generation);
    // Later zero-depth GUI turn AFTER released listener/capture unwind. This
    // signal has no tracked Frame; no native lease/capture remains to finalize.
    void settled(quint64 generation);
    void quarantined(quint64 generation);
private:
    friend class detail::Relay;
    void completed(detail::Slot *s,QObject *signalSource,qulonglong id) {
        if(!gui() || !bound_)return;
        if(!s || s!=storage_->current || s->facade!=this || s->phase==Phase::Quarantined || s->phase==Phase::Released)return;
        const Frame frame(s);
        if(signalSource!=s->source || !s->source) { quarantine(s,QStringLiteral("completion-source"));return; }
        if(expired(s)) { quarantine(s,QStringLiteral("deadline"));return; }
        if(s->submitting) {
            if(s->earlyCount==s->early.size())quarantine(s,QStringLiteral("completion-overflow"));
            else s->early[s->earlyCount++]=id;
        } else if(s->returned && id==s->jobId) { ++s->completionCount;evaluate(s); }
    }
    friend class Frame;
    ResidentStorage *storage_=nullptr;bool bound_=false;
    QPointer<detail::Relay> reserve_;QTimer timer_;quint64 timerGeneration_=0;
    bool gui() const {
        auto *app=qobject_cast<QGuiApplication*>(QCoreApplication::instance());
        return app && !QCoreApplication::closingDown() && thread()==app->thread() && QThread::currentThread()==app->thread();
    }
    static bool validCompletion(QObject *source,const QMetaMethod &m) {
        return m.isValid() && m.methodType()==QMetaMethod::Signal && m.methodSignature()==QByteArrayLiteral("jobCompleted(qulonglong)") &&
            m.returnMetaType().id()==QMetaType::Void && m.parameterCount()==1 && m.parameterMetaType(0).id()==QMetaType::ULongLong &&
            m.methodIndex()<source->metaObject()->methodCount() && source->metaObject()->method(m.methodIndex())==m;
    }
    detail::Slot *resolveHistory(const Token &t) const {
        return bound_ && t.slot_ && t.generation_==t.slot_->generation && t.slot_->storage==storage_?t.slot_:nullptr;
    }
    detail::Slot *resolve(const Token &t) const {
        auto *s=resolveHistory(t);return s && s==storage_->current && s->facade==this?s:nullptr;
    }
    static bool expired(const detail::Slot *s) { return s->elapsed.elapsed()>=s->budget; }
    static void quarantine(detail::Slot *s,const QString &reason) {
        if(s->phase==Phase::Released && s->settled)return;
        s->plain->canceled.store(true,std::memory_order_release);
        if(s->phase!=Phase::Quarantined) { s->phase=Phase::Quarantined;s->refusal=reason.left(128); }
        schedule(s);
    }
    void cancelToken(detail::Slot *s,const QString &reason) {
        if(s->storage!=storage_ || storage_->current!=s)return;
        const Frame frame(s);quarantine(s,reason);
    }
    static bool facts(const detail::Slot *s) {
        const auto &p=*s->plain;
        return p.returning.load(std::memory_order_acquire)==1 &&
            s->returned && s->jobId && s->completionCount==1 && p.entered.load()==1 && p.offGui.load()==1 && !p.duplicates.load() &&
            !p.skipped.load() && !p.canceled.load(std::memory_order_acquire);
    }
    static void evaluate(detail::Slot *s) {
        if(s->phase==Phase::Quarantined || !s->completionCount)return;
        if(!facts(s))quarantine(s,QStringLiteral("delivery-facts"));
        else { s->phase=Phase::Ready;schedule(s); }
    }
    static void schedule(detail::Slot *s) {
        if(s->depth || s->submitting || s->queued || !s->facade ||
           (s->phase!=Phase::Ready && s->phase!=Phase::Finalized && s->phase!=Phase::Quarantined && s->phase!=Phase::Released))return;
        if(s->phase==Phase::Quarantined && s->notified)return;
        if(s->phase==Phase::Released && s->settled)return;
        s->queued=true;
        const QPointer<Pool> weak=s->facade;
        QMetaObject::invokeMethod(weak,[weak,s] {
            s->queued=false;
            if(weak)weak->turn(s);
        },Qt::QueuedConnection);
    }
    void turn(detail::Slot *s) {
        if(!gui() || s->facade!=this || s!=storage_->current || s->depth || s->submitting)return;
        if(s->phase==Phase::Released) {
            if(s->settled || storage_->notifying)return;
            if(expired(s)) { quarantine(s,QStringLiteral("deadline"));return; }
            const QPointer<Pool> weak(this);
            // Previous Sample and cleanup Frames have unwound. Reserve the
            // NEXT receiver here, never by creating a QObject during install.
            if(storage_->generation<256 && !reserve_) {
                auto *next=new detail::Relay(nullptr,this);
                if(!weak)return;
                reserve_=next;
            }
            if(!weak)return;
            if(s->phase==Phase::Quarantined)return;
            timer_.stop();
            s->settled=true;
            auto *storage=storage_;storage->notifying=true;
            emit settled(s->generation);
            storage->notifying=false;
            if(storage->current==s)storage->current=nullptr;
            return; // No this access after signal; receiver may delete Pool.
        }
        const Frame frame(s);
        const QPointer<Pool> weak(this);
        if(s->phase==Phase::Quarantined) {
            timer_.stop();
            if(!s->notified) { s->notified=true;emit quarantined(s->generation); }return;
        }
        if(!s->source || expired(s) || !facts(s)) { quarantine(s,QStringLiteral("release-admission"));return; }
        if(s->phase==Phase::Ready) {
            s->phase=Phase::Finalizing;
            const auto connection=s->completion;s->completion={};QObject::disconnect(connection);
            if(!weak || s->phase==Phase::Quarantined) { quarantine(s,QStringLiteral("owner-destroyed"));return; }
            const auto destroyed=s->destroyed;s->destroyed={};QObject::disconnect(destroyed);
            if(!weak || s->phase==Phase::Quarantined) { quarantine(s,QStringLiteral("owner-destroyed"));return; }
            s->closeAttempted=true;
            try { s->closeSucceeded=s->lease.closeOnGui(s->lease.object); }
            catch(...) { quarantine(s,QStringLiteral("close-exception"));return; }
            if(!weak || s->phase==Phase::Quarantined || !s->closeSucceeded) { quarantine(s,QStringLiteral("close-failed"));return; }
            s->destroyAttempted=true;
            try { s->lease.destroyClosedOnGui(s->lease.object);s->destroySucceeded=true; }
            catch(...) { quarantine(s,QStringLiteral("destroy-exception"));return; }
            if(!weak || s->phase==Phase::Quarantined) { quarantine(s,QStringLiteral("owner-destroyed"));return; }
            s->lease={};
            s->plain->work.plainContext.reset(); // Passive captures destroyed under Frame.
            if(!weak || s->phase==Phase::Quarantined) { quarantine(s,QStringLiteral("owner-destroyed"));return; }
            // Historical disconnected Relays remain bounded/resident. A late
            // DeferredDelete must not inject ChildRemoved into a later Sample.
            s->phase=Phase::Finalized;return; // A separate later turn publishes.
        }
        if(s->phase==Phase::Finalized) {
            s->phase=Phase::Released;storage_->notifying=true;
            auto *storage=storage_;
            emit released(s->generation);
            storage->notifying=false;
            // The Frame epilogue schedules settled, without nested-loop spin.
            // Keep the slot occupied until that later notification unwinds.
        }
    }
};
inline Frame::~Frame() {
    if(!slot_)return;
    --slot_->depth;Pool::schedule(slot_);
}
inline void detail::Relay::completed(qulonglong id) {
    if(!slot_)return;
    if(std::this_thread::get_id()!=slot_->plain->guiThread)return;
    if(slot_->phase==Phase::Released)return;
    const Frame frame(slot_); // First local, including receiver callback unwind.
    const QPointer<Pool> weak=slot_->facade;
    if(weak)weak->completed(slot_,sender(),id);
}
} // namespace reader_worker_quarantine
