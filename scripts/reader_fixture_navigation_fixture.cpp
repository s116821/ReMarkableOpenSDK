// Entirely owned interfaces. These fixtures do not reproduce firmware behavior.
#include "reader_fixture_navigation.h"
#include <QTextStream>
#include <chrono>
#include <memory>
#include <thread>

namespace xofm { namespace bindings { namespace qt {
class WindowNavigatorWrapper : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    int calls = 0;
    QString route;
    QVariantMap configuration;
    std::function<void()> onOpen;
public slots:
    void open(QString requestedRoute, QVariantMap requestedConfiguration = {}) {
        ++calls; route = std::move(requestedRoute); configuration = std::move(requestedConfiguration);
        const auto callback = onOpen;
        if (callback) callback(); // No member access after a self-deleting callback.
    }
};
} } }

class NavigationOwner : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QObject *windowNavigator READ windowNavigator WRITE setWindowNavigator NOTIFY windowNavigatorChanged)
public:
    using QQuickItem::QQuickItem;
    QPointer<QObject> navigator;
    mutable int reads = 0;
    mutable std::function<void()> onRead;
    QObject *windowNavigator() const {
        ++reads;
        QObject *returned = navigator.data();
        const auto callback = std::move(onRead);
        onRead = {};
        if (callback) callback();
        return returned; // Fixtures promise a live return; caller guards it next.
    }
    void setWindowNavigator(QObject *value) {
        if (navigator == value) return;
        navigator = value; emit windowNavigatorChanged();
    }
signals:
    void windowNavigatorChanged();
};

class ShadowNavigation final : public xofm::bindings::qt::WindowNavigatorWrapper {
    Q_OBJECT
public slots:
    bool open(QString, QVariantMap) { return false; }
};

