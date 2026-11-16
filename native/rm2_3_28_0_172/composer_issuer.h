#pragma once
// Development-only producer on the maintained selected-view/worker stack.
// These process-local capabilities are not a conversation store or restart IDs.
#include "composer_scene_delivery.h"
#include "resident_mutation.h"
#include "order_observation.h"
#include "../resident/endpoint.h"
#include <QKeyEvent>
#include <limits>

namespace remarkable::composer_writer {
class Issuer final:public QObject {
    using Slot=reader_worker_native::ResidentSlot;
    using Clock=composer_text::Clock;
    using Gate=rm2_3_28_0_172::ResidentEffectGate;
    struct Visit {
        const std::shared_ptr<std::atomic<bool>> revoked=std::make_shared<std::atomic<bool>>(false);
        resident::Facts facts;
        resident::Id id,rangeId;
        QPointer<QQuickItem> receiver;
        QPointer<QQuickWindow> window;
        QPointer<QObject> worker;
        std::shared_ptr<OwnedEdit> engine;
        std::shared_ptr<OwnedEdit::Range> range;
        QList<QMetaObject::Connection> connections;
        quint64 inputSerial=0;
        ~Visit(){revoked->store(true,std::memory_order_release);for(const auto &c:connections)QObject::disconnect(c);}
    };
    struct Active {
        resident::Endpoint::Lease lease;
        Clock::time_point deadline;
        QPointer<resident::Endpoint> endpoint;
        std::shared_ptr<Visit> visit;
        const std::shared_ptr<std::atomic<bool>> canceled=std::make_shared<std::atomic<bool>>(false);
        Gate::Token effect;
        std::shared_ptr<composer_scene::SnapshotRetirement> retirement;
        std::shared_ptr<Attachment> attachment;
        std::unique_ptr<composer_scene::Delivery> delivery;
        resident::Composer key;
        bool awaiting=false,keyCaptured=false,starting=false,finishing=false;
    };
    reader_worker_native::ResidentMutation *const executor_;
    Slot *const slot_;
    Gate &effects_=Gate::process();
    std::shared_ptr<Visit> visit_;
    std::shared_ptr<Active> active_;
    QTimer timer_;
    // Bounded process-local request admission, not durable deduplication. The
    // consumer's existing durable journal owns replay across process restart.
    QSet<resident::Id> consumed_;
    bool quarantined_=false;
    explicit Issuer(reader_worker_native::ResidentMutation *executor,QObject *parent)
        :QObject(parent),executor_(executor),slot_(executor->owners_) {
        timer_.setInterval(5);connect(&timer_,&QTimer::timeout,this,[this]{
            try{maintain();}catch(...){
                // Do not let a result-allocation failure unwind through Qt.
                // Preserve quarantined native ownership and the original lease.
                quarantined_=true;if(active_)active_->canceled->store(true,std::memory_order_release);
                detachVisit();timer_.stop();
            }
        });
    }
    static bool sameFacts(const resident::Facts &a,const resident::Facts &b) {
        return a.document==b.document&&a.page==b.page&&a.order==b.order;
    }
    static bool watch(const std::shared_ptr<Visit> &visit,QObject *object,QMetaMethod signal) {
        if(!object||!signal.isValid()||signal.methodType()!=QMetaMethod::Signal)return false;
#if QT_VERSION >= QT_VERSION_CHECK(6,10,0)
        auto revoke=[gate=visit->revoked]{gate->store(true,std::memory_order_release);};
        auto connection=QMetaObject::connect(object,signal,object,std::move(revoke),Qt::DirectConnection);
        if(!connection)return false;
        try{visit->connections.append(connection);}catch(...){QObject::disconnect(connection);throw;}
        return true;
#else
        (void)visit;return false; // Host CI compiles; it cannot install the target issuer.
#endif
    }
    static bool signal(const std::shared_ptr<Visit> &visit,QObject *object,const char *name) {
        if(!object)return false;
        return watch(visit,object,object->metaObject()->method(object->metaObject()->indexOfSignal(name)));
    }
    static bool property(const std::shared_ptr<Visit> &visit,QObject *object,const char *name) {
        if(!object)return false;
        const auto p=object->metaObject()->property(object->metaObject()->indexOfProperty(name));
        return p.isValid()&&(p.isConstant()||watch(visit,object,p.notifySignal()));
    }
    bool install(const std::shared_ptr<Visit> &visit) {
        auto *receiver=visit->receiver.data();auto *window=visit->window.data();
        if(!receiver||!window||receiver->thread()!=QThread::currentThread())return false;
        for(const auto *s:{"destroyed(QObject*)","documentChanged()","controllerChanged()","pageIdChanged()",
                "activeFocusChanged(bool)","parentChanged(QQuickItem*)","windowChanged(QQuickWindow*)","visibleChanged()","enabledChanged()"})
            if(!signal(visit,receiver,s))return false;
        for(const auto *s:{"destroyed(QObject*)","activeFocusItemChanged()","activeChanged()","visibleChanged(bool)"})
            if(!signal(visit,window,s))return false;
        if(!signal(visit,QCoreApplication::instance(),"aboutToQuit()")||
                !signal(visit,QCoreApplication::instance(),"focusWindowChanged(QWindow*)"))return false;
        receiver->installEventFilter(this);return true;
    }
    bool allowed(const std::shared_ptr<Active> &a)const {
        const auto &v=a->visit;
        return active_==a&&a->endpoint&&a->lease->active()&&Clock::now()<a->deadline&&!a->canceled->load(std::memory_order_acquire)&&
            !v->revoked->load(std::memory_order_acquire)&&v->receiver&&v->window&&
            QGuiApplication::focusWindow()==v->window&&v->receiver->window()==v->window;
    }
    void detachVisit() {
        if(visit_){visit_->revoked->store(true,std::memory_order_release);if(visit_->receiver)visit_->receiver->removeEventFilter(this);}
        visit_.reset();
    }
    void reply(const std::shared_ptr<Active> &a,resident::Frame value,bool safeRelease) {
        if(active_!=a)return;
        if(safeRelease&&a->effect)effects_.release(a->effect);
        if(value.composer.status==2){quarantined_=true;detachVisit();}
        const auto endpoint=a->endpoint;const auto lease=a->lease;
        active_.reset();timer_.stop();
        // Destruction/retirement stays independent of connection liveness.
        if(endpoint&&lease->active())endpoint->finish(lease,std::move(value));
    }
    void refuse(const std::shared_ptr<Active> &a) {
        if(a->finishing)return;
        a->finishing=true;
        // Leave the selected Sample, key filter and adapter frame before
        // releasing effect admission or calling the endpoint's completion.
        QTimer::singleShot(0,this,[this,a] {
            if(active_!=a)return;
            resident::Frame value;value.kind=a->lease->request().kind+2;value.composer.status=1;
            if(a->lease->request().kind==12)detachVisit();
            reply(a,std::move(value),!slot_->occupied());
        });
    }
    void dispatch(const std::shared_ptr<Active> &a) {
        if(!allowed(a)||a->starting||a->delivery)return;
        a->starting=true;
        const auto deadline=a->deadline;
        const bool mutation=a->lease->request().kind==12||a->lease->request().kind==14;
        if(!a->retirement){a->starting=false;refuse(a);return;}
        const reader_request_scope::Scope frame(a->retirement->binding_);
        QPointer<Issuer> self(this);
        const auto permit=[self,a]{return self&&self->allowed(a);};
        const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(deadline-Clock::now()).count();
        if(ms<1){a->starting=false;refuse(a);return;}
        reader_selected::observeCreationJoin(a->visit->window,permit,int(std::min<qint64>(ms,reader_selected::MaxObservationMs)),{},{},a->visit->receiver,
            [this,self,a,deadline,mutation](QObject *reader,QObject *document,QObject *controller,const QVariant &id,QObject *worker,
                    const reader_selected::Observation &before,const std::function<bool()> &joined) {
                if(!self||!allowed(a)||!joined())return;
                const auto order=rm2_3_28_0_172::observeOrder(qmlEngine(a->visit->window),document,id,before,joined);
                if(!order.observed||!sameFacts(order.facts,a->visit->facts)||!joined()||!allowed(a))return;
                auto &v=*a->visit;
                if(!v.worker) {
                    // These connections retain only a sticky latch; the joined
                    // QObject arguments themselves never escape this callback.
                    if(!signal(a->visit,document,"destroyed(QObject*)")||!signal(a->visit,document,"pageMapChanged()")||
                            !property(a->visit,document,"pageCount")||!property(a->visit,reader,"document")||
                            !property(a->visit,reader,"currentPageId")||!property(a->visit,reader,"sceneController"))return;
                    v.worker=worker;
                    v.engine=OwnedEdit::prepare(reinterpret_cast<std::uintptr_t>(worker));
                }
                if(v.worker!=worker||!v.engine||!joined()||!allowed(a))return;
                auto prepared=slot_->prepare(controller,id,worker,joined);
                if(!prepared.ready)return;
                const auto &request=a->lease->request();
                QString expected;expected.resize(qsizetype(request.composer.text.size()));
                for(qsizetype i=0;i<expected.size();++i)expected[i]=QChar(request.composer.text[std::size_t(i)]);
                const auto command=request.kind==12?Attachment::Command::Provision:
                    request.kind==14?Attachment::Command::Clear:Attachment::Command::Observe;
                a->attachment=Attachment::prepare(command,v.engine,deadline,a->canceled,v.revoked,v.range,
                    request.composer.revision,std::move(expected),a->lease->retirement());
                if(a->attachment&&joined()&&allowed(a))a->delivery=composer_scene::Delivery::prepareWriter(*slot_,prepared.token,
                    before.pageId,a->attachment,a->canceled,a->retirement);
                if(a->delivery)a->delivery->submit(joined);else slot_->cancel(prepared.token);
                (void)mutation;
            });
        a->starting=false;
        if(!self)return;
        if(!a->delivery){refuse(a);return;}
        timer_.start();
    }
    void maintain() {
        const auto a=active_;if(!a)return;
        if(!allowed(a))a->canceled->store(true,std::memory_order_release);
        if(a->awaiting&&!a->keyCaptured) {
            if(a->canceled->load(std::memory_order_acquire))refuse(a);
            return;
        }
        if(!a->delivery||a->starting)return;
        const auto scene=a->delivery->snapshot();
        if(scene.submission.attempted&&!scene.retirement.settled&&Clock::now()<a->deadline)return;
        const auto result=a->attachment->snapshot();
        resident::Frame value;value.kind=a->lease->request().kind+2;auto &c=value.composer;
        c.status=scene.submission.attempted?2:1;c.attempts=scene.submission.attempted?1:0;
        if(result&&result->edit.action.submissionAttempted)c.attempts|=2;
        if(scene.retirement.matched)c.attempts|=4;
        bool settled=scene.retirement.reclaimed&&scene.retirement.closed&&scene.retirement.settled&&
            scene.retirement.matched&&scene.submission.accepted&&!scene.submission.unknown&&result&&result->finished;
        if(settled&&!scene.canceled&&!a->visit->revoked->load(std::memory_order_acquire)&&a->lease->active()) {
            const bool edit=a->lease->request().kind==12||a->lease->request().kind==14;
            const bool verified=edit?result->edit.status==EditStatus::ProjectionVerified:result->observed.observed;
            if(verified) {
                c.status=0;value.facts=a->visit->facts;c.visit=a->visit->id;c.range=a->visit->rangeId;
                if(a->lease->request().kind==12){a->visit->range=result->range;c.revision=result->edit.afterRevision;}
                else if(a->lease->request().kind==14){c.revision=result->edit.beforeRevision;c.afterRevision=result->edit.afterRevision;}
                else {
                    c.revision=result->observed.revision;
                    for(const auto unit:result->observed.text)c.text.push_back(unit.unicode());
                    if(a->lease->request().kind==15) {
                        const auto text=std::move(c.text);const auto rev=c.revision;const auto visit=c.visit,range=c.range;
                        c=a->key;c.status=0;c.attempts=5;c.visit=visit;c.range=range;c.revision=rev;c.text=text;
                    }
                }
            } else if(!(c.attempts&2))c.status=1;
        }
        // A known no-effect callback can safely refuse after retirement even
        // when input/visit cancellation prevented publication of copied text.
        if(settled&&!(c.attempts&2))c.status=c.status==0?0:1;
        if(c.status!=0){c.visit.clear();c.range.clear();c.text.clear();c.keyText.clear();c.event.clear();}
        if(c.status==1&&a->lease->request().kind==12)detachVisit();
        const bool safeRelease=settled&&c.status!=2;
        reply(a,std::move(value),safeRelease);
    }
    bool eventFilter(QObject *receiver,QEvent *event)override {
        const auto v=visit_;if(!v||receiver!=v->receiver||!event)return false;
        if(event->type()!=QEvent::KeyPress&&event->type()!=QEvent::InputMethod)return false;
        if(v->inputSerial==std::numeric_limits<quint64>::max()){v->revoked->store(true,std::memory_order_release);return false;}
        ++v->inputSerial;
        const auto a=active_;if(!a||!a->awaiting)return false;
        const reader_request_scope::Scope frame(a->retirement?a->retirement->binding_:reader_request_scope::Binding{});
        if(!frame.active()){a->canceled->store(true,std::memory_order_release);return false;}
        if(a->keyCaptured){a->canceled->store(true,std::memory_order_release);return false;}
        if(event->type()!=QEvent::KeyPress||!allowed(a))return false;
        const auto &key=*static_cast<QKeyEvent*>(event);const auto &keys=a->lease->request().composer.keys;
        if(std::find(keys.begin(),keys.end(),key.key())==keys.end())return false;
        const auto text=key.text();if(text.size()>64||key.count()<0||key.count()>65535){a->canceled->store(true,std::memory_order_release);return false;}
        resident::Composer facts;facts.event=resident::freshNonce();facts.inputSerial=v->inputSerial;
        facts.key=key.key();facts.count=quint16(key.count());facts.modifiers=quint32(key.modifiers());
        facts.nativeScan=key.nativeScanCode();facts.nativeVirtual=key.nativeVirtualKey();facts.nativeModifiers=key.nativeModifiers();
        facts.autoRepeat=key.isAutoRepeat();facts.spontaneous=key.spontaneous();for(auto unit:text)facts.keyText.push_back(unit.unicode());
        if(!resident::validId(facts.event)){a->canceled->store(true,std::memory_order_release);return false;}
        a->keyCaptured=true;a->key=std::move(facts);
        const QPointer<QQuickItem> target(v->receiver);
        try{dispatch(a);}catch(...){a->canceled->store(true,std::memory_order_release);quarantined_=true;}
        // The copied result is historical pre-key input. Forwarding Enter may
        // enqueue a separate native edit; no +1 or causal clear is inferred.
        return !target;
    }
public:
    static Issuer *create(reader_worker_native::ResidentMutation *executor,QObject *parent) {
        if(!reader_worker_native::detail::gui()||!executor||!executor->owners_)return nullptr;
        return new Issuer(executor,parent);
    }
    ~Issuer()override{if(active_)active_->canceled->store(true,std::memory_order_release);detachVisit();}
    void submit(resident::Endpoint *endpoint,resident::Endpoint::Lease lease) {
        if(!reader_worker_native::detail::gui()||!endpoint||!lease||!lease->active())return;
        const auto reject=[&](quint8 reason){resident::Frame r;r.kind=3;r.rejection=reason;endpoint->finish(lease,r);};
        const auto &request=lease->request();
        if(!resident::isComposerRequest(request.kind)||!resident::validFacts(request.facts)){reject(0);return;}
        if(quarantined_||active_||executor_->busy()||slot_->occupied()||consumed_.size()>=256||consumed_.contains(request.operation)){reject(4);return;}
        consumed_.insert(request.operation);
        if(request.kind==12) {
            if(visit_){reject(4);return;}
            auto v=std::make_shared<Visit>();v->facts=request.facts;v->id=resident::freshNonce();v->rangeId=resident::freshNonce();
            v->window=qobject_cast<QQuickWindow*>(QGuiApplication::focusWindow());
            v->receiver=v->window?v->window->activeFocusItem():nullptr;
            if(!resident::validId(v->id)||!resident::validId(v->rangeId)||!install(v)){reject(3);return;}
            visit_=std::move(v);
        } else if(!visit_||visit_->revoked->load(std::memory_order_acquire)||!visit_->range||
                !sameFacts(visit_->facts,request.facts)||visit_->id!=request.composer.visit||visit_->rangeId!=request.composer.range) {reject(3);return;}
        auto a=std::make_shared<Active>();a->lease=lease;a->endpoint=endpoint;a->visit=visit_;
        a->deadline=std::min(lease->deadline(),Clock::now()+std::chrono::seconds(20));
        a->retirement=composer_scene::SnapshotRetirement::createWriter(a->deadline,request.kind==12||request.kind==14);
        if(!a->retirement){if(request.kind==12)detachVisit();reject(0);return;}
        if(request.kind==12||request.kind==14){a->effect=effects_.acquire();if(!a->effect){reject(4);return;}}
        a->awaiting=request.kind==15;active_=a;timer_.start();
        if(!a->awaiting)dispatch(a);
    }
};
} // namespace remarkable::composer_writer
