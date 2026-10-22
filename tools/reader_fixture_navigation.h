#pragma once
// Original development diagnostic. Dispatch is neither target selection nor readiness.
#include "reader_selected_view_probe.h"
#include <QMap>

namespace reader_selected {
struct FixtureNavigation {
    QString documentId;
    int pageIndex = -1;
    bool valid() const {
        // Research input cap, not a claim about the application's page limit.
        static const QRegularExpression canonical(QStringLiteral("^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$"));
        return documentId.size() == 36 && canonical.match(documentId).hasMatch() &&
            documentId != QStringLiteral("00000000-0000-0000-0000-000000000000") &&
            pageIndex >= 0 && pageIndex <= 9999;
    }
};
struct NavigationObservation {
    bool attempted = false, dispatched = false;
    QString refusal;
};

namespace navigation_detail {
inline constexpr char NativeWrapper[] = "xofm::bindings::qt::WindowNavigatorWrapper";

class Dispatch {
    QPointer<QGuiApplication> app;
    QPointer<QQuickWindow> window;
    QPointer<QQuickItem> root;
    QPointer<QQmlEngine> engine;
    detail::Epoch epoch;
    QList<QPointer<QObject>> retained;
    QList<QPair<QObject *, int>> watched;
    QList<QPointer<QQuickItem>> owners;
    QPointer<QObject> receiver;
    const QElapsedTimer &elapsed;
    FixtureNavigation configuration;
    const std::function<bool()> &permit;
    int budget;
    QString failure;

