// Original owned Qt composition controls. No target binary or native calls.
#define READER_IDENTITY_CAPTURE_TESTING
#define VISUAL_CAPTURE_TESTING
#include "reader_identity_capture.h"
#define main selected_view_fixture_uninvoked_main
#include "reader_selected_view_fixture.cpp"
#undef main
#include <QBuffer>
#include <QEvent>
#include <QImageReader>
#include <QTemporaryFile>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
using namespace reader_identity_capture;
unsigned checks = 0, scenarios = 0;
void check(bool value, const char *message) {
    ++checks; if (!value) throw std::runtime_error(message);
}
void scenario(const char *name, const std::function<void()> &body) {
    const auto only = qEnvironmentVariable("READER_IDENTITY_CAPTURE_ONLY");
    if (!only.isEmpty() && only != QLatin1String(name)) return;
    ++scenarios;
    try { body(); } catch (const std::exception &e) { std::cerr << name << ": " << e.what() << '\n'; throw; }
}
QString uuid(int i) { return QStringLiteral("00000000-0000-4000-8000-%1").arg(i, 12, 16, QLatin1Char('0')); }
const Guard yes = [] { return true; };
struct Owned {
    Fixture fixture{false, true};
    Library library;
    DocumentController creation;
    Expected expected;
    Owned() {
        fixture.document.pages = {uuid(1), uuid(2), uuid(3)};
        fixture.device->setProperty("page", 1);
        fixture.controller.setPageId(uuid(2));
        emit fixture.document.pageMapChanged();
        emit fixture.document.pageCountChanged();
        creation.setLibrary(&library); library.result = &fixture.document;
        fixture.engine.rootContext()->setContextProperty("externalCreation", &creation);
        fixture.window.resize(160, 120);
        QCoreApplication::processEvents();
        expected = {fixture.document.key, uuid(2), 1, 3};
    }
    reader_selected::Observation sample() {
        return reader_selected::observeCreationJoin(&fixture.window, yes, 20000);
    }
};
struct Output {
    QTemporaryFile file;
    std::unique_ptr<visual_capture::Sink> sink;
    explicit Output(bool badFd = false) {
        check(file.open(), "owned output created before sample");
        sink = std::make_unique<visual_capture::Sink>(badFd ? -1 : file.handle());
    }
    QByteArray bytes() { check(file.flush(), "output flush"); check(file.seek(0), "output seek"); return file.readAll(); }
};
QImage pixels() {
    QImage image(32, 24, QImage::Format_ARGB32);
    image.fill(QColor(17, 43, 91)); image.setPixelColor(3, 7, QColor(211, 19, 31));
    return image;
}
void positive(const Result &r) {
    check(r.captured && r.attempted && !r.unknown && r.refusal.isEmpty(), "accepted capture conjunction");
    check(r.before.observed && r.after.observed && r.before.creationJoinObserved && r.after.creationJoinObserved,
          "two independent selected wrapper joins");
    check(!r.nativeAuthority && !r.renderAuthority && !r.durableAuthority, "scalar evidence has no authority");
}
void negative(const Result &r, int grabs, int maximum = 1) {
    check(!r.captured && !r.refusal.isEmpty(), "refusal explicit");
    check(grabs <= maximum, "bounded grab count");
    check(r.unknown == r.attempted, "consumed unsuccessful image remains unknown");
    check(!r.nativeAuthority && !r.renderAuthority && !r.durableAuthority, "refusal has no authority");
}
// The sampler below is explicitly an owned timing model. The capture core,
// focus/window/root checks, image quotas, encoder and Sink remain real.
struct BorrowStats { int calls = 0, late = 0; };
struct Model {
    Owned &owned;
    reader_selected::Observation baseline;
    int samples = 0;
    BorrowStats borrowed;
    unsigned actionCount = 1;
    std::function<void(int, reader_selected::Observation &)> before;
    std::function<void(int, reader_selected::Observation &)> after;
    std::function<bool(int)> borrowPermit;
    QObject *reader = nullptr, *document = nullptr, *controller = nullptr;
    explicit Model(Owned &o) : owned(o), baseline(o.sample()), reader(o.fixture.reader),
        document(&o.fixture.document), controller(&o.creation) {
        check(baseline.observed && baseline.ready && baseline.creationJoinObserved, "real baseline for modeled sampler");
    }
    TestBoundary::Sampler sampler() {
        return [this](QQuickWindow *w, const Guard &permit, int ms, const reader_selected::ValidatedSourceAction &action) {
            check(w == &owned.fixture.window && ms > 0 && ms <= 20000, "modeled boundary exact window and remaining budget");
            const int number = ++samples;
            auto value = baseline;
            if (before) before(number, value);
            auto active = std::make_shared<bool>(true);
            const Guard guard = [this, active, number, permit] {
                ++borrowed.calls;
                if (!*active) { ++borrowed.late; return false; }
                return (!borrowPermit || borrowPermit(number)) && permit();
            };
            const auto done = qScopeGuard([active] { *active = false; });
            if (permit()) for (unsigned i = 0; i < actionCount; ++i)
                action(reader, document, controller, QVariant(owned.fixture.document.id()), value, guard);
            if (after) after(number, value);
            return value;
        };
    }
};
TestBoundary fake(int &grabs) {
    TestBoundary t; t.grab = [&](QQuickWindow *) { ++grabs; return pixels(); }; return t;
}
void validatePng(Output &output, const Result &r) {
    const auto encoded = output.bytes();
    check(encoded.size() == r.encoded && r.encoded > 0 && r.encoded <= visual_capture::PngCap, "encoded byte count");
    check(QCryptographicHash::hash(encoded, QCryptographicHash::Sha256).toHex() == r.sha, "exact PNG digest");
    QImage decoded; check(decoded.loadFromData(encoded, "PNG"), "full encoded PNG decode");
    check(decoded.size() == QSize(r.width, r.height) && r.width == 32 && r.height == 24, "decoded dimensions");
    check(decoded.pixelColor(0, 0) == QColor(17, 43, 91) && decoded.pixelColor(3, 7) == QColor(211, 19, 31), "known owned pixels preserved");
    check(r.storage == 32 * 24 * 4 && r.dpr == 1, "image storage and DPR");
}
}

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    // The pinned host image omits optional QML module files. This original
    // QObject registration supplies only the owned fixed helper's QtObject.
    qmlRegisterType<QObject>("QtQml", 2, 0, "QtObject");
    qmlRegisterModule("QtQml.WorkerScript", 2, 0);
    qmlRegisterModule("QtQml.Models", 2, 0);
    qmlRegisterModule("QtQml.WorkerScript", QT_VERSION_MAJOR, QT_VERSION_MINOR);
    qmlRegisterModule("QtQml.Models", QT_VERSION_MAJOR, QT_VERSION_MINOR);
    qmlRegisterType<QObject>("OwnedProbe", 1, 0, "QtObject");
    qmlRegisterType<QQuickItem>("OwnedProbe", 1, 0, "Item");
    qmlRegisterType<FocusFixture>("OwnedProbe", 1, 0, "FocusScope");
    qmlRegisterType<SceneView>("OwnedProbe", 1, 0, "SceneFixture");
    registerCreationFixture();
    try {
        scenario("real-samples-known-pixels", [] {
            Owned o; Output output; int grabs = 0; const auto hooks = fake(grabs);
            const int lookups = o.library.calls;
            auto claim = std::make_shared<Claim>();
            const auto r = run(&o.fixture.window, o.expected, *output.sink, claim, yes, 20000, hooks);
            if (!r.captured) {
                std::cerr << "actual sample refusal " << r.refusal.toStdString() << " before=" << r.before.refusal.toStdString() << " after=" << r.after.refusal.toStdString() << '\n';
                QQmlComponent diagnostic(&o.fixture.engine);
                diagnostic.setData("import QtQml\nimport xofm.libs.library\nQtObject { readonly property QtObject observedController: DocumentController }", QUrl::fromLocalFile(QStringLiteral("/__owned_diagnostic__.qml")));
                for (const auto &error : diagnostic.errors()) std::cerr << error.toString().toStdString() << '\n';
            }
            positive(r); check(grabs == 1 && o.library.calls == lookups + 2, "one image and exactly two actual native-shaped owned lookups");
            validatePng(output, r);
            Output second;
            const auto again = run(&o.fixture.window, o.expected, *second.sink, claim, yes, 20000, hooks);
            negative(again, grabs); check(!again.attempted && grabs == 1 && second.sink->count() == 0, "shared claim never reacquires");
        });
        scenario("real-grab-diagnostic", [] {
            Owned o; Output output;
            // Empty TestBoundary uses both real Sample and real grabWindow.
            const auto r = run(&o.fixture.window, o.expected, *output.sink, std::make_shared<Claim>(), yes);
            if (r.captured) {
                positive(r); const auto bytes = output.bytes(); QImage decoded;
                check(decoded.loadFromData(bytes, "PNG") && decoded.width() == r.width && decoded.height() == r.height, "real grab decoded");
                check(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex() == r.sha, "real grab digest");
            } else {
                negative(r, 0); check(r.attempted, "real grab entered strict epoch capture control");
            }
            std::cout << "REAL_GRAB captured=" << r.captured << " attempted=" << r.attempted
                      << " encoded=" << r.encoded << " sink_bytes=" << output.sink->count()
                      << " refusal=" << r.refusal.toStdString() << '\n';
        });
        scenario("modeled-positive-scalar-lifetime", [] {
            Result saved;
            { Owned o; Model m(o); Output output; int grabs = 0; auto t = fake(grabs); t.sample = m.sampler();
              saved = run(&o.fixture.window, o.expected, *output.sink, std::make_shared<Claim>(), yes, 20000, t);
              positive(saved); validatePng(output, saved); check(m.samples == 2 && m.borrowed.late == 0, "borrowed guards used only synchronously"); }
            check(saved.captured && saved.before.pageId == uuid(2) && saved.after.pageId == uuid(2) && saved.sha.size() == 64, "result survives all source owners with scalar values only");
        });
        const std::vector<std::function<void(Expected &)>> badExpected = {
            [](auto &e) { e.documentId = uuid(777); }, [](auto &e) { e.pageId = uuid(777); },
            [](auto &e) { e.pageIndex = 0; }, [](auto &e) { e.pageCount = 4; },
            [](auto &e) { e.documentId = QStringLiteral("bad"); }, [](auto &e) { e.pageId = QString(36, QLatin1Char('0')); },
            [](auto &e) { e.pageCount = 0; }, [](auto &e) { e.pageCount = 17; },
            [](auto &e) { e.pageIndex = -1; }, [](auto &e) { e.pageIndex = 3; },
            [](auto &e) { e.pageId = QStringLiteral("00000000-0000-0000-0000-000000000000"); }
        };
        for (const auto &change : badExpected) scenario("expected-refusal", [&] {
            Owned o; Output output; int grabs = 0; auto t = fake(grabs); auto expected = o.expected; change(expected);
            const auto r = run(&o.fixture.window, expected, *output.sink, std::make_shared<Claim>(), yes, 20000, t);
            negative(r, grabs, 0); check(!r.attempted && output.sink->count() == 0, "wrong identity/config before capture");
        });
        const std::vector<std::function<void(reader_selected::Observation &)>> badJoin = {
            [](auto &s) { s.observed = false; }, [](auto &s) { s.ready = false; }, [](auto &s) { s.mapCoherent = false; },
            [](auto &s) { s.creationJoinObserved = false; }, [](auto &s) { s.documentId = uuid(99); },
            [](auto &s) { s.pageId = uuid(99); }, [](auto &s) { ++s.pageIndex; }, [](auto &s) { ++s.pageCount; },
            [](auto &s) { s.loading = true; }, [](auto &s) { s.pageError = true; }, [](auto &s) { s.documentError = true; },
            [](auto &s) { s.awaitingTiles = true; }, [](auto &s) { s.pendingTiles = true; }, [](auto &s) { s.working = true; },
            [](auto &s) { s.refusal = QStringLiteral("owned-refusal"); }, [](auto &s) { s.status = -1; }
        };
        for (int phase : {1, 2}) for (const auto &change : badJoin) scenario("modeled-join-refusal", [&] {
            Owned o; Model m(o); Output output; int grabs = 0; auto t = fake(grabs);
            m.before = [&](int n, auto &s) { if (n == phase) change(s); }; t.sample = m.sampler();
            const auto r = run(&o.fixture.window, o.expected, *output.sink, std::make_shared<Claim>(), yes, 20000, t);
            negative(r, grabs, phase == 1 ? 0 : 1); check(m.borrowed.late == 0, "no expired borrowed guard on refusal");
            if (phase == 2) check(grabs == 1 && output.sink->count() > 0, "fresh failure leaves orphan forensic bytes only");
        });
        for (int phase : {1, 2}) for (unsigned count : {0u, 2u}) scenario("modeled-callback-count", [&] {
            Owned o; Model m(o); Output output; int grabs = 0; auto t = fake(grabs);
            m.before = [&](int n, auto &) { m.actionCount = n == phase ? count : 1; }; t.sample = m.sampler();
            const auto r = run(&o.fixture.window, o.expected, *output.sink, std::make_shared<Claim>(), yes, 20000, t);
            negative(r, grabs); check(m.borrowed.late == 0, "missing/duplicate action never escapes guard");
        });
        for (int phase : {1, 2}) scenario("modeled-return-action-disagreement", [&] {
            Owned o; Model m(o); Output output; int grabs = 0; auto t = fake(grabs);
            m.after = [&](int n, auto &s) { if (n == phase) ++s.status; }; t.sample = m.sampler();
            const auto r = run(&o.fixture.window, o.expected, *output.sink, std::make_shared<Claim>(), yes, 20000, t);
            negative(r, grabs); check(grabs == 1, "valid action cannot relabel differing returned scalar");
        });
        for (int which = 0; which < 3; ++which) scenario("modeled-owner-replaced", [which] {
            Owned o; Model m(o); QObject replacement; Output output; int grabs = 0; auto t = fake(grabs);
            m.before = [&](int n, auto &) { if (n == 2) { if (which == 0) m.reader = &replacement; if (which == 1) m.document = &replacement; if (which == 2) m.controller = &replacement; } };
            t.sample = m.sampler(); const auto r = run(&o.fixture.window, o.expected, *output.sink, std::make_shared<Claim>(), yes, 20000, t);
            negative(r, grabs); check(grabs == 1 && m.borrowed.late == 0, "fresh same logical ID cannot substitute different owner");
        });
        for (int phase : {1, 2}) scenario("modeled-borrowed-refusal", [phase] {
            Owned o; Model m(o); Output output; int grabs = 0; auto t = fake(grabs);
            m.borrowPermit = [phase](int n) { return n != phase; }; t.sample = m.sampler();
            const auto r = run(&o.fixture.window, o.expected, *output.sink, std::make_shared<Claim>(), yes, 20000, t);
            negative(r, grabs, phase == 1 ? 0 : 1); check(m.borrowed.late == 0, "borrowed refusal no later use");
        });
        for (int which = 0; which < 4; ++which) scenario("grab-refusal", [which] {
            Owned o; Output output(which == 3); int grabs = 0; TestBoundary t;
            t.grab = [&](QQuickWindow *) -> QImage { ++grabs; if (which == 1) throw std::runtime_error("owned-grab"); if (which == 2) { QImage big(1873, 1, QImage::Format_RGB32); big.fill(Qt::white); return big; } return which == 0 ? QImage() : pixels(); };
            auto claim = std::make_shared<Claim>(); const auto r = run(&o.fixture.window, o.expected, *output.sink, claim, yes, 20000, t);
            negative(r, grabs); check(grabs == 1 && claim->consumed.load(), "failed/throwing/oversize/encoding grab consumed once");
        });
        scenario("real-sample-event-epoch", [] {
            Owned o; Output output; int grabs = 0; TestBoundary t;
            t.grab = [&](QQuickWindow *) { ++grabs; QEvent event(QEvent::User); QCoreApplication::sendEvent(o.fixture.reader, &event); return pixels(); };
            const auto r = run(&o.fixture.window, o.expected, *output.sink, std::make_shared<Claim>(), yes, 20000, t);
            negative(r, grabs); check(grabs == 1 && !r.captured, "delivered QEvent during grab poisons actual Sample epoch");
        });
        scenario("real-sample-map-notification", [] {
            Owned o; Output output; int grabs = 0; TestBoundary t;
            t.grab = [&](QQuickWindow *) { ++grabs; emit o.fixture.document.pageMapChanged(); return pixels(); };
            const auto r = run(&o.fixture.window, o.expected, *output.sink, std::make_shared<Claim>(), yes, 20000, t);
            negative(r, grabs); check(grabs == 1, "map signal during grab invalidates actual borrowed guard");
        });
        scenario("caller-loss-during-grab-sticky", [] {
            Owned o; Output output; int grabs = 0, permitCalls = 0; bool live = true;
            auto t = fake(grabs); t.grab = [&](QQuickWindow *) { ++grabs; live = false; return pixels(); };
            const auto r = run(&o.fixture.window, o.expected, *output.sink, std::make_shared<Claim>(), [&] { ++permitCalls; if (!live) { live = true; return false; } return true; }, 20000, t);
            negative(r, grabs); check(grabs == 1 && permitCalls > 0, "one false permission remains terminal despite later true");
        });
        scenario("guard-throw", [] {
            Owned o; Output output; int grabs = 0; auto t = fake(grabs);
            const auto r = run(&o.fixture.window, o.expected, *output.sink, std::make_shared<Claim>(), []() -> bool { throw std::runtime_error("owned-permit"); }, 20000, t);
            negative(r, grabs, 0); check(!r.attempted, "pre-sample guard exception fail closed");
        });
        scenario("deadline-during-grab", [] {
            Owned o; Model m(o); Output output; int grabs = 0; auto t = fake(grabs); const auto start = Clock::now(); auto clock = start;
            // Modeled sampling isolates the injected helper clock: a real ARM
            // census may correctly exhaust a 10ms budget before reaching grab.
            t.sample = m.sampler(); t.now = [&] { return clock; }; t.grab = [&](QQuickWindow *) { ++grabs; clock = start + std::chrono::milliseconds(10); return pixels(); };
            const auto r = run(&o.fixture.window, o.expected, *output.sink, std::make_shared<Claim>(), yes, 10, t);
            negative(r, grabs); check(grabs == 1, "original cooperative deadline not renewed after grab");
        });
        scenario("deadline-between-joins", [] {
            Owned o; Model m(o); Output output; int grabs = 0; auto t = fake(grabs); const auto start = Clock::now(); auto clock = start;
            m.after = [&](int n, auto &) { if (n == 1) clock += std::chrono::milliseconds(10); }; t.sample = m.sampler(); t.now = [&] { return clock; };
            const auto r = run(&o.fixture.window, o.expected, *output.sink, std::make_shared<Claim>(), yes, 10, t);
            negative(r, grabs); check(m.samples == 1 && grabs == 1, "expired original budget prevents fresh join");
        });
        scenario("reentry-from-grab", [] {
            Owned o; Output output, nestedOutput; int grabs = 0; auto t = fake(grabs); auto claim = std::make_shared<Claim>();
            t.grab = [&](QQuickWindow *) { ++grabs; const auto nested = run(&o.fixture.window, o.expected, *nestedOutput.sink, claim, yes); check(!nested.captured && nested.refusal == QStringLiteral("reentered"), "nested helper refuses"); return pixels(); };
            const auto r = run(&o.fixture.window, o.expected, *output.sink, claim, yes, 20000, t);
            negative(r, grabs); check(grabs == 1 && nestedOutput.sink->count() == 0, "reentry poisons outer and never grabs twice");
        });
        for (int which = 0; which < 3; ++which) scenario("modeled-owner-death", [which] {
            Owned o; Model m(o); auto owner = std::make_unique<QObject>();
            if (which == 0) m.reader = owner.get();
            if (which == 1) m.document = owner.get();
            if (which == 2) m.controller = owner.get();
            Output output; int grabs = 0; auto t = fake(grabs);
            t.grab = [&](QQuickWindow *) { ++grabs; owner.reset(); return pixels(); }; t.sample = m.sampler();
            const auto r = run(&o.fixture.window, o.expected, *output.sink, std::make_shared<Claim>(), yes, 20000, t);
            negative(r, grabs); check(grabs == 1 && m.borrowed.late == 0, "guarded sampled owner death terminal");
        });
        for (int phase : {1, 2}) for (int which = 0; which < 3; ++which) scenario("modeled-owner-death-after-action", [phase, which] {
            Owned o; Model m(o); auto owner = std::make_unique<QObject>();
            if (which == 0) m.reader = owner.get();
            if (which == 1) m.document = owner.get();
            if (which == 2) m.controller = owner.get();
            Output output; int grabs = 0; auto t = fake(grabs);
            m.after = [&](int n, auto &) { if (n == phase) owner.reset(); }; t.sample = m.sampler();
            const auto r = run(&o.fixture.window, o.expected, *output.sink, std::make_shared<Claim>(), yes, 20000, t);
            negative(r, grabs); check(grabs == 1 && m.samples == phase && m.borrowed.late == 0, "post-Sample unwind destruction refused before next join/acceptance");
        });
        for (int phase : {1, 2}) scenario("modeled-owner-engine-after-action", [phase] {
            Owned o; Model m(o); QQmlEngine foreign; QObject owner;
            m.document = &owner; Output output; int grabs = 0; auto t = fake(grabs);
            m.after = [&](int n, auto &) { if (n == phase) QQmlEngine::setContextForObject(&owner, foreign.rootContext()); }; t.sample = m.sampler();
            const auto r = run(&o.fixture.window, o.expected, *output.sink, std::make_shared<Claim>(), yes, 20000, t);
            negative(r, grabs); check(grabs == 1 && m.samples == phase && qmlEngine(&owner) == &foreign, "post-action foreign engine rejected");
        });
        for (int phase : {1, 2}) scenario("modeled-owner-thread-after-action", [phase, &app] {
            Owned o; Model m(o); QObject owner; QThread worker; worker.start();
            const auto cleanup = qScopeGuard([&] {
                if (owner.thread() != app.thread()) QMetaObject::invokeMethod(&owner, [&] { owner.moveToThread(app.thread()); }, Qt::BlockingQueuedConnection);
                worker.quit(); worker.wait();
            });
            m.document = &owner; Output output; int grabs = 0; auto t = fake(grabs);
            m.after = [&](int n, auto &) { if (n == phase) owner.moveToThread(&worker); }; t.sample = m.sampler();
            const auto r = run(&o.fixture.window, o.expected, *output.sink, std::make_shared<Claim>(), yes, 20000, t);
            negative(r, grabs); check(grabs == 1 && m.samples == phase && owner.thread() == &worker, "post-action foreign thread rejected");
        });
        scenario("fresh-owner-unwind-regression", [] {
            Owned o; Model m(o); auto owner = std::make_unique<QObject>(); m.document = owner.get();
            Output output; int grabs = 0; auto t = fake(grabs);
            m.after = [&](int n, auto &) { if (n == 2) { owner.reset(); m.document = nullptr; } }; t.sample = m.sampler();
            const auto r = run(&o.fixture.window, o.expected, *output.sink, std::make_shared<Claim>(), yes, 20000, t);
            negative(r, grabs); check(m.samples == 2 && grabs == 1, "second Sample unwind owner loss cannot accept image");
        });
        for (int phase : {1, 2}) scenario("modeled-borrow-callout-owner-death", [phase] {
            Owned o; Model m(o); auto owner = std::make_unique<QObject>(); m.document = owner.get();
            Output output; int grabs = 0; auto t = fake(grabs);
            m.borrowPermit = [&](int n) { if (n == phase) { owner.reset(); m.document = nullptr; } return true; }; t.sample = m.sampler();
            const auto r = run(&o.fixture.window, o.expected, *output.sink, std::make_shared<Claim>(), yes, 20000, t);
            negative(r, grabs, phase == 1 ? 0 : 1); check(m.samples == phase && m.borrowed.late == 0, "borrowed permit cannot delete a sampled owner and still admit");
        });
        for (int kind = 0; kind < 4; ++kind) scenario("modeled-real-boundary-loss", [kind] {
            auto original = std::make_unique<Owned>();
            const auto observation = original->sample(); const auto expected = original->expected;
            check(observation.observed, "real baseline before rare lifetime model");
            original.reset(); // Avoid unrelated live-window engine ambiguity.
            auto engine = std::make_unique<QQmlEngine>();
            auto window = std::make_unique<QQuickWindow>();
            QQmlEngine::setContextForObject(window.get(), engine->rootContext());
            window->resize(64, 48); window->show(); window->requestActivate(); QCoreApplication::processEvents();
            QObject reader, document, controller; Output output; int grabs = 0, samples = 0;
            TestBoundary t;
            t.sample = [&](QQuickWindow *, const Guard &guard, int, const reader_selected::ValidatedSourceAction &action) {
                ++samples; action(&reader, &document, &controller, QVariant(), observation, guard); return observation;
            };
            t.grab = [&](QQuickWindow *) {
                ++grabs;
                if (kind == 0) window.reset(); // This also destroys its real content root.
                if (kind == 1) engine.reset();
                if (kind == 2) output.sink.reset();
                if (kind == 3) window->hide();
                return pixels();
            };
            const auto r = run(window.get(), expected, *output.sink, std::make_shared<Claim>(), yes, 20000, t);
            negative(r, grabs); check(grabs == 1 && samples == 1, "actual window/root, engine, Sink or visibility loss prevents fresh join");
        });
        scenario("permission-reentry-before-sample", [] {
            Owned o; Output output, nestedOutput; int grabs = 0; bool nested = false; auto t = fake(grabs);
            auto claim = std::make_shared<Claim>();
            const auto r = run(&o.fixture.window, o.expected, *output.sink, claim, [&] {
                if (!nested) { nested = true; const auto child = run(&o.fixture.window, o.expected, *nestedOutput.sink, claim, yes);
                    check(!child.attempted && child.refusal == QStringLiteral("reentered"), "permission callout nested request refused"); }
                return true;
            }, 20000, t);
            negative(r, grabs, 0); check(!r.attempted && !claim->consumed.load() && o.library.calls == 0, "pre-sample reentry poisons outer without taking claim");
        });
        scenario("input-copies-and-shared-claim-lifetime", [] {
            Owned o; Output output; int grabs = 0; auto expected = o.expected; auto claim = std::make_shared<Claim>(); auto retained = claim;
            auto t = fake(grabs); t.grab = [&](QQuickWindow *) { ++grabs; claim.reset(); expected.pageId = uuid(900); return pixels(); };
            const auto r = run(&o.fixture.window, expected, *output.sink, claim, yes, 20000, t);
            positive(r); check(grabs == 1 && retained->consumed.load() && !claim && r.after.pageId == o.expected.pageId, "owned expectation and shared claim survive caller mutation");
        });
        for (int ms : {0, -1, 20001}) scenario("invalid-budget", [ms] {
            Owned o; Output output; int grabs = 0; auto t = fake(grabs);
            const auto r = run(&o.fixture.window, o.expected, *output.sink, std::make_shared<Claim>(), yes, ms, t);
            negative(r, grabs, 0); check(!r.attempted, "invalid budget cannot consume claim");
        });
        scenario("null-window", [] { Output output; int grabs = 0; auto t = fake(grabs); const auto r = run(nullptr, {uuid(100), uuid(2), 1, 3}, *output.sink, std::make_shared<Claim>(), yes, 20000, t); negative(r, grabs, 0); check(!r.attempted, "null window before claim"); });
        scenario("null-claim", [] { Owned o; Output output; int grabs = 0; auto t = fake(grabs); const auto r = run(&o.fixture.window, o.expected, *output.sink, {}, yes, 20000, t); negative(r, grabs, 0); check(!r.attempted, "null claim before capture"); });
        scenario("nonempty-sink", [] { Owned o; Output output; check(output.sink->write("owned", 5) == 5, "preexisting output"); int grabs = 0; auto t = fake(grabs); const auto r = run(&o.fixture.window, o.expected, *output.sink, std::make_shared<Claim>(), yes, 20000, t); negative(r, grabs, 0); check(!r.attempted, "nonempty sink before claim"); });
        scenario("used-claim", [] { Owned o; Output output; auto c = std::make_shared<Claim>(); c->consumed.store(true); int grabs = 0; auto t = fake(grabs); const auto r = run(&o.fixture.window, o.expected, *output.sink, c, yes, 20000, t); negative(r, grabs, 0); check(!r.attempted, "used claim cannot acquire"); });
        std::cout << "PASS identity capture owned scenarios=" << scenarios << " checks=" << checks
                  << " actual_selected_samples=true real_capture_core=true modeled_boundaries_explicit=true native_authority=false render_authority=false durable_authority=false\n";
    } catch (const std::exception &) { return 1; }
    return 0;
}
