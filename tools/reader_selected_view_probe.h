#pragma once
// Experimental public-accessor diagnostic. Never an action or lifetime lease.
#include <QElapsedTimer>
#include <QEvent>
#include <QGuiApplication>
#include <QMetaMethod>
#include <QMetaProperty>
#include <QPointer>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QThread>
#include <QVariant>
#include <functional>

namespace reader_selected {
// Cooperative research cap, not a preemptive timeout or UI responsiveness SLA.
inline constexpr int MaxObservationMs = 20000;
struct Observation {
    QString refusal;
    QString documentId, pageId;
    int pageIndex = -1, pageCount = -1;
    int status = -1, pageStatus = -1;
    bool observed = false, mapCoherent = false;
    bool loading = true, pageError = true, documentError = true, awaitingTiles = true, pendingTiles = true, working = true;
    // A sampled status conjunction only: not a render fence or an action gate.
    bool ready = false;
    // Optional source relationship only; never a mutation or ownership lease.
    bool creationJoinObserved = false;
    // Even an accepted snapshot establishes none of these authorities.
    bool nativeAuthority = false, renderAuthority = false, durableAuthority = false;
};

namespace detail {
inline constexpr char CreationControllerClass[] = "xofm::libs::library::DocumentController";
inline constexpr char CreationLibraryClass[] = "xofm::libs::library::Library";
class Epoch final : public QObject {
    Q_OBJECT
public:
    quint64 value = 0;
    bool eventFilter(QObject *, QEvent *) override { ++value; return false; }
public slots:
    void changed() { ++value; }
};
struct Entry { bool busy = false, reentered = false; };
inline thread_local Entry entry;
struct EntryScope {
    EntryScope() { entry.busy = true; entry.reentered = false; }
    ~EntryScope() { entry.busy = false; }
};

class Sample {
    QPointer<QGuiApplication> app;
    QPointer<QQuickWindow> window;
    QPointer<QQuickItem> root;
    QPointer<QQmlEngine> engine;
    QPointer<QQmlContext> lexicalSeed;
    QPointer<QObject> creationController;
    bool joinRequested;
    QElapsedTimer timer;
    int budget;
    const std::function<bool()> &permit;
    Epoch epoch;
    QList<QPointer<QObject>> retained;
    // A separate bounded discovery census; these objects do not consume the
    // smaller relationship-object budget merely because they are direct children.
    QList<QPointer<QObject>> discoveryGuards;
    QList<QPair<QObject *, int>> watched;
    quint64 baseline = 0;
    QString failure;

