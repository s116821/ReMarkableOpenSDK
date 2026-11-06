#define COMPOSER_OBSERVER_TESTING
#define COMPOSER_SCENE_TESTING
#include "composer_scene_attachment.h"
#include <QGuiApplication>
#include <QTextCursor>
#include <cassert>
#include <future>
#include <iostream>
#include <new>
using namespace std::chrono_literals;
using namespace remarkable::composer_scene;
namespace remarkable::composer_diagnostic::testing {
struct Access {
    static Clock::time_point deadline(const Observer &o) { return o.state_->deadline; }
};
}
namespace remarkable::composer_scene::testing {
struct Access {
    static std::unique_ptr<Profile> profile() { return std::unique_ptr<Profile>(new Profile); }
    static void join(Attachment &a, Join (*join)(::Scene *)) { a.state_->join = join; }
    static void stopHook(Attachment &a, void *p, void (*hook)(void *)) {
        a.state_->hookContext = p; a.state_->stopContended = hook;
    }
    static void publishHook(Attachment &a, void (*hook)(void *)) { a.state_->beforePublish = hook; }
    static Clock::time_point deadline(const Attachment &a) { return a.state_->deadline; }
    static Clock::time_point recorderDeadline(const Attachment &a) {
        return composer_diagnostic::testing::Access::deadline(*a.state_->observer);
    }
    static std::unique_ptr<Attachment> expired() {
        return std::unique_ptr<Attachment>(new Attachment(
            std::make_shared<Attachment::State>(Clock::now() - 1ms)));
    }
};
}
struct Fixture {
    QTextDocument *document;
    std::promise<void> *entered = nullptr;
    std::shared_future<void> release;
    unsigned calls = 0;
};
static Join fixtureJoin(::Scene *pointer) {
    auto &fixture = *reinterpret_cast<Fixture *>(pointer);
    ++fixture.calls;
    if (fixture.entered) { fixture.entered->set_value(); fixture.release.wait(); }
    Join result; result.document = fixture.document;
    result.facts.documentAddress = reinterpret_cast<std::uintptr_t>(fixture.document);
    return result;
}
static std::unique_ptr<Attachment> prepare() {
    auto profile = testing::Access::profile(); // Fixture-only, never production load.
    auto result = Attachment::prepare(*profile, 120s);
    testing::Access::join(*result, fixtureJoin);
    return result;
}
static void admissionAndLifetime() {
    assert(!Profile::load()); // This process is a fixture, not loaded Xochitl.
    auto profile = testing::Access::profile();
    assert(!Attachment::prepare(*profile, 0ms));
    assert(!Attachment::prepare(*profile, 120001ms));
    QTextDocument doc;
    Fixture fixture{&doc, nullptr, {}, 0};
    auto a = prepare();
    auto callback = a->callback();
    assert(!callback(reinterpret_cast<::Scene *>(&fixture)));
    auto state = a->snapshot();
    assert(state.available && state.attached && state.callbackEntered && fixture.calls == 1);
    assert(testing::Access::deadline(*a) == testing::Access::recorderDeadline(*a));
    doc.setPlainText(QString::fromUtf8("A\xcc\x81 \xf0\x9f\x91\xa9\xe2\x80\x8d\xf0\x9f\x92\xbb"));
    auto edited = a->snapshot();
    assert(edited.observations.count > state.observations.count);
    a->stop();
    auto stopped = a->snapshot();
    assert(stopped.retired && !stopped.attached);
    assert(stopped.observations.end == remarkable::composer_diagnostic::End::Canceled);
    for (int i = 0; i < 20; ++i) doc.contentsChanged();
    assert(a->snapshot().observations.count == stopped.observations.count);
    a.reset();
    // A saved callback after handle destruction never touches this invalid pointer.
    assert(!callback(reinterpret_cast<::Scene *>(1)));
}
static void rejectionAndDuplicate() {
    QTextDocument doc;
    Fixture fixture{&doc, nullptr, {}, 0};
    auto a = prepare();
    auto callback = a->callback();
    assert(!callback(reinterpret_cast<::Scene *>(&fixture)));
    assert(!callback(reinterpret_cast<::Scene *>(1))); // Duplicate never joins.
    auto state = a->snapshot();
    assert(state.retired && state.refusal == Refusal::Duplicate && fixture.calls == 1);
    assert(state.observations.end == remarkable::composer_diagnostic::End::Canceled);
    auto canceled = prepare();
    auto late = canceled->callback();
    canceled->stop();
    assert(!late(reinterpret_cast<::Scene *>(1)));
    auto expired = testing::Access::expired();
    assert(!expired->callback()(reinterpret_cast<::Scene *>(1)));
    assert(expired->snapshot().refusal == Refusal::Deadline);
    auto rejects = prepare();
    testing::Access::join(*rejects, [](::Scene *) { Join j; j.refusal = Refusal::FocusMismatch; return j; });
    assert(!rejects->callback()(nullptr));
    assert(rejects->snapshot().refusal == Refusal::FocusMismatch);
    auto throws = prepare();
    testing::Access::join(*throws, [](::Scene *) -> Join { throw 1; });
    assert(!throws->callback()(nullptr));
    assert(throws->snapshot().refusal == Refusal::Exception);
    assert(!remarkable::composer_diagnostic::Observer::attachUntil(doc, Clock::now() - 1ms));
}
static void destruction() {
    auto doc = std::make_unique<QTextDocument>();
    Fixture fixture{doc.get(), nullptr, {}, 0};
    auto a = prepare();
    assert(!a->callback()(reinterpret_cast<::Scene *>(&fixture)));
    doc.reset();
    auto state = a->snapshot();
    assert(!state.attached && state.observations.end == remarkable::composer_diagnostic::End::Destroyed);
    a->stop();
}
static void stopDuringJoin() {
    std::promise<void> entered, release, blocked;
    auto atJoin = entered.get_future();
    auto stopBlocked = blocked.get_future();
    auto permission = release.get_future().share();
    auto a = prepare();
    testing::Access::stopHook(*a, &blocked,
        [](void *p) { static_cast<std::promise<void> *>(p)->set_value(); });
    auto callback = a->callback();
    std::thread emitter([&] {
        QTextDocument doc;
        Fixture fixture{&doc, &entered, permission, 0};
        assert(!callback(reinterpret_cast<::Scene *>(&fixture)));
        assert(fixture.calls == 1);
    });
    assert(atJoin.wait_for(5s) == std::future_status::ready);
    assert(!a->snapshot().available);
    std::atomic<bool> returned{false};
    std::thread closer([&] { a->stop(); returned.store(true); });
    assert(stopBlocked.wait_for(5s) == std::future_status::ready);
    assert(!returned.load());
    release.set_value();
    emitter.join(); closer.join();
    auto state = a->snapshot();
    assert(state.available && state.retired && !state.attached);
    assert(!state.observations.available); // Canceled before recorder creation.
}