#ifndef READER_NAVIGATION_OWNED_CLASSES_ONLY
static void requireNavigation(bool condition, const char *description) {
    if (!condition) qFatal("FAIL navigation fixture: %s", description);
}
static void pumpNavigation() {
    for (int index = 0; index < 12; ++index) {
        QCoreApplication::sendPostedEvents();
        QCoreApplication::processEvents();
    }
}
struct NavigationFixture {
    QQmlEngine engine;
    QQuickWindow window;
    xofm::bindings::qt::WindowNavigatorWrapper receiver;
    NavigationOwner owner;
    reader_selected::FixtureNavigation configuration{QStringLiteral("10000000-0000-0000-0000-000000000001"), 4};
    NavigationFixture() : owner(window.contentItem()) {
        QQmlEngine::setContextForObject(&window, engine.rootContext());
        QQmlEngine::setContextForObject(&owner, engine.rootContext());
        owner.setWindowNavigator(&receiver);
        owner.setSize(QSizeF(100, 100));
        window.setGeometry(0, 0, 200, 200);
        window.show(); window.requestActivate(); pumpNavigation();
        requireNavigation(QGuiApplication::focusWindow() == &window && window.isActive(), "owned window has actual focus");
    }
    reader_selected::NavigationObservation run(const std::function<bool()> &permit = [] { return true; }, int budget = 1000) {
        return reader_selected::openFixture(&window, configuration, permit, budget);
    }
};

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    {
        NavigationFixture fixture;
        auto result = fixture.run();
        requireNavigation(result.attempted && result.dispatched && result.refusal.isEmpty() && fixture.receiver.calls == 1,
                          "one typed native-like slot dispatched once despite default-argument clone");
        const auto &map = fixture.receiver.configuration;
        requireNavigation(fixture.receiver.route == QStringLiteral("legacydevice/window/main") && map.size() == 2 &&
            map.value("documentId").metaType() == QMetaType::fromType<QString>() && map.value("documentId").toString() == fixture.configuration.documentId &&
            map.value("page").metaType() == QMetaType::fromType<int>() && map.value("page").toInt() == 4,
            "fixed route and exact QString/int map without source reconstruction");
        int openMethods = 0;
        for (int index = 0; index < fixture.receiver.metaObject()->methodCount(); ++index)
            if (fixture.receiver.metaObject()->method(index).name() == "open") ++openMethods;
        requireNavigation(openMethods == 2, "owned slot metadata contains the one-argument clone");
        pumpNavigation();
        requireNavigation(fixture.receiver.calls == 1, "no queued retry after dispatch");
    }
    for (bool same : {true, false}) {
        NavigationFixture fixture;
        NavigationOwner other(fixture.window.contentItem());
        xofm::bindings::qt::WindowNavigatorWrapper second;
        other.setWindowNavigator(same ? &fixture.receiver : &second);
        const auto result = fixture.run();
        requireNavigation(result.dispatched == same && result.attempted == same && fixture.receiver.calls == (same ? 1 : 0) && second.calls == 0,
                          "complete census coalesces same live receiver and refuses different receivers");
    }
    {
        NavigationFixture fixture;
        QQuickItem boundary(fixture.window.contentItem()); boundary.setVisible(false);
        NavigationOwner hidden(&boundary);
        xofm::bindings::qt::WindowNavigatorWrapper second;
        hidden.setWindowNavigator(&second);
        const auto result = fixture.run();
        requireNavigation(result.dispatched && hidden.reads == 0 && second.calls == 0, "hidden branch is pruned before property access");
    }
    for (const QByteArray &mode : {QByteArray("cancel-before"), QByteArray("cancel-after-read"), QByteArray("reentry"),
                                 QByteArray("deadline"), QByteArray("owner-aba"), QByteArray("focus-aba"), QByteArray("reveal-aba")}) {
        NavigationFixture fixture;
        bool canceled = mode == "cancel-before";
        QQuickItem hidden(fixture.window.contentItem()); hidden.setVisible(false);
        if (mode == "reveal-aba") {
            fixture.owner.setParentItem(nullptr);
            fixture.owner.setParentItem(fixture.window.contentItem());
        }
        fixture.owner.onRead = [&] {
            if (mode == "cancel-after-read") canceled = true;
            else if (mode == "reentry") {
                const auto nested = fixture.run();
                requireNavigation(!nested.attempted && nested.refusal == "sample-reentered", "shared reentry guard");
            } else if (mode == "deadline") std::this_thread::sleep_for(std::chrono::milliseconds(25));
            else if (mode == "owner-aba") {
                fixture.owner.setWindowNavigator(nullptr); fixture.owner.setWindowNavigator(&fixture.receiver);
            } else if (mode == "focus-aba") { fixture.window.hide(); fixture.window.show(); }
            else if (mode == "reveal-aba") { hidden.setVisible(true); hidden.setVisible(false); }
        };
        const auto result = fixture.run([&] { return !canceled; }, mode == "deadline" ? 10 : 1000);
        requireNavigation(!result.attempted && !result.dispatched && fixture.receiver.calls == 0, "pre-dispatch cancellation/reentry/deadline/ABA never calls slot");
    }
    {
        NavigationFixture fixture;
        auto receiver = std::make_unique<xofm::bindings::qt::WindowNavigatorWrapper>();
        fixture.owner.setWindowNavigator(receiver.get());
        bool read = false;
        fixture.owner.onRead = [&] { read = true; };
        const auto result = fixture.run([&] { if (read && receiver) receiver.reset(); return true; });
        requireNavigation(!result.attempted && !result.dispatched && fixture.receiver.calls == 0,
                          "borrowed receiver is guarded before post-read permit can delete it");
    }
    {
        NavigationFixture fixture;
        fixture.owner.setWindowNavigator(nullptr);
        auto owner = std::make_unique<NavigationOwner>(fixture.window.contentItem());
        owner->setWindowNavigator(&fixture.receiver);
        // The original owner is an eligible null property: remove it from this
        // independent census before constructing the self-deleting owner case.
        fixture.owner.setVisible(false);
        owner->onRead = [&] { owner.reset(); };
        const auto result = fixture.run();
        requireNavigation(!result.attempted && !result.dispatched && fixture.receiver.calls == 0, "owner destruction during getter refuses safely");
    }
    {
        NavigationFixture fixture;
        ShadowNavigation shadow; fixture.owner.setWindowNavigator(&shadow);
        const auto result = fixture.run();
        requireNavigation(!result.attempted && !result.dispatched && shadow.calls == 0, "shadowed exact signature is not silently selected");
    }
    {
        NavigationFixture fixture;
        QQmlEngine other;
        QQmlEngine::setContextForObject(&fixture.receiver, other.rootContext());
        const auto result = fixture.run();
        requireNavigation(!result.attempted && !result.dispatched && fixture.receiver.calls == 0, "foreign associated engine refuses");
    }
    {
        NavigationFixture fixture;
        auto receiver = std::make_unique<xofm::bindings::qt::WindowNavigatorWrapper>();
        fixture.owner.setWindowNavigator(receiver.get());
        bool called = false;
        receiver->onOpen = [&] { called = true; fixture.owner.setVisible(false); receiver.reset(); };
        const auto result = fixture.run();
        requireNavigation(called && result.attempted && result.dispatched && !receiver,
                          "dispatch may change UI and delete receiver without post-action dereference or retry");
    }
    {
        NavigationFixture fixture;
        const QString expected = fixture.configuration.documentId;
        const auto result = fixture.run([&] {
            fixture.configuration.documentId = QStringLiteral("00000000-0000-0000-0000-000000000000");
            fixture.configuration.pageIndex = -1;
            return true;
        });
        requireNavigation(result.dispatched && fixture.receiver.configuration.value("documentId").toString() == expected &&
                          fixture.receiver.configuration.value("page").toInt() == 4, "owned configuration snapshot survives permit-side caller mutation");
    }
    {
        NavigationFixture fixture;
        for (const QString &id : {QString(), QStringLiteral("00000000-0000-0000-0000-000000000000"),
             QStringLiteral("10000000-0000-0000-0000-00000000000A"), QStringLiteral("10000000-0000-0000-0000-000000000001\n")}) {
            fixture.configuration.documentId = id;
            requireNavigation(!fixture.run().attempted, "invalid/noncanonical document ID never dispatches");
        }
        fixture.configuration.documentId = QStringLiteral("10000000-0000-0000-0000-000000000001");
        for (int page : {-1, 10000}) {
            fixture.configuration.pageIndex = page;
            requireNavigation(!fixture.run().attempted, "bounded research page index");
        }
        requireNavigation(fixture.receiver.calls == 0, "invalid configuration has no calls");
    }
    QTextStream(stdout) << "PASS owned fixture navigation: typed route/map, clone selection, complete census, same/different receivers, hidden pruning, pre-dispatch invalidation, deletion and once-only dispatch; no firmware truth\n";
    return 0;
}
#endif
#include "reader_fixture_navigation_fixture.moc"
