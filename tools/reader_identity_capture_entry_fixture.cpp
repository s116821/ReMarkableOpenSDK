// Real owned Entry files/navigation; capture producer is explicitly injected.
// The real two-Sample/capture composition has its own separate fixture.
#define READER_ENTRY_OWNED_HELPERS_ONLY
#include "reader_selected_view_entry_fixture.cpp"
#undef READER_ENTRY_OWNED_HELPERS_ONLY
#include <QImage>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
using Entry = reader_selected::Entry;
using Capture = reader_identity_capture::Result;
using Expected = reader_identity_capture::Expected;
unsigned scenarios = 0, checks = 0;
void check(bool value, const char *message) { ++checks; if (!value) throw std::runtime_error(message); }
void scenario(const char *name, const std::function<void()> &body) {
    ++scenarios; try { body(); } catch (const std::exception &e) { std::cerr << name << ": " << e.what() << '\n'; throw; }
}
QString id(int number) { return QStringLiteral("00000000-0000-4000-8000-%1").arg(number, 12, 16, QLatin1Char('0')); }
QImage pixels() { QImage image(24, 16, QImage::Format_ARGB32); image.fill(QColor(31, 67, 101)); image.setPixelColor(2, 3, QColor(199, 11, 23)); return image; }
reader_selected::Observation observation(const Expected &e) {
    reader_selected::Observation r;
    r.observed = r.mapCoherent = r.creationJoinObserved = r.ready = true;
    r.loading = r.pageError = r.documentError = r.awaitingTiles = r.pendingTiles = r.working = false;
    r.documentId = e.documentId; r.pageId = e.pageId; r.pageIndex = e.pageIndex; r.pageCount = e.pageCount;
    r.status = int(SceneView::Active); r.pageStatus = int(SceneView::Ready); return r;
}
bool fileStat(const QString &root, const char *name, struct stat &s) {
    return ::lstat(QFile::encodeName(root + '/' + QLatin1String(name)).constData(), &s) == 0;
}
bool privateFile(const QString &root, const char *name, nlink_t links = 1) {
    struct stat s{}; return fileStat(root, name, s) && S_ISREG(s.st_mode) && (s.st_mode & 07777) == 0600 && s.st_uid == ::geteuid() && s.st_nlink == links;
}
struct Harness {
    QGuiApplication &app;
    Fixture view{false, true};
    xofm::bindings::qt::WindowNavigatorWrapper navigator;
    xofm::bindings::qt::WindowLayout layout;
    std::unique_ptr<QObject> composition;
    QTemporaryDir directory;
    QString root;
    QByteArray nonce = QByteArray(32, 'c'), identity = reader_selected::selfIdentity();
    Expected expected;
    std::unique_ptr<Entry> entry;
    int producers = 0, publicationChecks = 0, producerBudget = -1;
    Entry::CaptureRunTest producer;
    std::function<void()> publication;
    explicit Harness(QGuiApplication &application, int count = 3) : app(application) {
        check(directory.isValid() && !identity.isEmpty(), "owned entry process/root");
        root = directory.path() + "/root";
        check(QDir().mkdir(root) && !::chmod(QFile::encodeName(root).constData(), 0700), "owner-only entry root");
        putEntry(root + "/owner", nonce); putEntry(root + "/attempt.identity", identity);
        view.document.pages.clear(); for (int i = 0; i < count; ++i) view.document.pages.append(id(i + 1));
        expected = {view.document.key, view.document.pages[1], 1, count};
        view.reader->setVisible(false);
        layout.activeValue = QStringLiteral("legacydevice/window/main");
        view.engine.rootContext()->setContextProperty("entryNavigator", &navigator);
        view.engine.rootContext()->setContextProperty("entryLayout", &layout);
        QQmlComponent component(&view.engine);
        component.setData(R"QML(
import OwnedLegacyEntry 1.0
Owner {
 id: root; width:100; height:100
 windowNavigator:entryNavigator; windowLayout:entryLayout; contentItem:mainView
 Owner { id:mainView; width:100; height:100; windowNavigator:root.windowNavigator; windowLayout:root.windowLayout }
}
)QML", QUrl(QStringLiteral("qrc:/qt/qml/xofm/modules/legacydevice/qml/LegacyMainWindow.qml")));
        check(component.isReady(), "actual lexical legacy composition compiled"); composition.reset(component.create());
        auto *visual = qobject_cast<QQuickItem *>(composition.get()); check(visual, "actual legacy visual root");
        visual->setParentItem(view.window.contentItem()); visual->setParent(view.window.contentItem());
        navigator.onOpen = [&] {
            check(entry && entry->samplingForTest() && producers == 0, "one ordinary open precedes capture");
            check(navigator.route == QStringLiteral("legacydevice/window/main") && navigator.configuration.size() == 2 &&
                navigator.configuration.value("documentId").toString() == expected.documentId &&
                navigator.configuration.value("page").metaType() == QMetaType::fromType<int>() && navigator.configuration.value("page").toInt() == expected.pageIndex,
                "fixed typed document/page bootstrap");
            view.reader->setVisible(true); view.reader->setProperty("currentPage", expected.pageIndex);
            view.controller.setPageId(expected.pageId);
        };
    }
    void create() {
        entry = std::make_unique<Entry>(&app, root, nonce, nullptr, Entry::Diagnostic::NavigateIdentityCapture,
            reader_selected::FixtureNavigation{expected.documentId, expected.pageIndex, true}, reader_joined_creation::Request{}, expected);
        entry->captureRunForTest([&](QQuickWindow *window, const Expected &e, visual_capture::Sink &sink,
                const std::shared_ptr<reader_identity_capture::Claim> &claim, const std::function<bool()> &guard, int budget) {
            ++producers; producerBudget = budget;
            check(window == &view.window && e.documentId == expected.documentId && e.pageId == expected.pageId && e.pageIndex == expected.pageIndex && e.pageCount == expected.pageCount,
                "injected producer receives exact fixed target");
            check(entry && entry->samplingForTest() && budget > 0 && budget <= 20000, "bounded helper stack and remaining budget");
            check(readEntry(root + "/selected-result").isEmpty() && !QFile::exists(root + "/visual-image.png"), "no pair before helper unwinds");
            return producer ? producer(window, e, sink, claim, guard, budget) : produce(e, sink, claim, guard);
        });
        entry->capturePublicationForTest([&] {
            ++publicationChecks;
            if (publication) publication();
        });
    }
    static Capture produce(const Expected &e, visual_capture::Sink &sink, const std::shared_ptr<reader_identity_capture::Claim> &claim,
                           const std::function<bool()> &guard) {
        check(guard && guard(), "producer starts with entry-owned permission"); bool fresh = false;
        check(claim && claim->consumed.compare_exchange_strong(fresh, true), "producer claims exactly one image");
        const auto image = pixels(); check(image.save(&sink, "PNG") && sink.valid(), "producer writes actual bounded PNG to real Sink");
        Capture r; r.captured = r.attempted = true; r.before = r.after = observation(e);
        r.width = image.width(); r.height = image.height(); r.storage = image.sizeInBytes(); r.dpr = image.devicePixelRatio();
        r.encoded = sink.count(); r.sha = sink.digest(); return r;
    }
    void start() {
        create(); check(entry->start(), "capture entry starts"); pumpEntry();
        check(readEntry(root + "/selected-ready") == nonce + ' ' + identity && producers == 0 && navigator.calls == 0,
            "ready before any bootstrap/capture");
    }
    void request(bool bypassWait = true) {
        putEntry(root + "/selected-request", nonce + ' ' + identity); entry->checkForTest(); pumpEntry();
        check(entry && navigator.calls == 1 && entry->navigationPendingForTest() && producers == 0, "request consumed once and queued opportunity pending");
        if (bypassWait) { entry->afterNavigationForTest(); pumpEntry(); }
    }
    QJsonObject receipt() const { return QJsonDocument::fromJson(readEntry(root + "/selected-result")).object(); }
    void terminal() {
        check(entry && entry->terminal() && entry->disarmedForTest(), "capture request terminal and disarmed");
        const auto bytes = readEntry(root + "/selected-result"); const int oldProducers = producers, oldOpen = navigator.calls;
        entry->checkForTest(); entry->afterNavigationForTest(); pumpEntry();
        check(!entry->start() && producers == oldProducers && navigator.calls == oldOpen && readEntry(root + "/selected-result") == bytes,
            "no bootstrap/helper/publication replay");
    }
    void noAcceptedImage() const { check(!QFile::exists(root + "/visual-image.png"), "malformed/refused helper never commits image"); }
    void positivePair() {
        const auto bytes = readEntry(root + "/selected-result"); validReceipt(bytes, nonce, identity, QStringLiteral("identity-image-observed"), 4096);
        const auto json = QJsonDocument::fromJson(bytes).object();
        check(json["schema"].toInt() == 1 && json["diagnostic"] == "identity-capture" && json["accepted"].toBool() && json["captured"].toBool() && json["attempted"].toBool() &&
            !json["unknown"].toBool() && json["refusal"].toString().isEmpty() && json["image_committed"].toBool(), "closed positive purpose/conjunction");
        check(json["navigation_attempted"].toBool() && json["navigation_dispatched"].toBool() && json["navigation_scope"] == "legacy-main-composition", "bootstrap distinguished from capture");
        for (const char *name : {"before", "after"}) {
            const auto o = json[name].toObject();
            check(o["observed"].toBool() && o["map_coherent"].toBool() && o["creation_join"].toBool() && o["ready"].toBool() &&
                o["document_id"].toString() == expected.documentId && o["page_id"].toString() == expected.pageId &&
                o["index"].toInt() == expected.pageIndex && o["count"].toInt() == expected.pageCount, "distinct expected before/after scalar identity");
        }
        check(json["coordinates"] == "full-window" && json["identity_semantics"] == "sampled-before-around-after" &&
            json["content_correspondence"] == "unverified" && json["physical_freshness"] == "unverified" && !json["atomic_authority"].toBool(), "honest capture scope");
        check(privateFile(root, "visual-image.png") && privateFile(root, "selected-result") && !QFile::exists(root + "/.pending-image") && !QFile::exists(root + "/.pending-result"), "closed private single-link pair");
        const auto encoded = readEntry(root + "/visual-image.png"); QImage decoded;
        check(decoded.loadFromData(encoded, "PNG") && decoded == pixels(), "complete PNG decode and owned pixels");
        check(json["width"].toInt() == decoded.width() && json["height"].toInt() == decoded.height() &&
            json["encoded_bytes"].toInt() == encoded.size() && json["output_bytes"].toInt() == encoded.size() &&
            json["sha256"].toString().toLatin1() == QCryptographicHash::hash(encoded, QCryptographicHash::Sha256).toHex(), "original image bytes/dimensions/hash bound");
        check(json["setup_limit_ms"].toInt() == 90000 && json["sample_begin_ms"].toInt() <= json["capture_begin_ms"].toInt() &&
            json["capture_begin_ms"].toInt() <= json["capture_end_ms"].toInt() && json["capture_end_ms"].toInt() <= json["publication_ms"].toInt() &&
            json["publication_ms"].toInt() < json["operation_end_ms"].toInt() && json["operation_end_ms"].toInt() <= json["sample_begin_ms"].toInt() + 30000,
            "original setup/operation/helper/publication brackets");
        check(producers == 1 && navigator.calls == 1 && publicationChecks >= 10, "single bootstrap/producer with distinct fresh publication checks");
        const QString exports = qEnvironmentVariable("READER_IDENTITY_CAPTURE_ENTRY_EXPORT");
        if (!exports.isEmpty()) {
            check(QDir().mkpath(exports) && !::chmod(QFile::encodeName(exports).constData(), 0700), "owned serializer export directory");
            const QString destination = exports + QStringLiteral("/case-%1").arg(scenarios);
            check(QDir().mkdir(destination) && !::chmod(QFile::encodeName(destination).constData(), 0700), "exclusive serializer case directory");
            putEntry(destination + "/selected-result", bytes); // Exact original scalar, not reserialized.
            putEntry(destination + "/visual-image.png", encoded);
            const auto parts = identity.trimmed().split(' ');
            const QJsonObject facts{{"source",QStringLiteral("owned-injected-capture-producer")},
                {"nonce",QString::fromLatin1(nonce)},{"pid",QString::fromLatin1(parts[0])},{"start",QString::fromLatin1(parts[1])},
                {"document_id",expected.documentId},{"page_id",expected.pageId},{"index",expected.pageIndex},{"count",expected.pageCount},
                {"scalar_bytes",bytes.size()},{"scalar_sha256",QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex())},
                {"image_bytes",encoded.size()},{"image_sha256",QString::fromLatin1(QCryptographicHash::hash(encoded,QCryptographicHash::Sha256).toHex())}};
            putEntry(destination + "/expectation.json", QJsonDocument(facts).toJson(QJsonDocument::Compact) + '\n');
            check(readEntry(destination + "/selected-result") == bytes && readEntry(destination + "/visual-image.png") == encoded, "export preserves exact original pair bytes");
        }
    }
};
}

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    qmlRegisterType<QObject>("QtQml", 2, 0, "QtObject");
    qmlRegisterModule("QtQml.WorkerScript", QT_VERSION_MAJOR, QT_VERSION_MINOR);
    qmlRegisterModule("QtQml.Models", QT_VERSION_MAJOR, QT_VERSION_MINOR);
    qmlRegisterType<QObject>("OwnedProbe", 1, 0, "QtObject");
    qmlRegisterType<QQuickItem>("OwnedProbe", 1, 0, "Item");
    qmlRegisterType<FocusFixture>("OwnedProbe", 1, 0, "FocusScope");
    qmlRegisterType<SceneView>("OwnedProbe", 1, 0, "SceneFixture");
    qmlRegisterType<OwnershipOwner>("OwnedLegacyEntry", 1, 0, "Owner"); registerCreationFixture();
    try {
        for (int count : {3, 16}) scenario("positive-pair", [&] { Harness h(app, count); h.start(); h.request(); h.terminal(); h.positivePair(); });
        scenario("real-five-second-opportunity", [&] {
            Harness h(app); h.start(); QElapsedTimer elapsed; elapsed.start(); h.request(false);
            while (h.entry && !h.entry->terminal() && elapsed.elapsed() < 8000) { pumpEntry(); std::this_thread::sleep_for(std::chrono::milliseconds(2)); }
            check(elapsed.elapsed() >= 4900, "actual precise timer opportunity precedes helper"); h.terminal(); h.positivePair();
        });
        const std::vector<std::function<void(Capture &, const std::shared_ptr<reader_identity_capture::Claim> &)>> bad = {
            [](auto &r, const auto &) { r.attempted = false; }, [](auto &r, const auto &) { r.unknown = true; },
            [](auto &r, const auto &) { r.refusal = QStringLiteral("contradictory"); }, [](auto &, const auto &c) { c->consumed.store(false); },
            [](auto &r, const auto &) { r.before.documentId = id(90); }, [](auto &r, const auto &) { r.after.pageId = id(90); },
            [](auto &r, const auto &) { ++r.after.pageIndex; }, [](auto &r, const auto &) { ++r.before.pageCount; },
            [](auto &r, const auto &) { r.before.ready = false; }, [](auto &r, const auto &) { r.after.loading = true; },
            [](auto &r, const auto &) { r.after.mapCoherent = false; }, [](auto &r, const auto &) { r.before.creationJoinObserved = false; },
            [](auto &r, const auto &) { r.after.nativeAuthority = true; }, [](auto &r, const auto &) { r.before.pageError = true; },
            [](auto &r, const auto &) { r.width = 0; }, [](auto &r, const auto &) { r.height = 1873; },
            [](auto &r, const auto &) { r.width = r.height = 1872; }, [](auto &r, const auto &) { r.storage = 0; },
            [](auto &r, const auto &) { r.storage = visual_capture::StorageCap + 1; }, [](auto &r, const auto &) { r.dpr = 0; },
            [](auto &r, const auto &) { r.dpr = std::numeric_limits<double>::infinity(); },
            [](auto &r, const auto &) { ++r.encoded; }, [](auto &r, const auto &) { r.encoded = visual_capture::PngCap + 1; },
            [](auto &r, const auto &) { r.sha = QByteArray(64, 'a'); }, [](auto &r, const auto &) { r.sha = QByteArray(64, 'G'); },
            [](auto &r, const auto &) { r.sha.clear(); }
        };
        for (const auto &mutate : bad) scenario("malformed-positive", [&] {
            Harness h(app); h.producer = [&](auto *, const auto &e, auto &sink, const auto &claim, const auto &guard, int) {
                auto r = Harness::produce(e, sink, claim, guard); mutate(r, claim); return r;
            }; h.start(); h.request(); h.terminal(); h.noAcceptedImage();
            const auto j = h.receipt(); check(!j.isEmpty() && !j["accepted"].toBool() && j["stage"] == "helper-result-refused" && j["refusal"] == "helper-result-contract", "contradictory positive refused before image commit");
            check(privateFile(h.root, ".pending-image"), "malformed PNG remains forensic");
        });
        for (const QByteArray &mode : {QByteArray("no-effect"), QByteArray("unknown"), QByteArray("throw"), QByteArray("partial")}) scenario("helper-failure", [&] {
            Harness h(app); h.producer = [&](auto *, const auto &, auto &sink, const auto &claim, const auto &, int) -> Capture {
                Capture r;
                if (mode == "no-effect") { r.refusal = QStringLiteral("owned-no-effect"); return r; }
                claim->consumed.store(true); check(sink.write("partial", 7) == 7, "real partial output");
                if (mode == "throw") throw std::runtime_error("owned-producer");
                r.attempted = r.unknown = true; r.refusal = QStringLiteral("owned-unknown"); return r;
            }; h.start(); h.request(); h.terminal(); h.noAcceptedImage(); const auto j = h.receipt();
            check(!j.isEmpty() && !j["accepted"].toBool() && !j["captured"].toBool() && j["unknown"].toBool() == (mode != "no-effect"), "no-effect distinguished from consumed unknown");
            check(privateFile(h.root, ".pending-image") && readEntry(h.root + "/.pending-image").size() == (mode == "no-effect" ? 0 : 7), "partial/missing image only forensic");
        });
        for (const QByteArray &mode : {QByteArray("cancel"), QByteArray("restore"), QByteArray("expiry"), QByteArray("operation"), QByteArray("replace"), QByteArray("quit")}) scenario("helper-unwind-stop", [&] {
            Harness h(app); h.producer = [&](auto *, const auto &e, auto &sink, const auto &claim, const auto &guard, int) {
                auto r = Harness::produce(e, sink, claim, guard);
                if (mode == "expiry") h.entry->expireForTest();
                else if (mode == "operation") h.entry->advanceForTest(30001);
                else if (mode == "replace") { check(QFile::rename(h.root + "/selected-request", h.root + "/retained-request"), "pin old request"); putEntry(h.root + "/selected-request", h.nonce + ' ' + h.identity); }
                else if (mode == "quit") check(QMetaObject::invokeMethod(&app, "aboutToQuit", Qt::DirectConnection) && !QCoreApplication::closingDown(), "actual shutdown signal while helper stack remains live");
                else putEntry(h.root + (mode == "cancel" ? "/selected-cancel" : "/restore.claim"), "stop");
                h.entry->checkForTest(); pumpEntry();
                check(h.entry->samplingForTest() && !h.entry->terminal() && readEntry(h.root + "/selected-result").isEmpty(), "nested interruption never publishes on helper stack");
                return r;
            }; h.start(); h.request(); h.terminal(); h.noAcceptedImage(); check(readEntry(h.root + "/selected-result").isEmpty(), "stop/expiry suppresses final pair");
        });
        scenario("entry-death-on-helper-stack", [&] {
            Harness h(app); h.producer = [&](auto *, const auto &e, auto &sink, const auto &claim, const auto &guard, int) {
                auto r = Harness::produce(e, sink, claim, guard); h.entry.reset(); return r;
            }; h.start(); putEntry(h.root + "/selected-request", h.nonce + ' ' + h.identity); h.entry->checkForTest(); pumpEntry();
            check(h.entry && h.entry->navigationPendingForTest(), "queued before destructive helper"); h.entry->afterNavigationForTest(); pumpEntry();
            check(!h.entry && h.producers == 1 && privateFile(h.root, ".pending-image") && !QFile::exists(h.root + "/selected-result") && !QFile::exists(h.root + "/visual-image.png"), "local Sink and owned Files outlive Entry then close without publishing");
        });
        for (int phase = 0; phase < 5; ++phase) for (const QByteArray &mode : {QByteArray("cancel"), QByteArray("expiry"), QByteArray("operation"), QByteArray("replace"), QByteArray("death"), QByteArray("quit")}) scenario("publication-boundary", [&] {
            Harness h(app); bool acted = false; int quits = 0;
            const auto connection = QObject::connect(&app, &QCoreApplication::aboutToQuit, &app, [&] { ++quits; }, Qt::DirectConnection);
            const auto disconnect = qScopeGuard([&] { QObject::disconnect(connection); });
            h.publication = [&] {
                struct stat image{}, result{}, pendingImage{}, pendingResult{};
                const bool i = fileStat(h.root, "visual-image.png", image), r = fileStat(h.root, "selected-result", result);
                const bool pi = fileStat(h.root, ".pending-image", pendingImage), pr = fileStat(h.root, ".pending-result", pendingResult);
                const bool selected = phase == 0 ? pi && !i : phase == 1 ? pi && i && image.st_nlink == 2 :
                    phase == 2 ? !pi && i && !pr && !r : phase == 3 ? pr && !r : pr && r && result.st_nlink == 2;
                if (acted || !selected) return;
                acted = true;
                if (mode == "death") h.entry.reset();
                else if (mode == "expiry") h.entry->expireForTest();
                else if (mode == "operation") h.entry->advanceForTest(30001);
                else if (mode == "replace") { check(QFile::rename(h.root + "/selected-request", h.root + "/retained-request"), "publication request inode replacement"); putEntry(h.root + "/selected-request", h.nonce + ' ' + h.identity); }
                else if (mode == "quit") check(QMetaObject::invokeMethod(&app, "aboutToQuit", Qt::DirectConnection) && !QCoreApplication::closingDown(), "shutdown signal before destruction still suppresses publication");
                else putEntry(h.root + "/selected-cancel", "stop");
            };
            h.start(); putEntry(h.root + "/selected-request", h.nonce + ' ' + h.identity); h.entry->checkForTest(); pumpEntry();
            check(h.entry && h.entry->navigationPendingForTest(), "publication destructive boundary queued"); h.entry->afterNavigationForTest(); pumpEntry();
            check(acted && h.producers == 1 && h.navigator.calls == 1, "fresh publication hook reached exact filesystem phase");
            if (mode == "quit") check(quits == 1, "actual app signal delivered once at publication phase");
            if (h.entry) h.terminal();
            if (phase == 0) check(!QFile::exists(h.root + "/visual-image.png") && privateFile(h.root, ".pending-image"), "prelink refusal preserves single pending image");
            if (phase == 1) check(privateFile(h.root, ".pending-image", 2) && privateFile(h.root, "visual-image.png", 2), "image nlink2 uncertainty preserved");
            if (phase == 2) check(privateFile(h.root, "visual-image.png") && !QFile::exists(h.root + "/selected-result"), "committed image alone orphan");
            if (phase == 3) check(privateFile(h.root, ".pending-result") && !QFile::exists(h.root + "/selected-result"), "pending scalar forensic");
            if (phase == 4) check(privateFile(h.root, ".pending-result", 2) && privateFile(h.root, "selected-result", 2), "scalar linked uncertainty remains invalid pair");
        });
        scenario("publication-reentry", [&] {
            Harness h(app); int reentries = 0; h.publication = [&] { ++reentries; check(h.entry->terminal() && h.entry->disarmedForTest(), "publication permission independent of active Sample"); h.entry->checkForTest(); h.entry->afterNavigationForTest(); check(!h.entry->start(), "public rearm refused during publication"); pumpEntry(); };
            h.start(); h.request(); h.terminal(); h.positivePair(); check(reentries > 5, "reentrant watcher events never duplicate operation");
        });
        scenario("queued-original-deadline", [&] { Harness h(app); h.start(); h.request(false); h.entry->advanceForTest(30001); h.entry->afterNavigationForTest(); pumpEntry(); h.terminal(); check(h.producers == 0 && !QFile::exists(h.root + "/selected-result") && !QFile::exists(h.root + "/.pending-image"), "queued delay consumes original thirty seconds"); });
        scenario("queued-budget-clamp", [&] { Harness h(app); h.start(); h.request(false); h.entry->advanceForTest(22000); h.entry->afterNavigationForTest(); pumpEntry(); h.terminal(); h.positivePair(); check(h.producerBudget > 0 && h.producerBudget <= 8000, "remaining operation budget clamps helper instead of resetting"); });
        scenario("setup-before-request-clamp", [&] { Harness h(app); h.start(); h.entry->advanceForTest(81000); h.request(); h.terminal(); h.positivePair(); check(h.producerBudget > 0 && h.producerBudget <= 9000 && h.receipt()["operation_end_ms"].toInt() == 90000, "original setup deadline clamps whole operation"); });
        for (const char *name : {"visual-image.png", ".pending-image", "selected-result", ".pending-result"}) scenario("stale-output-start", [&] { Harness h(app); putEntry(h.root + '/' + QLatin1String(name), "stale"); h.create(); check(!h.entry->start() && h.entry->terminal() && h.navigator.calls == 0 && h.producers == 0 && readEntry(h.root + '/' + QLatin1String(name)) == "stale", "stale artifact preserved before any action"); });
        scenario("untouched-default-mode", [&] {
            Harness h(app); h.view.reader->setVisible(true);
            h.view.reader->setProperty("currentPage", h.expected.pageIndex); h.view.controller.setPageId(h.expected.pageId); pumpEntry();
            reader_selected::Entry normal(&app, h.root, h.nonce);
            check(normal.start(), "default Entry starts"); putEntry(h.root + "/selected-request", h.nonce + ' ' + h.identity); normal.checkForTest(); pumpEntry();
            const auto bytes = readEntry(h.root + "/selected-result"); validReceipt(bytes, h.nonce, h.identity, QStringLiteral("observed"));
            const auto json = QJsonDocument::fromJson(bytes).object(); check(!json.contains("diagnostic") && !json.contains("image") && !QFile::exists(h.root + "/.pending-image") && !QFile::exists(h.root + "/visual-image.png") && h.navigator.calls == 0, "old default scalar behavior unchanged");
        });
        std::cout << "PASS identity capture Entry owned scenarios=" << scenarios << " checks=" << checks
                  << " injected_capture_producer=true real_files_png_and_legacy_navigation=true native_execution=false\n";
    } catch (const std::exception &) { return 1; }
    return 0;
}