static void stopDuringPublication() {
    struct Phases {
        std::promise<void> entered, blocked, release, afterStop;
        std::shared_future<void> permission = release.get_future().share();
    } phases;
    auto entered = phases.entered.get_future(), blocked = phases.blocked.get_future();
    auto afterStop = phases.afterStop.get_future();
    auto a = prepare();
    testing::Access::stopHook(*a, &phases, [](void *p) {
        static_cast<Phases *>(p)->blocked.set_value();
    });
    testing::Access::publishHook(*a, [](void *p) {
        auto &phase = *static_cast<Phases *>(p);
        phase.entered.set_value(); phase.permission.wait();
    });
    auto callback = a->callback();
    std::thread emitter([&] {
        QTextDocument doc;
        Fixture fixture{&doc, nullptr, {}, 0};
        assert(!callback(reinterpret_cast<::Scene *>(&fixture)));
        afterStop.wait();
        for (int i=0;i<100;++i) doc.contentsChanged();
    });
    assert(entered.wait_for(5s) == std::future_status::ready);
    std::atomic<bool> returned{false};
    std::thread closer([&] { a->stop(); returned.store(true); });
    assert(blocked.wait_for(5s) == std::future_status::ready);
    assert(!returned.load());
    phases.release.set_value(); closer.join();
    auto frozen = a->snapshot();
    assert(frozen.available && frozen.retired && !frozen.attached);
    assert(frozen.observations.available && frozen.observations.count == 1);
    assert(frozen.observations.end == remarkable::composer_diagnostic::End::Canceled);
    phases.afterStop.set_value(); emitter.join();
    assert(a->snapshot().observations.count == frozen.observations.count);
}

