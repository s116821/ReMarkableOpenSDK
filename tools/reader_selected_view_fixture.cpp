#include "reader_selected_view_probe.h"
#include <QQmlComponent>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
#include <chrono>
#include <memory>
#include <thread>

class FocusFixture : public QQuickItem {
    Q_OBJECT
public:
    explicit FocusFixture(QQuickItem *parent = nullptr) : QQuickItem(parent) { setFlag(ItemIsFocusScope); }
};

class DocumentFixture final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY pageCountChanged)
    Q_PROPERTY(bool loadError READ loadError WRITE setLoadError NOTIFY loadErrorChanged)
public:
    using QObject::QObject;
    QString key = QStringLiteral("10000000-0000-0000-0000-000000000001");
    QStringList pages{QStringLiteral("fixture-page-a"), QStringLiteral("fixture-page-b")};
    bool error = false;
    QString id() const { return key; }
    int pageCount() const { return pages.size(); }
    bool loadError() const { return error; }
    void setLoadError(bool e) { if (error != e) { error = e; emit loadErrorChanged(); } }
    Q_INVOKABLE QString idForPage(int i) const { return i >= 0 && i < pages.size() ? pages[i] : QString(); }
    Q_INVOKABLE int pageForId(const QString &id) const { return pages.indexOf(id); }
signals:
    void pageCountChanged();
    void pageMapChanged();
    void loadErrorChanged();
};
class ControllerFixture final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString pageId READ pageId WRITE setPageId NOTIFY pageIdChanged)
    Q_PROPERTY(bool working READ working WRITE setWorking NOTIFY workingChanged)
public:
    using QObject::QObject;
    QString page = QStringLiteral("fixture-page-a");
    bool busy = false;
    QString pageId() const { return page; }
    void setPageId(const QString &p) { if (p != page) { page = p; emit pageIdChanged(); } }
    bool working() const { return busy; }
    void setWorking(bool b) { if (b != busy) { busy = b; emit workingChanged(); } }
signals:
    void pageIdChanged();
    void workingChanged();
};
class TilesFixture final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool pendingTiles READ pendingTiles WRITE setPendingTiles NOTIFY pendingTilesChanged)
public:
    using QObject::QObject;
    bool pending = false;
    bool pendingTiles() const { return pending; }
    void setPendingTiles(bool b) { if (b != pending) { pending = b; emit pendingTilesChanged(); } }
signals:
    void pendingTilesChanged();
};
class SceneView : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QObject* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(QObject* controller READ controller WRITE setController NOTIFY controllerChanged)
    Q_PROPERTY(QObject* tileManager READ tileManager WRITE setTileManager NOTIFY tileManagerChanged)
    Q_PROPERTY(QString pageId READ pageId WRITE setPageId NOTIFY pageIdChanged)
    Q_PROPERTY(Status status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(PageStatus pageStatus READ pageStatus WRITE setPageStatus NOTIFY pageStatusChanged)
    Q_PROPERTY(bool awaitingInitialTileSet READ awaitingInitialTileSet WRITE setAwaiting NOTIFY awaitingInitialTileSetChanged)
public:
    enum Status { Inactive = 4, Active = 17 }; Q_ENUM(Status)
    enum PageStatus { None = 2, Ready = 31, Error = 47 }; Q_ENUM(PageStatus)
    using QQuickItem::QQuickItem;
    QPointer<QObject> doc, control, tiles;
    QString page;
    Status state = Active;
    PageStatus pageState = Ready;
    bool awaiting = false;
    mutable std::function<void()> pageRead;
    mutable std::function<void()> tilesRead;
    QObject *document() const { return doc; }
    QObject *controller() const { return control; }
    QObject *tileManager() const { if (tilesRead) { auto f = std::move(tilesRead); tilesRead = {}; f(); } return tiles; }
    void setDocument(QObject *p) { if (p != doc) { doc = p; emit documentChanged(); } }
    void setController(QObject *p) { if (p != control) { control = p; emit controllerChanged(); } }
    void setTileManager(QObject *p) { if (p != tiles) { tiles = p; emit tileManagerChanged(); } }
    QString pageId() const { if (pageRead) { auto f = std::move(pageRead); pageRead = {}; f(); } return page; }
    void setPageId(const QString &p) { if (p != page) { page = p; emit pageIdChanged(); } }
    Status status() const { return state; }
    void setStatus(Status s) { if (s != state) { state = s; emit statusChanged(); } }
    PageStatus pageStatus() const { return pageState; }
    void setPageStatus(PageStatus s) { if (s != pageState) { pageState = s; emit pageStatusChanged(); } }
    bool awaitingInitialTileSet() const { return awaiting; }
    void setAwaiting(bool a) { if (a != awaiting) { awaiting = a; emit awaitingInitialTileSetChanged(); } }
signals:
    void documentChanged(); void controllerChanged(); void tileManagerChanged();
    void pageIdChanged(); void statusChanged(); void pageStatusChanged(); void awaitingInitialTileSetChanged();
};

