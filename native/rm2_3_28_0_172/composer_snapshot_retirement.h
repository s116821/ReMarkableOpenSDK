#pragma once
// Private typed-read proof. Completion is not Task destruction or write authority.
#include "reader_retirement_scope.h"
#include "reader_native_worker_bridge.h"
#include <QMetaMethod>
#include <QTimer>
#include <QUuid>
#include <chrono>

namespace remarkable::composer_scene {
class Delivery;
class KeyDelivery;
#ifdef COMPOSER_DELIVERY_TESTING
namespace testing { struct DeliveryAccess; }
#endif
struct SnapshotRetirementStatus {
    bool watching=false, matched=false, closed=false, settled=false, reclaimed=false;
    QString refusal;
};
class SnapshotRetirement final : public QObject {
    Q_OBJECT
    friend class Delivery;
    friend class KeyDelivery;
#ifdef COMPOSER_DELIVERY_TESTING
    friend struct testing::DeliveryAccess;
#endif
    using Clock=std::chrono::steady_clock;
    const Clock::time_point deadline_;
    reader_retirement::Fence fence_{this};
    reader_retirement::Token frameToken_;
    reader_request_scope::Binding binding_;
    QPointer<QObject> worker_;
    QMetaObject::Connection completion_;
    QTimer timer_;
    QList<qulonglong> early_;
    qulonglong expected_=0;
    unsigned matches_=0;
    bool known_=false, finishing_=false;
    SnapshotRetirementStatus status_;
    std::function<bool()> seal_;
    std::function<bool()> release_;
    // Keep the private witness alive after a public handle is destroyed. Native
    // callbacks never own this QObject or either GUI function.
    std::shared_ptr<SnapshotRetirement> keepAlive_;
    explicit SnapshotRetirement(Clock::time_point deadline):deadline_(deadline) {
        frameToken_=fence_.begin(QUuid::createUuid().toString(QUuid::Id128).toLatin1(),reader_retirement::Kind::ReadOnly);
        binding_=reader_retirement::binding(&fence_,frameToken_);
        timer_.setInterval(5);
        connect(&timer_,&QTimer::timeout,this,[this]{maintain();});
    }
    static std::shared_ptr<SnapshotRetirement> create(Clock::time_point deadline) {
        if(!reader_worker_native::detail::gui() || deadline<=Clock::now())return {};
        auto result=std::shared_ptr<SnapshotRetirement>(new SnapshotRetirement(deadline));
        return result->frameToken_?result:nullptr;
    }
    bool watch(QObject *worker,std::function<bool()> seal,std::function<bool()> release,
               const std::shared_ptr<SnapshotRetirement> &self) {
        const reader_request_scope::Scope scope(binding_);
        if(!scope.active() || status_.watching || !worker || !seal || !release ||
                self.get()!=this || Clock::now()>=deadline_)return false;
        const int index=worker->metaObject()->indexOfSignal("jobCompleted(qulonglong)");
        if(index<0)return false;
        const auto signal=worker->metaObject()->method(index);
        if(signal.methodType()!=QMetaMethod::Signal || signal.name()!="jobCompleted" ||
                signal.parameterCount()!=1 || signal.parameterMetaType(0)!=QMetaType::fromType<qulonglong>() ||
                signal.returnMetaType()!=QMetaType::fromType<void>())return false;
        worker_=worker; seal_=std::move(seal); release_=std::move(release);
        const auto slot=metaObject()->method(metaObject()->indexOfSlot("completed(qulonglong)"));
        completion_=QObject::connect(worker,signal,this,slot,Qt::QueuedConnection);
        if(!completion_ || !worker_) { seal_={}; release_={}; return false; }
        status_.watching=true; keepAlive_=self; timer_.start(); return true;
    }
    void returned(const reader_worker_native::Submission &sent) {
        const reader_request_scope::Scope scope(binding_);
        if(!scope.active() || status_.settled)return;
        if(!status_.watching || !sent.attempted || !sent.accepted || sent.unknown || !sent.jobId) {
            fail(QStringLiteral("snapshot-request-not-known")); return;
        }
        known_=true; expected_=sent.jobId;
        matches_=unsigned(early_.count(expected_)); early_.clear();
    }
    void fail(QString reason) {
        if(status_.settled)return;
        status_.refusal=std::move(reason); status_.settled=true;
        finish();
    }
    void finish() {
        if(finishing_)return;
        finishing_=true; timer_.stop(); QObject::disconnect(completion_);
        // Capture disposal never releases native owners. Successful release has
        // already passed handoffReady; a failed Fence may invalidate remaining
        // frames, but its original slot stays quarantined.
        const auto self=keepAlive_;
        if(self)QMetaObject::invokeMethod(this,[self] {
            self->seal_={}; self->release_={}; self->keepAlive_.reset();
        },Qt::QueuedConnection);
        else { seal_={}; release_={}; }
    }
    void maintain() {
        if(status_.settled)return;
        if(!worker_) { fail(QStringLiteral("snapshot-worker-gone")); return; }
        if(Clock::now()>=deadline_) { fail(QStringLiteral("snapshot-retirement-deadline")); return; }
        if(!known_ || !matches_)return;
        if(matches_!=1) { fail(QStringLiteral("snapshot-completion-repeated")); return; }
        status_.matched=true;
        if(!status_.closed) {
            const reader_request_scope::Scope scope(binding_);
            if(!scope.active())return;
            // seal closes admission under the very mutex used for native reads;
            // a false result is a nonblocking refusal, never a count-based drain.
            if(!seal_())return;
            status_.closed=true;
            if(!fence_.requestRetire(frameToken_,[]{},reader_retirement::Effect::NoMutationAttempted))
                fail(QStringLiteral("snapshot-frame-retirement-refused"));
            return;
        }
        if(!fence_.handoffReady(frameToken_))return;
        // Distinct later turn, no adapter frame. The private release function
        // rechecks the opaque slot generation and consumes only nonlast owners.
        status_.reclaimed=release_(); status_.settled=true;
        if(!status_.reclaimed)status_.refusal=QStringLiteral("snapshot-owner-quarantined");
        finish();
    }
private slots:
    void completed(qulonglong id) {
        const reader_request_scope::Scope scope(binding_);
        if(status_.settled || !status_.watching)return;
        if(!known_) {
            if(early_.size()>=16) { fail(QStringLiteral("snapshot-early-completion-cap")); return; }
            early_.append(id); return;
        }
        if(id==expected_)++matches_;
    }
public:
    ~SnapshotRetirement() override { QObject::disconnect(completion_); }
};
} // namespace remarkable::composer_scene
