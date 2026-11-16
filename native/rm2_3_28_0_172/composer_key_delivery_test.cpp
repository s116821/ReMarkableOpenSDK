#define COMPOSER_KEY_DELIVERY_TESTING
#define main scene_delivery_fixture_main
#include "composer_scene_delivery_test.cpp"
#undef main
#include "composer_key_delivery.h"

class KeyReceiver final : public QQuickItem {
    Q_OBJECT
public:
    using QQuickItem::QQuickItem;
    unsigned forwarded = 0;
    std::function<void()> stock;
    void keyPressEvent(QKeyEvent *event) override {
        ++forwarded;
        if (stock) stock();
        event->accept();
    }
signals:
    void documentChanged(); void controllerChanged(); void pageIdChanged();
};
namespace remarkable::composer_scene::testing {
struct KeyAccess {
    static auto make(QQuickItem *receiver, std::chrono::milliseconds budget, KeyDelivery::Acquire acquire) {
        return KeyDelivery::make(receiver, budget, std::move(acquire));
    }
    static void beforeSnapshot(KeyDelivery &delivery,std::function<void()> hook) {
        delivery.state_->beforeSceneSnapshot=std::move(hook);
    }
    static auto framed(QQuickItem *receiver,Clock::time_point deadline,KeyDelivery::Acquire acquire,
                       reader_request_scope::Binding binding) {
        return KeyDelivery::makeUntil(receiver,deadline,std::move(acquire),std::move(binding));
    }
};
}
namespace {
auto arm(KeyReceiver &receiver, Session &s, std::chrono::milliseconds budget = 10s,
         std::function<void()> before = {}, std::function<void(Delivery &)> configure = {}) {
    return testing::KeyAccess::make(&receiver, budget,
        [&s, before, configure](QQuickItem *, Clock::time_point deadline,
                const std::shared_ptr<std::atomic<bool>> &revoked,const reader_worker_native::Permit &permit) {
            if (before) before();
            if (!permit()) return std::unique_ptr<Delivery>{};
            // Owned callback fake only. The separate receiver fixture exercises
            // the real production selected-view join; this is not native proof.
            auto d = testing::DeliveryAccess::make(*s.profile,*s.slot,s.token,s.page,scene,
                0ms,Mode::Snapshot,{},deadline,revoked);
            assert(d && testing::DeliveryAccess::deadline(*d)==deadline);
            if(configure)configure(*d);
            d->submit(permit);
            return d;
        });
}
void factsAndOrdering() {
    for (const bool repeat : {false,true}) {
        Session s;
        QQuickWindow window;
        KeyReceiver receiver(window.contentItem());
        auto ingress = arm(receiver,s);
        assert(ingress);
        const QString text = QString::fromUtf8("new \xf0\x9f\x99\x82");
        QKeyEvent event(QEvent::KeyPress,Qt::Key_Return,Qt::ShiftModifier,38,13,7,text,repeat,repeat?3:1);
        receiver.stock = [&] { assert(s.fixture.requests==1 && s.fixture.copies.size()==1); };
        QCoreApplication::sendEvent(&receiver,&event);
        auto r=ingress->snapshot();
        assert(receiver.forwarded==1 && event.isAccepted() && r.entered && !r.canceled && r.key);
        assert(r.key->key==Qt::Key_Return && r.key->modifiers==Qt::ShiftModifier &&
            r.key->autoRepeat==repeat && r.key->count==(repeat?3:1) && r.key->text==text &&
            r.key->nativeScanCode==38 && r.key->nativeVirtualKey==13 && r.key->nativeModifiers==7);
        QTextDocument doc;
        assert(!s.fixture.copies[0](reinterpret_cast<::Scene *>(&doc)));
        r=ingress->snapshot();
        assert(r.scene.entered==1 && r.scene.returned==1 && r.scene.attachment.callbackEntered &&
            r.scene.attachment.available && !r.scene.attachment.attached);
        assert(r.scene.ownersPinned && r.scene.submission.accepted);
        s.retained();
    }
}
void invalidationAndNoRelease() {
    for (int mode=0;mode<5;++mode) {
        Session s; QQuickWindow window; KeyReceiver receiver(window.contentItem());
        auto ingress=arm(receiver,s); assert(ingress);
        QKeyEvent enter(QEvent::KeyPress,Qt::Key_Enter,Qt::NoModifier,QString(),false,1);
        QCoreApplication::sendEvent(&receiver,&enter); // OSK-style, no KeyRelease.
        assert(s.fixture.requests==1 && receiver.forwarded==1);
        if(mode==0) { QKeyEvent next(QEvent::KeyPress,Qt::Key_A,Qt::NoModifier,"a"); QCoreApplication::sendEvent(&receiver,&next); }
        if(mode==1) { QInputMethodEvent ime; QCoreApplication::sendEvent(&receiver,&ime); }
        if(mode==2) { emit receiver.pageIdChanged(); emit receiver.pageIdChanged(); }
        if(mode==3) { QFocusEvent out(QEvent::FocusOut); QCoreApplication::sendEvent(&receiver,&out); }
        if(mode==4) ingress.reset();
        assert(!s.fixture.copies[0](reinterpret_cast<::Scene *>(1)));
        if(ingress) { auto r=ingress->snapshot();
            if(!r.canceled)std::cerr<<"revocation mode "<<mode<<" did not latch\n";
            assert(r.canceled && !r.scene.attachment.attached); }
        assert(s.fixture.requests==1); s.retained();
    }
}
void refusalAndLifetime() {
    {
        Session s; QQuickWindow window; KeyReceiver receiver(window.contentItem());
        auto ingress=arm(receiver,s,20ms);
        QKeyEvent event(QEvent::KeyPress,Qt::Key_Enter,Qt::NoModifier);
        QCoreApplication::sendEvent(&receiver,&event);
        assert(s.fixture.requests==1);
        std::this_thread::sleep_for(25ms);
        // No intervening GUI snapshot or timer is required for expiry.
        assert(!s.fixture.copies[0](reinterpret_cast<::Scene *>(1)));
        assert(ingress->snapshot().scene.attachment.refusal==Refusal::Deadline);
        s.retained();
    }
    for(const bool throws : {false,true}) {
        Session s; QQuickWindow window; KeyReceiver receiver(window.contentItem());
        s.fixture.throws=throws; if(!throws)s.fixture.job=0;
        auto ingress=arm(receiver,s);
        QKeyEvent event(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);
        QCoreApplication::sendEvent(&receiver,&event);
        auto r=ingress->snapshot();
        assert(receiver.forwarded==1 && s.fixture.requests==1 && r.canceled);
        assert(r.scene.submission.attempted && r.scene.submission.unknown &&
            !r.scene.submission.accepted && r.scene.ownersPinned);
        assert(!s.fixture.copies[0](reinterpret_cast<::Scene *>(1))); s.retained();
    }
    {
        Session s; QQuickWindow window; KeyReceiver receiver(window.contentItem());
        auto ingress=arm(receiver,s);
        QKeyEvent huge(QEvent::KeyPress,Qt::Key_A,Qt::NoModifier,QString(65,'a'));
        QCoreApplication::sendEvent(&receiver,&huge);
        assert(receiver.forwarded==1 && s.fixture.requests==0 && ingress->snapshot().refusal=="key-text-limit" && !ingress->snapshot().key);
    }
    {
        Session s; QQuickWindow window; KeyReceiver receiver(window.contentItem());
        auto ingress=arm(receiver,s,1ms); std::this_thread::sleep_for(3ms);
        QKeyEvent event(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);
        QCoreApplication::sendEvent(&receiver,&event);
        assert(receiver.forwarded==1 && s.fixture.requests==0 && ingress->snapshot().canceled);
    }
    {
        Session s; QQuickWindow window; auto receiver=std::make_unique<KeyReceiver>(window.contentItem());
        auto ingress=arm(*receiver,s,10s,[&]{receiver.reset();});
        QKeyEvent event(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);
        QCoreApplication::sendEvent(receiver.get(),&event);
        assert(!receiver && s.fixture.requests==0 && ingress->snapshot().canceled && !ingress->snapshot().key);
    }
    {
        Session s; QQuickWindow window; auto receiver=std::make_unique<KeyReceiver>(window.contentItem());
        s.fixture.onRequest=[&]{receiver.reset();};
        auto ingress=arm(*receiver,s);
        QKeyEvent event(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);
        QCoreApplication::sendEvent(receiver.get(),&event);
        const auto r=ingress->snapshot();
        assert(!receiver && s.fixture.requests==1 && r.canceled && r.scene.submission.attempted && r.scene.ownersPinned);
        assert(!s.fixture.copies[0](reinterpret_cast<::Scene *>(1))); s.retained();
    }
    {
        Session s; QQuickWindow window; KeyReceiver receiver(window.contentItem());
        std::unique_ptr<KeyDelivery> ingress;
        ingress=arm(receiver,s,10s,[&]{ingress.reset();});
        QKeyEvent event(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);
        QCoreApplication::sendEvent(&receiver,&event);
        assert(!ingress && receiver.forwarded==1 && s.fixture.requests==0);
    }
    {
        Session s; QQuickWindow window; KeyReceiver receiver(window.contentItem());
        auto ingress=arm(receiver,s,10s,[&]{
            QKeyEvent nested(QEvent::KeyPress,Qt::Key_Enter,Qt::NoModifier);
            QCoreApplication::sendEvent(&receiver,&nested);
        });
        QKeyEvent event(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);
        QCoreApplication::sendEvent(&receiver,&event);
        assert(receiver.forwarded==2 && s.fixture.requests==0 && ingress->snapshot().canceled);
    }
}
void exactTextAtOwnedQueueBoundary() {
    Session s; QQuickWindow window; KeyReceiver receiver(window.contentItem());
    QTextDocument doc;
    const QString draft=QString::fromUtf8("new \xf0\x9f\x99\x82 question");
    doc.setPlainText(draft);
    auto ingress=testing::KeyAccess::make(&receiver,10s,
        [&](QQuickItem *,Clock::time_point deadline,const std::shared_ptr<std::atomic<bool>> &revoked,
                const reader_worker_native::Permit &permit) {
            auto d=testing::DeliveryAccess::make(*s.profile,*s.slot,s.token,s.page,scene,0ms,
                Mode::Snapshot,remarkable::composer_text::ReadRange{0,draft.size()},deadline,revoked);
            assert(d && d->submit(permit).accepted);
            return d;
        });
    receiver.stock=[&] {
        // Owned queue fake: execute the enqueued read before the stock newline
        // job. This proves plumbing/copy only, not the native queue's behavior.
        assert(s.fixture.requests==1);
        assert(!s.fixture.copies[0](reinterpret_cast<::Scene *>(&doc)));
    };
    QKeyEvent event(QEvent::KeyPress,Qt::Key_Enter,Qt::NoModifier,QString(),false,1);
    QCoreApplication::sendEvent(&receiver,&event);
    const auto r=ingress->snapshot();
    if(r.scene.attachment.text.status!=remarkable::composer_text::Status::Copied)
        std::cerr<<"owned text refusal: attachment="<<int(r.scene.attachment.refusal)
                 <<" text="<<int(r.scene.attachment.text.failure)<<" canceled="<<r.canceled<<'\n';
    assert(receiver.forwarded==1 && r.key && r.scene.attachment.text.status==remarkable::composer_text::Status::Copied);
    assert(r.scene.attachment.text.count==std::size_t(draft.size()));
    for(qsizetype i=0;i<draft.size();++i) assert(r.scene.attachment.text.units[std::size_t(i)]==draft.at(i).unicode());
    assert(doc.toPlainText()==draft); // Handoff itself neither clears nor inserts.
    // Force foreign revocation between the outer cancellation sample and the
    // nested Scene snapshot. Preserve copied history, never a contradictory
    // top-level usable receipt.
    std::promise<void> notify,notified;
    std::thread notifier([&] { notify.get_future().wait(); emit receiver.pageIdChanged(); notified.set_value(); });
    testing::KeyAccess::beforeSnapshot(*ingress,[&] { notify.set_value(); notified.get_future().wait(); });
    const auto revoked=ingress->snapshot(); notifier.join();
    testing::KeyAccess::beforeSnapshot(*ingress,{});
    assert(revoked.canceled && revoked.scene.canceled && revoked.scene.attachment.text.status==remarkable::composer_text::Status::Copied);
    assert(revoked.scene.attachment.text.count==r.scene.attachment.text.count);
    s.retained();
}
void offThreadRevocationDuringHeldBorrow() {
    Session s; QQuickWindow window; KeyReceiver receiver(window.contentItem());
    std::promise<void> entered, release;
    struct Held { std::promise<void> &entered; std::shared_future<void> released; } held{entered,release.get_future().share()};
    auto ingress=arm(receiver,s,10s,{},[&](Delivery &d) {
        testing::DeliveryAccess::beforePublish(d,&held,[](void *p) {
            auto &h=*static_cast<Held *>(p); h.entered.set_value(); h.released.wait();
        });
    });
    QKeyEvent enter(QEvent::KeyPress,Qt::Key_Enter,Qt::NoModifier);
    QCoreApplication::sendEvent(&receiver,&enter);
    const auto callback=s.fixture.copies[0];
    std::thread reader([&]{QTextDocument doc; assert(!callback(reinterpret_cast<::Scene *>(&doc)));});
    entered.get_future().wait(); // Worker deliberately still owns borrow mutex.
    std::atomic<bool> notify{true}, emitted{false};
    std::thread notifier([&]{
        while(notify.load(std::memory_order_acquire)) {
            emit receiver.pageIdChanged(); emitted.store(true,std::memory_order_release);
        }
    });
    // No event loop is pumped: the foreign-thread latch must already revoke.
    while(!emitted.load(std::memory_order_acquire))std::this_thread::yield();
    for(int i=0;i<64;++i) assert(ingress->snapshot().canceled);
    auto r=ingress->snapshot();
    assert(r.canceled && !r.scene.attachment.available && !r.scene.attachment.attached);
    QKeyEvent next(QEvent::KeyPress,Qt::Key_A,Qt::NoModifier,"a");
    QCoreApplication::sendEvent(&receiver,&next);
    assert(receiver.forwarded==2 && s.fixture.requests==1);
    ingress.reset(); // Neither forwarding, polling nor destruction may wait.
    notify.store(false,std::memory_order_release); notifier.join();
    release.set_value(); reader.join();
    assert(!callback(reinterpret_cast<::Scene *>(1))); s.retained();
}
using OwnerKind=reader_insertion_native_profile::NativeOwnerKind;
using ReleaseResult=reader_insertion_native_profile::OwnerReleaseResult;
std::vector<OwnerKind> releases;
int releaseRefusal=0;
ReleaseResult nonlast(OwnerKind kind,std::uintptr_t raw,std::uintptr_t control) {
    assert(raw && control); releases.push_back(kind);
    if(releaseRefusal==1 && kind==OwnerKind::Worker)return ReleaseResult::RetainedLast;
    if(releaseRefusal==2 && kind==OwnerKind::Document)return ReleaseResult::RetainedBusy;
    return ReleaseResult::Released;
}
void pumpFor(std::chrono::milliseconds duration) {
    const auto end=Clock::now()+duration;
    do { QCoreApplication::processEvents(); std::this_thread::sleep_for(1ms); } while(Clock::now()<end);
}
void armSnapshot(Session &s,std::chrono::milliseconds budget=300ms) {
    releases.clear(); releaseRefusal=0;
    reader_worker_native::testing::SlotAccess::releaser(*s.slot,nonlast);
    s.delivery=testing::DeliveryAccess::reusable(*s.profile,*s.slot,s.token,s.page,scene,budget,
        remarkable::composer_text::ReadRange{0,3});
    assert(s.delivery);
}
void readAndComplete(Session &s,qulonglong id,bool emitCompletion=true,bool duplicate=false) {
    s.fixture.worker.ownedWork=[&] {
        QTextDocument doc; doc.setPlainText(QStringLiteral("new"));
        assert(!s.fixture.copies.back()(reinterpret_cast<::Scene *>(&doc)));
        if(duplicate)assert(!s.fixture.copies.back()(reinterpret_cast<::Scene *>(1)));
        if(emitCompletion)emit s.fixture.worker.jobCompleted(id);
    };
    s.fixture.worker.start(); assert(s.fixture.worker.wait(2000)); s.fixture.worker.ownedWork={};
}
void waitSnapshot(Session &s) {
    const auto limit=Clock::now()+2s;
    while(!s.delivery->snapshot().retirement.settled && Clock::now()<limit)pumpFor(2ms);
    assert(s.delivery->snapshot().retirement.settled);
    pumpFor(2ms); // Drop private GUI captures; not a native Task destruction claim.
}
void reusableSnapshotControls() {
    for(int mode=0;mode<15;++mode) {
        Session s; armSnapshot(s,120ms);
        if(mode==4)s.fixture.job=0;
        if(mode==5)s.fixture.throws=true;
        if(mode==14)s.fixture.onRequest=[&] {s.delivery->stop();};
        if(mode==1)s.fixture.onRequest=[&] {
            readAndComplete(s,s.fixture.job);
            pumpFor(8ms); // Exact completion before native request return.
            assert(s.slot->occupied() && releases.empty());
        };
        const auto sent=s.delivery->submit([]{return true;});
        if(mode==4||mode==5||mode==14) {
            assert(sent.attempted && sent.unknown && !sent.accepted);
            assert(!s.fixture.copies.back()(reinterpret_cast<::Scene *>(1)));
        } else {
            assert(sent.accepted);
            if(mode==2)readAndComplete(s,s.fixture.job^0x100000000ULL); // Full ID, not low word.
            else if(mode==3)readAndComplete(s,s.fixture.job,false);
            else if(mode==6) {
                QTextDocument doc; doc.setPlainText("new");
                assert(!s.fixture.copies.back()(reinterpret_cast<::Scene *>(&doc)));
                emit s.fixture.worker.jobCompleted(s.fixture.job); // GUI callback is insufficient.
            } else if(mode==7)readAndComplete(s,s.fixture.job,true,true);
            else if(mode==8) {
                s.delivery->stop();
                readAndComplete(s,s.fixture.job); // Revoked callback borrows nothing; exact completion still retires owners.
            } else if(mode==9) {
                readAndComplete(s,s.fixture.job);
                emit s.fixture.worker.jobCompleted(s.fixture.job);
            } else if(mode==12) {
                readAndComplete(s,s.fixture.job,false);
                DocumentWorker unrelated; emit unrelated.jobCompleted(s.fixture.job);
            } else if(mode==13) {
                std::thread unrelated([&] {
                    QTextDocument doc; doc.setPlainText("new");
                    assert(!s.fixture.copies.back()(reinterpret_cast<::Scene *>(&doc)));
                    emit s.fixture.worker.jobCompleted(s.fixture.job);
                });
                unrelated.join(); // Off-GUI alone does not establish exact worker.
            } else if(mode!=1)readAndComplete(s,s.fixture.job);
            if(mode==10)releaseRefusal=1;
            if(mode==11)releaseRefusal=2;
        }
        waitSnapshot(s);
        const auto result=s.delivery->snapshot();
        const bool reclaimed=mode==0||mode==1||mode==8;
        assert(result.retirement.reclaimed==reclaimed && s.slot->occupied()!=reclaimed);
        if(reclaimed || mode==10 || mode==11) {
            assert((releases==std::vector<OwnerKind>{OwnerKind::Worker,OwnerKind::Document}));
            assert(result.retirement.closed && result.retirement.matched);
        } else assert(releases.empty());
        if(mode==0||mode==1) {
            assert(!result.canceled && result.attachment.text.status==remarkable::composer_text::Status::Copied);
            assert(result.attachment.text.count==3 && result.attachment.text.revisionBefore==result.attachment.text.revisionAfter);
        }
        if(mode==8)assert(result.canceled && !result.attachment.callbackEntered);
        // Retained native function copies can outlive owner release, but cannot
        // dereference even a deliberately invalid Scene after admission closes.
        assert(!s.fixture.copies.back()(reinterpret_cast<::Scene *>(1)));
        if(mode==0||mode==1) {
            const auto late=s.delivery->snapshot();
            assert(!late.canceled && late.attachment.refusal==Refusal::None &&
                late.attachment.text.status==remarkable::composer_text::Status::Copied);
        }
        if(!reclaimed)assert(s.slot->occupied());
    }
}
void reusableFramesAndGenerations() {
    Session s; armSnapshot(s,500ms);
    const auto first=s.token;
    assert(s.delivery->submit([]{return true;}).accepted);
    {
        const reader_request_scope::Scope frame(testing::DeliveryAccess::frames(*s.delivery));
        assert(frame.active());
        readAndComplete(s,s.fixture.job); pumpFor(25ms);
        assert(s.delivery->snapshot().retirement.closed && s.slot->occupied() && releases.empty());
        // An apparently later nested event-loop turn cannot release under this frame.
        pumpFor(20ms); assert(s.slot->occupied() && releases.empty());
    }
    waitSnapshot(s); assert(!s.slot->occupied());
    for(int generation=0;generation<3;++generation) {
        auto prep=s.slot->prepare(&s.fixture.controller,QVariant::fromValue(NativeId{}),&s.fixture.worker,[]{return true;});
        assert(prep.ready); s.token=prep.token;
        assert(!s.slot->cancel(first)); // Old weak generation cannot revoke replacement.
        const auto priorJob=s.fixture.job++;
        armSnapshot(s); assert(s.delivery->submit([]{return true;}).accepted);
        emit s.fixture.worker.jobCompleted(priorJob); pumpFor(8ms);
        assert(s.slot->occupied() && releases.empty());
        readAndComplete(s,s.fixture.job); waitSnapshot(s);
        assert(!s.slot->occupied() && !s.delivery->snapshot().ownersPinned);
    }
}
void reusableHeldBorrowAndDeletedHandle() {
    Session s; armSnapshot(s,500ms);
    std::promise<void> entered,release;
    struct Held { std::promise<void> &entered; std::shared_future<void> release; } held{entered,release.get_future().share()};
    testing::DeliveryAccess::beforePublish(*s.delivery,&held,[](void *p) {
        auto &h=*static_cast<Held*>(p); h.entered.set_value(); h.release.wait();
    });
    assert(s.delivery->submit([]{return true;}).accepted);
    s.fixture.worker.ownedWork=[&] {
        QTextDocument doc; doc.setPlainText("new");
        assert(!s.fixture.copies.back()(reinterpret_cast<::Scene *>(&doc)));
        emit s.fixture.worker.jobCompleted(s.fixture.job);
    };
    s.fixture.worker.start();
    entered.get_future().wait();
    // A dishonest premature completion cannot drain the active borrow.
    emit s.fixture.worker.jobCompleted(s.fixture.job);
    pumpFor(20ms); assert(s.slot->occupied() && releases.empty());
    s.delivery.reset(); // Nonblocking public handle destruction under the borrow.
    release.set_value(); assert(s.fixture.worker.wait(2000)); s.fixture.worker.ownedWork={};
    pumpFor(520ms);
    assert(s.slot->occupied() && releases.empty()); // Two completions quarantine.
    assert(!s.fixture.copies.back()(reinterpret_cast<::Scene *>(1)));

    Session clean; armSnapshot(clean,500ms);
    assert(clean.delivery->submit([]{return true;}).accepted);
    clean.delivery.reset();
    readAndComplete(clean,clean.fixture.job); pumpFor(30ms);
    assert(!clean.slot->occupied() && releases.size()==2); // Private witness survives handle.
}
void keyFrameOwnsWholeAcquisition() {
    Session s; QQuickWindow window; KeyReceiver receiver(window.contentItem());
    releases.clear(); releaseRefusal=0;
    reader_worker_native::testing::SlotAccess::releaser(*s.slot,nonlast);
    const auto deadline=Clock::now()+500ms;
    const auto retirement=testing::DeliveryAccess::retirement(deadline);
    auto ingress=testing::KeyAccess::framed(&receiver,deadline,
        [&](QQuickItem *,Clock::time_point original,const std::shared_ptr<std::atomic<bool>> &revoked,
                const reader_worker_native::Permit &permit) {
            auto d=testing::DeliveryAccess::make(*s.profile,*s.slot,s.token,s.page,scene,0ms,Mode::Snapshot,
                remarkable::composer_text::ReadRange{0,3},original,revoked,retirement);
            assert(d && d->submit(permit).accepted);
            readAndComplete(s,s.fixture.job);
            // The submit frame is gone, but the real key filter's acquisition
            // and its local capture destruction are still on the GUI stack.
            pumpFor(20ms);
            assert(d->snapshot().retirement.closed && s.slot->occupied() && releases.empty());
            return d;
        },testing::DeliveryAccess::frames(retirement));
    QKeyEvent enter(QEvent::KeyPress,Qt::Key_Enter,Qt::NoModifier);
    QCoreApplication::sendEvent(&receiver,&enter);
    assert(receiver.forwarded==1 && s.slot->occupied());
    pumpFor(25ms);
    const auto result=ingress->snapshot();
    assert(result.key && !result.canceled && result.scene.retirement.reclaimed && !s.slot->occupied());
    assert(result.scene.attachment.text.status==remarkable::composer_text::Status::Copied);
}
void countsDoNotReplaceBorrowClosure() {
    Session s; armSnapshot(s,500ms);
    assert(s.delivery->submit([]{return true;}).accepted);
    readAndComplete(s,s.fixture.job,false);
    assert(s.delivery->snapshot().returned==1);
    std::promise<void> held,release;
    auto ready=held.get_future(); auto resume=release.get_future().share();
    std::thread lockHolder([&] { testing::DeliveryAccess::holdBorrowLock(*s.delivery,held,resume); });
    ready.wait();
    emit s.fixture.worker.jobCompleted(s.fixture.job);
    pumpFor(25ms);
    const auto pending=s.delivery->snapshot();
    assert(pending.entered==1 && pending.returned==1 && pending.retirement.matched);
    assert(!pending.retirement.closed && s.slot->occupied() && releases.empty());
    release.set_value(); lockHolder.join();
    waitSnapshot(s); assert(!s.slot->occupied());
}
}
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);
    factsAndOrdering(); invalidationAndNoRelease(); refusalAndLifetime(); exactTextAtOwnedQueueBoundary();
    offThreadRevocationDuringHeldBorrow();
    reusableSnapshotControls(); reusableFramesAndGenerations(); reusableHeldBorrowAndDeletedHandle();
    keyFrameOwnsWholeAcquisition();
    countsDoNotReplaceBorrowClosure();
    std::cout<<"key-to-Scene handoff: owned Qt dispatch, exact facts, stock forwarding, ordering, cancellation, lifetime and 15 snapshot retirement controls plus generations/frames/held-borrow passed; no native execution\n";
}
#include "composer_key_delivery_test.moc"
