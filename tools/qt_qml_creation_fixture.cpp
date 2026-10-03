/* All documents, native-like IDs and callbacks are owned synthetic QML. */
#include "qt_qml_access_probe_core.h"
#include <QQmlContext>
#include <QTemporaryDir>
#include <QFile>
#include <QEventLoop>
#include <cassert>
#include <cstdio>

class FixtureEntry final : public QObject {
    Q_OBJECT
public:
    enum Values { Document = 1, Exporting = 9 };
    Q_ENUM(Values)
};
class CollisionEntry final : public QObject {
    Q_OBJECT
public:
    enum Values { Document = 2, Exporting = 10 };
    Q_ENUM(Values)
};
class FixtureHooks final : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    int calls = 0;
public slots:
    void scheduleReady(QObject *library) {
        const QString mode = library->property("fixtureCase").toString();
        const QPointer<QObject> target(library);
        if (mode == "dev-ready-cancel") QTimer::singleShot(20, library, [this] { cancel(); });
        QTimer::singleShot(mode == "dev-ready-late" ? 450 : 40, library, [this, target, mode] {
            if (!target) return;
            if (mode != "dev-ready-still-false") target->setProperty("isReady", true);
            assert(QMetaObject::invokeMethod(target, "readyChanged", Qt::DirectConnection, Q_ARG(bool, true)));
            assert(QMetaObject::invokeMethod(target, "readyChanged", Qt::DirectConnection, Q_ARG(bool, true)));
            if (mode == "dev-ready-queued-cancel") cancel();
            if (mode == "dev-ready-queued-deadline") QThread::msleep(450);
        });
    }
    void cancel() {
        auto *bridge = QCoreApplication::instance()->findChild<qml_access::CreationBridge *>();
        assert(bridge);
        QPointer<qml_access::CreationBridge> guard(bridge);
        assert(QMetaObject::invokeMethod(QCoreApplication::instance(), "aboutToQuit", Qt::DirectConnection));
        assert(guard); // No owned teardown inside the native/getter stack.
    }
    void foreign() {
        auto *bridge = QCoreApplication::instance()->findChild<qml_access::CreationBridge *>();
        assert(bridge);
        auto *worker = QThread::create([bridge] {
            assert(!bridge->callbackAllowed() && !bridge->preflightAllowed() && !bridge->claimMutation());
            bridge->observeCallback(); bridge->observeReturn(true, true);
            bridge->observeException(QStringLiteral("native-call")); bridge->refuse(QStringLiteral("identity-mismatch"));
            bridge->observePrivateException(QStringLiteral("native-call"), QStringLiteral("Error"), QStringLiteral("synthetic"), QStringLiteral("synthetic"));
            bridge->queueLibraryContinuation();
        });
        worker->start(); assert(worker->wait(1000)); delete worker;
    }
    void nested() {
        ++calls;
        QEventLoop loop;
        QTimer::singleShot(300, &loop, &QEventLoop::quit);
        loop.exec();
    }
};
int main(int argc, char **argv) {
    assert(argc == 2);
    const QString mode = QString::fromLatin1(argv[1]);
    QGuiApplication app(argc, argv);
    if (mode == "marker-privacy") {
        qml_access::CreationObservation trial;
        trial.enabled = true; trial.exception = true;
        trial.exceptionOperation = qml_access::creationExceptionOperation(QStringLiteral("private attacker marker /home/name/template"));
        const auto bytes = QJsonDocument(qml_access::creationJson(trial)).toJson(QJsonDocument::Compact);
        assert(bytes.contains("exception-stage-unknown") && !bytes.contains("private attacker marker") && !bytes.contains("/home/") && !bytes.contains("template"));
        puts("creation-marker-privacy: unknown exception marker maps to fixed literal, no raw input retained");
        return 0;
    }
    QTemporaryDir dir;
    assert(dir.isValid());
    auto write = [&](const char *name, const QByteArray &source) {
        QFile file(dir.filePath(QString::fromLatin1(name)));
        assert(file.open(QIODevice::WriteOnly));
        assert(file.write(source) == source.size()); file.close();
        return QUrl::fromLocalFile(file.fileName());
    };
    const QString documentId = QStringLiteral("00000000-0000-4000-8000-000000000001");
    QStringList pages;
    for (int i = 0; i < 5; ++i) pages.append(QStringLiteral("00000000-0000-4000-8000-00000000000") + QString::number(i + 2));
    qml_access::CreationConfig config{documentId, pages, true, mode.startsWith("dev-")};
    QByteArray encodedPages = QJsonDocument(QJsonArray::fromStringList(pages)).toJson(QJsonDocument::Compact);
    const auto libraryUrl = write("Library.qml", QByteArray(R"QML(pragma Singleton
import QtQml
QtObject {
    property string fixtureCase: fixtureMode
    property var isReady: fixtureMode === "dev-ready-absent" ? undefined : fixtureMode === "dev-ready-type" ? "true" : fixtureMode.startsWith("dev-ready-") && fixtureMode !== "dev-ready-success" ? false : true
    signal readyChanged(bool ready)
    property int lookups: 0
    Component.onCompleted: {
        if (["dev-ready-transition", "dev-ready-duplicate", "dev-ready-late", "dev-ready-cancel", "dev-ready-queued-cancel", "dev-ready-queued-deadline", "dev-ready-still-false"].indexOf(fixtureMode) >= 0) fixtureHooks.scheduleReady(this);
    }
    property QtObject doc: QtObject {
        property var id: ({ toString: function() { if (fixtureMode === "id-string-throw") throw new Error("private id string details"); return (fixtureMode === "identity" || fixtureMode === "dev-identity") ? "bad" : "00000000-0000-4000-8000-000000000001"; } })
        property var type: fixtureMode === "type" ? 2 : 1
        property var status: fixtureMode === "exporting" ? 9 : fixtureMode === "status" ? "unknown" : 0
        property var isExporting: fixtureMode === "dev-export-true" ? true : fixtureMode === "dev-export-type" ? "false" : fixtureMode === "dev-export-absent" ? undefined : false
        property var pageCount: (fixtureMode === "count" || fixtureMode === "dev-count") ? 6 : 5
        property var ids: )QML") + encodedPages + R"QML(
        function idForPage(i) {
            if (fixtureMode === "getter-throw") throw new Error("private getter details");
            if (fixtureMode === "page0" && i === 0) return "wrong";
            if ((fixtureMode === "later-page" || fixtureMode === "dev-page-order") && i === 4) return ids[3];
            if (fixtureMode === "reorder" && i === 2) return ids[3];
            return ids[i];
        }
        function pageForId(key) { if (fixtureMode === "index-getter-throw") throw new Error("private reverse getter details"); if ((fixtureMode === "roundtrip" || fixtureMode === "dev-reverse")) return 4; return ids.indexOf(key); }
        function templateForPage(i) {
            if (i !== 0) throw new Error("wrong template page");
            if ((fixtureMode === "preclaim-cancel" || fixtureMode === "dev-claim-cancel")) fixtureHooks.cancel();
            if (fixtureMode === "template-throw") throw new Error("private template details");
            return (fixtureMode === "template" || fixtureMode === "dev-template") ? undefined : fixtureMode === "empty-template" ? "" : "SyntheticBackground";
        }
    }
    function entryForId(key) {
        lookups += 1;
        if (fixtureMode === "dev-lookup-error") throw new TypeError("synthetic private conversion detail");
        if (fixtureMode === "dev-error-long") throw { name: "N".repeat(400), message: "M".repeat(400) };
        if (fixtureMode === "dev-error-surrogate") throw { name: "N".repeat(255)+"\uD83D\uDE00", message: "M".repeat(255)+"\uD83D\uDE00" };
        if (fixtureMode === "dev-error-escaped") throw { name: "\u0001".repeat(400), message: "\u0002".repeat(400) };
        if (fixtureMode === "dev-error-properties") { const e = {}; Object.defineProperty(e,"name",{get:function(){throw 1;}}); Object.defineProperty(e,"message",{get:function(){throw 2;}}); throw e; }
        if (fixtureMode === "lookup-throw") throw new Error("private lookup details");
        if (key !== "00000000-0000-4000-8000-000000000001") throw new Error("wrong lookup");
        if (fixtureMode.endsWith("read-throw") || fixtureMode === "dev-export-throw" || fixtureMode === "dev-no-enum-reads") {
            const source = doc;
            const proxy = { idForPage: function(i) { return source.idForPage(i); },
                pageForId: function(key) { return source.pageForId(key); },
                templateForPage: function(i) { return source.templateForPage(i); } };
            const target = fixtureMode === "id-read-throw" ? "id" : fixtureMode === "type-read-throw" ? "type" :
                fixtureMode === "status-read-throw" ? "status" : fixtureMode === "dev-export-throw" ? "isExporting" : "pageCount";
            for (const key of ["id", "type", "status", "pageCount", "isExporting"])
                Object.defineProperty(proxy, key, { get: function() {
                    if ((fixtureMode === "dev-no-enum-reads" && (key === "type" || key === "status")) || (fixtureMode !== "dev-no-enum-reads" && key === target)) throw new Error("private property getter details");
                    return source[key];
                } });
            return proxy;
        }
        return fixtureMode === "missing" ? null : doc;
    }
})QML");
    const auto controllerUrl = write("DocumentController.qml", R"QML(pragma Singleton
import QtQml
import xofm.libs.library
QtObject {
    property int calls: 0
    property int arity: 0
    property int callbackArity: -1
    property bool argumentsCorrect: false
    property var retainedCallback: null
    property Timer delayed: Timer { interval: 10; onTriggered: retainedCallback() }
    function addPageWithTemplateAndPageSize(nativeId,index,template,paper,callback) {
        calls += 1; arity = arguments.length; callbackArity = callback.length;
        argumentsCorrect = nativeId === Library.doc.id && typeof nativeId === "object" && index === 1 &&
            template === (fixtureMode === "empty-template" ? "" : "SyntheticBackground") &&
            paper.width === 1404 && paper.height === 1872;
        retainedCallback = callback;
        if (fixtureMode === "foreign-bridge") fixtureHooks.foreign();
        if (fixtureMode === "native-throw") throw new Error("private native details");
        if (fixtureMode === "reentrant-cancel") { fixtureHooks.cancel(); callback(); return true; }
        if (fixtureMode === "nested-timeout") { fixtureHooks.nested(); callback(); return true; }
        if (fixtureMode === "sync" || fixtureMode === "duplicate") callback();
        if (fixtureMode === "duplicate") { callback(); callback(); }
        if (fixtureMode === "burst") for (let i = 0; i < 128; ++i) callback();
        if (fixtureMode !== "sync" && fixtureMode !== "duplicate" && fixtureMode !== "no-callback" && fixtureMode !== "false-no-callback") delayed.start();
        return fixtureMode === "false-async" || fixtureMode === "false-no-callback" ? false : fixtureMode === "unknown-return" ? undefined : true;
    }
})QML");
    if (mode != "import-missing" && !mode.startsWith("dev-")) qmlRegisterModule("com.remarkable", 1, 0);
    QObject unavailableEnum;
    if (mode == "entry-absent" || mode == "import-com-owned" || mode.startsWith("dev-")) {}
    else if (mode == "enum") qmlRegisterSingletonInstance("xofm.libs.library", 1, 0, "Entry", &unavailableEnum);
    else qmlRegisterUncreatableType<FixtureEntry>("xofm.libs.library", 1, 0, "Entry", "fixture enum only");
    if (mode == "import-com-owned") qmlRegisterUncreatableType<FixtureEntry>("com.remarkable", 1, 0, "Entry", "synthetic com owner");
    if (mode == "import-collision") qmlRegisterUncreatableType<CollisionEntry>("com.remarkable", 1, 0, "Entry", "synthetic conflicting enum");
    int libraryType = -1;
    const auto noMethodUrl = write("NoMethodLibrary.qml", "pragma Singleton\nimport QtQml\nQtObject { property bool isReady: true }");
    if (mode == "dev-library-provider") libraryType = qmlRegisterSingletonType<QObject>("xofm.libs.library", 1, 0, "Library", [](QQmlEngine *, QJSEngine *) -> QObject * { return nullptr; });
    else if (mode != "library-absent" && mode != "dev-library-absent") libraryType = qmlRegisterSingletonType(mode == "dev-library-method" ? noMethodUrl : libraryUrl, "xofm.libs.library", 1, 0, "Library");
    const auto missingMethodUrl = write("MissingController.qml", R"QML(pragma Singleton
import QtQml
QtObject { property int calls: 0; property var retainedCallback: null })QML");
    const int controllerType = qmlRegisterSingletonType(mode == "method-missing" ? missingMethodUrl : controllerUrl, "xofm.libs.library", 1, 0, "DocumentController");
    FixtureHooks hooks;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("fixtureMode", mode);
    engine.rootContext()->setContextProperty("fixtureHooks", &hooks);
    engine.loadData("import QtQml\nQtObject {}", QUrl());
    int collisionSelected = 1;
    if (mode == "import-collision") {
        QQmlComponent selection(&engine);
        selection.setData("import QtQml\nimport com.remarkable\nimport xofm.libs.library\nQtObject { property var selected: Entry.Document }", QUrl());
        assert(selection.isReady());
        std::unique_ptr<QObject> observed(selection.create());
        assert(observed);
        bool numeric = false;
        collisionSelected = observed->property("selected").toInt(&numeric);
        assert(numeric && (collisionSelected == 1 || collisionSelected == 2));
        // Test expectation follows actual owned Qt import resolution, never a guessed native owner.
    }
    QWindow window;
    QQmlEngine::setContextForObject(&window, engine.rootContext());
    if (mode == "repeat") {
        QQmlComponent component(&engine);
        component.setData(qml_access::creationHelper(config), QUrl());
        assert(component.isReady());
        QPointer<QObject> helper = component.create();
        assert(helper);
        qml_access::CreationBridge bridge(&app);
        int claims = 0, callbacks = 0, returns = 0;
        bridge.check = [&](bool claim) { if (claim) return ++claims == 1; return true; };
        bridge.refusal = [](const QString &) { assert(false && "unexpected repeat guard refusal"); };
        bridge.exception = [](const QString &) { assert(false && "unexpected repeat exception"); };
        bridge.returned = [&](bool known, bool value) { assert(known && value); ++returns; };
        bridge.callback = [&] { ++callbacks; };
        assert(helper->setProperty("bridge", QVariant::fromValue<QObject *>(&bridge)));
        for (int i = 0; i < 3; ++i) assert(QMetaObject::invokeMethod(helper, "createOnce", Qt::DirectConnection));
        QEventLoop loop; QTimer::singleShot(50, &loop, &QEventLoop::quit); loop.exec();
        auto *controller = engine.singletonInstance<QObject *>(controllerType);
        assert(claims == 1 && returns == 1 && callbacks == 1 && controller->property("calls").toInt() == 1);
        auto late = controller->property("retainedCallback").value<QJSValue>();
        assert(helper->property("callbackCount").toInt() == 1);
        bridge.disarm();
        assert(!late.call().isError() && helper->property("callbackCount").toInt() == 1 && callbacks == 1);
        delete helper.data(); assert(!helper);
        assert(!late.call().isError() && callbacks == 1);
        puts("creation-repeat: three actual createOnce entries, exactly one claim/native call, late callback inert");
        return 0;
    }
    QPointer<qml_access::Probe> probe;
    int receipts = 0;
    QJsonObject summary;
    QString terminal;
    if (mode == "bad-config") config.pageIds.removeLast();
    probe = new qml_access::Probe(&app, [&](const char *stage, bool a, bool e, bool h, bool c, const QByteArray &bytes) {
        ++receipts; terminal = QString::fromLatin1(stage);
        summary = QJsonDocument::fromJson(bytes).object();
        assert(!summary.isEmpty());
        assert(a && e && bytes.size() <= 8192);
        if (mode == "import-missing") assert(!h && !c);
        else assert(h && (c || terminal == "deadline"));
        assert(!bytes.contains(documentId.toUtf8()) && !bytes.contains(pages[0].toUtf8()));
        auto publicTrial = summary.value("creation_trial").toObject();
        publicTrial.remove("private_error_name"); publicTrial.remove("private_error_message");
        assert(!QJsonDocument(publicTrial).toJson().contains("synthetic private"));
        assert(!bytes.contains("SyntheticBackground"));
        assert(summary.value("metadata").toObject().value("items").toInt() == 0);
        if (mode == "import-missing") assert(summary.value("component_ready_at_ms").isNull());
        else assert(summary.value("component_ready_at_ms").isDouble() && summary.value("component_ready_at_ms").toInteger() >= 0);
        QTimer::singleShot(50, &app, [&] {
            assert(!probe);
            auto *controller = engine.singletonInstance<QObject *>(controllerType);
            assert(controller);
            // An external/native holder invokes the old QML callback after cleanup.
            const QJSValue callback = controller->property("retainedCallback").value<QJSValue>();
            if (callback.isCallable()) {
                auto late = callback;
                assert(!late.call().isError()); assert(!late.call().isError());
            }
            QTimer::singleShot(20, &app, &QCoreApplication::quit);
        });
    }, 3000, mode == "nested-timeout" ? 150 : 400, config);
    QTimer::singleShot(5000, &app, [] { assert(false && "creation fixture timeout"); });
    assert(app.exec() == 0);
    assert(receipts == 1);
    auto *controller = engine.singletonInstance<QObject *>(controllerType);
    const auto trial = summary.value("creation_trial").toObject();
    const bool libraryBoundaryRefusal = mode == "dev-library-absent" || mode == "dev-library-provider" || mode == "dev-library-method" || mode == "dev-ready-absent" || mode == "dev-ready-type" || mode == "dev-ready-late" || mode == "dev-ready-queued-deadline" || mode == "dev-ready-cancel" || mode == "dev-ready-queued-cancel" || mode == "dev-ready-still-false" || mode == "dev-lookup-error" || mode.startsWith("dev-error-");
    const bool preCall = libraryBoundaryRefusal || mode == "enum" || mode == "missing" || mode == "identity" || mode == "type" ||
        mode == "exporting" || mode == "status" || mode == "count" || mode == "page0" || mode == "later-page" ||
        mode == "reorder" || mode == "roundtrip" || mode == "template" || mode == "getter-throw" ||
        mode == "template-throw" || mode == "lookup-throw" || mode == "preclaim-cancel" || mode == "bad-config" || mode == "method-missing" || mode == "entry-absent" || mode == "library-absent" || (mode.endsWith("-throw") && mode != "native-throw") || mode == "dev-export-true" || mode == "dev-export-type" || mode == "dev-export-absent" || mode == "dev-count" || mode == "dev-reverse" || mode == "dev-template" || mode == "dev-claim-cancel" || mode == "dev-identity" || mode == "dev-page-order" || mode == "import-missing" || (mode == "import-collision" && collisionSelected != 1);
    assert(trial.value("mutation_attempted").toBool() == !preCall);
    assert(controller->property("calls").toInt() == (preCall ? 0 : 1));
    assert(!trial.value("durable_success").toBool());
    assert(trial.value("effect_requires_reconciliation").toBool() == !preCall);
    if (!preCall) {
        assert(controller->property("arity").toInt() == 5 && controller->property("callbackArity").toInt() == 0);
        assert(controller->property("argumentsCorrect").toBool());
    }
    assert(trial.value("com_remarkable_import_selected").toBool() == !mode.startsWith("dev-"));
    assert(trial.value("development_explicit_fixture").toBool() == mode.startsWith("dev-"));
    if (mode == "dev-ready-late" || mode == "dev-ready-queued-deadline") {
        assert(terminal == "deadline" && trial.value("callback_count").toInt() == 0);
    } else if (mode == "dev-library-provider") {
        assert(terminal == "creation-exception" || (terminal == "creation-refused" && trial.value("guard_stage").toString() == "library-unavailable"));
    } else if (mode == "import-missing") {
        assert(terminal == "module-missing" && !trial.value("exception").toBool() && !trial.value("returned").toBool());
    } else if (mode == "native-throw") {
        assert(terminal == "creation-exception" && trial.value("exception").toBool() && !trial.value("returned").toBool());
    } else if (mode == "no-callback" || mode == "false-no-callback" || mode == "nested-timeout") {
        assert(terminal == "deadline" && trial.value("phase").toString() == "effect-uncertain");
        assert(trial.value("callback_count").toInt() == 0);
    } else if (mode == "reentrant-cancel" || mode == "preclaim-cancel" || mode == "dev-claim-cancel" || mode == "dev-ready-cancel" || mode == "dev-ready-queued-cancel") {
        assert(terminal == "creation-cancelled" && trial.value("callback_count").toInt() == 0);
    } else if (preCall) {
        assert(terminal == (mode.endsWith("throw") || mode == "entry-absent" || mode == "library-absent" || mode == "dev-library-absent" || mode == "dev-lookup-error" || mode.startsWith("dev-error-") ? "creation-exception" : mode == "bad-config" ? "creation-config" : "creation-refused"));
    } else {
        assert(terminal == "resolved" && trial.value("phase").toString() == "call-observed");
        assert(trial.value("returned").toBool());
        assert(trial.value("callback_count").toInt() == (mode == "duplicate" || mode == "burst" ? 2 : 1));
        assert(trial.value("duplicate_callback").toBool() == (mode == "duplicate" || mode == "burst"));
        if (mode == "false-async") assert(trial.value("returned_bool_known").toBool() && !trial.value("returned_bool").toBool());
        if (mode == "unknown-return") assert(!trial.value("returned_bool_known").toBool());
        if (mode == "sync" || mode == "duplicate") assert(trial.value("first_callback_at_ms").toInteger() <= trial.value("returned_at_ms").toInteger());
    }
    if (mode == "dev-export-true" || mode == "dev-export-type" || mode == "dev-export-absent") assert(trial.value("guard_stage").toString() == "exporting-refusal");
    if (mode == "dev-library-method") assert(trial.value("guard_stage").toString() == "library-method-unavailable");
    if (mode == "dev-ready-absent" || mode == "dev-ready-type") assert(trial.value("guard_stage").toString() == "library-readiness-invalid");
    if (mode == "dev-ready-still-false") assert(trial.value("guard_stage").toString() == "library-not-ready");
    if (trial.value("exception").toBool()) {
        const QString expected = mode == "dev-library-absent" || mode == "dev-library-provider" ? "library-resolve" : mode == "entry-absent" ? "enum-document" : mode == "library-absent" || mode == "lookup-throw" || mode == "dev-lookup-error" || mode.startsWith("dev-error-") ? "library-lookup" :
            mode == "id-read-throw" ? "document-id-read" : mode == "id-string-throw" ? "document-id-string" :
            mode == "type-read-throw" ? "document-type-read" : mode == "status-read-throw" ? "document-status-read" :
            mode == "dev-export-throw" ? "document-exporting-read" : mode == "count-read-throw" ? "document-count-read" : mode == "getter-throw" ? "page-id-read" :
            mode == "index-getter-throw" ? "page-index-read" : mode == "template-throw" ? "template-read" : "native-call";
        assert(trial.value("exception_operation").toString() == expected);
    } else assert(trial.value("exception_operation").isNull());
    if (libraryType >= 0 && mode != "dev-library-provider" && mode != "dev-library-method") {
        auto *lib = engine.singletonInstance<QObject *>(libraryType);
        assert(lib && lib->property("lookups").toInt() <= 1);
        if (mode.startsWith("dev-ready-")) assert(lib->property("lookups").toInt() == (preCall ? 0 : 1));
    }
    if (mode == "dev-lookup-error") {
        assert(trial.value("exception_category").toString() == "TypeError");
        assert(trial.value("private_error_name").toString() == "TypeError");
        assert(trial.value("private_error_message").toString() == "synthetic private conversion detail");
    }
    if (mode == "dev-error-long") assert(trial.value("private_error_name").toString().size() == 256 && trial.value("private_error_message").toString().size() == 256 && trial.value("exception_category").toString() == "unknown");
    if (mode == "dev-error-surrogate") assert(trial.value("private_error_name").toString() == QString(255,'N') && trial.value("private_error_message").toString() == QString(255,'M'));
    if (mode == "dev-error-escaped") assert(trial.value("private_error_name").toString() == QString(256,QChar(1)) && trial.value("private_error_message").toString() == QString(256,QChar(2)));
    if (mode == "dev-error-properties") assert(trial.value("private_error_name").toString().isEmpty() && trial.value("private_error_message").toString().isEmpty() && trial.value("exception_category").toString() == "unknown");
    printf("creation-%s: stage=%s calls=%d callback=%d, guarded one-call and late cleanup passed\n", argv[1], qPrintable(terminal), controller->property("calls").toInt(), trial.value("callback_count").toInt());
}
#include "qt_qml_creation_fixture.moc"
