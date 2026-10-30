// Owned synthetic metadata only; native getters and creation stay tripwires.
#include "qt_qml_access_probe_core.h"
#include <QQmlContext>
#include <QJSValue>
#include <cassert>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

static int getters = 0, invocations = 0, emissions = 0;
class EmissionTripwire : public QObject {
    Q_OBJECT
public:
    Q_INVOKABLE void invoked() { ++invocations; }
public slots:
    void emitted() { ++emissions; }
};
namespace entry { struct Id {}; }
class QmlDocumentWrapper : public QObject { Q_OBJECT };
class SceneController : public QObject {
    Q_OBJECT
public:
    Q_INVOKABLE bool addPageWithTemplateAndPageSize(entry::Id, int, QString, QSizeF, QJSValue, QString) {
        ++invocations; return false;
    }
};
class WrongReturnController : public QObject {
    Q_OBJECT
public:
    Q_INVOKABLE int addPageWithTemplateAndPageSize(entry::Id, int, QString, QSizeF, QJSValue, QString) {
        ++invocations; return 0;
    }
};
class SceneView : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QString pageId READ pageId NOTIFY changed)
    Q_PROPERTY(QmlDocumentWrapper* document READ document)
    Q_PROPERTY(SceneController* controller READ controller)
public:
    QString pageId() const { ++getters; return {}; }
    QmlDocumentWrapper *document() const { ++getters; return nullptr; }
    SceneController *controller() const { ++getters; return nullptr; }
signals:
    void changed();
};
class WrongSceneView : public SceneView {
    Q_OBJECT
    Q_PROPERTY(int pageId READ wrongPageId)
public:
    int wrongPageId() const { ++getters; return 0; }
};
class DocumentView : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QObject* sceneController READ pointer)
    Q_PROPERTY(QObject* pageSelection READ pointer)
public:
    QObject *pointer() const { ++getters; return nullptr; }
};
class ShortcutsDocumentView : public DocumentView { Q_OBJECT };
struct Opaque {};
Q_DECLARE_METATYPE(Opaque *)
namespace owned {
class WindowNavigator : public QObject { Q_OBJECT };
}
class NavigationTypes : public QObject {
    Q_OBJECT
    Q_PROPERTY(owned::WindowNavigator* windowNavigator READ navigator NOTIFY changed)
public:
    owned::WindowNavigator *navigator() const { ++getters; return nullptr; }
signals:
    void changed();
};
class FocusScope : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QObject* windowNavigator READ navigator)
public:
    QObject *navigator() const { ++getters; return nullptr; }
signals:
    void requestOpenDocumentOnPage(QVariant, QVariant);
};
class UnreadableNavigation : public QQuickItem {
    Q_OBJECT
    // Deliberate write-only owned metadata: moc warns about lack of READ, but
    // retains the property with isReadable=false for this negative fixture.
    Q_PROPERTY(QObject* windowNavigator WRITE setNavigator)
public:
    void setNavigator(QObject *) { ++invocations; }
signals:
    void requestOpenDocumentOnPage(QVariant, QVariant);
};
class SlotsNavigation : public QQuickItem {
    Q_OBJECT
public slots:
    void requestOpenDocumentOnPage(QVariant, QVariant) { ++invocations; }
};
class OpaqueNavigation : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(Opaque* windowNavigator READ navigator)
public:
    Opaque *navigator() const { ++getters; return nullptr; }
signals:
    void requestOpenDocumentOnPage(QVariant, QVariant);
};
class OpaqueDocumentView : public DocumentView {
    Q_OBJECT
    Q_PROPERTY(Opaque* pageSelection READ opaque)
public:
    Opaque *opaque() const { ++getters; return nullptr; }
};
class SceneSelectionHandler : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QObject* controller READ pointer)
    Q_PROPERTY(QRectF viewSelectionRect READ rect)
    Q_PROPERTY(QRectF sceneSelectionRect READ rect)
