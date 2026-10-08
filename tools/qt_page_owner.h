#pragma once
#include <QGuiApplication>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQmlEngine>
#include <QPointer>
#include <QMetaProperty>
#include <QMetaMethod>
#include <QThread>
#include <QByteArrayView>
#include <functional>

namespace qml_access {
// Development-only finite live topology evidence. No getter result is a lease.
struct PageOwner {
    QPointer<QQuickWindow> window;
    QPointer<QQuickItem> receiver, scene;
    QPointer<QObject> document;
};
inline bool propertyType(QObject *object, const char *name, QMetaType type) {
    if (!object) return false;
    const int index = object->metaObject()->indexOfProperty(name);
    return index >= 0 && object->metaObject()->property(index).isReadable() &&
        object->metaObject()->property(index).metaType() == type;
}
inline bool classInherits(QObject *object, const char *name) {
    if (!object) return false;
    const QMetaObject *meta = object->metaObject();
    for (int depth = 0; meta && depth < 24; ++depth, meta = meta->superClass())
        if (QByteArrayView(meta->className()) == QByteArrayView(name)) return true;
    return false;
}
inline bool objectProperty(QObject *object, const char *name, QPointer<QObject> &value) {
    const QPointer<QObject> guard = object;
    if (!object) return false;
    const int index = object->metaObject()->indexOfProperty(name);
    if (index < 0) return false;
    const auto property = object->metaObject()->property(index);
    if (!property.isReadable() || !(property.metaType().flags() & QMetaType::PointerToQObject)) return false;
    const QVariant observed = property.read(object);
    if (!guard || !observed.isValid() || !(observed.metaType().flags() & QMetaType::PointerToQObject)) return false;
    value = *static_cast<QObject *const *>(observed.constData());
    return value && value->thread() == object->thread();
}
struct PageOwnerDiagnostics {
    int visited=-1, receivers=-1, scenes=-1, matches=-1;
    int firstReceiver=-1, firstScene=-1;
    const char *firstRejection=nullptr, *finalRejection=nullptr;
};
inline bool activeOwner(const PageOwner &owner, QQmlEngine *engine, const char **reason=nullptr) {
    const auto refuse=[reason](const char *value){if(reason)*reason=value;return false;};
    if (!owner.window || !owner.receiver || !owner.scene || !owner.document || !engine ||
        QThread::currentThread() != engine->thread() || owner.window->thread() != engine->thread() ||
        owner.receiver->thread() != engine->thread() || owner.scene->thread() != engine->thread() ||
        owner.document->thread() != engine->thread()) return refuse("pointers-or-threads");
    if (QGuiApplication::focusWindow() != owner.window || !owner.window->isActive() ||
        !owner.window->isVisible()) return refuse("window-focus-active-visible");
    if (qmlEngine(owner.receiver) != engine || qmlEngine(owner.scene) != engine) return refuse("engine-association");
    if (owner.receiver->window() != owner.window || owner.scene->window() != owner.window) return refuse("window-association");
    if (!owner.receiver->isVisible() || !owner.receiver->isEnabled() ||
        !owner.scene->isVisible() || !owner.scene->isEnabled()) return refuse("item-visible-enabled");
    if (!owner.scene->hasActiveFocus()) return refuse("scene-active-focus");
    bool descendant = false;
    QPointer<QQuickItem> parent = owner.scene->parentItem();
    for (int depth = 0; parent && depth < 24; ++depth, parent = parent->parentItem())
        if (parent == owner.receiver) { descendant = true; break; }
    if (!descendant) return refuse("receiver-ancestor");
    const QVariant focused = owner.receiver->property("drawingAreaFocused");
    if (!owner.receiver || !owner.scene || focused.metaType() != QMetaType::fromType<bool>() || !focused.toBool()) return refuse("drawing-area-focused");
    QPointer<QObject> rootDocument, sceneDocument;
    if (!objectProperty(owner.receiver, "document", rootDocument)) return refuse("receiver-document");
    if (!objectProperty(owner.scene, "document", sceneDocument)) return refuse("scene-document");
    if (!(owner.document && rootDocument == owner.document && sceneDocument == owner.document)) return refuse("document-identity");
    return true;
}
// One bounded traversal of the focused window only. Lexical QML ids are ignored.
// Compatibility is structural plus the saved SceneView class relationship;
// identity/index mapping and callable exposure must still be verified in QML.
inline const char *findPageOwner(QQmlEngine *engine, const std::function<bool()> &progress, PageOwner &out, PageOwnerDiagnostics *diagnostics=nullptr) {
    out = {};
    const QPointer<QQmlEngine> producer = engine;
    if (!engine || QThread::currentThread() != engine->thread()) return "open-engine-thread";
    const QPointer<QQuickWindow> window = qobject_cast<QQuickWindow *>(QGuiApplication::focusWindow());
    if (!window || !window->isActive() || !window->isVisible()) return "open-current-window-unavailable";
    struct Node { QPointer<QQuickItem> item; int depth; };
    QList<Node> queue{{window->contentItem(), 0}};
    QList<QPointer<QQuickItem>> receivers, scenes;
    int visited = 0;
    if(diagnostics){diagnostics->visited=0;diagnostics->receivers=0;diagnostics->scenes=0;}
    for (int index = 0; index < queue.size(); ++index) {
        if (!progress() || !window || !producer) return "open-context-lost";
        const auto node = queue[index];
        if (!node.item || node.item->thread() != engine->thread()) return "open-item-lost";
        ++visited;
        if(diagnostics)diagnostics->visited=visited;
        if (visited > 4096 || node.depth > 24) return "open-topology-bound";
        if (qmlEngine(node.item) == engine &&
            (node.item->flags() & QQuickItem::ItemIsFocusScope) &&
            propertyType(node.item, "currentPage", QMetaType::fromType<int>()) &&
            propertyType(node.item, "currentPageId", QMetaType::fromType<QString>()) &&
            propertyType(node.item, "drawingAreaFocused", QMetaType::fromType<bool>()) &&
            node.item->metaObject()->indexOfProperty("document") >= 0) receivers.append(node.item);
        if (qmlEngine(node.item) == engine && classInherits(node.item, "SceneView") &&
            propertyType(node.item, "pageId", QMetaType::fromType<QString>()) &&
            node.item->metaObject()->indexOfSignal("pageIdChanged()") >= 0 &&
            node.item->metaObject()->indexOfSignal("documentWrapperChanged()") >= 0) scenes.append(node.item);
        if(diagnostics){diagnostics->receivers=receivers.size();diagnostics->scenes=scenes.size();}
        if (receivers.size() > 8 || scenes.size() > 8) return "open-candidate-bound";
        const auto children = node.item->childItems();
        if (children.size() > 4096 - queue.size()) return "open-topology-bound";
        for (auto *child : children) queue.append({child, node.depth + 1});
    }
    int matches = 0;
    if(diagnostics)diagnostics->matches=0;
    int receiverOrdinal=-1;
    for (const auto &receiver : receivers) {
      ++receiverOrdinal;
      int sceneOrdinal=-1;
      for (const auto &scene : scenes) {
        ++sceneOrdinal;
        const auto rejected=[&](const char *reason){
            if(diagnostics && diagnostics->firstReceiver<0){diagnostics->firstReceiver=receiverOrdinal;diagnostics->firstScene=sceneOrdinal;diagnostics->firstRejection=reason;}
        };
        if (!progress() || !producer) return "open-context-lost";
        PageOwner candidate{window, receiver, scene, {}};
        if (!objectProperty(receiver, "document", candidate.document)) {rejected("receiver-document");continue;}
        const char *reason=nullptr;
        if (!activeOwner(candidate, engine,diagnostics ? &reason:nullptr)) {rejected(reason);continue;}
        ++matches;
        if(diagnostics)diagnostics->matches=matches;
        if (matches > 1) { out = {}; return "open-owner-ambiguous"; }
        out = candidate;
    }
    }
    if (!matches) return "open-owner-unavailable";
    if (!progress() || !producer || !activeOwner(out, producer,diagnostics ? &diagnostics->finalRejection:nullptr)) { out = {}; return "open-context-lost"; }
    return "open-owner-observed";
}
}