static void duplicateDuringJoin() {
    std::promise<void> entered, release;
    auto atJoin = entered.get_future();
    auto permission = release.get_future().share();
    auto a = prepare();
    auto callback = a->callback(), second = callback;
    std::thread emitter([&] {
        QTextDocument doc;
        Fixture fixture{&doc, &entered, permission, 0};
        assert(!callback(reinterpret_cast<::Scene *>(&fixture)));
    });
    assert(atJoin.wait_for(5s) == std::future_status::ready);
    assert(!second(reinterpret_cast<::Scene *>(1)));
    release.set_value(); emitter.join();
    auto state = a->snapshot();
    assert(state.retired && state.refusal == Refusal::Duplicate && !state.attached);
}
#if defined(__arm__) && defined(__ARM_PCS_VFP)
static void nativeLayout() {
    static_assert(sizeof(void *) == 4 && sizeof(Callback) == 16 && alignof(Callback) == 4);
    alignas(16) unsigned char scene[0x270]{}, participant[0x48]{}, item[0x280]{};
    auto put = [](void *base, std::size_t offset, std::uint32_t value) {
        std::memcpy(static_cast<unsigned char *>(base) + offset, &value, 4);
    };
    auto address = [](void *p) { return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(p)); };
    put(scene,0x26c,address(participant)); put(scene,0x190,address(item)); put(scene,0x194,1);
    put(participant,8,address(scene)); put(participant,0x10,7); put(participant,0x14,9);
    put(item,0,0x133e700); put(item,8,0x133e758); put(item,0x10,7); put(item,0x14,9);
    put(item,0x260,0xffffffffU);
    auto *doc = new(item + 0x1ec) QTextDocument;
    auto *borrow = reinterpret_cast<::Scene *>(scene);
    auto joined = detail::join(borrow);
    assert(joined.document == doc && joined.refusal == Refusal::None);
    assert(joined.facts.idHigh == 7 && joined.facts.idLow == 9);
    assert(detail::join(nullptr).refusal == Refusal::NullScene);
    struct Case { void *base; std::size_t offset; std::uint32_t value; Refusal refusal; };
    const Case cases[] = {
        {scene,0x26c,0,Refusal::MissingRoot}, {scene,0x190,0,Refusal::MissingRoot},
        {scene,0x194,0,Refusal::MissingRoot}, {participant,8,0,Refusal::SceneBackReference},
        {item,0,0,Refusal::ItemType}, {item,8,0,Refusal::ItemType},
        {participant,0x10,8,Refusal::FocusMismatch}, {participant,0x14,10,Refusal::FocusMismatch},
        {participant,0x30,1,Refusal::NativeTransaction}, {participant,0x34,1,Refusal::NativeTransaction},
        {participant,0x44,1,Refusal::NativeTransaction}, {item,0x260,0,Refusal::Composition},
        {item,0x264,1,Refusal::Composition}, {item,0x268,1,Refusal::Composition}};
    for (const auto &test : cases) {
        const auto old = detail::word(reinterpret_cast<std::uintptr_t>(test.base),test.offset);
        put(test.base,test.offset,test.value);
        auto result = detail::join(borrow);
        assert(!result.document && result.refusal == test.refusal);
        put(test.base,test.offset,old);
    }
    QTextCursor cursor(doc);
    cursor.beginEditBlock();
    assert(detail::join(borrow).refusal == Refusal::QtTransaction);
    cursor.endEditBlock();
    std::thread foreign([&] { assert(detail::join(borrow).refusal == Refusal::DocumentThread); });
    foreign.join();
    auto a = prepare();
    testing::Access::join(*a, detail::join); // Same production join, owned synthetic layout.
    assert(!a->callback()(borrow));
    assert(a->snapshot().attached);
    a->stop(); doc->~QTextDocument();
}
#endif
int main(int argc, char **argv) {
    QGuiApplication app(argc,argv);
    admissionAndLifetime(); rejectionAndDuplicate(); destruction();
    stopDuringJoin(); stopDuringPublication(); duplicateDuringJoin();
#if defined(__arm__) && defined(__ARM_PCS_VFP)
    nativeLayout();
    std::cout << "scene attachment: 6 lifecycle groups and ARM synthetic layout/refusal matrix passed; ";
#else
    assert(detail::join(nullptr).refusal == Refusal::Unsupported);
    std::cout << "scene attachment: 6 lifecycle groups passed; native layout not exercised; ";
#endif
    std::cout << "Qt " << qVersion() << "; no native callback executed\n";
}