static void require(bool ok, const char *message) {
    if (!ok) { QTextStream(stderr) << "FAIL " << message << '\n'; std::exit(1); }
}
static void refused(const reader_selected::Observation &r, const char *message) {
    require(!r.observed && !r.refusal.isEmpty() && r.documentId.isEmpty() && r.pageId.isEmpty() &&
            !r.mapCoherent && !r.ready && !r.nativeAuthority && !r.renderAuthority && !r.durableAuthority, message);
}
static void write(const QString &path, const QByteArray &data) {
    QFile f(path); require(f.open(QIODevice::WriteOnly), "fixture file open");
    require(f.write(data) == data.size(), "fixture file write");
}

static const QByteArray deviceQml = R"QML(
import OwnedProbe 1.0
FocusScope {
    id: root
    property QtObject document
    property QtObject controller
    property int page: 0
    property string pageId: document ? document.idForPage(page) : ""
    property Item sceneView: current
    property SceneFixture nextSceneView: next
    property SceneFixture previousSceneView: previous
    readonly property int status: sceneView ? sceneView.status : SceneFixture.Inactive
    readonly property int pageStatus: sceneView ? sceneView.pageStatus : SceneFixture.None
    readonly property bool pageError: pageStatus !== SceneFixture.None && pageStatus !== SceneFixture.Ready
    readonly property bool isLoading: !pageError && (status !== SceneFixture.Active || sceneView.awaitingInitialTileSet || externalTiles.pendingTiles || (controller && controller.working))
    Item {
        objectName: "SceneViewsRoot"
        width: 100; height: 100
        SceneFixture { id: next; visible: false; document: root.document; controller: root.controller; tileManager: externalTiles; pageId: "fixture-next" }
        SceneFixture { id: previous; visible: false; document: root.document; controller: root.controller; tileManager: externalTiles; pageId: "fixture-previous" }
        SceneFixture { id: current; objectName: "owned-selected"; document: root.document; controller: root.controller; tileManager: externalTiles; pageId: root.pageId }
        Item { objectName: "owned-lookalike"; visible: false
            property QtObject document: root.document
            property QtObject controller: root.controller
            property QtObject tileManager: externalTiles
            property string pageId: root.pageId
            property int status: 17
            property int pageStatus: 31
            property bool awaitingInitialTileSet: false
        }
    }
    // Deliberately deceptive; this object is not a native selected scene.
    Item { objectName: "scene-view" }
}
)QML";
static const QByteArray readerQml = R"QML(
import OwnedProbe 1.0
FocusScope {
    id: root
    objectName: "owned-reader-root"
    width: 100; height: 100
    property QtObject document: externalDocument
    property alias currentPage: sceneView.page
    property alias currentPageId: sceneView.pageId
    property QtObject sceneController: sceneView.controller
    QtObject { objectName: "DocumentView.ToolSettings"; property QtObject document: root.document }
    DeviceFixture { id: sceneView; width: 100; height: 100; document: root.document; controller: externalController }
}
)QML";