    bool no(const char *reason) { if (failure.isEmpty()) failure = QString::fromLatin1(reason); return false; }
    bool retain(QObject *p) {
        QPointer<QObject> pin = p;
        if (!pin || pin->thread() != QThread::currentThread()) return no("object-thread-or-lifetime");
        for (const auto &old : retained) if (old == p) return true;
        if (retained.size() >= 640) return no("object-cap");
        retained.append(p);
        QObject::connect(p, &QObject::destroyed, &epoch, &Epoch::changed, Qt::DirectConnection);
        return pin ? true : no("object-thread-or-lifetime");
    }
    bool live(bool consultPermit = true) {
        if (!app || QCoreApplication::closingDown() || QThread::currentThread() != app->thread() ||
            !window || !root || !engine || window->thread() != app->thread() || engine->thread() != app->thread())
            return no("context-thread-or-lifetime");
        if (entry.reentered) return no("sample-reentered");
        if (epoch.value != baseline) return no("generation-changed");
        if (timer.elapsed() >= budget) return no("deadline");
        if (consultPermit && (!permit || !permit())) return no("canceled");
        if (!app || !window || !root || !engine || entry.reentered || epoch.value != baseline)
            return no("generation-changed");
        for (const auto &p : retained) if (!p || p->thread() != app->thread()) return no("object-thread-or-lifetime");
        if (timer.elapsed() >= budget) return no("deadline");
        if (window->contentItem() != root || !window->isVisible() || !window->isActive() ||
            QGuiApplication::focusWindow() != window || qmlEngine(window) != engine)
            return no("window-or-engine-changed");
        return true;
    }
    bool signal(QObject *p, const char *signature) {
        QPointer<QObject> pin = p;
        if (!retain(pin) || !live() || !pin) return false;
        p = pin.data();
        const int i = p->metaObject()->indexOfSignal(signature);
        if (i < 0) return no("notification-missing");
        const QPair<QObject *, int> key(p, i);
        if (watched.contains(key)) return true;
        const int slot = epoch.metaObject()->indexOfSlot("changed()");
        if (!QObject::connect(p, p->metaObject()->method(i), &epoch, epoch.metaObject()->method(slot), Qt::DirectConnection))
            return no("notification-connect");
        watched.append(key);
        return live();
    }
    bool watch(QObject *p, const char *name) {
        QPointer<QObject> pin = p;
        if (!retain(pin) || !live() || !pin) return false;
        p = pin.data();
        const QMetaObject *m = p->metaObject();
        if (m->propertyCount() > 512) return no("property-cap");
        const int i = m->indexOfProperty(name);
        if (i < 0) return no("property-missing");
        const QMetaProperty property = m->property(i);
        if (!property.isReadable()) return no("property-unreadable");
        if (property.isConstant()) return live();
        if (!property.hasNotifySignal()) return no("notification-missing");
        return signal(p, property.notifySignal().methodSignature().constData());
    }
    bool watchMany(QObject *p, std::initializer_list<const char *> names) {
        QPointer<QObject> pin = p;
        for (const char *name : names) if (!pin || !watch(pin, name)) return false;
        return true;
    }
    bool guardDiscovery(QObject *p) {
        QPointer<QObject> pin = p;
        if (!pin || pin->thread() != QThread::currentThread()) return no("object-thread-or-lifetime");
        if (discoveryGuards.size() >= 512 * 64) return no("discovery-guard-cap");
        discoveryGuards.append(pin);
        // Install before consulting permit: a callback can otherwise rename or
        // delete a previously scanned direct child without delivering an event.
        if (!QObject::connect(pin, &QObject::destroyed, &epoch, &Epoch::changed, Qt::DirectConnection) || !pin ||
            !QObject::connect(pin, &QObject::objectNameChanged, &epoch, &Epoch::changed, Qt::DirectConnection))
            return no("notification-connect");
        return live();
    }
    QVariant value(QObject *p, const char *name) {
        QPointer<QObject> pin = p;
        if (!retain(pin) || !live() || !pin) return {};
        p = pin.data();
        const int i = p->metaObject()->indexOfProperty(name);
        if (i < 0) { no("property-missing"); return {}; }
        const QVariant v = p->metaObject()->property(i).read(p);
        // A QVariant holds a bare QObject pointer. Pin a nonnull result before
        // post-read permit callbacks; null prefetched neighbors remain valid.
        // This assumes an accessor returns a live pointer, not corrupt storage.
        if (v.metaType().flags().testFlag(QMetaType::PointerToQObject)) {
            QPointer<QObject> returned = v.value<QObject *>();
            if (returned && !retain(returned)) return {};
        }
        if (!live() || !v.isValid()) { no("property-read"); return {}; }
        return v;
    }
    QObject *object(QObject *p, const char *name) {
        const QVariant v = value(p, name);
        if (!failure.isEmpty()) return nullptr;
        if (!v.metaType().flags().testFlag(QMetaType::PointerToQObject)) { no("object-type"); return nullptr; }
        QObject *result = v.value<QObject *>();
        if (!retain(result) || !live()) return nullptr;
        // Native wrappers may have external QObject ownership and no QML context.
        // A different associated engine is refused; absence grants no ownership.
        QQmlEngine *associated = qmlEngine(result);
        if (associated && associated != engine) { no("foreign-engine"); return nullptr; }
        return live() ? result : nullptr;
    }
    bool string(QObject *p, const char *name, QString &out) {
        const QVariant v = value(p, name);
        if (v.metaType().id() != QMetaType::QString) return no("string-type");
        out = v.toString(); return failure.isEmpty() && live();
    }
    bool integer(QObject *p, const char *name, int &out) {
        const QVariant v = value(p, name);
        if (v.metaType().id() != QMetaType::Int) return no("integer-type");
        out = v.toInt(); return failure.isEmpty() && live();
    }
    bool boolean(QObject *p, const char *name, bool &out) {
        const QVariant v = value(p, name);
        if (v.metaType().id() != QMetaType::Bool) return no("boolean-type");
        out = v.toBool(); return failure.isEmpty() && live();
    }
    bool enumeration(QObject *p, const char *name, const char *key, int &out, int &wanted) {
        QPointer<QObject> pin = p;
        if (!retain(pin) || !live() || !pin) return false;
        p = pin.data();
        const int i = p->metaObject()->indexOfProperty(name);
        if (i < 0) return no("enum-missing");
        const QMetaProperty property = p->metaObject()->property(i);
        const QMetaEnum e = property.enumerator();
        bool found = false; wanted = e.keyToValue(key, &found);
        if (!property.isEnumType() || !found) return no("enum-key-missing");
        const QVariant v = value(p, name);
        bool ok = false; out = v.toInt(&ok);
        return ok && live() ? true : no("enum-value");
    }
    QQmlContext *scope(QObject *owner, QObject *seed, QObject *expectedDevice) {
        QPointer<QObject> ownerPin = owner, seedPin = seed, devicePin = expectedDevice;
        if (!retain(ownerPin) || !retain(seedPin) || !retain(devicePin) || !live()) return nullptr;
        if (seedPin->parent() != ownerPin || qmlEngine(seedPin) != engine) {
            no("foreign-helper-context"); return nullptr;
        }
        // A composite root can expose its caller's creation context. Its direct
        // in-component helper exposes the Reader's internal lexical namespace.
        // Only bounded parent ascent is allowed, with both exact scoped ids.
        QPointer<QQmlContext> c = qmlContext(seedPin);
        lexicalSeed = c;
        for (int depth = 0; c && depth < 16; ++depth) {
            if (!retain(c) || !live() || !c->isValid() || c->engine() != engine) return nullptr;
            if (c->objectForName(QStringLiteral("root")) == ownerPin) {
                if (c->objectForName(QStringLiteral("sceneView")) != devicePin) {
                    no("lexical-device-mismatch"); return nullptr;
                }
                return live() ? c.data() : nullptr;
            }
            if (!live()) return nullptr;
            c = c->parentContext();
        }
        no("lexical-root-missing"); return nullptr;
    }
    bool visual(QQuickItem *p, bool consultPermit = true) {
        QPointer<QQuickItem> pin = p;
        if (!retain(pin) || !live(consultPermit) || !pin) return false;
        p = pin.data();
        return p->thread() == app->thread() && p->window() == window && p->isVisible() && p->isEnabled() && live(consultPermit);
    }
    bool id(QObject *document, QString &out) {
        if (!live()) return false;
        // Fixed expression only; neither caller input nor source text is evaluated.
        // A canonical textual key is not proof of backing-Document identity.
        QQmlExpression expression(engine->rootContext(), document, QStringLiteral("String(id)"));
        const QVariant v = expression.evaluate();
        if (!live() || expression.hasError() || v.metaType().id() != QMetaType::QString) return no("document-id-conversion");
        out = v.toString();
        static const QRegularExpression uuid(QStringLiteral("^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$"));
        return uuid.match(out).hasMatch() && out != QStringLiteral("00000000-0000-0000-0000-000000000000")
            ? true : no("document-id-format");
    }
    bool nativeClass(QObject *p, const char *name) {
        QPointer<QObject> pin = p;
        if (!retain(pin) || !live()) return false;
        QQmlEngine *associated = qmlEngine(pin);
        if (associated && associated != engine) return no("foreign-engine");
        const QMetaObject *m = pin->metaObject();
        for (int depth = 0; m && depth < 16; ++depth, m = m->superClass())
            if (qstrcmp(m->className(), name) == 0) return live();
        return no("creation-type");
    }
    QObject *lookup(QObject *library, QObject *document) {
        QPointer<QObject> lib = library, doc = document;
        if (!nativeClass(lib, CreationLibraryClass) || !live() || !doc) return nullptr;
        // Preserve the native metatype and its owned QVariant storage. Neither
        // a QString conversion nor a reconstructed entry::Id layout is needed.
        const QVariant nativeId = value(doc, "id");
        if (!failure.isEmpty() || !nativeId.isValid() || nativeId.metaType().flags().testFlag(QMetaType::IsPointer) || !live()) return nullptr;
        QMetaMethod method;
        int matches = 0;
        const QMetaObject *m = lib->metaObject();
        if (m->methodCount() > 512) { no("method-cap"); return nullptr; }
        for (int i = 0; i < m->methodCount(); ++i) {
            const QMetaMethod candidate = m->method(i);
            if (candidate.name() != "entryForId") continue;
            ++matches;
            method = candidate;
        }
        if (matches != 1 || !method.isValid() || method.access() != QMetaMethod::Public ||
            (method.methodType() != QMetaMethod::Method && method.methodType() != QMetaMethod::Slot) ||
            qstrcmp(method.enclosingMetaObject()->className(), CreationLibraryClass) != 0 ||
            method.parameterCount() != 1 || method.parameterMetaType(0) != nativeId.metaType() ||
            !method.returnMetaType().flags().testFlag(QMetaType::PointerToQObject)) {
            no("creation-lookup-method"); return nullptr;
        }
        const QMetaType resultType = method.returnMetaType();
        const QByteArray argumentName = method.parameterTypes().first();
        const char *resultName = method.typeName();
        QVariant result(resultType);
        if (argumentName.isEmpty() || !resultName || !*resultName || !result.isValid() || !live() || !lib || !doc) return nullptr;
        const bool called = method.invoke(lib, Qt::DirectConnection,
            QGenericReturnArgument(resultName, result.data()),
            QGenericArgument(argumentName.constData(), nativeId.constData()));
        // The lookup may allocate/cache a wrapper. Guard its returned pointer
        // before any post-call callback; do not change QObject/QML ownership.
        QPointer<QObject> returned = called ? result.value<QObject *>() : nullptr;
        if (!called || !returned) { no("creation-lookup-refused"); return nullptr; }
        if (!retain(returned) || !live()) return nullptr;
        QQmlEngine *associated = qmlEngine(returned);
        if (associated && associated != engine) { no("foreign-engine"); return nullptr; }
        return live() ? returned.data() : nullptr;
    }
public:
    Sample(QQuickWindow *w, const std::function<bool()> &permission, int ms,
           QObject *creation = nullptr, bool join = false)
        : creationController(creation), joinRequested(join), budget(ms), permit(permission) {
        timer.start();
        app = qobject_cast<QGuiApplication *>(QCoreApplication::instance());
        if (!app || QThread::currentThread() != app->thread() || !w || w->thread() != app->thread()) {
            no("application-thread"); return;
        }
        window = w; root = w->contentItem(); engine = qmlEngine(w);
        if (ms < 1 || ms > MaxObservationMs) no("budget-invalid");
        // Any nested event delivery invalidates this cooperative sample. Never
        // suppress the event or pump an event loop to obtain a better result.
        app->installEventFilter(&epoch);
    }
    Observation run() {
        Observation r;
        const auto refuse = [&] { Observation f; f.refusal = failure.isEmpty() ? QStringLiteral("relationship-refused") : failure; return f; };
        if (!failure.isEmpty() || !live() || !retain(window) || !retain(root) || !retain(engine)) return refuse();
        if (!signal(app, "focusWindowChanged(QWindow*)") || !signal(window, "activeChanged()") ||
            !signal(window, "visibleChanged(bool)")) return refuse();
        QPointer<QObject> creationLibrary;
        if (joinRequested) {
            if (!nativeClass(creationController, CreationControllerClass) ||
                !watchMany(creationController, {"library"})) return refuse();
            creationLibrary = object(creationController, "library");
            bool ready = false;
            if (!creationLibrary || !nativeClass(creationLibrary, CreationLibraryClass) ||
                !watchMany(creationLibrary, {"isReady"}) || !boolean(creationLibrary, "isReady", ready) || !ready) {
                no("creation-library-unready"); return refuse();
            }
        }
        // Search every effectively visible and enabled visual branch within its
        // bounds. A valid selected chain requires both states. Retain/watch an
        // excluded boundary before pruning, so later reveal/enable (even ABA)
        // invalidates the census. Nonvisual service/model subtrees cannot hold
        // a valid direct helper of an eligible visual Reader root.
        struct Node { QPointer<QQuickItem> p; int depth; };
        QList<Node> nodes{{root.data(), 0}};
        QList<QPointer<QObject>> helpers, containers;
        for (qsizetype n = 0; n < nodes.size(); ++n) {
            QPointer<QQuickItem> p = nodes[n].p;
            if (!live() || !p || !retain(p)) return refuse();
            if (!signal(p, "childrenChanged()") || !watchMany(p, {"objectName", "visible", "enabled"})) return refuse();
            const bool eligible = p->isVisible() && p->isEnabled();
            if (!live()) return refuse();
            if (!eligible) continue;
            const QString name = p->objectName();
            if (!live()) return refuse();
            if (name == QStringLiteral("SceneViewsRoot")) containers.append(QPointer<QObject>(p.data()));
            QList<QPointer<QObject>> direct;
            for (QObject *child : p->children()) direct.append(child);
            if (direct.size() > 64) { no("direct-child-cap"); return refuse(); }
            for (const auto &candidate : direct) {
                if (!guardDiscovery(candidate)) return refuse();
                if (candidate->objectName() == QStringLiteral("DocumentView.ToolSettings")) {
                    if (!retain(candidate) || !live()) return refuse();
                    helpers.append(candidate);
                }
            }
            QList<QPointer<QQuickItem>> children;
            for (QQuickItem *child : p->childItems()) children.append(child);
            if (!live()) return refuse();
            if (children.size() > 64) { no("visual-child-cap"); return refuse(); }
            for (const auto &child : children) {
                bool seen = false;
                for (const auto &old : nodes) if (old.p == child) seen = true;
                if (seen) continue;
                if (nodes.size() >= 512) { no("visual-node-cap"); return refuse(); }
                if (nodes[n].depth >= 32) { no("visual-depth-cap"); return refuse(); }
                nodes.append({child, nodes[n].depth + 1});
            }
        }
        if (helpers.size() != 1 || containers.size() != 1) { no("anchor-ambiguity"); return refuse(); }
        QPointer<QObject> helper = helpers.first();
        QPointer<QQuickItem> container = qobject_cast<QQuickItem *>(containers.first().data());
        QPointer<QQuickItem> reader = helper ? qobject_cast<QQuickItem *>(helper->parent()) : nullptr;
        QPointer<QQuickItem> device = container ? container->parentItem() : nullptr;
        if (!reader || !device || device->parentItem() != reader || !visual(reader) || !visual(device) || !visual(container)) return refuse();
        QPointer<QQmlContext> lexical = scope(reader, helper, device);
        if (!lexical || lexical->objectForName(QStringLiteral("sceneView")) != device || !live()) {
            no("lexical-device-mismatch"); return refuse();
        }
        if (!watchMany(reader, {"document", "currentPage", "currentPageId", "sceneController"}) ||
            !watchMany(helper, {"document"}) ||
            !watchMany(device, {"document", "sceneView", "nextSceneView", "previousSceneView", "page", "pageId", "controller", "status", "pageStatus", "isLoading", "pageError"})) return refuse();
        QPointer<QObject> document = object(reader, "document");
        QPointer<QObject> controller = object(reader, "sceneController");
        QPointer<QQuickItem> current = qobject_cast<QQuickItem *>(object(device, "sceneView"));
        if (!document || !controller || !current || current->parentItem() != container || !visual(current)) return refuse();
        bool nativeScene = false;
        const QMetaObject *sceneMeta = current->metaObject();
        for (int depth = 0; sceneMeta && depth < 16; ++depth, sceneMeta = sceneMeta->superClass())
            if (qstrcmp(sceneMeta->className(), "SceneView") == 0) { nativeScene = true; break; }
        if (!nativeScene || !live()) { no("native-scene-type"); return refuse(); }
        const QList<QPointer<QQuickItem>> visualOwners{reader, device, container, current};
        for (const auto &item : visualOwners)
            if (!item || !watchMany(item, {"parent", "visible", "enabled"}) || !signal(item, "windowChanged(QQuickWindow*)")) return refuse();
        if (!watchMany(document, {"id", "pageCount", "loadError"}) || !signal(document, "pageMapChanged()") ||
            !watchMany(controller, {"pageId", "working"}) ||
            !watchMany(current, {"document", "controller", "pageId", "status", "pageStatus", "awaitingInitialTileSet", "tileManager"})) return refuse();
        QPointer<QObject> tiles = object(current, "tileManager");
        if (!tiles || !watchMany(tiles, {"pendingTiles"})) return refuse();
        const auto owners = [&](bool consultPermit = true) {
            return live(consultPermit) && helper && reader && device && container && current && document && controller && tiles && lexical &&
                helper->parent() == reader && qmlContext(helper) == lexicalSeed && qmlEngine(helper) == engine &&
                device->parentItem() == reader && container->parentItem() == device &&
                current->parentItem() == container && lexical->isValid() && lexical->engine() == engine &&
                lexical->objectForName(QStringLiteral("root")) == reader && lexical->objectForName(QStringLiteral("sceneView")) == device &&
                visual(reader, consultPermit) && visual(device, consultPermit) && visual(container, consultPermit) && visual(current, consultPermit);
        };
        const auto relationship = [&] {
            if (!owners()) return false;
            if (object(reader, "document") != document || object(helper, "document") != document || object(device, "document") != document ||
                object(current, "document") != document || object(reader, "sceneController") != controller ||
                object(device, "controller") != controller || object(current, "controller") != controller ||
                object(device, "sceneView") != current || object(current, "tileManager") != tiles) return false;
            // Null neighbors are valid. Read their typed values without retaining a null.
            for (const char *name : {"nextSceneView", "previousSceneView"}) {
                const QVariant neighbor = value(device, name);
                if (!neighbor.metaType().flags().testFlag(QMetaType::PointerToQObject) || neighbor.value<QObject *>() == current) return false;
            }
            return owners();
        };
        if (!relationship()) return refuse();
        QString deviceId, nativeId, controllerId;
        int devicePage = -1;
        if (!id(document, r.documentId) || !string(reader, "currentPageId", r.pageId) ||
            !string(device, "pageId", deviceId) || !string(current, "pageId", nativeId) || !string(controller, "pageId", controllerId) ||
            !integer(reader, "currentPage", r.pageIndex) || !integer(device, "page", devicePage) || !integer(document, "pageCount", r.pageCount)) return refuse();
        if (r.pageId.isEmpty() || r.pageId != deviceId || r.pageId != nativeId || r.pageId != controllerId ||
            r.pageIndex != devicePage || r.pageIndex < 0 || r.pageIndex >= r.pageCount) return refuse();
        QString mapped; int roundtrip = -1;
        if (!live() || !QMetaObject::invokeMethod(document, "idForPage", Qt::DirectConnection,
                Q_RETURN_ARG(QString, mapped), Q_ARG(int, r.pageIndex)) || !live() ||
            !QMetaObject::invokeMethod(document, "pageForId", Qt::DirectConnection,
                Q_RETURN_ARG(int, roundtrip), Q_ARG(QString, r.pageId)) || !live() || mapped != r.pageId || roundtrip != r.pageIndex) {
            no("page-map-refused"); return refuse();
        }
        int active = -1, pageReady = -1, facadeStatus = -1, facadePageStatus = -1;
        if (!enumeration(current, "status", "Active", r.status, active) || !enumeration(current, "pageStatus", "Ready", r.pageStatus, pageReady) ||
            !boolean(device, "isLoading", r.loading) || !boolean(device, "pageError", r.pageError) ||
            !boolean(document, "loadError", r.documentError) ||
            !integer(device, "status", facadeStatus) || !integer(device, "pageStatus", facadePageStatus) ||
            !boolean(current, "awaitingInitialTileSet", r.awaitingTiles) || !boolean(tiles, "pendingTiles", r.pendingTiles) ||
            !boolean(controller, "working", r.working)) return refuse();
        if (facadeStatus != r.status || facadePageStatus != r.pageStatus) return refuse();
        if (joinRequested) {
            if (!relationship() || !creationController || !creationLibrary ||
                object(creationController, "library") != creationLibrary || !live()) return refuse();
            if (lookup(creationLibrary, document) != document) {
                no("creation-wrapper-inconclusive"); return refuse();
            }
            bool ready = false;
            if (!creationController || !creationLibrary || !live() ||
                object(creationController, "library") != creationLibrary ||
                !boolean(creationLibrary, "isReady", ready) || !ready || !relationship()) return refuse();
            mapped.clear(); roundtrip = -1;
            if (!live() || !QMetaObject::invokeMethod(document, "idForPage", Qt::DirectConnection,
                    Q_RETURN_ARG(QString, mapped), Q_ARG(int, r.pageIndex)) || !live() ||
                !QMetaObject::invokeMethod(document, "pageForId", Qt::DirectConnection,
                    Q_RETURN_ARG(int, roundtrip), Q_ARG(QString, r.pageId)) || !live() ||
                mapped != r.pageId || roundtrip != r.pageIndex) {
                no("page-map-refused"); return refuse();
            }
            r.creationJoinObserved = true;
        }
        QString finalId, finalPage;
        if (!id(document, finalId) || !string(reader, "currentPageId", finalPage) || finalId != r.documentId || finalPage != r.pageId ||
            !relationship() || !owners(false) || !live(false)) return refuse();
        r.observed = true; r.mapCoherent = true;
        r.ready = !r.loading && !r.pageError && !r.documentError && r.status == active && r.pageStatus == pageReady &&
            !r.awaitingTiles && !r.pendingTiles && !r.working;
        return r;
    }
};
} // namespace detail

inline Observation observe(QQuickWindow *window, const std::function<bool()> &permit = [] { return true; }, int budgetMs = 1000) {
    if (detail::entry.busy) { detail::entry.reentered = true; Observation r; r.refusal = QStringLiteral("sample-reentered"); return r; }
    detail::EntryScope scope;
    return detail::Sample(window, permit, budgetMs).run();
}

// The caller resolves the existing controller on this engine's GUI thread before
// entering the sample. No pointer escapes the call; no singleton is resolved here.
inline Observation observeWithCreationController(QQuickWindow *window, QObject *controller,
        const std::function<bool()> &permit = [] { return true; }, int budgetMs = 1000) {
    if (detail::entry.busy) { detail::entry.reentered = true; Observation r; r.refusal = QStringLiteral("sample-reentered"); return r; }
    detail::EntryScope scope;
    return detail::Sample(window, permit, budgetMs, controller, true).run();
}
} // namespace reader_selected
