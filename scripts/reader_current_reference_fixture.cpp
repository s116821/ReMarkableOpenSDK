#include "reader_current_reference_owned.h"
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QTextStream>
#include <memory>
#include <vector>
#include <cstdlib>
#include <thread>
#include <chrono>

static const char qml[] = R"QML(
import QtQuick
FocusScope {
    id: outer
    width: 100; height: 100
    QtObject { id: doc }
    QtObject { id: ctrl }
    FocusScope {
        id: reader
        property QtObject document: doc
        property alias currentPageId: device.pageId
        property QtObject sceneController: ctrl
        QtObject {
            objectName: "DocumentView.ToolSettings"
            property QtObject document: reader.document
        }
        FocusScope {
            id: device
            property QtObject document: reader.document
            property string pageId: "synthetic-current"
            property QtObject controller: ctrl
            property Item sceneView: current
            property Item nextSceneView: next
            property Item previousSceneView: previous
            Item {
                objectName: "SceneViewsRoot"
                Item { id: next; objectName: "fixture-next"; visible: false
                    property QtObject document: doc; property string pageId: "synthetic-next"; property QtObject controller: ctrl }
                Item { id: previous; visible: false
                    property QtObject document: doc; property string pageId: "synthetic-previous"; property QtObject controller: ctrl }
                Item { id: current; objectName: "fixture-current"
                    property QtObject document: doc; property string pageId: device.pageId; property QtObject controller: ctrl }
            }
        }
    }
    Item { objectName: "fixture-keyboard"; focus: true }
}
)QML";

static void require(bool ok, const char *message) {
    if (!ok) { QTextStream(stderr) << "FAIL " << message << '\n'; std::exit(1); }
}
static void emptyRefusal(const reader_owned::Result &r) {
    require(!r.accepted() && !r.refusal.isEmpty() && !r.document && !r.controller && !r.current && r.page.isEmpty()
            && !r.nativeAuthority, "refusal leaked a partial reference");
}
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(qml, QUrl(QStringLiteral("owned-reader-fixture.qml")));
    if (component.isError()) {
        for (const auto &error : component.errors()) QTextStream(stderr) << error.toString() << '\n';
        return 1;
    }
    QQuickWindow window;
    std::unique_ptr<QObject> object(component.create());
    require(bool(object), "actual QML creation");
    QQuickItem *outer = qobject_cast<QQuickItem *>(object.get());
    require(outer != nullptr, "owned visual root");
    outer->setParentItem(window.contentItem());
    outer->setParent(window.contentItem());
    window.show(); app.processEvents();
    QObject *helper = outer->findChild<QObject *>(QStringLiteral("DocumentView.ToolSettings"));
    QQuickItem *container = outer->findChild<QQuickItem *>(QStringLiteral("SceneViewsRoot"));
    require(helper && container, "fixture setup anchors");
    QQuickItem *device = container->parentItem();
    QObject *reader = helper->parent();
    QQuickItem *current = outer->findChild<QQuickItem *>(QStringLiteral("fixture-current"));
    require(current && container->childItems().first() != current, "current must not be first");
    require(window.activeFocusItem() != current, "keyboard focus must not select current");
    auto r = reader_owned::select(&window);
    if (!r.accepted()) QTextStream(stderr) << "refusal=" << r.refusal << '\n';
    require(r.accepted() && r.current == current && r.page == QStringLiteral("synthetic-current") && !r.nativeAuthority,
            "public reference/real alias selection");
    require(device->setProperty("pageId", QStringLiteral("synthetic-changed")), "fixture page change");
    r = reader_owned::select(&window);
    require(r.accepted() && r.page == QStringLiteral("synthetic-changed") &&
            reader->property("currentPageId").toString() == r.page, "actual alias propagation");
    QObject duplicate(reader); duplicate.setObjectName(QStringLiteral("DocumentView.ToolSettings"));
    emptyRefusal(reader_owned::select(&window)); duplicate.setObjectName(QString());
    require(current->setProperty("pageId", QStringLiteral("mismatch")), "owned mismatch setup");
    emptyRefusal(reader_owned::select(&window));
    current->setProperty("pageId", device->property("pageId"));
    const QVariant saved = device->property("sceneView");
    device->setProperty("sceneView", QVariant::fromValue<QQuickItem *>(nullptr));
    emptyRefusal(reader_owned::select(&window)); device->setProperty("sceneView", saved);
    const QVariant doc = device->property("document");
    device->setProperty("document", QVariant::fromValue<QObject *>(nullptr));
    emptyRefusal(reader_owned::select(&window)); device->setProperty("document", doc);
    const QVariant controller = device->property("controller");
    device->setProperty("controller", doc);
    emptyRefusal(reader_owned::select(&window)); device->setProperty("controller", controller);
    emptyRefusal(reader_owned::select(&window, [] { return false; }));
    bool delayed = false;
    emptyRefusal(reader_owned::select(&window, [&] {
        if (!delayed) { delayed = true; std::this_thread::sleep_for(std::chrono::milliseconds(1100)); }
        return true;
    }));
    std::vector<std::unique_ptr<QObject>> excessive; // retain owned extra nodes for concrete cap refusal
    for (int i = 0; i < 65; ++i) excessive.emplace_back(std::make_unique<QObject>(reader));
    emptyRefusal(reader_owned::select(&window));
    QTextStream(stdout) << "PASS owned reader current reference: alias, non-first selection, duplicate, inconsistent/null, cap/cancel/deadline; native_authority=false\n";
    return 0;
}