public:
    QObject *pointer() const { ++getters; return nullptr; }
    QRectF rect() const { ++getters; return {}; }
};
// A virtual metadata accessor models reentrant loss without a production hook.
class LossItem : public QQuickItem {
public:
    QObject **victim = nullptr;
    const QMetaObject *metaObject() const override {
        if (victim && *victim) { delete *victim; *victim = nullptr; }
        return &QQuickItem::staticMetaObject;
    }
};
class Layer0 : public QQuickItem { Q_OBJECT };
class Layer1 : public Layer0 { Q_OBJECT };
class Layer2 : public Layer1 { Q_OBJECT };
class Layer3 : public Layer2 { Q_OBJECT };
class Layer4 : public Layer3 { Q_OBJECT };
class Layer5 : public Layer4 { Q_OBJECT };
class Layer6 : public Layer5 { Q_OBJECT };
class Layer7 : public Layer6 { Q_OBJECT };
class Layer8 : public Layer7 { Q_OBJECT };
class Layer9 : public Layer8 { Q_OBJECT };
class Layer10 : public Layer9 { Q_OBJECT };
class Layer11 : public Layer10 { Q_OBJECT };
class Layer12 : public Layer11 { Q_OBJECT };
class Layer13 : public Layer12 { Q_OBJECT };
class Layer14 : public Layer13 { Q_OBJECT };
class Layer15 : public Layer14 { Q_OBJECT };
class Layer16 : public Layer15 { Q_OBJECT };
class Layer17 : public Layer16 { Q_OBJECT };
class Layer18 : public Layer17 { Q_OBJECT };
class Layer19 : public Layer18 { Q_OBJECT };
class Layer20 : public Layer19 { Q_OBJECT };
class Layer21 : public Layer20 { Q_OBJECT };
class Layer22 : public Layer21 { Q_OBJECT };
class Layer23 : public Layer22 { Q_OBJECT };
class Layer24 : public Layer23 { Q_OBJECT };
class Layer25 : public Layer24 { Q_OBJECT };
class Layer26 : public Layer25 { Q_OBJECT };
class Layer27 : public Layer26 { Q_OBJECT };
class Layer28 : public Layer27 { Q_OBJECT };
class Layer29 : public Layer28 { Q_OBJECT };
class Layer30 : public Layer29 { Q_OBJECT };

