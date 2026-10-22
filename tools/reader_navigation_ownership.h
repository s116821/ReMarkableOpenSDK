#pragma once
// Original observational diagnostic. Neither groups nor completion grant authority.
#include "reader_selected_view_probe.h"
#include <QJsonArray>
#include <QJsonObject>

namespace reader_selected {
struct NavigationOwnership {
    bool complete = false;
    int visited = 0;
    QJsonArray owners;
    QString refusal;
};
namespace ownership_detail {
inline constexpr char NavigatorClass[] = "xofm::bindings::qt::WindowNavigatorWrapper";
inline constexpr char LayoutClass[] = "xofm::bindings::qt::WindowLayout";

class Census {
    QPointer<QGuiApplication> app;
    QPointer<QQuickWindow> window;
    QPointer<QQuickItem> root;
    QPointer<QQmlEngine> engine;
    detail::Epoch epoch;
    QList<QPointer<QObject>> retained, navigators, layouts;
    QList<QPair<QObject *, int>> watched;
    struct Scope { QPointer<QQmlContext> context; QPointer<QObject> root, main; };
    QList<Scope> scopes;
    const QElapsedTimer &elapsed;
    const std::function<bool()> &permit;
    int budget;
    NavigationOwnership result;

    bool no(const char *reason) {
        if (result.refusal.isEmpty()) result.refusal = QString::fromLatin1(reason);
        return false;
    }
    bool retain(QObject *object) {
        QPointer<QObject> guard = object;
        if (!guard || guard->thread() != QThread::currentThread()) return no("ownership-object-lifetime");
        for (const auto &old : retained) if (old == guard) return true;
        if (retained.size() >= 640) return no("ownership-object-cap");
        retained.append(guard);
        if (!QObject::connect(guard, &QObject::destroyed, &epoch, &detail::Epoch::changed, Qt::DirectConnection))
            return no("ownership-notification");
        return guard ? true : no("ownership-object-lifetime");
    }
    bool context() {
        if (!result.refusal.isEmpty()) return false;
        if (!app || !window || !root || !engine || QCoreApplication::closingDown() ||
            QThread::currentThread() != app->thread() || window->thread() != app->thread() || engine->thread() != app->thread())
            return no("ownership-context");
        if (detail::entry.reentered) return no("sample-reentered");
        if (epoch.value != 0) return no("ownership-changed");
        if (elapsed.elapsed() >= budget) return no("deadline");
        for (const auto &guard : retained) {
            if (!guard || guard->thread() != app->thread()) return no("ownership-object-lifetime");
            const auto scope = qobject_cast<QQmlContext *>(guard.data());
            if (scope && (!scope->isValid() || scope->engine() != engine)) return no("ownership-context-changed");
        }
        if (window->contentItem() != root || qmlEngine(window) != engine || !window->isVisible() ||
            !window->isActive() || QGuiApplication::focusWindow() != window) return no("ownership-window");
        return true;
    }
    bool allowed() {
        if (!context()) return false;
        if (!permit || !permit()) return no("canceled");
        return context();
    }
    bool signal(QObject *object, const char *signature) {
        QPointer<QObject> guard = object;
        if (!retain(guard) || !allowed()) return false;
        const int index = guard->metaObject()->indexOfSignal(signature);
        if (index < 0) return no("ownership-notification");
        const QPair<QObject *, int> key(guard.data(), index);
        if (watched.contains(key)) return true;
        const int target = epoch.metaObject()->indexOfSlot("changed()");
        if (!QObject::connect(guard, guard->metaObject()->method(index), &epoch,
                epoch.metaObject()->method(target), Qt::DirectConnection)) return no("ownership-notification");
        watched.append(key);
        return allowed();
    }
    bool property(QObject *object, const char *name, QMetaProperty &out) {
        QPointer<QObject> guard = object;
        if (!retain(guard) || !allowed()) return false;
        const QMetaObject *meta = guard->metaObject();
        if (meta->propertyCount() > 512) return no("ownership-property-cap");
        const int index = meta->indexOfProperty(name);
        if (index < 0) return false;
        out = meta->property(index);
        if (!out.isReadable()) return no("ownership-property-unreadable");
        if (!out.isConstant() && (!out.hasNotifySignal() ||
            !signal(guard, out.notifySignal().methodSignature().constData()))) return no("ownership-notification");
        return allowed();
    }
    bool watch(QObject *object, const char *name) {
        QMetaProperty p;
        return property(object, name, p) ? true : no("ownership-property-missing");
    }
    bool visualGuard(QQuickItem *object) {
        QPointer<QQuickItem> guard = object;
        if (!retain(guard) || !allowed()) return false;
        if (!watch(guard, "parent") || !watch(guard, "visible") || !watch(guard, "enabled") ||
            !signal(guard, "childrenChanged()") || !signal(guard, "windowChanged(QQuickWindow*)")) return false;
        return allowed();
    }
    bool item(QQuickItem *object) {
        QPointer<QQuickItem> guard = object;
        if (!visualGuard(guard)) return false;
        QQmlEngine *associated = qmlEngine(guard);
        if ((associated && associated != engine) || guard->window() != window) return no("ownership-foreign-owner");
        return allowed();
    }
    struct ObjectRead { QPointer<QObject> object; QString state = QStringLiteral("missing"); };
    ObjectRead object(QObject *owner, const char *name) {
        ObjectRead out;
        QPointer<QObject> guard = owner;
        QMetaProperty p;
        if (!property(guard, name, p)) return out;
        if (!p.metaType().flags().testFlag(QMetaType::PointerToQObject)) { out.state = QStringLiteral("wrong-type"); return out; }
        const QVariant value = p.read(guard);
        // Pin a live borrowed return before permit can destroy or replace it.
        out.object = value.metaType().flags().testFlag(QMetaType::PointerToQObject) ? value.value<QObject *>() : nullptr;
        if (out.object && !retain(out.object)) return out;
        if (!allowed()) return out;
        if (!value.isValid() || !value.metaType().flags().testFlag(QMetaType::PointerToQObject)) {
            no("ownership-property-read"); return out;
        }
        if (!out.object) { out.state = QStringLiteral("null"); return out; }
        QQmlEngine *associated = qmlEngine(out.object);
        out.state = associated && associated != engine ? QStringLiteral("foreign-engine") : QStringLiteral("ok");
        return out;
    }
    bool native(QObject *object, const char *name) {
        QPointer<QObject> guard = object;
        if (!guard || !retain(guard) || !allowed()) return false;
        const QMetaObject *meta = guard->metaObject();
        for (int depth = 0; meta && depth < 16; ++depth, meta = meta->superClass())
            if (qstrcmp(meta->className(), name) == 0) return true;
        return false;
    }
    int group(QList<QPointer<QObject>> &groups, const ObjectRead &read) {
        if (read.state != QStringLiteral("ok") || !read.object) return 0;
        for (qsizetype index = 0; index < groups.size(); ++index) if (groups[index] == read.object) return int(index + 1);
        groups.append(read.object); return int(groups.size());
    }
    static void bounded(QJsonObject &row, const char *name, const QString &text) {
        row.insert(QString::fromLatin1(name), text.left(128));
        row.insert(QString::fromLatin1(name) + QStringLiteral("_truncated"), text.size() > 128);
    }
    bool layoutScalars(QJsonObject &row, const ObjectRead &layout) {
        const bool matching = layout.state == QStringLiteral("ok") && native(layout.object, LayoutClass);
        row.insert(QStringLiteral("layout_native"), matching);
        if (!result.refusal.isEmpty()) return false;
        if (!matching) return allowed();
        QMetaProperty ready, active;
        if (!property(layout.object, "ready", ready) || !property(layout.object, "activeWindow", active))
            return no("ownership-layout-property");
        if (ready.metaType() != QMetaType::fromType<bool>() || active.metaType() != QMetaType::fromType<QString>())
            return no("ownership-layout-property-type");
        const QVariant readyValue = ready.read(layout.object);
        if (!allowed() || readyValue.metaType() != QMetaType::fromType<bool>()) return no("ownership-layout-read");
        const QVariant activeValue = active.read(layout.object);
        if (!allowed() || activeValue.metaType() != QMetaType::fromType<QString>()) return no("ownership-layout-read");
        if (activeValue.toString().size() > 128) return no("ownership-active-window-cap");
        row.insert(QStringLiteral("layout_ready"), readyValue.toBool());
        row.insert(QStringLiteral("layout_active_window"), activeValue.toString());
        return allowed();
    }
    bool lexical(QQuickItem *owner, const ObjectRead &nav, const ObjectRead &layout, QJsonObject &row) {
        QPointer<QQuickItem> ownerGuard = owner;
        QPointer<QQmlContext> scope = qmlContext(ownerGuard);
        row.insert(QStringLiteral("lexical_pair"), false);
        row.insert(QStringLiteral("composition_match"), false);
        row.insert(QStringLiteral("lexical_state"), scope ? QStringLiteral("no-pair") : QStringLiteral("no-context"));
        for (int depth = 0; scope && depth < 16; ++depth) {
            if (!retain(scope) || !allowed() || !scope->isValid() || scope->engine() != engine)
                return no("ownership-lexical-context");
            QPointer<QObject> lexicalRoot = scope->objectForName(QStringLiteral("root"));
            if (lexicalRoot && !retain(lexicalRoot)) return false;
            QPointer<QObject> mainView = scope->objectForName(QStringLiteral("mainView"));
            if (mainView && !retain(mainView)) return false;
            if (!allowed()) return false;
            scopes.append({scope, lexicalRoot, mainView});
            if (lexicalRoot && mainView) {
                row.insert(QStringLiteral("lexical_state"), QStringLiteral("pair"));
                row.insert(QStringLiteral("lexical_pair"), true);
                row.insert(QStringLiteral("lexical_depth"), depth);
                row.insert(QStringLiteral("owner_is_root"), ownerGuard == lexicalRoot);
                row.insert(QStringLiteral("owner_is_main_view"), ownerGuard == mainView);
                QPointer<QQuickItem> rootItem = qobject_cast<QQuickItem *>(lexicalRoot.data());
                QPointer<QQuickItem> mainItem = qobject_cast<QQuickItem *>(mainView.data());
                // Watch before the first relationship/visibility read, including
                // objects that have not yet reached their BFS turn. A foreign
                // or ineligible pair is still row data, not an owner gate.
                if ((rootItem && !visualGuard(rootItem)) || (mainItem && !visualGuard(mainItem))) return false;
                QQmlEngine *rootEngine = rootItem ? qmlEngine(rootItem) : nullptr;
                QQmlEngine *mainEngine = mainItem ? qmlEngine(mainItem) : nullptr;
                const bool visual = rootItem && mainItem && rootItem->window() == window && mainItem->window() == window &&
                    (!rootEngine || rootEngine == engine) && (!mainEngine || mainEngine == engine) &&
                    rootItem->isVisible() && rootItem->isEnabled() && mainItem->isVisible() && mainItem->isEnabled();
                row.insert(QStringLiteral("pair_visual"), visual);
                if (!visual) return allowed();
                if (!item(rootItem) || !item(mainItem)) return false;
                const ObjectRead content = object(rootItem, "contentItem");
                const bool linked = content.state == QStringLiteral("ok") && content.object == mainView;
                row.insert(QStringLiteral("root_content_is_main_view"), linked);
                row.insert(QStringLiteral("main_parent_is_root"), mainItem->parentItem() == rootItem);
                const ObjectRead rootNav = object(rootItem, "windowNavigator"), mainNav = object(mainItem, "windowNavigator");
                const ObjectRead rootLayout = object(rootItem, "windowLayout"), mainLayout = object(mainItem, "windowLayout");
                if (!allowed()) return false;
                const bool navSame = rootNav.state == QStringLiteral("ok") && mainNav.state == QStringLiteral("ok") &&
                    rootNav.object == mainNav.object && nav.state == QStringLiteral("ok") && nav.object == rootNav.object;
                const bool layoutSame = rootLayout.state == QStringLiteral("ok") && mainLayout.state == QStringLiteral("ok") &&
                    rootLayout.object == mainLayout.object && layout.state == QStringLiteral("ok") && layout.object == rootLayout.object;
                row.insert(QStringLiteral("pair_navigator_equal"), navSame);
                row.insert(QStringLiteral("pair_layout_equal"), layoutSame);
                row.insert(QStringLiteral("composition_match"), (ownerGuard == lexicalRoot || ownerGuard == mainView) && linked && navSame && layoutSame);
                return allowed();
            }
            QPointer<QQmlContext> parent = scope->parentContext();
            if (parent && !retain(parent)) return false;
            if (!allowed()) return false;
            scope = parent;
        }
        if (scope) return no("ownership-context-cap");
        return allowed();
    }
    bool owner(QQuickItem *candidate) {
        if (result.owners.size() >= 8) return no("ownership-owner-cap");
        QPointer<QQuickItem> guard = candidate;
        const ObjectRead nav = object(guard, "windowNavigator"), layout = object(guard, "windowLayout");
        if (!allowed()) return false;
        QJsonObject row;
        bounded(row, "class_name", QString::fromLatin1(guard->metaObject()->className()));
        QPointer<QQmlContext> scope = qmlContext(guard);
        if (scope && (!retain(scope) || !allowed())) return false;
        bounded(row, "context_url", scope ? scope->baseUrl().toString() : QString());
        row.insert(QStringLiteral("nav_state"), nav.state);
        row.insert(QStringLiteral("nav_group"), group(navigators, nav));
        row.insert(QStringLiteral("nav_native"), nav.state == QStringLiteral("ok") && native(nav.object, NavigatorClass));
        row.insert(QStringLiteral("layout_state"), layout.state);
        row.insert(QStringLiteral("layout_group"), group(layouts, layout));
        const ObjectRead content = object(guard, "contentItem");
        row.insert(QStringLiteral("content_state"), content.state);
        QPointer<QQuickItem> contentItem = content.state == QStringLiteral("ok") ? qobject_cast<QQuickItem *>(content.object.data()) : nullptr;
        if (contentItem && !visualGuard(contentItem)) return false;
        row.insert(QStringLiteral("content_parent_is_owner"), contentItem && contentItem->parentItem() == guard);
        row.insert(QStringLiteral("owner_parent_is_window_root"), guard->parentItem() == root);
        if (!layoutScalars(row, layout) || !lexical(guard, nav, layout, row) || !allowed()) return false;
        result.owners.append(row);
        return true;
    }
public:
    Census(QQuickWindow *candidate, const std::function<bool()> &permission, int milliseconds, const QElapsedTimer &clock)
        : elapsed(clock), permit(permission), budget(milliseconds) {
        app = qobject_cast<QGuiApplication *>(QCoreApplication::instance());
        if (!app || QThread::currentThread() != app->thread() || !candidate || candidate->thread() != app->thread()) {
            no("application-thread"); return;
        }
        window = candidate; root = candidate->contentItem(); engine = qmlEngine(candidate);
        if (!engine || !root) { no("ownership-context"); return; }
        app->installEventFilter(&epoch);
    }
    NavigationOwnership run() {
        if (!result.refusal.isEmpty() || !retain(app) || !retain(window) || !retain(root) || !retain(engine) || !allowed()) return result;
        if (!signal(app, "focusWindowChanged(QWindow*)") || !signal(window, "activeChanged()") || !signal(window, "visibleChanged(bool)")) return result;
        struct Node { QPointer<QQuickItem> object; int depth; };
        QList<Node> nodes{{root, 0}};
        for (qsizetype index = 0; index < nodes.size(); ++index) {
            const QPointer<QQuickItem> current = nodes[index].object;
            ++result.visited;
            if (!current || !item(current) || !allowed()) return result;
            // Excluded boundaries remain watched, including notified ABA changes.
            if (!current->isVisible() || !current->isEnabled()) continue;
            const QMetaObject *meta = current->metaObject();
            if (meta->propertyCount() > 512) { no("ownership-property-cap"); return result; }
            const int nav = meta->indexOfProperty("windowNavigator");
            // This is a declared QObject-valued owner census. A differently
            // typed property with the same spelling is outside that definition.
            if (nav >= 0 && meta->property(nav).metaType().flags().testFlag(QMetaType::PointerToQObject) && !owner(current)) return result;
            QList<QPointer<QQuickItem>> children;
            for (QQuickItem *child : current->childItems()) children.append(child);
            if (!allowed()) return result;
            if (children.size() > 64) { no("ownership-child-cap"); return result; }
            for (const auto &child : children) {
                bool seen = false;
                for (const auto &old : nodes) if (old.object == child) { seen = true; break; }
                if (seen) continue;
                if (!child || nodes.size() >= 512 || nodes[index].depth >= 32) { no("ownership-census-cap"); return result; }
                nodes.append({child, nodes[index].depth + 1});
            }
        }
        // Recheck id-table bindings: they are context ids, not notify properties.
        for (const auto &scope : scopes) {
            if (!allowed() || !scope.context) return result;
            QPointer<QObject> r = scope.context->objectForName(QStringLiteral("root"));
            if (r && !retain(r)) return result;
            QPointer<QObject> m = scope.context->objectForName(QStringLiteral("mainView"));
            if (m && !retain(m)) return result;
            if (!allowed() || r != scope.root || m != scope.main) { no("ownership-lexical-changed"); return result; }
        }
        if (allowed()) result.complete = true;
        return result;
    }
};
} // namespace ownership_detail
inline NavigationOwnership inspectNavigationOwnership(QQuickWindow *window, const std::function<bool()> &permit, int budgetMs) {
    QElapsedTimer elapsed; elapsed.start();
    NavigationOwnership refused;
    if (detail::entry.busy) { detail::entry.reentered = true; refused.refusal = QStringLiteral("sample-reentered"); return refused; }
    detail::EntryScope scope;
    if (budgetMs < 1 || budgetMs > MaxObservationMs) { refused.refusal = QStringLiteral("budget-invalid"); return refused; }
    return ownership_detail::Census(window, permit, budgetMs, elapsed).run();
}
} // namespace reader_selected