struct Fixture {
    QTemporaryDir dir;
    DocumentFixture document;
    ControllerFixture controller;
    TilesFixture tiles;
    QQmlEngine engine;
    QQuickWindow window;
    std::unique_ptr<QObject> object;
    QQuickItem *reader = nullptr, *device = nullptr, *container = nullptr;
    SceneView *current = nullptr;
    explicit Fixture(bool badContext = false, bool composite = false) {
        require(dir.isValid(), "temporary directory");
        write(dir.filePath("DeviceFixture.qml"), deviceQml);
        QByteArray source = readerQml;
        if (badContext) source.replace("sceneView", "wrongLexicalId");
        write(dir.filePath("ReaderFixture.qml"), source);
        write(dir.filePath("Main.qml"), R"QML(
import OwnedProbe 1.0
Item {
    id: root
    width: 100; height: 100
    ReaderFixture { id: nestedReader }
}
)QML");
        engine.rootContext()->setContextProperty("externalDocument", &document);
        engine.rootContext()->setContextProperty("externalController", &controller);
        engine.rootContext()->setContextProperty("externalTiles", &tiles);
        // The external objects intentionally have no QObject parent under the window.
        QQmlEngine::setContextForObject(&window, engine.rootContext());
        QQmlComponent component(&engine, QUrl::fromLocalFile(dir.filePath(composite ? "Main.qml" : "ReaderFixture.qml")));
        if (component.isError()) for (const auto &e : component.errors()) QTextStream(stderr) << e.toString() << '\n';
        object.reset(component.create()); require(bool(object), "actual QML component creation");
        auto *visualRoot = qobject_cast<QQuickItem *>(object.get()); require(visualRoot, "component visual root");
        reader = composite ? visualRoot->findChild<QQuickItem *>("owned-reader-root") : visualRoot;
        require(reader != nullptr, "reader root item");
        visualRoot->setParentItem(window.contentItem()); visualRoot->setParent(window.contentItem());
        container = reader->findChild<QQuickItem *>("SceneViewsRoot"); require(container, "container");
        device = container->parentItem();
        current = reader->findChild<SceneView *>("owned-selected"); require(current, "selected scene");
        window.show(); window.requestActivate(); QCoreApplication::processEvents();
        require(qmlContext(reader) != qmlContext(container), "actual nested component lexical contexts are distinct");
        require(container->childItems().first() != current, "selected scene is not first child");
        require(document.parent() == nullptr && controller.parent() == nullptr, "external object ownership");
    }
    reader_selected::Observation sample() { return reader_selected::observe(&window); }
    ~Fixture() { object.reset(); }
};

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    qmlRegisterType<QObject>("OwnedProbe", 1, 0, "QtObject");
    qmlRegisterType<QQuickItem>("OwnedProbe", 1, 0, "Item");
    qmlRegisterType<FocusFixture>("OwnedProbe", 1, 0, "FocusScope");
    qmlRegisterType<SceneView>("OwnedProbe", 1, 0, "SceneFixture");
    {
        Fixture f(false, true);
        QObject *helper = f.reader->findChild<QObject *>("DocumentView.ToolSettings", Qt::FindDirectChildrenOnly);
        QQmlContext *outer = qmlContext(f.reader), *inner = qmlContext(helper);
        require(helper && outer && inner && outer != inner && outer->engine() == &f.engine && inner->engine() == &f.engine,
                "nested composite root exposes outer creation context while direct helper exposes inner context");
        require(outer->objectForName("root") == f.object.get() && inner->objectForName("root") == f.reader &&
                inner->objectForName("sceneView") == f.device, "owned composite scope has exact distinct root and device ids");
        const auto r = f.sample();
        require(r.observed && r.ready && r.pageId == f.document.pages[0], "direct helper context resolves exact nested composite Reader and selected device");
    }
    for (const bool foreign : {false, true}) {
        Fixture f(false, true);
        QObject *original = f.reader->findChild<QObject *>("DocumentView.ToolSettings", Qt::FindDirectChildrenOnly);
        require(original, "original direct helper"); original->setObjectName("ignored-helper");
        QQmlEngine otherEngine;
        QQmlComponent component(foreign ? &otherEngine : &f.engine);
        component.setData("import OwnedProbe 1.0\nQtObject { id: root; objectName: \"DocumentView.ToolSettings\"; property QtObject document }", QUrl());
        std::unique_ptr<QObject> replacement(component.create()); require(bool(replacement), "real replacement helper component");
        replacement->setParent(f.reader);
        require(qmlContext(replacement.get())->objectForName("root") != f.reader, "replacement helper has wrong lexical root despite direct QObject ownership");
        const auto r = f.sample(); refused(r, "foreign helper engine or wrong exact lexical root refuses");
        require(r.refusal == (foreign ? "foreign-helper-context" : "lexical-root-missing"), "context refusal preserves engine and exact root guards");
    }
    {
        Fixture f(true, true);
        const auto r = f.sample(); refused(r, "nested exact Reader root with incorrect scoped sceneView refuses");
        require(r.refusal == "lexical-device-mismatch", "exact device id remains mandatory in helper-seeded scope");
    }
    {
        Fixture f;
        auto r = f.sample();
        if (!r.observed) QTextStream(stderr) << "first refusal: " << r.refusal << '\n';
        require(r.observed && r.refusal.isEmpty() && r.ready && r.mapCoherent && r.pageIndex == 0 && r.pageCount == 2 &&
                r.pageId == f.document.pages[0] && r.documentId == f.document.key &&
                !r.nativeAuthority && !r.renderAuthority && !r.durableAuthority, "selected ref, scoped ids, external ownership, map and enum readiness");
        f.current->setPageStatus(SceneView::Error);
        r = f.sample();
        require(r.observed && !r.loading && r.pageError && !r.ready, "page error is separate from not-loading");
        f.current->setPageStatus(SceneView::Ready);
        f.document.setLoadError(true);
        r = f.sample(); require(r.observed && r.documentError && !r.ready, "document load error is independent");
        f.document.setLoadError(false);
        f.tiles.setPendingTiles(true);
        r = f.sample(); require(r.observed && r.pendingTiles && !r.ready, "pending tiles are not ready");
        f.tiles.setPendingTiles(false);
        f.controller.setWorking(true);
        r = f.sample(); require(r.observed && r.working && !r.ready, "controller work is not ready");
        f.controller.setWorking(false);
        f.current->setAwaiting(true);
        r = f.sample(); require(r.observed && r.awaitingTiles && !r.ready, "initial tiles are not ready");
        f.current->setAwaiting(false);
        f.current->setPageId("inconsistent");
        refused(f.sample(), "inconsistent key refuses"); f.current->setPageId(f.document.pages[0]);
        f.document.pages[0] = "different-map";
        refused(f.sample(), "map roundtrip mismatch refuses"); f.document.pages[0] = "fixture-page-a";
        f.document.key = "[object Object]";
        refused(f.sample(), "unproven textual id conversion refuses"); f.document.key = "10000000-0000-0000-0000-000000000001";
        QObject duplicate(f.reader); duplicate.setObjectName("DocumentView.ToolSettings");
        refused(f.sample(), "ambiguous anchor refuses"); duplicate.setObjectName({});
        QQuickItem *lookalike = f.reader->findChild<QQuickItem *>("owned-lookalike");
        require(lookalike, "lookalike setup"); lookalike->setVisible(true);
        require(f.device->setProperty("sceneView", QVariant::fromValue<QQuickItem *>(lookalike)), "lookalike selection setup");
        r = f.sample(); refused(r, "shaped item without native SceneView base refuses");
        require(r.refusal == "native-scene-type", "native SceneView type reason");
        require(f.device->setProperty("sceneView", QVariant::fromValue<QQuickItem *>(f.current)), "restore actual scene");
        lookalike->setVisible(false);
        f.current->pageRead = [&] { f.current->setPageId("temporary"); f.current->setPageId("fixture-page-a"); };
        r = f.sample(); refused(r, "same-key ABA getter refuses"); require(r.refusal == "generation-changed", "ABA epoch reason");
        f.current->pageRead = [&] { const QVariant old = f.device->property("sceneView");
            f.device->setProperty("sceneView", f.device->property("nextSceneView")); f.device->setProperty("sceneView", old); };
        refused(f.sample(), "selected pointer ABA getter refuses");
        f.current->pageRead = [&] { const QString saved = f.document.pages[0];
            f.document.pages[0] = "temporary-map"; emit f.document.pageMapChanged();
            f.document.pages[0] = saved; emit f.document.pageMapChanged(); };
        refused(f.sample(), "page map ABA refuses");
        f.current->pageRead = [&] { f.current->setParentItem(f.reader); f.current->setParentItem(f.container); };
        refused(f.sample(), "selected visual owner ABA refuses");
        f.current->pageRead = [&] { auto inner = f.sample(); refused(inner, "nested sampler refuses"); };
        r = f.sample(); refused(r, "outer reentry refuses"); require(r.refusal == "sample-reentered", "outer reentry reason");
        f.current->pageRead = [&] { QEvent event(QEvent::User); QCoreApplication::sendEvent(&f.window, &event); };
        refused(f.sample(), "nested event delivery inside getter refuses");
        f.current->pageRead = [&] { f.window.hide(); f.window.show(); f.window.requestActivate(); QCoreApplication::processEvents(); };
        refused(f.sample(), "window visibility ABA inside getter refuses");
        require(f.window.isVisible() && QGuiApplication::focusWindow() == &f.window, "visibility ABA restored window");
        {
            QQuickWindow alternate;
            QQmlEngine::setContextForObject(&alternate, f.engine.rootContext());
            alternate.show(); f.window.requestActivate(); QCoreApplication::processEvents();
            int focusChanges = 0;
            const auto connection = QObject::connect(&app, &QGuiApplication::focusWindowChanged, &app, [&](QWindow *) { ++focusChanges; });
            f.current->pageRead = [&] { alternate.requestActivate(); QCoreApplication::processEvents();
                f.window.requestActivate(); QCoreApplication::processEvents(); };
            refused(f.sample(), "focus window ABA inside getter refuses");
            require(focusChanges >= 2 && QGuiApplication::focusWindow() == &f.window, "actual focus ABA restored window");
            QObject::disconnect(connection);
        }
        refused(reader_selected::observe(&f.window, [] { return false; }), "cancellation refuses");
        r = reader_selected::observe(&f.window, [] { std::this_thread::sleep_for(std::chrono::milliseconds(10)); return true; }, 2);
        refused(r, "absolute deadline refuses"); require(r.refusal == "deadline", "deadline reason");
        refused(reader_selected::observe(&f.window, [] { return true; }, 0), "invalid budget refuses");
        reader_selected::Observation crossThread;
        std::thread worker([&] { crossThread = f.sample(); }); worker.join();
        refused(crossThread, "foreign thread refuses before GUI access");
        QQuickWindow other; QQmlEngine::setContextForObject(&other, f.engine.rootContext());
        other.show(); other.requestActivate(); QCoreApplication::processEvents();
        refused(f.sample(), "different current window refuses");
    }
    {
        Fixture f(true); refused(f.sample(), "wrong lexical id refuses");
    }
    {
        Fixture f;
        QObject services(f.window.contentItem());
        QObject *tail = &services;
        for (int i = 0; i < 700; ++i) tail = new QObject(tail);
        tail->setObjectName("DocumentView.ToolSettings");
        require(f.sample().observed, "irrelevant nonvisual descendants cannot be a direct Reader helper");
    }
    for (const bool hidden : {true, false}) {
        Fixture f;
        QQuickItem excluded(f.window.contentItem());
        if (hidden) excluded.setVisible(false); else excluded.setEnabled(false);
        auto *duplicate = new QQuickItem(&excluded);
        duplicate->setObjectName("SceneViewsRoot");
        duplicate->setVisible(true); duplicate->setEnabled(true);
        QQuickItem *tail = duplicate;
        for (int i = 0; i < 700; ++i) {
            tail = new QQuickItem(tail);
            tail->setVisible(true); tail->setEnabled(true);
        }
        QObject misleadingHelper(duplicate); misleadingHelper.setObjectName("DocumentView.ToolSettings");
        require(hidden ? (!duplicate->isVisible() && !tail->isVisible()) : (!duplicate->isEnabled() && !tail->isEnabled()),
                "explicit child true cannot override effective hidden/disabled ancestry");
        require(f.sample().observed, "huge ineffective visual subtree cannot contain an eligible anchor");
    }
    for (const bool hidden : {true, false}) for (const bool aba : {false, true}) {
        Fixture f;
        QQuickItem excluded(f.window.contentItem());
        if (hidden) excluded.setVisible(false); else excluded.setEnabled(false);
        QQuickItem duplicate(&excluded); duplicate.setObjectName("SceneViewsRoot");
        duplicate.setVisible(true); duplicate.setEnabled(true);
        require(hidden ? !duplicate.isVisible() : !duplicate.isEnabled(), "excluded boundary starts effectively ineligible");
        f.current->pageRead = [&] {
            if (hidden) excluded.setVisible(true); else excluded.setEnabled(true);
            require(duplicate.isVisible() && duplicate.isEnabled(), "boundary reveal/enable makes descendant effectively eligible");
            if (aba) { if (hidden) excluded.setVisible(false); else excluded.setEnabled(false); }
        };
        const auto r = f.sample();
        refused(r, "late reveal/reenable and ABA invalidate pruned census");
        require(r.refusal == "generation-changed", "pruned boundary state notification invalidates epoch");
    }
    {
        Fixture f;
        // Discovery guards have their own bounded storage, not the 640-object
        // relationship budget. These direct objects still need name epochs.
        for (int i = 0; i < 20; ++i) {
            auto *branch = new QQuickItem(f.window.contentItem());
            for (int j = 0; j < 40; ++j) new QObject(branch);
        }
        require(f.sample().observed, "800 unrelated direct nonvisual children do not exhaust relationship retention");
    }
    for (const bool aba : {false, true}) {
        Fixture f;
        QObject lateHelper(f.reader);
        f.current->pageRead = [&] {
            lateHelper.setObjectName("DocumentView.ToolSettings");
            if (aba) lateHelper.setObjectName({});
        };
        const auto r = f.sample();
        refused(r, "late direct helper rename and rename ABA invalidate completed census");
        require(r.refusal == "generation-changed", "late name change is captured by discovery epoch");
    }
    for (const bool aba : {false, true}) {
        Fixture f;
        QObject externalOwner;
        QQuickItem lateContainer;
        lateContainer.setParent(&externalOwner);
        lateContainer.setObjectName("SceneViewsRoot");
        f.current->pageRead = [&] {
            lateContainer.setParentItem(f.window.contentItem());
            if (aba) lateContainer.setParentItem(nullptr);
        };
        const auto r = f.sample();
        refused(r, "external QObject-owned visual attachment and ABA invalidate completed census");
        require(r.refusal == "generation-changed", "visual child change is captured without QObject reparenting");
    }
    {
        Fixture f;
        auto returned = std::make_unique<TilesFixture>();
        f.current->setTileManager(returned.get());
        bool propertyReturned = false, deletedInPermit = false;
        f.current->tilesRead = [&] { propertyReturned = true; };
        const auto r = reader_selected::observe(&f.window, [&] {
            if (propertyReturned && returned) { returned.reset(); deletedInPermit = true; }
            return true;
        });
        require(deletedInPermit, "parentless result deleted during immediate post-read permit callback");
        refused(r, "newly returned QObject is retained before post-read permit can delete it");
        require(r.refusal == "generation-changed", "returned object destruction invalidates epoch before pointer reuse");
    }
    {
        Fixture f;
        QQuickItem chain(f.window.contentItem());
        QQuickItem *tail = &chain;
        for (int i = 0; i < 20; ++i) tail = new QQuickItem(tail);
        f.reader->setParentItem(tail);
        require(f.sample().observed, "visual Reader below legacy depth remains in the bounded visual census");
        // Restore visual ownership before the temporary chain is destroyed.
        f.reader->setParentItem(f.window.contentItem());
    }
    {
        Fixture f;
        QQuickItem duplicate(f.window.contentItem()); duplicate.setObjectName("SceneViewsRoot");
        refused(f.sample(), "visual census preserves container ambiguity");
    }
    {
        Fixture f;
        QQuickItem branch(f.window.contentItem());
        for (int i = 0; i < 30; ++i) {
            auto *group = new QQuickItem(&branch);
            for (int j = 0; j < 20; ++j) new QQuickItem(group);
        }
        const auto r = f.sample(); refused(r, "incomplete wide visual census cannot select an early candidate");
        require(r.refusal == "visual-node-cap", "node cap has a distinct refusal");
    }
    {
        Fixture f;
        QQuickItem chain(f.window.contentItem());
        QQuickItem *tail = &chain;
        for (int i = 0; i < 34; ++i) tail = new QQuickItem(tail);
        const auto r = f.sample(); refused(r, "incomplete deep visual census cannot select an early candidate");
        require(r.refusal == "visual-depth-cap", "depth cap has a distinct refusal");
    }
    {
        Fixture f;
        auto replacement = std::make_unique<ControllerFixture>();
        require(f.device->setProperty("controller", QVariant::fromValue<QObject *>(replacement.get())), "external replacement setup");
        require(f.sample().observed, "external replacement is observed");
        f.current->pageRead = [&] { replacement.reset(); };
        refused(f.sample(), "external controller destroyed inside accessor refuses");
    }
    QTextStream(stdout) << "PASS selected view owned Qt: separate lexical contexts, external ownership, native type/nonfirst selection, bounded eligible visual discovery/effective hidden-disabled pruning/reveal-enable ABA/nonvisual exclusion/ambiguity/distinct caps, discovery name/visual ABA and separate guards, returned QObject post-read lifetime, map/UUID/refusals, error/loading/tiles/work, value/pointer/map/visual/focus/visibility ABA, destruction, sampler/event reentry, cancellation/deadline/thread/window/context; native_authority=false render_authority=false durable_authority=false\n";
    return 0;
}
#include "reader_selected_view_fixture.moc"
