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
};
}
namespace {
auto arm(KeyReceiver &receiver, Session &s, std::chrono::milliseconds budget = 10s,
         std::function<void()> before = {}) {
    return testing::KeyAccess::make(&receiver, budget,
        [&s, before](QQuickItem *, Clock::time_point deadline, const reader_worker_native::Permit &permit) {
            if (before) before();
            if (!permit()) return std::unique_ptr<Delivery>{};
            // Owned callback fake only. The separate receiver fixture exercises
            // the real production selected-view join; this is not native proof.
            auto d = testing::DeliveryAccess::make(*s.profile,*s.slot,s.token,s.page,scene,
                0ms,Mode::Observe,{},deadline);
            assert(d && testing::DeliveryAccess::deadline(*d)==deadline); d->submit(permit);
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
        assert(r.scene.entered==1 && r.scene.returned==1 && r.scene.attachment.attached);
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
        if(ingress) { auto r=ingress->snapshot(); assert(r.canceled && !r.scene.attachment.attached); }
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
        assert(receiver.forwarded==1 && s.fixture.requests==0 && ingress->snapshot().refusal=="key-text-limit");
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
        assert(!receiver && s.fixture.requests==0 && ingress->snapshot().canceled);
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
        [&](QQuickItem *,Clock::time_point deadline,const reader_worker_native::Permit &permit) {
            auto d=testing::DeliveryAccess::make(*s.profile,*s.slot,s.token,s.page,scene,0ms,
                Mode::Observe,remarkable::composer_text::ReadRange{0,draft.size()},deadline);
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
    s.retained();
}
}
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);
    factsAndOrdering(); invalidationAndNoRelease(); refusalAndLifetime(); exactTextAtOwnedQueueBoundary();
    std::cout<<"key-to-Scene handoff: owned Qt dispatch, exact facts, stock forwarding, ordering, cancellation and lifetime passed; no native execution\n";
}
#include "composer_key_delivery_test.moc"
