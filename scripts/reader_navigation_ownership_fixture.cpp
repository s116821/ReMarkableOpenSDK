// Owned Qt/QML interfaces only. They establish no firmware behavior.
#include "reader_navigation_ownership.h"
#define READER_NAVIGATION_OWNED_CLASSES_ONLY
#include "reader_fixture_navigation_fixture.cpp"
#undef READER_NAVIGATION_OWNED_CLASSES_ONLY
#include <QJsonDocument>
#include <qqml.h>
#include <vector>

namespace xofm { namespace bindings { namespace qt {
class WindowLayout : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)
    Q_PROPERTY(QString activeWindow READ activeWindow NOTIFY activeWindowChanged)
public:
    using QObject::QObject;
    bool readyValue = true;
    QString activeValue = QStringLiteral("owned-value-with-no-route-interpretation");
    mutable int reads = 0;
    mutable std::function<void()> onRead;
    bool ready() const {
        ++reads; const bool value = readyValue;
        const auto callback = std::move(onRead); onRead = {};
        if (callback) callback();
        return value;
    }
    QString activeWindow() const { ++reads; return activeValue; }
signals:
    void readyChanged(bool);
    void activeWindowChanged(QString);
};
} } }
class OwnershipOwner : public NavigationOwner {
    Q_OBJECT
    Q_PROPERTY(QObject *windowLayout READ windowLayout WRITE setWindowLayout NOTIFY windowLayoutChanged)
    Q_PROPERTY(QObject *contentItem READ contentItem WRITE setContentItem NOTIFY contentItemChanged)
public:
    using NavigationOwner::NavigationOwner;
    QPointer<QObject> layout, content;
    QObject *windowLayout() const { return layout; }
    QObject *contentItem() const { return content; }
    void setWindowLayout(QObject *value) { if (layout != value) { layout = value; emit windowLayoutChanged(); } }
    void setContentItem(QObject *value) { if (content != value) { content = value; emit contentItemChanged(); } }
signals:
    void windowLayoutChanged();
    void contentItemChanged();
};