int main(int argc, char **argv) {
    assert(argc == 2);
    QGuiApplication app(argc, argv);
    const std::string mode = argv[1];
    if (mode.rfind("navigation-", 0) == 0 && mode.rfind("navigation-probe-", 0) != 0) {
        QQmlEngine engine;
        QQuickWindow window;
        QQmlEngine::setContextForObject(&window, engine.rootContext());
        SceneController controller;
        EmissionTripwire recorder;
        engine.rootContext()->setContextProperty("tripwire", &recorder);
        qmlRegisterType<NavigationTypes>("fixture.navigation", 1, 0, "NavigationTypes");
        qmlRegisterUncreatableType<owned::WindowNavigator>("fixture.navigation", 1, 0, "WindowNavigator", "fixture only");
        const QByteArray signal = "signal requestOpenDocumentOnPage(var entry, var page); ";
        const QByteArray property = "property QtObject windowNavigator: null; ";
        const auto create = [&](const QByteArray &body) {
            QQmlComponent component(&engine);
            component.setData("import QtQuick\nimport fixture.navigation\n" + body, QUrl());
            if (!component.isReady()) { for (const auto &e : component.errors()) fprintf(stderr, "%s\n", qPrintable(e.description())); }
            assert(component.isReady());
            QObject *object = component.create();
            assert(object);
            auto *item = qobject_cast<QQuickItem *>(object);
            assert(item);
            item->setParentItem(window.contentItem()); item->setParent(&window);
            // Fixture-owned signal connection only; never emit or invoke it.
            const int index = item->metaObject()->indexOfSignal("requestOpenDocumentOnPage(QVariant,QVariant)");
            if (index >= 0) {
                assert(item->metaObject()->method(index).methodType() == QMetaMethod::Signal);
                assert(QObject::connect(item, item->metaObject()->method(index), &recorder,
                    recorder.metaObject()->method(recorder.metaObject()->indexOfSlot("emitted()"))));
            }
            return item;
        };
        const char *location = "absent", *result = "metadata-observed";
        QObject *lossVictim = nullptr;
        if (mode == "navigation-good" || mode == "navigation-multiple" || mode == "navigation-mixed" || mode == "navigation-namespace" || mode == "navigation-other-pointer") {
            const QByteArray selectedProperty = mode == "navigation-namespace" ?
                QByteArray("property alias windowNavigator: typed.windowNavigator; NavigationTypes { id: typed } ") :
                mode == "navigation-other-pointer" ? QByteArray("property Item windowNavigator: null; ") : property;
            create("FocusScope { " + signal + selectedProperty + " }");
            location = "located";
            if (mode == "navigation-multiple" || mode == "navigation-mixed") {
                create("FocusScope { " + signal + (mode == "navigation-multiple" ? property : QByteArray()) + " }");
                location = "ambiguous";
            }
        } else if (mode == "navigation-missing" || mode == "navigation-nonpointer") {
            create("FocusScope { " + signal + (mode == "navigation-nonpointer" ? QByteArray("property int windowNavigator: 0;") : QByteArray()) + " }");
            location = "incompatible";
        } else if (mode == "navigation-fake-focus") {
            auto *item = new FocusScope; item->setParentItem(window.contentItem()); item->setParent(&window);
            QObject::connect(item, &FocusScope::requestOpenDocumentOnPage, &app, [] { ++emissions; });
            location = "incompatible";
        } else if (mode == "navigation-wrong-slot") {
            auto *item = new SlotsNavigation; item->setParentItem(window.contentItem()); item->setParent(&window);
        } else if (mode == "navigation-positive-loss" || mode == "navigation-positive-deadline") {
            create("FocusScope { " + signal + property + " }");
            location = "not-checked";
            result = mode == "navigation-positive-loss" ? "metadata-controller-lost" : "metadata-deadline";
            if (mode == "navigation-positive-loss") {
                lossVictim = new QObject;
                auto *item = new LossItem;
                item->victim = &lossVictim;
                item->setParentItem(window.contentItem()); item->setParent(&window);
            }
        } else if (mode == "navigation-split") {
            create("FocusScope { " + signal + " }"); create("FocusScope { " + property + " }"); location = "incompatible";
        } else if (mode == "navigation-wrong-kind") {
            create("FocusScope { " + property + " function requestOpenDocumentOnPage(entry, page) { tripwire.invoked(); } }");
        } else if (mode == "navigation-wrong-arity") {
            create("FocusScope { signal requestOpenDocumentOnPage(var entry); " + property + " }");
        } else if (mode == "navigation-wrong-parameters") {
            create("FocusScope { signal requestOpenDocumentOnPage(int entry, int page); " + property + " }");
        } else if (mode == "navigation-candidate-cap" || mode == "navigation-candidate-boundary") {
            for (int i = 0; i < (mode == "navigation-candidate-cap" ? 17 : 16); ++i) create("FocusScope { " + signal + property + " }");
            if (mode == "navigation-candidate-cap") { result = "metadata-navigation-candidate-cap"; location = "not-checked"; }
            else location = "ambiguous";
        } else if (mode == "navigation-unreadable" || mode == "navigation-opaque") {
            // Owned static-property helper tests isolate readability/interface
            // classification; full topology tests above use actual QML ancestry.
            UnreadableNavigation unreadable; OpaqueNavigation opaque;
            qml_access::NavigationMetadata n;
            assert(!qml_access::observeNavigation(mode == "navigation-unreadable" ? unreadable.metaObject() : opaque.metaObject(), true, n));
            assert(n.candidates == 1 && n.complete == 0 && n.present == 1);
            if (mode == "navigation-unreadable") assert(n.readable == 0 && n.pointer == 1);
            else { assert(n.typeIncompatible == 1 && n.pointer == 0); assert(!QMetaType::fromName("Opaque*").isValid()); }
            assert(getters == 0 && invocations == 0 && emissions == 0);
            printf("%s: static interface/readability tripwire passed\n", argv[1]); return 0;
        } else if (mode == "navigation-lexical") {
            for (const char *name : {"WindowNavigator*", "owned::WindowNavigator*", "a_1::b2::WindowNavigator*"}) assert(qml_access::navigatorTypeNameCompatible(name));
            for (const char *name : {"", "::WindowNavigator*", "WindowNavigator**", "Other*", "const WindowNavigator*", "WindowNavigator *", "a::::WindowNavigator*", "a::WindowNavigator&", "9a::WindowNavigator*"}) assert(!qml_access::navigatorTypeNameCompatible(name));
            assert(!qml_access::navigatorTypeNameCompatible(nullptr));
            const std::string boundary = std::string(110, 'a') + "::WindowNavigator*";
            assert(boundary.size() == 128 && qml_access::navigatorTypeNameCompatible(boundary.c_str()));
            assert(!qml_access::navigatorTypeNameCompatible(("a" + boundary).c_str()));
            puts("navigation-lexical: exact bounded identifier grammar passed"); return 0;
        } else if (mode != "navigation-empty") assert(false);
        assert(!QMetaType::fromName("entry::Id").isValid() && !QMetaType::fromName("Opaque*").isValid());
        qml_access::MetadataSummary summary;
        const char *observed = qml_access::observeMetadata(&app, &engine, lossVictim ? lossVictim : &controller,
            [&] { return mode != "navigation-positive-deadline" || summary.navigation.complete == 0; }, summary);
        if (std::string(observed) != result) fprintf(stderr, "navigation result=%s expected=%s\n", observed, result);
        assert(std::string(observed) == result && std::string(summary.navigation.location) == location);
        assert(!QMetaType::fromName("entry::Id").isValid() && !QMetaType::fromName("Opaque*").isValid());
        const auto &n = summary.navigation;
        assert(n.located == (std::string(location) == "located"));
        assert(n.lookups <= 256 && n.candidates <= 16);
        if (mode == "navigation-good" || mode == "navigation-other-pointer") assert(n.candidates == 1 && n.complete == 1 && n.focusScope == 1 && n.variantPair == 1 && n.nameUnknown == 1);
        if (mode == "navigation-namespace") assert(n.complete == 1 && n.nameCompatible == 1);
        if (mode == "navigation-mixed") assert(n.candidates == 2 && n.complete == 1);
        if (mode == "navigation-split" || mode == "navigation-fake-focus") assert(n.complete == 0 && n.candidates == 1);
        if (mode == "navigation-positive-loss" || mode == "navigation-positive-deadline") assert(n.complete == 1 && n.candidates == 1 && !n.located);
        summary.result = observed;
        const auto json = qml_access::metadataJson(summary);
        assert(!json.value("source_authority").toBool() && json.value("owner_relation").toString() == "unproven" && json.value("receiver_relation").toString() == "unproven");
        const QByteArray bytes = QJsonDocument(json).toJson(QJsonDocument::Compact);
        assert(bytes.size() <= 8192 && !bytes.contains("requestOpenDocumentOnPage") && !bytes.contains("QQuickFocusScope") && !bytes.contains("owned::"));
        assert(getters == 0 && invocations == 0 && emissions == 0);
        printf("%s: %s location=%s raw=%d complete=%d, tripwires=0\n", argv[1], observed, location, n.candidates, n.complete);
        return 0;
    }
    const bool navigationProbe = mode.rfind("navigation-probe-", 0) == 0;
    const std::string probeMode = navigationProbe ? "metadata-probe-" + mode.substr(17) : mode;
    if (mode.rfind("metadata-probe-", 0) == 0 || navigationProbe) {
        auto ownedEngine = std::make_unique<QQmlApplicationEngine>();
        QQuickWindow ownedWindow;
        QQmlEngine::setContextForObject(&ownedWindow, ownedEngine->rootContext());
        const QByteArray navSignal = "signal requestOpenDocumentOnPage(var entry, var page); ";
        const QByteArray navProperty = "property QtObject windowNavigator: null; ";
        QByteArray capChildren;
        if (probeMode == "metadata-probe-navigation-cap")
            for (int i = 0; i < 16; ++i) capChildren += "FocusScope { " + navSignal + navProperty + " }";
        const QByteArray navBody = "FocusScope { " + (probeMode == "metadata-probe-absent" ? QByteArray() : navSignal) +
            (probeMode == "metadata-probe-incompatible" ? QByteArray() : navProperty) +
            (probeMode == "metadata-probe-mixed" ? "FocusScope { " + navSignal + " }" : QByteArray()) + capChildren + " }";
        ownedEngine->loadData(navigationProbe ? "import QtQuick\n" + navBody : QByteArray("import QtQml\nQtObject {}"));
        assert(ownedEngine->rootObjects().size() == 1);
        EmissionTripwire recorder;
        if (navigationProbe) {
            auto *root = qobject_cast<QQuickItem *>(ownedEngine->rootObjects().first());
            assert(root); root->setParentItem(ownedWindow.contentItem());
            const int index = root->metaObject()->indexOfSignal("requestOpenDocumentOnPage(QVariant,QVariant)");
            if (index >= 0) assert(QObject::connect(root, root->metaObject()->method(index), &recorder,
                recorder.metaObject()->method(recorder.metaObject()->indexOfSlot("emitted()"))));
        }
        auto *ownedController = new SceneController;
        QPointer<SceneController> controllerGuard = ownedController;
        QPointer<qml_access::Probe> probe;
        std::unique_ptr<QQuickWindow> ambiguousWindow;
        QQmlEngine ambiguousEngine;
        int receipts = 0;
        std::string stage;
        qmlRegisterSingletonType<SceneController>("xofm.libs.library", 1, 0, "DocumentController",
            [&](QQmlEngine *, QJSEngine *) -> QObject * {
                QQmlEngine::setObjectOwnership(ownedController, QQmlEngine::CppOwnership);
                QTimer::singleShot(0, &app, [&] {
                    assert(probe && receipts == 0); // No access-only early receipt.
                    if (probeMode == "metadata-probe-cancel")
                        assert(QMetaObject::invokeMethod(&app, "aboutToQuit", Qt::DirectConnection));
                    if (probeMode == "metadata-probe-deadline") QThread::msleep(150);
                    if (probeMode == "metadata-probe-controller-lost") delete controllerGuard.data();
                    if (probeMode == "metadata-probe-engine-lost") ownedEngine.reset();
                    if (probeMode == "metadata-probe-ambiguous") {
                        ambiguousWindow = std::make_unique<QQuickWindow>();
                        QQmlEngine::setContextForObject(ambiguousWindow.get(), ambiguousEngine.rootContext());
                    }
                });
                return ownedController;
            });
        if (probeMode == "metadata-probe-refusal")
            for (int i = 0; i < 17; ++i) { auto *item = new SceneView; item->setParentItem(ownedWindow.contentItem()); item->setParent(&ownedWindow); }
        probe = new qml_access::Probe(&app, [&](const char *s, bool a, bool e, bool h, bool c, const QByteArray &bytes) {
            ++receipts; stage = s;
            assert(a && e && h && bytes.size() <= 8192);
            const auto metadata = QJsonDocument::fromJson(bytes).object().value("metadata").toObject();
            fprintf(stderr, "queued stage=%s metadata_result=%s receipts=%d\n", s,
                qPrintable(metadata.value("metadata_result").toString()), receipts);
            assert(!metadata.value("source_authority").toBool() && metadata.value("owner_relation").toString() == "unproven");
            assert(metadata.value("metadata_result").toString() == (stage == "resolved" ? "metadata-observed" : QString::fromLatin1(s)));
            const auto navigation = metadata.value("navigation_candidate").toObject();
            assert(metadata.value("receiver_relation").toString() == "unproven");
            if (stage != "resolved") assert(!navigation.value("located").toBool() && navigation.value("location_result").toString() == "not-checked");
            if (navigationProbe && stage == "resolved") {
                const char *location = probeMode == "metadata-probe-absent" ? "absent" :
                    probeMode == "metadata-probe-incompatible" ? "incompatible" : probeMode == "metadata-probe-mixed" ? "ambiguous" : "located";
                assert(navigation.value("location_result").toString() == location);
                assert(navigation.value("located").toBool() == (std::string(location) == "located"));
            }
            if (stage == "resolved") assert(c && metadata.value("creation_signature").toObject().value("returns_bool").toBool());
            app.quit();
        }, 3000, probeMode == "metadata-probe-deadline" ? 100 : 3000);
        QTimer::singleShot(4000, &app, &QCoreApplication::quit);
        if (probeMode == "metadata-probe-cancel") QTimer::singleShot(300, &app, &QCoreApplication::quit);
        app.exec();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        assert(!probe && getters == 0 && invocations == 0 && emissions == 0);
        if (probeMode == "metadata-probe-cancel") assert(receipts == 0);
        else {
            assert(receipts == 1);
            const char *expected = probeMode == "metadata-probe-refusal" ? "metadata-candidate-cap" :
                probeMode == "metadata-probe-navigation-cap" ? "metadata-navigation-candidate-cap" :
                probeMode == "metadata-probe-deadline" ? "deadline" :
                probeMode == "metadata-probe-controller-lost" ? "metadata-controller-lost" :
                probeMode == "metadata-probe-engine-lost" ? "engine-lost" :
                probeMode == "metadata-probe-ambiguous" ? "metadata-engine-changed" : "resolved";
            if (stage != expected) fprintf(stderr, "queued fixture stage=%s expected=%s\n", stage.c_str(), expected);
            assert(stage == expected);
        }
        delete controllerGuard.data();
        printf("%s: %s, queued metadata envelope and teardown passed\n", argv[1], stage.c_str());
        return 0;
    }
    QQmlEngine engine;
    QQuickWindow window;
    QQmlEngine::setContextForObject(&window, engine.rootContext());
    SceneController controller;
    WrongReturnController wrongReturn;
    QObject plainController;
    QObject *selected = &controller;
    std::vector<std::unique_ptr<QQuickWindow>> extraWindows;
    std::unique_ptr<QQmlEngine> otherEngine;
    auto attach = [&](QQuickItem *item) { item->setParentItem(window.contentItem()); item->setParent(&window); };
    const char *expected = "metadata-observed";
    bool budget = true;
    int checks = 0;
    QObject *victim = nullptr;
    QThread foreign;
    std::unique_ptr<QObject> foreignController;
    if (mode == "metadata-good" || mode == "metadata-multiple" || mode == "metadata-mismatch") {
        attach(new SceneView);
        attach(new DocumentView);
        attach(new SceneSelectionHandler);
        attach(new ShortcutsDocumentView);
        if (mode == "metadata-multiple") { attach(new SceneView); attach(new DocumentView); attach(new SceneSelectionHandler); }
        if (mode == "metadata-mismatch") { attach(new WrongSceneView); attach(new OpaqueDocumentView); }
    } else if (mode == "metadata-empty") selected = &plainController;
    else if (mode == "metadata-superclass-cap") { attach(new Layer30); expected = "metadata-superclass-cap"; }
    else if (mode == "metadata-superclass-boundary") attach(new Layer29);
    else if (mode == "metadata-wrong-return") selected = &wrongReturn;
    else if (mode == "metadata-node-cap" || mode == "metadata-node-boundary") {
        for (int i = 0; i < (mode == "metadata-node-cap" ? 256 : 255); ++i) attach(new QQuickItem);
        if (mode == "metadata-node-cap") expected = "metadata-node-cap";
    } else if (mode == "metadata-depth-cap" || mode == "metadata-depth-boundary") {
        QQuickItem *parent = window.contentItem();
        for (int i = 0; i < (mode == "metadata-depth-cap" ? 17 : 16); ++i) { auto *child = new QQuickItem(parent); parent = child; }
        if (mode == "metadata-depth-cap") expected = "metadata-depth-cap";
    } else if (mode == "metadata-candidate-cap" || mode == "metadata-candidate-boundary") {
        for (int i = 0; i < (mode == "metadata-candidate-cap" ? 17 : 16); ++i) attach(new SceneView);
        if (mode == "metadata-candidate-cap") expected = "metadata-candidate-cap";
    } else if (mode == "metadata-window-cap" || mode == "metadata-unknown-engine" || mode == "metadata-other-engine") {
        const int count = mode == "metadata-window-cap" ? 16 : 1;
        if (mode == "metadata-other-engine") otherEngine = std::make_unique<QQmlEngine>();
        for (int i = 0; i < count; ++i) {
            extraWindows.emplace_back(new QQuickWindow);
            if (mode != "metadata-unknown-engine") QQmlEngine::setContextForObject(extraWindows.back().get(), otherEngine ? otherEngine->rootContext() : engine.rootContext());
        }
        expected = mode == "metadata-window-cap" ? "metadata-window-cap" : mode == "metadata-other-engine" ? "metadata-engine-changed" : "metadata-window-engine";
    } else if (mode == "metadata-controller-lost" || mode == "metadata-item-lost" || mode == "metadata-window-lost") {
        auto *loss = new LossItem;
        attach(loss);
        if (mode == "metadata-controller-lost") { victim = new QObject; selected = victim; }
        else if (mode == "metadata-item-lost") { auto *item = new QQuickItem; attach(item); victim = item; }
        else if (mode == "metadata-window-lost") {
            auto *extra = new QQuickWindow;
            QQmlEngine::setContextForObject(extra, engine.rootContext());
            victim = extra;
        }
        loss->victim = &victim;
        expected = mode == "metadata-controller-lost" ? "metadata-controller-lost" :
            mode == "metadata-item-lost" ? "metadata-item-lost" :
            "metadata-window-lost";
    } else if (mode == "metadata-thread") {
        foreignController = std::make_unique<QObject>();
        foreignController->moveToThread(&foreign);
        selected = foreignController.get();
        expected = "metadata-thread";
    } else if (mode == "metadata-deadline") { budget = false; expected = "metadata-deadline"; }
    else if (mode == "metadata-mid-deadline") expected = "metadata-deadline";
    else assert(false);
    // Fixture-only registry queries: interface inspection must not register these
    // previously unknown source names. The payload never calls fromName/id.
    assert(!QMetaType::fromName("entry::Id").isValid());
    assert(!QMetaType::fromName("Opaque*").isValid());
    qml_access::MetadataSummary summary;
    const char *result = qml_access::observeMetadata(&app, &engine, selected, [&] {
        ++checks; return budget && (mode != "metadata-mid-deadline" || checks < 12);
    }, summary);
    assert(std::string(result) == expected);
    assert(getters == 0 && invocations == 0);
    assert(!QMetaType::fromName("entry::Id").isValid());
    assert(!QMetaType::fromName("Opaque*").isValid());
    summary.result = result;
    const auto json = qml_access::metadataJson(summary);
    const auto bytes = QJsonDocument(json).toJson(QJsonDocument::Compact);
    assert(bytes.size() < 8192 && !json.value("source_authority").toBool());
    assert(json.value("owner_relation").toString() == "unproven");
    assert(!bytes.contains("addPageWith") && !bytes.contains("QmlDocumentWrapper") && !bytes.contains("Opaque"));
    if (mode == "metadata-good" || mode == "metadata-multiple" || mode == "metadata-mismatch") {
        const int valid = mode == "metadata-multiple" ? 2 : 1;
        assert(summary.scene.complete == valid && summary.document.complete == valid && summary.selection.complete == valid);
        assert(summary.scene.candidates == (mode == "metadata-mismatch" ? 2 : valid));
        assert(summary.document.candidates == (mode == "metadata-mismatch" ? 2 : valid));
        assert(summary.scene.properties[0].notify == valid);
        assert(summary.signaturePresent && summary.signatureReturnsBool);
        if (mode == "metadata-mismatch") {
            assert(summary.scene.properties[0].present == 2 && summary.scene.properties[0].typeMatch == 1);
            assert(summary.document.properties[1].present == 2 && summary.document.properties[1].typeMatch == 1);
        }
    }
    if (mode == "metadata-empty") assert(summary.items == 1 && summary.scene.candidates == 0 && !summary.signaturePresent);
    if (mode == "metadata-wrong-return") assert(summary.signaturePresent && !summary.signatureReturnsBool);
    printf("%s: %s, getters=0 invocations=0 authority=false\n", argv[1], result);
    return 0;
}
#include "qt_qml_access_probe_metadata_fixture.moc"