    bool no(const char *reason) {
        if (failure.isEmpty()) failure = QString::fromLatin1(reason);
        return false;
    }
    bool retain(QObject *object) {
        QPointer<QObject> guard = object;
        if (!guard || guard->thread() != QThread::currentThread()) return no("navigation-object-lifetime");
        for (const auto &old : retained) if (old == guard) return true;
        if (retained.size() >= 640) return no("navigation-object-cap");
        retained.append(guard);
        if (!QObject::connect(guard, &QObject::destroyed, &epoch, &detail::Epoch::changed, Qt::DirectConnection))
            return no("navigation-notification");
        return guard ? true : no("navigation-object-lifetime");
    }
    bool context() {
        if (!app || !window || !root || !engine || QCoreApplication::closingDown() ||
            QThread::currentThread() != app->thread() || window->thread() != app->thread() || engine->thread() != app->thread())
            return no("navigation-context");
        if (detail::entry.reentered) return no("sample-reentered");
        if (epoch.value != 0) return no("navigation-changed");
        if (elapsed.elapsed() >= budget) return no("deadline");
        for (const auto &guard : retained)
            if (!guard || guard->thread() != app->thread()) return no("navigation-object-lifetime");
        if (window->contentItem() != root || qmlEngine(window) != engine ||
            !window->isVisible() || !window->isActive() || QGuiApplication::focusWindow() != window)
            return no("navigation-window");
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
        if (index < 0) return no("navigation-notification");
        const QPair<QObject *, int> key(guard.data(), index);
        if (watched.contains(key)) return true;
        const int target = epoch.metaObject()->indexOfSlot("changed()");
        if (!QObject::connect(guard, guard->metaObject()->method(index), &epoch,
                epoch.metaObject()->method(target), Qt::DirectConnection)) return no("navigation-notification");
        watched.append(key);
        return allowed();
    }
    bool watch(QObject *object, const char *name) {
        QPointer<QObject> guard = object;
        if (!retain(guard) || !allowed()) return false;
        const QMetaObject *meta = guard->metaObject();
        if (meta->propertyCount() > 512) return no("navigation-property-cap");
        const int index = meta->indexOfProperty(name);
        if (index < 0) return no("navigation-property-missing");
        const QMetaProperty property = meta->property(index);
        if (!property.isReadable()) return no("navigation-property-unreadable");
        return property.isConstant() ? allowed() : property.hasNotifySignal()
            ? signal(guard, property.notifySignal().methodSignature().constData()) : no("navigation-notification");
    }
    bool item(QQuickItem *object) {
        QPointer<QQuickItem> guard = object;
        if (!retain(guard) || !allowed()) return false;
        if (!watch(guard, "parent") || !watch(guard, "visible") || !watch(guard, "enabled") ||
            !signal(guard, "childrenChanged()") || !signal(guard, "windowChanged(QQuickWindow*)")) return false;
        QQmlEngine *associated = qmlEngine(guard);
        if ((associated && associated != engine) || guard->window() != window) return no("navigation-foreign-owner");
        return allowed();
    }
    bool native(QObject *object) {
        QPointer<QObject> guard = object;
        if (!retain(guard) || !allowed()) return false;
        QQmlEngine *associated = qmlEngine(guard);
        if (associated && associated != engine) return no("navigation-foreign-engine");
        for (const QMetaObject *meta = guard->metaObject(); meta; meta = meta->superClass())
            if (qstrcmp(meta->className(), NativeWrapper) == 0) return allowed();
        return no("navigation-receiver-type");
    }
    QObject *read(QQuickItem *owner) {
        QPointer<QQuickItem> guard = owner;
        if (!retain(guard) || !watch(guard, "windowNavigator") || !allowed()) return nullptr;
        const QMetaObject *meta = guard->metaObject();
        const QMetaProperty property = meta->property(meta->indexOfProperty("windowNavigator"));
        if (!property.metaType().flags().testFlag(QMetaType::PointerToQObject)) {
            no("navigation-property-type"); return nullptr;
        }
        const QVariant value = property.read(guard);
        // Guard the borrowed return before any subsequent permit callback.
        // As with other public getters, corrupt/dangling returns are not supported.
        QPointer<QObject> returned = value.metaType().flags().testFlag(QMetaType::PointerToQObject)
            ? value.value<QObject *>() : nullptr;
        if (!value.isValid() || !returned || !retain(returned) || !allowed()) {
            no("navigation-property-read"); return nullptr;
        }
        return native(returned) && allowed() ? returned.data() : nullptr;
    }
    QMetaMethod method(QObject *object) {
        QPointer<QObject> guard = object;
        if (!native(guard) || !allowed()) return {};
        const QMetaObject *meta = guard->metaObject();
        if (meta->methodCount() > 512) { no("navigation-method-cap"); return {}; }
        QMetaMethod selected;
        int matches = 0;
        for (int index = 0; index < meta->methodCount(); ++index) {
            const QMetaMethod candidate = meta->method(index);
            if (candidate.name() != "open" || candidate.parameterCount() != 2 ||
                candidate.parameterMetaType(0) != QMetaType::fromType<QString>() ||
                candidate.parameterMetaType(1) != QMetaType::fromType<QVariantMap>()) continue;
            if (qstrcmp(candidate.enclosingMetaObject()->className(), NativeWrapper) != 0 ||
                candidate.access() != QMetaMethod::Public || candidate.methodType() != QMetaMethod::Slot ||
                candidate.returnMetaType() != QMetaType::fromType<void>()) {
                no("navigation-method-shadowed"); return {};
            }
            selected = candidate; ++matches;
        }
        if (matches != 1 || !selected.isValid()) { no("navigation-method"); return {}; }
        return selected;
    }
public:
    Dispatch(QQuickWindow *candidate, const FixtureNavigation &config,
             const std::function<bool()> &permission, int milliseconds, const QElapsedTimer &clock)
        : elapsed(clock), configuration(config), permit(permission), budget(milliseconds) {
        app = qobject_cast<QGuiApplication *>(QCoreApplication::instance());
        if (!app || QThread::currentThread() != app->thread() || !candidate || candidate->thread() != app->thread()) {
            no("application-thread"); return;
        }
        window = candidate; root = candidate->contentItem(); engine = qmlEngine(candidate);
        if (!engine || !root) { no("navigation-context"); return; }
        app->installEventFilter(&epoch);
    }
    NavigationObservation run() {
        const auto refused = [&] {
            NavigationObservation result;
            result.refusal = failure.isEmpty() ? QStringLiteral("navigation-refused") : failure;
            return result;
        };
        if (!failure.isEmpty() || !retain(app) || !retain(window) || !retain(root) || !retain(engine) || !allowed()) return refused();
        if (!signal(app, "focusWindowChanged(QWindow*)") || !signal(window, "activeChanged()") ||
            !signal(window, "visibleChanged(bool)")) return refused();
        struct Node { QPointer<QQuickItem> object; int depth; };
        QList<Node> nodes{{root, 0}};
        for (qsizetype index = 0; index < nodes.size(); ++index) {
            const QPointer<QQuickItem> current = nodes[index].object;
            if (!current || !item(current) || !allowed()) return refused();
            // Watch excluded boundaries before pruning. A reveal/enable, even
            // an ABA change, invalidates this census before any action.
            if (!current->isVisible() || !current->isEnabled()) continue;
            if (current->metaObject()->indexOfProperty("windowNavigator") >= 0) {
                QPointer<QObject> found = read(current);
                if (!found || !allowed()) return refused();
                if (receiver && receiver != found) { no("navigation-ambiguity"); return refused(); }
                receiver = found; owners.append(current);
            }
            QList<QPointer<QQuickItem>> children;
            for (QQuickItem *child : current->childItems()) children.append(child);
            if (!allowed()) return refused();
            if (children.size() > 64) { no("navigation-child-cap"); return refused(); }
            for (const auto &child : children) {
                bool seen = false;
                for (const auto &old : nodes) if (old.object == child) { seen = true; break; }
                if (seen) continue;
                if (!child || nodes.size() >= 512 || nodes[index].depth >= 32) { no("navigation-census-cap"); return refused(); }
                nodes.append({child, nodes[index].depth + 1});
            }
        }
        if (!receiver || owners.isEmpty()) { no("navigation-not-found"); return refused(); }
        for (const auto &owner : owners) {
            if (!owner || !allowed() || !owner->isVisible() || !owner->isEnabled() ||
                owner->window() != window || read(owner) != receiver || !allowed()) return refused();
        }
        const QMetaMethod selected = method(receiver);
        if (!selected.isValid() || !allowed()) return refused();
        const QString route = QStringLiteral("legacydevice/window/main");
        const QVariantMap arguments{{QStringLiteral("documentId"), configuration.documentId},
                                    {QStringLiteral("page"), configuration.pageIndex}};
        NavigationObservation result;
        result.attempted = true; // Consume before the public call, including reentry.
        result.dispatched = selected.invoke(receiver, Qt::DirectConnection,
            Q_ARG(QString, route), Q_ARG(QVariantMap, arguments));
        // Navigation may replace the complete discovered UI or delete its
        // receiver. Do not consult that epoch or dereference the receiver here.
        if (!result.dispatched) result.refusal = QStringLiteral("navigation-invoke-refused");
        else if (elapsed.elapsed() >= budget) result.refusal = QStringLiteral("deadline-after-dispatch");
        return result;
    }
};
} // namespace navigation_detail

inline NavigationObservation openFixture(QQuickWindow *window, const FixtureNavigation &configuration,
        const std::function<bool()> &permit, int budgetMs) {
    QElapsedTimer elapsed; elapsed.start();
    NavigationObservation refused;
    if (detail::entry.busy) {
        detail::entry.reentered = true; refused.refusal = QStringLiteral("sample-reentered"); return refused;
    }
    detail::EntryScope scope;
    if (!configuration.valid()) { refused.refusal = QStringLiteral("navigation-config"); return refused; }
    if (budgetMs < 1 || budgetMs > MaxObservationMs) { refused.refusal = QStringLiteral("budget-invalid"); return refused; }
    return navigation_detail::Dispatch(window, configuration, permit, budgetMs, elapsed).run();
}
} // namespace reader_selected
