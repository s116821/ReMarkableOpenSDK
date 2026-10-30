// Original owned Qt/QML fixture, not copied application UI or firmware behavior.
#define READER_OWNERSHIP_OWNED_CLASSES_ONLY
#include "reader_navigation_ownership_fixture.cpp"
#undef READER_OWNERSHIP_OWNED_CLASSES_ONLY

#ifndef READER_LEGACY_NAVIGATION_OWNED_CLASSES_ONLY
static void requireLegacy(bool condition, const char *description) {
    if (!condition) qFatal("FAIL legacy navigation fixture: %s", description);
}
static void pumpLegacy() {
    for (int i = 0; i < 12; ++i) { QCoreApplication::sendPostedEvents(); QCoreApplication::processEvents(); }
}
struct LegacyFixture {
    QQmlEngine engine;
    QQuickWindow window;
    std::unique_ptr<xofm::bindings::qt::WindowNavigatorWrapper> navigator = std::make_unique<xofm::bindings::qt::WindowNavigatorWrapper>();
    std::unique_ptr<xofm::bindings::qt::WindowLayout> layout = std::make_unique<xofm::bindings::qt::WindowLayout>();
    QPointer<OwnershipOwner> owner, main;
    reader_selected::FixtureNavigation configuration{QStringLiteral("10000000-0000-0000-0000-000000000001"), 4, true};
    explicit LegacyFixture(const QByteArray &variant = {}) {
        QQmlEngine::setContextForObject(&window, engine.rootContext());
        layout->activeValue = QStringLiteral("legacydevice/window/main");
        engine.rootContext()->setContextProperty(QStringLiteral("ownedNavigator"), navigator.get());
        engine.rootContext()->setContextProperty(QStringLiteral("ownedLayout"), layout.get());
        auto pair = addComposition(variant); owner = pair.first; main = pair.second;
        window.setGeometry(0, 0, 200, 200); window.show(); window.requestActivate(); pumpLegacy();
        requireLegacy(window.isActive() && QGuiApplication::focusWindow() == &window, "real owned focused window");
    }
    QPair<OwnershipOwner *, OwnershipOwner *> addComposition(const QByteArray &variant = {}) {
        QByteArray source("import QtQuick\nimport Owned.Navigation 1.0\nOwner { id:root; windowNavigator:ownedNavigator; windowLayout:ownedLayout; contentItem:mainView; Owner { id:mainView; windowNavigator:ownedNavigator; windowLayout:ownedLayout } }");
        if (variant == "missing-main-id") { source.replace("mainView", "otherItem"); }
        if (variant == "missing-root-id") { source.replace("id:root", "id:otherRoot"); }
        QQmlComponent component(&engine);
        component.setData(source, variant == "bad-url" ? QUrl(QStringLiteral("file:///owned/other.qml")) :
            QUrl(QString::fromLatin1(reader_selected::navigation_detail::LegacyContext)));
        QObject *created = component.create();
        if (!created) qFatal("owned QML composition: %s", qPrintable(component.errorString()));
        auto root = qobject_cast<OwnershipOwner *>(created);
        requireLegacy(root, "owned QML root");
        root->setParent(&window); root->setParentItem(window.contentItem());
        auto child = qobject_cast<OwnershipOwner *>(root->contentItem());
        requireLegacy(child, "owned linked main");
        return {root, child};
    }
    reader_selected::NavigationObservation run(const std::function<bool()> &permit = [] { return true; }, int budget = 1000) {
        return reader_selected::openFixture(&window, configuration, permit, budget);
    }
};
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    qmlRegisterType<OwnershipOwner>("Owned.Navigation", 1, 0, "Owner");
    {
        LegacyFixture fixture;
        OwnershipOwner auxiliary(fixture.window.contentItem());
        xofm::bindings::qt::WindowNavigatorWrapper other;
        auxiliary.setWindowNavigator(&other);
        QQmlEngine::setContextForObject(&auxiliary, fixture.engine.rootContext());
        const auto result = fixture.run();
        requireLegacy(result.attempted && result.dispatched && result.refusal.isEmpty() && fixture.navigator->calls == 1 &&
            other.calls == 0 && auxiliary.reads == 0, "linked main selected, root not duplicated, unrelated distinct wrapper ignored");
        requireLegacy(result.visitedItems >= 4 && result.permissionCalls > quint64(result.visitedItems) && result.elapsedMs >= 0,
            "diagnostic costs accompany actual completed census");
        requireLegacy(fixture.navigator->route == QStringLiteral("legacydevice/window/main") &&
            fixture.navigator->configuration.value("documentId").metaType() == QMetaType::fromType<QString>() &&
            fixture.navigator->configuration.value("documentId").toString() == fixture.configuration.documentId &&
            fixture.navigator->configuration.value("page").metaType() == QMetaType::fromType<int>() &&
            fixture.navigator->configuration.value("page").toInt() == 4, "fixed route and exact QString/int map");
        pumpLegacy(); requireLegacy(fixture.navigator->calls == 1, "no queued retry");
    }
    {
        LegacyFixture fixture;
        quint64 calls = 0;
        const auto result = fixture.run([&] { ++calls; return calls < 20; }, reader_selected::MaxObservationMs);
        requireLegacy(!result.attempted && !result.dispatched && result.refusal == QStringLiteral("canceled") &&
            fixture.navigator->calls == 0 && result.permissionCalls == calls && calls == 20 && result.elapsedMs >= 0,
            "longer observation budget keeps cancellation and counts actual permission calls");
    }
    for (bool shared : {true, false}) {
        LegacyFixture fixture;
        auto second = fixture.addComposition();
        xofm::bindings::qt::WindowNavigatorWrapper other;
        if (!shared) { second.first->setWindowNavigator(&other); second.second->setWindowNavigator(&other); }
        const auto result = fixture.run();
        requireLegacy(!result.attempted && !result.dispatched && result.refusal == QStringLiteral("navigation-legacy-ambiguity") &&
            fixture.navigator->calls == 0 && other.calls == 0, "duplicate compositions refuse even shared receiver");
    }
    {
        LegacyFixture fixture;
        auto second = fixture.addComposition(); second.first->setVisible(false);
        const auto result = fixture.run();
        requireLegacy(result.dispatched && second.first->reads == 0 && second.second->reads == 0, "hidden composition pruned before getters");
    }
    for (const QByteArray &mode : {QByteArray("bad-url"), QByteArray("missing-main-id"), QByteArray("missing-root-id"),
            QByteArray("content"), QByteArray("parent"), QByteArray("navigator-equality"), QByteArray("layout-equality"),
            QByteArray("layout-null"), QByteArray("layout-type"), QByteArray("ready-false"), QByteArray("active-window")}) {
        LegacyFixture fixture(mode);
        xofm::bindings::qt::WindowNavigatorWrapper otherNavigator;
        xofm::bindings::qt::WindowLayout otherLayout;
        QObject wrongLayout;
        if (mode == "content") fixture.owner->setContentItem(nullptr);
        if (mode == "parent") fixture.main->setParentItem(fixture.window.contentItem());
        if (mode == "navigator-equality") fixture.main->setWindowNavigator(&otherNavigator);
        if (mode == "layout-equality") fixture.main->setWindowLayout(&otherLayout);
        if (mode == "layout-null") { fixture.owner->setWindowLayout(nullptr); fixture.main->setWindowLayout(nullptr); }
        if (mode == "layout-type") { fixture.owner->setWindowLayout(&wrongLayout); fixture.main->setWindowLayout(&wrongLayout); }
        if (mode == "ready-false") fixture.layout->readyValue = false;
        if (mode == "active-window") fixture.layout->activeValue = QStringLiteral("bootsplash/window/main");
        const auto result = fixture.run();
        requireLegacy(!result.attempted && !result.dispatched && !result.refusal.isEmpty() && fixture.navigator->calls == 0,
            "URL/IDs/content/parent/native binding/layout state mismatch refuses without call");
    }
    for (const QByteArray &mode : {QByteArray("cancel"), QByteArray("reentry"), QByteArray("deadline"), QByteArray("root-aba"),
            QByteArray("main-aba"), QByteArray("binding-aba"), QByteArray("layout-aba"), QByteArray("delete-root"),
            QByteArray("delete-main"), QByteArray("delete-returned-navigator")}) {
        LegacyFixture fixture;
        xofm::bindings::qt::WindowNavigatorWrapper other;
        bool canceled = false, borrowedReturned = false;
        fixture.owner->onRead = [&] {
            if (mode == "cancel") canceled = true;
            if (mode == "reentry") fixture.run();
            if (mode == "deadline") std::this_thread::sleep_for(std::chrono::milliseconds(10));
            if (mode == "root-aba") { fixture.owner->setVisible(false); fixture.owner->setVisible(true); }
            if (mode == "main-aba") { fixture.main->setEnabled(false); fixture.main->setEnabled(true); }
            if (mode == "binding-aba") { fixture.owner->setWindowNavigator(&other); fixture.owner->setWindowNavigator(fixture.navigator.get()); }
            if (mode == "delete-returned-navigator") borrowedReturned = true;
        };
        fixture.layout->onRead = [&] {
            if (mode == "layout-aba") { emit fixture.layout->readyChanged(false); emit fixture.layout->readyChanged(true); }
            if (mode == "delete-root") delete fixture.owner.data();
            if (mode == "delete-main") delete fixture.main.data();
        };
        const auto result = fixture.run([&] {
            // The getter has returned; Dispatch must have pinned its result
            // before this callback may destroy it.
            if (borrowedReturned && fixture.navigator) fixture.navigator.reset();
            return !canceled;
        }, mode == "deadline" ? 5 : 1000);
        requireLegacy(!result.attempted && !result.dispatched && !result.refusal.isEmpty() &&
            (!fixture.navigator || fixture.navigator->calls == 0), "cancellation/reentry/deadline/notified ABA/retained lifetime refuses");
    }
    {
        LegacyFixture fixture;
        fixture.layout->onRead = [&] {
            // The first scan is valid, but the last fresh recheck must read the
            // current state rather than reuse the first ready/route scalars.
            fixture.layout->onRead = [&] { fixture.layout->activeValue = QStringLiteral("bootsplash/window/main"); };
        };
        const auto result = fixture.run();
        requireLegacy(!result.attempted && !result.dispatched && fixture.navigator->calls == 0,
            "fresh preinvoke layout scalar recheck refuses changed unnotified state");
    }
    {
        LegacyFixture fixture;
        fixture.navigator->onOpen = [&] { delete fixture.owner.data(); };
        const auto result = fixture.run();
        requireLegacy(result.attempted && result.dispatched && fixture.navigator->calls == 1,
            "normal UI mutation after invoke does not require old discovery epoch");
    }
    QTextStream(stdout) << "PASS owned legacy composition navigation fixtures\n";
    return 0;
}
#include "moc_reader_selected_view_probe.cpp"
#endif
