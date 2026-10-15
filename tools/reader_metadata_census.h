#pragma once
// Independent structural diagnostic. Never read application property values.
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QMetaProperty>
#include <QPointer>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QThread>
#include <functional>

namespace reader_metadata {
struct Result {
    const char *stage = "context-refused";
    bool applicationThread = false, currentWindow = false, ownerLinks = false;
    bool metadata = false, pageStringDomain = false;
    int visited = 0, helpers = 0, containers = 0;
};
inline Result census(const std::function<bool()> &permit) {
    Result r;
    QPointer<QGuiApplication> app = qobject_cast<QGuiApplication *>(QCoreApplication::instance());
    if (!app || app->thread() != QThread::currentThread()) return r;
    r.applicationThread = true;
    QElapsedTimer elapsed; elapsed.start();
    const auto live = [&] {
        return app && !QCoreApplication::closingDown() && elapsed.elapsed() < 1000 && permit() &&
            app && !QCoreApplication::closingDown() && elapsed.elapsed() < 1000;
    };
    if (!live()) { r.stage = "canceled-or-expired"; return r; }
    const auto rawWindows = QGuiApplication::allWindows();
    if (rawWindows.size() > 16) { r.stage = "window-cap"; return r; }
    QList<QPointer<QWindow>> windows;
    for (auto *w : rawWindows) windows.append(w);
    QPointer<QQuickWindow> window = qobject_cast<QQuickWindow *>(QGuiApplication::focusWindow());
    QPointer<QQmlEngine> engine;
    for (const auto &w : windows) {
        if (!live() || !w || w->thread() != app->thread()) return r;
        QPointer<QQmlEngine> e = qmlEngine(w);
        if (!live() || !w) return r;
        if (e && engine && e != engine) { r.stage = "engine-ambiguous"; return r; }
        if (e) engine = e;
    }
    const auto context = [&] {
        if (!live() || !window || !engine || engine->thread() != app->thread()) return false;
        const bool ok = window->thread() == app->thread() && window->isVisible() && window->isActive() &&
            QGuiApplication::focusWindow() == window && qmlEngine(window) == engine;
        return live() && window && engine && ok;
    };
    if (!context()) return r;
    r.currentWindow = true;
    QPointer<QQuickItem> root = window->contentItem();
    struct Node { QPointer<QObject> p; int depth; };
    QList<Node> queue{{root.data(), 0}};
    QList<QPointer<QObject>> helpers, containers;
    r.stage = "census-refused";
    for (qsizetype i = 0; i < queue.size(); ++i) {
        QPointer<QObject> p = queue[i].p;
        if (!context() || !root || !p || p->thread() != app->thread()) return r;
        if (queue[i].depth > 12 || r.visited >= 128) { r.stage = "census-cap"; return r; }
        ++r.visited;
        const auto name = p->objectName();
        if (!context() || !p) return r;
        if (name == QStringLiteral("DocumentView.ToolSettings")) { helpers.append(p); ++r.helpers; }
        if (name == QStringLiteral("SceneViewsRoot")) { containers.append(p); ++r.containers; }
        const auto children = p->children();
        if (children.size() > 64) { r.stage = "census-cap"; return r; }
        QList<QPointer<QObject>> edges;
        for (auto *child : children) edges.append(child);
        if (!context() || !p) return r;
        QPointer<QQuickItem> visualNode = qobject_cast<QQuickItem *>(p.data());
        if (visualNode) {
            if (visualNode->window() != window) { r.stage = "foreign-window"; return r; }
            const auto visualChildren = visualNode->childItems();
            if (visualChildren.size() > 64) { r.stage = "census-cap"; return r; }
            for (auto *child : visualChildren) {
                QPointer<QObject> weak = child;
                if (!edges.contains(weak)) edges.append(weak);
            }
        }
        if (!context() || !p) return r;
        if (edges.size() > 64) { r.stage = "census-cap"; return r; }
        const int depth = queue[i].depth + 1;
        for (const auto &child : edges) {
            if (!child) return r;
            bool seen = false;
            for (const auto &node : queue) {
                if (!node.p) return r;
                if (node.p == child) seen = true;
            }
            if (seen) continue;
            if (queue.size() >= 128 || depth > 12) { r.stage = "census-cap"; return r; }
            queue.append({child, depth});
        }
        if (!context() || !p) return r;
    }
    if (helpers.size() != 1 || containers.size() != 1) { r.stage = "anchor-uniqueness"; return r; }
    QPointer<QObject> helper = helpers.first();
    QPointer<QQuickItem> container = qobject_cast<QQuickItem *>(containers.first().data());
    if (!context() || !helper || !container) return r;
    QPointer<QQuickItem> reader = qobject_cast<QQuickItem *>(helper->parent());
    QPointer<QQuickItem> device = container->parentItem();
    const auto visual = [&](const QPointer<QQuickItem> &item) {
        if (!context() || !item) return false;
        const bool ok = item->window() == window && item->isVisible() && item->isEnabled();
        return context() && item && ok;
    };
    const auto owners = [&] {
        return context() && root && window->contentItem() == root && helper && reader && device && container &&
            helper->parent() == reader && container->parentItem() == device && device->parentItem() == reader &&
            visual(reader) && visual(device) && visual(container);
    };
    if (!owners()) { r.stage = "owner-links"; return r; }
    r.ownerLinks = true;
    const auto metadata = [&](const QPointer<QObject> &owner, const char *name, bool pointer, bool &stringType) {
        if (!owners() || !owner) return false;
        const auto *meta = owner->metaObject();
        if (!owners() || !owner) return false;
        if (meta->propertyCount() > 512) return false;
        const int index = meta->indexOfProperty(name);
        if (index < 0) return false;
        const QMetaProperty property = meta->property(index);
        const auto type = property.metaType();
        const bool ok = property.isReadable() && (!pointer || type.flags().testFlag(QMetaType::PointerToQObject));
        stringType = type.id() == QMetaType::QString;
        return owners() && owner && ok;
    };
    bool unused = false, readerString = false, deviceString = false;
    r.metadata = metadata(reader, "document", true, unused) &&
        metadata(reader, "currentPageId", false, readerString) &&
        metadata(reader, "sceneController", true, unused) &&
        metadata(helper, "document", true, unused) &&
        metadata(device, "document", true, unused) &&
        metadata(device, "pageId", false, deviceString) &&
        metadata(device, "controller", true, unused) &&
        metadata(device, "sceneView", true, unused);
    r.pageStringDomain = readerString && deviceString;
    if (!r.metadata) { r.stage = "metadata-refused"; return r; }
    if (!owners()) { r.stage = "endpoint-refused"; return r; }
    r.stage = "metadata-observed";
    return r;
}
}
