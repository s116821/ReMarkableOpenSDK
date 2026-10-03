// Owned synthetic metadata only; native getters and creation stay tripwires.
#include "qt_qml_access_probe_core.h"
#include <QQmlContext>
#include <QJSValue>
#include <cassert>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

static int getters = 0, invocations = 0;
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
    if (mode.rfind("metadata-probe-", 0) == 0) {
        auto ownedEngine = std::make_unique<QQmlApplicationEngine>();
        QQuickWindow ownedWindow;
        QQmlEngine::setContextForObject(&ownedWindow, ownedEngine->rootContext());
        ownedEngine->loadData("import QtQml\nQtObject {}");
        assert(ownedEngine->rootObjects().size() == 1);
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
                    if (mode == "metadata-probe-cancel")
                        assert(QMetaObject::invokeMethod(&app, "aboutToQuit", Qt::DirectConnection));
                    if (mode == "metadata-probe-deadline") QThread::msleep(150);
                    if (mode == "metadata-probe-controller-lost") delete controllerGuard.data();
                    if (mode == "metadata-probe-engine-lost") ownedEngine.reset();
                    if (mode == "metadata-probe-ambiguous") {
                        ambiguousWindow = std::make_unique<QQuickWindow>();
                        QQmlEngine::setContextForObject(ambiguousWindow.get(), ambiguousEngine.rootContext());
                    }
                });
                return ownedController;
            });
        if (mode == "metadata-probe-refusal")
            for (int i = 0; i < 17; ++i) { auto *item = new SceneView; item->setParentItem(ownedWindow.contentItem()); item->setParent(&ownedWindow); }
        probe = new qml_access::Probe(&app, [&](const char *s, bool a, bool e, bool h, bool c, const QByteArray &bytes) {
            ++receipts; stage = s;
            assert(a && e && h && bytes.size() <= 8192);
            const auto metadata = QJsonDocument::fromJson(bytes).object().value("metadata").toObject();
            fprintf(stderr, "queued stage=%s metadata_result=%s receipts=%d\n", s,
                qPrintable(metadata.value("metadata_result").toString()), receipts);
            assert(!metadata.value("source_authority").toBool() && metadata.value("owner_relation").toString() == "unproven");
            assert(metadata.value("metadata_result").toString() == (stage == "resolved" ? "metadata-observed" : QString::fromLatin1(s)));
            if (stage == "resolved") assert(c && metadata.value("creation_signature").toObject().value("returns_bool").toBool());
            app.quit();
        }, 3000, mode == "metadata-probe-deadline" ? 100 : 3000);
        QTimer::singleShot(4000, &app, &QCoreApplication::quit);
        if (mode == "metadata-probe-cancel") QTimer::singleShot(300, &app, &QCoreApplication::quit);
        app.exec();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        assert(!probe && getters == 0 && invocations == 0);
        if (mode == "metadata-probe-cancel") assert(receipts == 0);
        else {
            assert(receipts == 1);
            const char *expected = mode == "metadata-probe-refusal" ? "metadata-candidate-cap" :
                mode == "metadata-probe-deadline" ? "deadline" :
                mode == "metadata-probe-controller-lost" ? "metadata-controller-lost" :
                mode == "metadata-probe-engine-lost" ? "engine-lost" :
                mode == "metadata-probe-ambiguous" ? "metadata-engine-changed" : "resolved";
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