#ifndef READER_OWNERSHIP_OWNED_CLASSES_ONLY
static void requireOwnership(bool condition, const char *description) {
    if (!condition) qFatal("FAIL ownership fixture: %s", description);
}
static void pumpOwnership() {
    for (int i = 0; i < 12; ++i) { QCoreApplication::sendPostedEvents(); QCoreApplication::processEvents(); }
}
struct OwnershipFixture {
    QQmlEngine engine;
    QQuickWindow window;
    xofm::bindings::qt::WindowNavigatorWrapper navigator;
    xofm::bindings::qt::WindowLayout layout;
    QPointer<OwnershipOwner> owner, main;
    explicit OwnershipFixture(bool nested = false) {
        QQmlEngine::setContextForObject(&window, engine.rootContext());
        engine.rootContext()->setContextProperty(QStringLiteral("ownedNavigator"), &navigator);
        engine.rootContext()->setContextProperty(QStringLiteral("ownedLayout"), &layout);
        QQmlComponent component(&engine);
        const QByteArray source = nested ? QByteArray(
            "import QtQuick\nimport Owned.Navigation 1.0\nItem { Loader { active:true; sourceComponent: Component { Owner { id:root; windowNavigator:ownedNavigator; windowLayout:ownedLayout; contentItem:mainView; Owner { id:mainView; windowNavigator:root.windowNavigator; windowLayout:root.windowLayout } } } } }") : QByteArray(
            "import QtQuick\nimport Owned.Navigation 1.0\nOwner { id:root; windowNavigator:ownedNavigator; windowLayout:ownedLayout; contentItem:mainView; Owner { id:mainView; windowNavigator:root.windowNavigator; windowLayout:root.windowLayout } }");
        component.setData(source, QUrl(QStringLiteral("file:///owned/navigation-composition.qml")));
        QObject *created = component.create();
        if (!created) qFatal("owned QML creation: %s", qPrintable(component.errorString()));
        auto item = qobject_cast<QQuickItem *>(created);
        requireOwnership(item, "real QML visual root");
        item->setParent(&window); item->setParentItem(window.contentItem());
        QList<QQuickItem *> queue{item};
        for (qsizetype i = 0; i < queue.size(); ++i) {
            if (auto candidate = qobject_cast<OwnershipOwner *>(queue[i])) {
                if (!owner) owner = candidate; else if (!main) main = candidate;
            }
            for (auto child : queue[i]->childItems()) queue.append(child);
        }
        requireOwnership(owner && main, "owned pair exists");
        window.setGeometry(0, 0, 200, 200); window.show(); window.requestActivate(); pumpOwnership();
        requireOwnership(window.isActive() && QGuiApplication::focusWindow() == &window, "real owned window focus");
    }
    reader_selected::NavigationOwnership run(const std::function<bool()> &permit = [] { return true; }, int budget = 1000) {
        return reader_selected::inspectNavigationOwnership(&window, permit, budget);
    }
};
static bool anyMatch(const reader_selected::NavigationOwnership &value) {
    for (const auto &row : value.owners) if (row.toObject().value("composition_match").toBool()) return true;
    return false;
}
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    qmlRegisterType<OwnershipOwner>("Owned.Navigation", 1, 0, "Owner");
    for (bool nested : {false, true}) {
        OwnershipFixture fixture(nested);
        const auto result = fixture.run();
        requireOwnership(result.complete && result.refusal.isEmpty() && result.owners.size() == 2 && anyMatch(result), "real lexical root/mainView composition including nested context");
        for (const auto &value : result.owners) {
            const auto row = value.toObject();
            requireOwnership(row.value("nav_group").toInt() == 1 && row.value("layout_group").toInt() == 1 && row.value("layout_native").toBool() &&
                row.value("layout_ready").toBool() && row.value("layout_active_window").toString() == fixture.layout.activeValue,
                "stable equal groups and typed native-like scalar getters");
        }
        requireOwnership(fixture.navigator.calls == 0, "census never dispatches");
        requireOwnership(QJsonDocument(result.owners).toJson(QJsonDocument::Compact).size() < 16384, "bounded owned wire size");
    }
    {
        OwnershipFixture fixture;
        xofm::bindings::qt::WindowNavigatorWrapper second;
        fixture.main->setWindowNavigator(&second);
        const auto result = fixture.run();
        requireOwnership(result.complete && result.owners.size() == 2 && !anyMatch(result) &&
            result.owners[0].toObject().value("nav_group").toInt() != result.owners[1].toObject().value("nav_group").toInt(),
            "two legitimate wrappers reported with mismatching composition, no ambiguity gate");
    }
    {
        OwnershipFixture fixture;
        fixture.main->setWindowLayout(nullptr);
        const auto result = fixture.run();
        requireOwnership(result.complete && !anyMatch(result) && result.owners[1].toObject().value("layout_state").toString() == QStringLiteral("null"),
            "null layout remains observational row");
    }
    {
        OwnershipFixture fixture;
        QObject wrong;
        fixture.main->setWindowLayout(&wrong);
        const auto result = fixture.run();
        requireOwnership(result.complete && !result.owners[1].toObject().value("layout_native").toBool(), "non-native layout does not run scalar getters");
    }
    {
        OwnershipFixture fixture;
        QQuickItem boundary(fixture.window.contentItem()); boundary.setVisible(false);
        OwnershipOwner hidden(&boundary); hidden.setWindowNavigator(&fixture.navigator);
        const auto result = fixture.run();
        requireOwnership(result.complete && result.owners.size() == 2 && hidden.reads == 0 && result.visited == 4, "hidden boundary visited and branch pruned");
    }
    for (const QByteArray &mode : {QByteArray("cancel"), QByteArray("reentry"), QByteArray("deadline"), QByteArray("owner-aba"),
                                 QByteArray("layout-aba"), QByteArray("hidden-aba"), QByteArray("destruction")}) {
        OwnershipFixture fixture;
        bool canceled = false;
        QQuickItem boundary(fixture.window.contentItem()); boundary.setVisible(false);
        // Put boundary before owners in BFS so its exclusion has been watched.
        fixture.owner->setParentItem(nullptr); fixture.owner->setParentItem(fixture.window.contentItem());
        fixture.owner->onRead = [&] {
            if (mode == "cancel") canceled = true;
            if (mode == "reentry") fixture.run();
            if (mode == "deadline") std::this_thread::sleep_for(std::chrono::milliseconds(10));
            if (mode == "owner-aba") { fixture.owner->setVisible(false); fixture.owner->setVisible(true); }
            if (mode == "layout-aba") { emit fixture.layout.readyChanged(false); emit fixture.layout.readyChanged(true); }
            if (mode == "hidden-aba") { boundary.setVisible(true); boundary.setVisible(false); }
        };
        // contentItem pins main before the typed layout getter is reached.
        if (mode == "destruction") fixture.layout.onRead = [&] { delete fixture.main.data(); };
        int calls = 0;
        const auto result = fixture.run([&] {
            ++calls;
            return !canceled;
        }, mode == "deadline" ? 5 : 1000);
        if (mode == "layout-aba") {
            // Navigator getter precedes layout subscription; use its getter for a watched ABA instead.
            requireOwnership(result.complete, "pre-subscription layout signal does not assert a prior snapshot");
            fixture.layout.onRead = [&] { emit fixture.layout.readyChanged(false); emit fixture.layout.readyChanged(true); };
            const auto watched = fixture.run();
            requireOwnership(!watched.complete && !watched.refusal.isEmpty(), "notified layout ABA refused after subscription");
        } else requireOwnership(!result.complete && !result.refusal.isEmpty() && calls > 0, "cancel/reentry/deadline/ABA/destruction refuse bounded census");
        requireOwnership(fixture.navigator.calls == 0, "refusal never invokes action");
    }
    for (const QByteArray &mode : {QByteArray("content-parent-aba"), QByteArray("main-visible-aba"), QByteArray("main-enabled-aba")}) {
        OwnershipFixture fixture;
        bool afterScalarRead = false, changed = false;
        fixture.layout.onRead = [&] { afterScalarRead = true; };
        const auto result = fixture.run([&] {
            if (afterScalarRead && !changed) {
                changed = true;
                // main has not reached its BFS turn. Its returned contentItem
                // relationship must already be guarded before this callback.
                if (mode == "content-parent-aba") {
                    fixture.main->setParentItem(fixture.window.contentItem());
                    fixture.main->setParentItem(fixture.owner);
                } else if (mode == "main-visible-aba") {
                    fixture.main->setVisible(false); fixture.main->setVisible(true);
                } else {
                    fixture.main->setEnabled(false); fixture.main->setEnabled(true);
                }
            }
            return true;
        });
        requireOwnership(changed && !result.complete && result.refusal == QStringLiteral("ownership-changed"),
            "returned content parent and not-yet-visited lexical main ABA refused after scalar read");
        requireOwnership(fixture.navigator.calls == 0, "ABA census never invokes action");
    }
    {
        OwnershipFixture fixture;
        std::vector<std::unique_ptr<OwnershipOwner>> extra;
        for (int i = 0; i < 7; ++i) {
            auto value = std::make_unique<OwnershipOwner>(fixture.window.contentItem());
            value->setWindowNavigator(&fixture.navigator); extra.push_back(std::move(value));
        }
        const auto result = fixture.run();
        requireOwnership(!result.complete && result.refusal == QStringLiteral("ownership-owner-cap") && result.owners.size() == 8, "ninth owner refuses without omitted-owner completion");
    }
    {
        OwnershipFixture fixture;
        std::vector<std::unique_ptr<QQuickItem>> extra;
        for (int i = 0; i < 65; ++i) extra.push_back(std::make_unique<QQuickItem>(fixture.main));
        const auto result = fixture.run();
        requireOwnership(!result.complete && result.refusal == QStringLiteral("ownership-child-cap"), "child cap bounded refusal");
    }
    requireOwnership(!reader_selected::inspectNavigationOwnership(nullptr, [] { return true; }, 0).complete, "invalid budget no census");
    QTextStream(stdout) << "PASS owned navigation ownership fixtures\n";
    return 0;
}
#include "moc_reader_selected_view_probe.cpp"
#endif
#include "reader_navigation_ownership_fixture.moc"
