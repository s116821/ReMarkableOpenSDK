#pragma once
// Local owned-fixture assessment only; no live/native adapter authority.
#include <QElapsedTimer>
#include <QMetaProperty>
#include <QPointer>
#include <QQuickItem>
#include <QQuickWindow>
#include <QThread>
#include <QVariant>
#include <functional>

namespace reader_owned {
struct Result {
    QString refusal;
    QPointer<QObject> document, controller;
    QPointer<QQuickItem> current;
    QString page;
    bool nativeAuthority = false;
    bool accepted() const { return refusal.isEmpty() && document && controller && current; }
};
inline Result select(QQuickWindow *supplied, const std::function<bool()> &guard = [] { return true; }) {
    const auto no = [](const char *reason) { Result r; r.refusal = QString::fromLatin1(reason); return r; };
    if (!supplied || supplied->thread() != QThread::currentThread()) return no("owned-window-thread");
    QPointer<QQuickWindow> window(supplied);
    QPointer<QQuickItem> root(supplied->contentItem());
    QElapsedTimer timer; timer.start();
    const auto live = [&] {
        return window && root && timer.elapsed() < 1000 && guard() &&
            window && root && timer.elapsed() < 1000;
    };
    if (!live()) return no("owned-guard");
    struct Node { QPointer<QObject> object; int depth; };
    QList<Node> queue{{root.data(), 0}};
    QList<QPointer<QObject>> all, helpers, containers;
    for (qsizetype i = 0; i < queue.size(); ++i) {
        if (!live() || !queue[i].object) return no("owned-guard");
        if (queue[i].depth > 12 || all.size() >= 128) return no("owned-census-cap");
        QPointer<QObject> item = queue[i].object;
        all.append(item);
        const QString name = item->objectName();
        if (!live() || !item) return no("owned-guard");
        if (name == QStringLiteral("DocumentView.ToolSettings")) helpers.append(item);
        if (name == QStringLiteral("SceneViewsRoot")) containers.append(item);
        const QObjectList children = item->children();
        if (!live() || !item) return no("owned-guard");
        if (children.size() > 64 || queue.size() + children.size() > 128) return no("owned-census-cap");
        for (QObject *child : children) queue.append({child, queue[i].depth + 1});
    }
    if (helpers.size() != 1 || containers.size() != 1) return no("owned-anchor-uniqueness");
    QPointer<QObject> helper = helpers.first();
    QPointer<QQuickItem> container = qobject_cast<QQuickItem *>(containers.first().data());
    if (!live() || !helper || !container) return no("owned-anchor-type");
    QPointer<QQuickItem> reader = qobject_cast<QQuickItem *>(helper->parent());
    QPointer<QQuickItem> device = container->parentItem();
    const auto visual = [&](const QPointer<QQuickItem> &p) {
        if (!live() || !p) return false;
        const bool ok = p->window() == window && p->isVisible() && p->isEnabled();
        return live() && p && ok;
    };
    if (!reader || !device || device->parentItem() != reader || !visual(reader) || !visual(device) || !visual(container))
        return no("owned-owner-links");
    const auto known = [&](QObject *raw) -> QPointer<QObject> {
        for (const QPointer<QObject> &p : all) if (p && p.data() == raw) return p;
        return {};
    };
    const auto read = [&](const QPointer<QObject> &owner, const char *name, bool pointer, QVariant &out) {
        if (!live() || !owner) return false;
        const QMetaObject *meta = owner->metaObject();
        if (!live() || !owner) return false;
        const int index = meta->indexOfProperty(name);
        if (index < 0) return false;
        const QMetaProperty property = meta->property(index);
        if (!property.isReadable() || (pointer ? !property.metaType().flags().testFlag(QMetaType::PointerToQObject)
                                              : property.metaType().id() != QMetaType::QString)) return false;
        out = property.read(owner);
        if (!live() || !owner || !out.isValid()) return false;
        return pointer ? out.metaType().flags().testFlag(QMetaType::PointerToQObject)
                       : out.metaType().id() == QMetaType::QString;
    };
    const auto ptr = [&](const QPointer<QObject> &owner, const char *name) -> QPointer<QObject> {
        QVariant v;
        return read(owner, name, true, v) ? known(v.value<QObject *>()) : QPointer<QObject>{};
    };
    const auto text = [&](const QPointer<QObject> &owner, const char *name, QString &out) {
        QVariant v;
        if (!read(owner, name, false, v)) return false;
        out = v.value<QString>();
        return !out.isEmpty();
    };
    struct Snapshot { QPointer<QObject> doc, controller; QPointer<QQuickItem> view; QString page; };
    const auto snapshot = [&](Snapshot &s) {
        if (!live() || !reader || !device || !container || !helper || helper->parent() != reader ||
            container->parentItem() != device || device->parentItem() != reader ||
            !visual(reader) || !visual(device) || !visual(container)) return false;
        s.doc = ptr(reader, "document"); s.controller = ptr(reader, "sceneController");
        const QPointer<QObject> rawView = ptr(device, "sceneView");
        s.view = qobject_cast<QQuickItem *>(rawView.data());
        if (!s.doc || !s.controller || !s.view || !visual(s.view) || s.view->parentItem() != container) return false;
        if (ptr(helper, "document") != s.doc || ptr(device, "document") != s.doc ||
            ptr(s.view, "document") != s.doc || ptr(device, "controller") != s.controller ||
            ptr(s.view, "controller") != s.controller) return false;
        QString devicePage, viewPage;
        return text(reader, "currentPageId", s.page) && text(device, "pageId", devicePage) &&
            text(s.view, "pageId", viewPage) && s.page == devicePage && s.page == viewPage && live();
    };
    Snapshot before, after;
    if (!snapshot(before) || !snapshot(after) || before.doc != after.doc ||
        before.controller != after.controller || before.view != after.view || before.page != after.page)
        return no("owned-reference-refusal");
    if (!live() || !after.doc || !after.controller || !after.view) return no("owned-guard");
    Result result; result.document = after.doc; result.controller = after.controller;
    result.current = after.view; result.page = after.page; return result;
}
}
