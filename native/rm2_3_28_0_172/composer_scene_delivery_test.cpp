#define RESIDENT_WORKER_SLOT_TESTING
#define COMPOSER_SCENE_TESTING
#define COMPOSER_DELIVERY_TESTING
#include "composer_scene_delivery.h"
#include <QGuiApplication>
#include <cassert>
#include <future>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace std::chrono_literals;
using namespace remarkable::composer_scene;
struct NativeId { std::uint32_t words[3]={17,23,31}; };
Q_DECLARE_METATYPE(NativeId)
namespace xofm::libs::library { class DocumentController final:public QObject { Q_OBJECT }; }
class DocumentWorker final:public QThread { Q_OBJECT };
namespace reader_worker_native::testing {
struct SlotAccess {
    static ResidentSlot *make(detail::Functions fn) { return new ResidentSlot(fn,nullptr); }
    static void destroy(ResidentSlot *slot) { delete slot; }
    static auto retire(ResidentSlot &slot,const ResidentSlot::Token &token) {
        return slot.retirePhysically(token);
    }
};
}
namespace remarkable::composer_scene::testing {
struct Access {
    static std::unique_ptr<Profile> profile() { return std::unique_ptr<Profile>(new Profile(true)); }
    static void fixture(Attachment &a) {
        a.state_->join=[](::Scene *scene) {
            Join result; result.document=reinterpret_cast<QTextDocument *>(scene);
            result.facts.documentAddress=reinterpret_cast<std::uintptr_t>(scene);
            result.root=1; result.control=2; result.participant=3; result.privateData=4;
            return result;
        };
        a.state_->readCaches=[](const Join &) -> detail::CacheBytes {
            return std::array<unsigned char, 2>{0, 1};
        };
    }
};
struct DeliveryAccess {
    static std::unique_ptr<Delivery> make(const Profile &p,reader_worker_native::ResidentSlot &slot,
            const reader_worker_native::ResidentSlot::Token &token,const QString &page,
            SceneRequest fn,std::chrono::milliseconds budget=120s,Mode mode=Mode::Observe,
            std::optional<composer_text::ReadRange> range={}) {
        auto value=Delivery::make(p,slot,token,page,budget,fn,range,mode);
        if(value)Access::fixture(*value->state_->attachment);
        return value;
    }
    static Clock::time_point deadline(const Delivery &d) { return d.state_->attachment->deadline(); }
};
}
namespace {
using Slot=reader_worker_native::ResidentSlot;
struct Fixture {
    xofm::libs::library::DocumentController controller;
    DocumentWorker worker;
    int document=0,docControl=0,workerControl=0;
    unsigned requests=0,resolves=0,acquires=0;
    QString received;
    std::vector<Callback> copies;
    bool throws=false;
    qulonglong job=0xabcdef0100000001ULL;
    std::function<void()> onRequest;
};
Fixture *active=nullptr;
void put(void *out,void *raw,void *control) {
    const std::array<std::uintptr_t,2> words{
        reinterpret_cast<std::uintptr_t>(raw),reinterpret_cast<std::uintptr_t>(control)};
    std::memcpy(out,words.data(),sizeof words);
}
void resolve(void *out,void *controller,const void *id) {
    ++active->resolves;
    assert(controller==&active->controller && static_cast<const NativeId *>(id)->words[0]==17);
    put(out,&active->document,&active->docControl);
}
void acquire(void *out,void *document,int mode) {
    ++active->acquires; assert(document==&active->document && mode==1);
    put(out,&active->worker,&active->workerControl);
}
void release(void *) { assert(false && "typed delivery must never perform native owner release"); }
qulonglong generic(void *,const std::function<void()> &) {
    assert(false && "generic dispatcher must never receive the Scene callback"); return 0;
}
qulonglong scene(void *worker,const QString &page,const Callback &callback) {
    auto &f=*active; ++f.requests; assert(worker==&f.worker);
    f.received=page; f.copies.push_back(callback);
    if(f.onRequest)f.onRequest();
    if(f.throws)throw std::runtime_error("retained then threw");
    return f.job;
}
using SlotPtr=std::unique_ptr<Slot,void(*)(Slot*)>;
struct Session {
    Fixture fixture;
    SlotPtr slot{nullptr,reader_worker_native::testing::SlotAccess::destroy};
    Slot::Token token;
    std::unique_ptr<Profile> profile=testing::Access::profile();
    std::unique_ptr<Delivery> delivery;
    QString page=QStringLiteral("12345678-1234-1234-1234-123456789abc");
    explicit Session(std::chrono::milliseconds budget=120s, Mode mode=Mode::Observe) {
        active=&fixture;
        slot.reset(reader_worker_native::testing::SlotAccess::make({
            resolve,acquire,generic,release,QMetaType::fromType<NativeId>().iface(),
            &DocumentWorker::staticMetaObject}));
        auto prep=slot->prepare(&fixture.controller,QVariant::fromValue(NativeId{}),
            &fixture.worker,[]{return true;});
        assert(prep.ready); token=prep.token;
        delivery=testing::DeliveryAccess::make(*profile,*slot,token,page,scene,budget,mode);
        assert(delivery);
    }
    void retained() {
        const auto r=reader_worker_native::testing::SlotAccess::retire(*slot,token);
        assert(r.matched && !r.retired && !r.reclaimed && r.documentRetained && r.workerRetained);
        assert(slot->occupied());
    }
};
void successAndLogicalStop() {
    Session s; const auto expected=s.page; s.page[0]=QChar('a');
    auto sent=s.delivery->submit([]{return true;});
    assert(sent.attempted && sent.accepted && !sent.unknown && sent.jobId==0xabcdef0100000001ULL);
    assert(s.fixture.received==expected && s.fixture.requests==1);
    assert(!s.delivery->submit([]{return true;}).attempted && s.fixture.requests==1);
    assert(!s.slot->submit(s.token,[]{},[]{return true;}).attempted);
    std::promise<void> observed,edit;
    auto ready=observed.get_future(),resume=edit.get_future();
    std::thread emitter([&] {
        QTextDocument doc;
        assert(!s.fixture.copies[0](reinterpret_cast<::Scene *>(&doc)));
        doc.setPlainText(QString::fromUtf8("A\xcc\x81 \xf0\x9f\x91\xa9"));
        observed.set_value(); resume.wait();
        for(int i=0;i<100;++i)doc.contentsChanged();
    });
    assert(ready.wait_for(5s)==std::future_status::ready);
    auto attached=s.delivery->snapshot();
    assert(attached.available && attached.ownersPinned && attached.entered==1 && attached.returned==1);
    assert(attached.attachment.attached && attached.attachment.observations.count>1);
    s.delivery->stop();
    auto frozen=s.delivery->snapshot();
    assert(frozen.canceled && frozen.attachment.retired);
    edit.set_value(); emitter.join();
    assert(s.delivery->snapshot().attachment.observations.count==frozen.attachment.observations.count);
    s.fixture.copies.clear(); s.retained(); // No generic release even after all copies return/die.
}
void lateAndSlotCancellation() {
    for(int kind=0;kind<3;++kind) {
        Session s; assert(s.delivery->submit([]{return true;}).accepted);
        Callback retained=s.fixture.copies[0];
        if(kind==0)s.delivery->stop();
        else if(kind==1)s.delivery.reset();
        else assert(s.slot->cancel(s.token));
        assert(!retained(reinterpret_cast<::Scene *>(1)));
        if(s.delivery) {
            auto state=s.delivery->snapshot();
            assert(state.canceled && !state.attachment.attached && !state.attachment.callbackEntered);
        }
        s.retained();
    }
}
void ambiguousRequest() {
    for(int kind=0;kind<4;++kind) {
        Session s;
        if(kind==0)s.fixture.throws=true;
        if(kind==1)s.fixture.job=0;
        if(kind==2)s.fixture.onRequest=[&]{s.delivery->stop();};
        if(kind==3)s.fixture.onRequest=[&]{assert(!s.delivery->submit([]{return true;}).attempted);};
        auto result=s.delivery->submit([]{return true;});
        assert(result.attempted && !result.accepted && result.unknown && s.fixture.requests==1);
        assert(!s.fixture.copies[0](reinterpret_cast<::Scene *>(1)));
        assert(!s.delivery->submit([]{return true;}).attempted && s.fixture.requests==1);
        s.retained();
    }
}
void permitAndInputRefusal() {
    for(int kind=0;kind<4;++kind) {
        Session s;
        auto *delivery=s.delivery.get();
        auto result=delivery->submit([&] {
            if(kind==0)return false;
            if(kind==1)throw std::runtime_error("permit");
            if(kind==2)s.delivery.reset();
            if(kind==3)assert(!s.delivery->submit([]{return true;}).attempted);
            return true;
        });
        assert(!result.attempted && !result.unknown && s.fixture.requests==0 && s.fixture.copies.empty());
    }
    Session s;
    assert(!testing::DeliveryAccess::make(*s.profile,*s.slot,Slot::Token{},s.page,scene));
    assert(!testing::DeliveryAccess::make(*s.profile,*s.slot,s.token,QString{},scene));
    assert(!testing::DeliveryAccess::make(*s.profile,*s.slot,s.token,QString(257,QChar('a')),scene));
    assert(!testing::DeliveryAccess::make(*s.profile,*s.slot,s.token,QString(QChar(0)),scene));
    assert(!testing::DeliveryAccess::make(*s.profile,*s.slot,s.token,s.page,nullptr));
    std::thread foreign([&] {
        assert(!testing::DeliveryAccess::make(*s.profile,*s.slot,s.token,s.page,scene));
    });
    foreign.join();
    auto competing=testing::DeliveryAccess::make(*s.profile,*s.slot,s.token,s.page,scene);
    assert(competing && s.delivery->submit([]{return true;}).accepted);
    assert(!competing->submit([]{return true;}).attempted);
    competing.reset();
    // Refusing another wrapper must not cancel the session that claimed the slot.
    assert(!s.delivery->snapshot().canceled);
    QTextDocument doc;
    assert(!s.fixture.copies[0](reinterpret_cast<::Scene *>(&doc)));
    assert(s.delivery->snapshot().attachment.attached);
}
void scalarDelivery() {
    Session s(120s, Mode::Scalars);
    auto ambiguous=testing::DeliveryAccess::make(*s.profile,*s.slot,s.token,s.page,scene,120s,
        Mode::Scalars,remarkable::composer_text::ReadRange{0, 1});
    assert(!ambiguous && s.fixture.requests==0);
    assert(s.delivery->snapshot().attachment.scalar.status==ScalarStatus::Pending);
    assert(s.delivery->submit([]{return true;}).accepted);
    QTextDocument doc;
    assert(!s.fixture.copies[0](reinterpret_cast<::Scene *>(&doc)));
    auto state=s.delivery->snapshot();
    assert(state.available && state.ownersPinned && state.entered==1 && state.returned==1);
    assert(state.attachment.scalar.status==ScalarStatus::Observed && state.attachment.scalar.sample);
    assert(!state.attachment.attached && !state.attachment.observations.available);
    assert(state.attachment.text.status==remarkable::composer_text::Status::NotRequested);
    assert(state.attachment.scalar.sample->cachedWasRedoAvailable);
    assert(!s.delivery->submit([]{return true;}).attempted && s.fixture.requests==1);
    s.delivery->stop();
    const auto historical=s.delivery->snapshot();
    assert(historical.attachment.retired && historical.attachment.scalar.sample==state.attachment.scalar.sample);
    s.retained();

    Session canceled(120s, Mode::Scalars);
    assert(canceled.delivery->submit([]{return true;}).accepted);
    assert(canceled.slot->cancel(canceled.token));
    assert(!canceled.fixture.copies[0](reinterpret_cast<::Scene *>(1)));
    auto refused=canceled.delivery->snapshot().attachment;
    assert(refused.retired && !refused.callbackEntered && refused.scalar.status==ScalarStatus::Refused);
    assert(!refused.scalar.sample);
    canceled.retained();
}
void deadlineAndDuplicate() {
    {
        Session s(1ms);
        std::this_thread::sleep_until(testing::DeliveryAccess::deadline(*s.delivery)+1ms);
        assert(!s.delivery->submit([]{return true;}).attempted && s.fixture.requests==0);
    }
    {
        Session s(10ms);
        auto deadline=testing::DeliveryAccess::deadline(*s.delivery);
        assert(s.delivery->submit([]{return true;}).accepted);
        std::this_thread::sleep_until(deadline+1ms);
        assert(!s.fixture.copies[0](reinterpret_cast<::Scene *>(1)));
        auto expired=s.delivery->snapshot();
        assert(!expired.attachment.attached && expired.attachment.refusal==Refusal::Deadline);
        s.retained();
    }
    Session s; assert(s.delivery->submit([]{return true;}).accepted);
    QTextDocument doc;
    assert(!s.fixture.copies[0](reinterpret_cast<::Scene *>(&doc)));
    Callback second=s.fixture.copies[0];
    assert(!second(reinterpret_cast<::Scene *>(1)));
    auto state=s.delivery->snapshot();
    assert(state.entered==2 && state.returned==2 && state.attachment.retired);
    assert(state.attachment.refusal==Refusal::Duplicate && !state.attachment.attached);
    s.retained();
}
}
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);
    successAndLogicalStop(); lateAndSlotCancellation(); ambiguousRequest();
    permitAndInputRefusal(); deadlineAndDuplicate(); scalarDelivery();
    std::cout<<"typed scene delivery: 6 owned lifecycle/scalar groups passed; Qt "<<qVersion()
             <<"; full 64-bit job preserved; no native enqueue executed\n";
}
#include "composer_scene_delivery_test.moc"
