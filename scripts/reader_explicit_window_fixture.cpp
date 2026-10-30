#include "reader_metadata_census.h"
#include <QQmlComponent>
#include <QTextStream>
#include <memory>
#include <vector>
#include <thread>
#include <cstdlib>
static int valueReads = 0;
class WindowTrap : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QObject* document READ objectValue)
    Q_PROPERTY(QObject* sceneController READ objectValue)
    Q_PROPERTY(QObject* controller READ objectValue)
    Q_PROPERTY(QQuickItem* sceneView READ itemValue)
    Q_PROPERTY(QString pageId READ stringValue)
    Q_PROPERTY(QString currentPageId READ stringValue)
public:
    QObject *objectValue() { ++valueReads; return nullptr; }
    QQuickItem *itemValue() { ++valueReads; return nullptr; }
    QString stringValue() { ++valueReads; return {}; }
};
class HelperTrap : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* document READ objectValue)
public:
    QObject *objectValue() { ++valueReads; return nullptr; }
};
static void require(bool ok, const char *why) {
    if (!ok) { QTextStream(stderr) << "FAIL " << why << '\n'; std::exit(1); }
}
static bool accepted(const reader_metadata::Result &r) { return QByteArray(r.stage) == "metadata-observed"; }
static bool same(const reader_metadata::Result &a, const reader_metadata::Result &b) {
    return QByteArray(a.stage) == b.stage && a.applicationThread == b.applicationThread &&
        a.currentWindow == b.currentWindow && a.ownerLinks == b.ownerLinks && a.metadata == b.metadata &&
        a.pageStringDomain == b.pageStringDomain && a.visited == b.visited &&
        a.helpers == b.helpers && a.containers == b.containers;
}
static void focus(QQuickWindow *w) {
    w->show(); w->requestActivate();
    for (int i = 0; i < 8; ++i) QCoreApplication::processEvents();
    require(QGuiApplication::focusWindow() == w && w->isActive(), "owned focus established");
}
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    qmlRegisterType<WindowTrap>("ExplicitOwned", 1, 0, "WindowTrap");
    qmlRegisterType<HelperTrap>("ExplicitOwned", 1, 0, "HelperTrap");
    auto engine = std::make_unique<QQmlEngine>();
    QQmlComponent component(engine.get());
    component.setData(R"QML(import QtQuick
import ExplicitOwned 1.0
Window { width: 100; height: 100; visible: true
    WindowTrap { objectName: "owned-reader"
        HelperTrap { objectName: "DocumentView.ToolSettings" }
        WindowTrap { Item { objectName: "SceneViewsRoot" } }
    }
})QML", QUrl("explicit-owned.qml"));
    std::unique_ptr<QObject> first(component.create()), second(component.create());
    auto *a = qobject_cast<QQuickWindow *>(first.get());
    auto *b = qobject_cast<QQuickWindow *>(second.get());
    require(a && b, "two original owned QML windows");
    new QObject(b->contentItem()); // Distinct domain count makes selection observable.
    const auto yes = [] { return true; };
    const auto ownerReparent = [&] {
        focus(b);
        QPointer<QQuickItem> container = b->findChild<QQuickItem *>(QStringLiteral("SceneViewsRoot"));
        require(bool(container), "owned structural endpoint");
        QPointer<QQuickItem> originalParent(container->parentItem());
        int total = 0;
        require(accepted(reader_metadata::censusWindow(b, b->contentItem(), [&] { ++total; return true; })),
            "count endpoint final permit");
        int calls = 0;
        const auto result = reader_metadata::censusWindow(b, b->contentItem(), [&] {
            if (++calls == total) container->setParentItem(b->contentItem());
            return true;
        });
        const bool changed = container && container->parentItem() == b->contentItem();
        container->setParentItem(originalParent);
        require(calls == total && changed && !accepted(result),
            "final permit same-window container reparent must refuse stale owner proof");
        QTextStream(stdout) << "PASS final-permit same-window owner reparent refusal\n";
    };
    if (argc == 2 && QByteArray(argv[1]) == "owner-reparent-only") { ownerReparent(); return 0; }
    ownerReparent();
    focus(a);
    const auto original = reader_metadata::census(yes);
    const auto explicitA = reader_metadata::censusWindow(a, a->contentItem(), yes);
    require(accepted(explicitA) && same(original, explicitA) && explicitA.visited == 6,
        "legacy compatibility and explicit first window");
    focus(b);
    const auto explicitB = reader_metadata::censusWindow(b, b->contentItem(), yes);
    require(accepted(explicitB) && explicitB.visited == 7 && same(reader_metadata::census(yes), explicitB),
        "explicit second window domain and legacy compatibility");
    require(!accepted(reader_metadata::censusWindow(a, a->contentItem(), yes)), "inactive supplied window no focus fallback");
    require(!accepted(reader_metadata::censusWindow(b, a->contentItem(), yes)), "mismatched content root refused");
    require(!accepted(reader_metadata::censusWindow({}, b->contentItem(), yes)) &&
        !accepted(reader_metadata::censusWindow(b, {}, yes)), "missing inputs no fallback");
    require(!accepted(reader_metadata::censusWindow(b, b->contentItem(), {})), "missing permit refused");
    const auto expired = reader_metadata::censusWindow(b, b->contentItem(), [] { return false; });
    require(QByteArray(expired.stage) == "canceled-or-expired" && expired.visited == 0,
        "caller deadline refuses before traversal, no renewed operation clock");
    reader_metadata::Result wrongThread;
    const QPointer<QQuickWindow> threadWindow(b);
    const QPointer<QQuickItem> threadRoot(b->contentItem());
    std::thread worker([&] { wrongThread = reader_metadata::censusWindow(threadWindow, threadRoot, yes); });
    worker.join();
    require(!wrongThread.applicationThread && !accepted(wrongThread), "non-GUI caller refused");
    auto *reader = b->findChild<QObject *>(QStringLiteral("owned-reader"));
    require(reader, "owned reader located by fixture");
    {
        std::vector<std::unique_ptr<QObject>> children;
        for (int i = 0; i < 65; ++i) children.emplace_back(std::make_unique<QObject>(reader));
        const auto capped = reader_metadata::censusWindow(b, b->contentItem(), yes);
        require(QByteArray(capped.stage) == "census-cap" && same(capped, reader_metadata::census(yes)),
            "explicit traversal retains legacy cap refusal");
    }
    {
        QObject duplicate(reader); duplicate.setObjectName("DocumentView.ToolSettings");
        require(QByteArray(reader_metadata::censusWindow(b, b->contentItem(), yes).stage) == "anchor-uniqueness",
            "duplicate structure not favorable selection");
    }
    int calls = 0;
    require(accepted(reader_metadata::censusWindow(b, b->contentItem(), [&] { ++calls; return true; })),
        "count valid-path permit checks");
    const int last = calls;
    calls = 0;
    const auto lateLoss = reader_metadata::censusWindow(b, b->contentItem(), [&] {
        if (++calls == last) b->hide();
        return true;
    });
    require(calls == last && !accepted(lateLoss), "last permit reentry cannot hide final context loss");
    focus(b);
    calls = 0;
    require(!accepted(reader_metadata::censusWindow(b, b->contentItem(), [&] { return ++calls < 5; })),
        "permit revoked during traversal");
    QPointer<QQuickWindow> retainedA(a);
    QPointer<QQuickItem> rootA(a->contentItem());
    first.reset();
    require(!retainedA && !rootA && !accepted(reader_metadata::censusWindow(retainedA, rootA, yes)),
        "deleted retained inputs refuse rather than select current second window");
    QPointer<QQuickWindow> retainedB(b);
    QPointer<QQuickItem> rootB(b->contentItem());
    calls = 0;
    require(!accepted(reader_metadata::censusWindow(retainedB, rootB, [&] {
        if (++calls == 3) second.reset();
        return true;
    })) && !retainedB && !rootB, "window/root deletion within permit guarded");
    require(valueReads == 0, "zero application VALUE getter calls through all new controls");
    QTextStream(stdout) << "PASS explicit-window/root census: compatibility, domain/no fallback, input/thread/permit/context/deletion, cap/ambiguity; value_reads=0 (owned only)\n";
}
#include "reader_explicit_window_fixture.moc"
