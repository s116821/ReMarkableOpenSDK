#include "qt_qml_access_probe_core.h"
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <cstdio>
#include <memory>
#include <QEventLoop>
#include <cassert>
using namespace qml_access;
class DocumentFixture final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY countChanged)
public:
    QStringList ids;
    QString id() const { return QStringLiteral("00000000-0000-4000-8000-000000000001"); }
    int pageCount() const { return ids.size(); }
    Q_INVOKABLE QString idForPage(int index) const { return ids.value(index); }
    Q_INVOKABLE int pageForId(const QString &id) const { return ids.indexOf(id); }
signals:
    void countChanged();
};
class SceneView : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QObject *document MEMBER document NOTIFY documentWrapperChanged)
    Q_PROPERTY(QString pageId MEMBER pageId NOTIFY pageIdChanged)
public:
    QObject *document = nullptr;
    QString pageId;
signals:
    void pageIdChanged();
    void documentWrapperChanged();
};
class OwnedReceiver : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(bool drawingAreaFocused READ focusEvidence NOTIFY focusEvidenceChanged)
public:
    OwnedReceiver() { setFlag(QQuickItem::ItemIsFocusScope); }
    std::function<bool()> evidence;
    std::function<void()> getterHook;
    bool focusEvidence() const { if (getterHook) getterHook(); return evidence && evidence(); }
signals:
    void focusEvidenceChanged();
};
class NativePump final : public QObject {
    Q_OBJECT
public:
    std::function<void()> run;
    Q_INVOKABLE void pump() { if (run) run(); }
};
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    if (argc != 2) return 2;
    const QByteArray requested(argv[1]);
    const bool integrated = requested.startsWith("probe-");
    const QByteArray mode = integrated ? requested.mid(6) : requested;
    QObject controller;
    qmlRegisterSingletonInstance("xofm.libs.library", 1, 0, "DocumentController", &controller);
    qmlRegisterType<SceneView>("OwnedFixture", 1, 0, "SceneView");
    qmlRegisterType<OwnedReceiver>("OwnedFixture", 1, 0, "OwnedReceiver");
    auto ownedEngine = std::make_unique<QQmlApplicationEngine>();
    auto ownedDocument = std::make_unique<DocumentFixture>();
    auto &engine = *ownedEngine;
    auto &document = *ownedDocument;
    PageOpenConfig config;
    config.enabled = true;
    config.documentId = document.id();
    for (int i = 2; i <= 7; ++i) config.pageIds << QStringLiteral("00000000-0000-4000-8000-00000000000%1").arg(i);
    config.sourcePageId = config.pageIds[0]; config.targetPageId = config.pageIds[1];
    document.ids = config.pageIds;
    engine.rootContext()->setContextProperty("ownedDocument", &document);
    engine.rootContext()->setContextProperty("ambiguousFixture", mode == "ambiguous");
    NativePump nativePump;
    engine.rootContext()->setContextProperty("nativePump", &nativePump);
    QByteArray qml = R"QML(import QtQuick
import QtQuick.Window
import OwnedFixture 1.0
Window { visible: true; width: 400; height: 400
    FocusScope { anchors.fill: parent
        property QtObject document: ownedDocument
        readonly property int currentPage: receiver.currentPage
        readonly property string currentPageId: scene ? scene.pageId : ""
        readonly property bool drawingAreaFocused: ambiguousFixture && scene && scene.activeFocus
    OwnedReceiver { id: receiver; objectName: "fixtureReceiver"; anchors.fill: parent; focus: true
        property QtObject document: ownedDocument
        property int currentPage: 0
        readonly property string currentPageId: scene ? scene.pageId : ""
        property int calls: 0
        property bool noMovement: false
        property bool wrongPage: false
        property bool pumpNative: false
        function openPage(index, position) {
            if (position !== undefined) throw new Error("optional position was not omitted");
            calls++;
            if (!noMovement) { currentPage = wrongPage ? 2 : index; scene.pageId = document.idForPage(currentPage); }
            if (pumpNative) nativePump.pump();
        }
        SceneView { id: scene; objectName: "fixtureScene"; anchors.fill: parent; focus: true
            document: receiver.document; pageId: receiver.document ? receiver.document.idForPage(receiver.currentPage) : ""
        }
    }
    }
})QML";
    engine.loadData(qml);
    if (engine.rootObjects().size() != 1) return 3;
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects()[0]);
    auto *receiver = window->findChild<OwnedReceiver *>("fixtureReceiver");
    auto *scene = window->findChild<SceneView *>("fixtureScene");
    receiver->evidence = [&] { return scene && scene->hasActiveFocus(); };
    window->requestActivate(); scene->forceActiveFocus();
    QCoreApplication::processEvents();
    bool progress = true;
    if (mode == "already") receiver->setProperty("currentPage", 1);
    if (mode == "hidden") receiver->setVisible(false);
    if (mode == "disabled") receiver->setEnabled(false);
    if (mode == "no-window") window->hide();
    if (mode == "document-mismatch") scene->setProperty("document", QVariant::fromValue<QObject *>(&app));
    if (mode == "alias") scene->setProperty("pageId", QStringLiteral("wrong"));
    if (mode == "mapping") document.ids.swapItemsAt(0, 1);
    if (mode == "missing") document.ids.removeLast();
    if (mode == "duplicate") document.ids[1] = document.ids[0];
    if (mode == "config") config.pageIds.removeLast();
    if (mode == "stale-source") config.sourcePageId = config.pageIds[3];
    if (mode == "node-bound") for (int i = 0; i < 4097; ++i) new QQuickItem(window->contentItem());
    if (mode == "depth-bound") {
        auto *parent = window->contentItem();
        for (int i = 0; i < 26; ++i) parent = new QQuickItem(parent);
    }
    if (mode == "cancel") progress = false;
    if (mode == "timeout") receiver->setProperty("noMovement", true);
    if (mode == "wrong-page") receiver->setProperty("wrongPage", true);
    if (mode == "cancel-after") QObject::connect(scene, &SceneView::pageIdChanged, &app, [&] {
        progress = false;
        if (integrated) QMetaObject::invokeMethod(&app, "aboutToQuit", Qt::DirectConnection);
    });
    QString result;
    QPointer<Probe> probe;
    bool pumping = false, nestedHeld = false;
    if (mode == "nested-cancel" || mode == "nested-deadline") {
        receiver->setProperty("pumpNative", true);
        nativePump.run = [&] {
            pumping = true;
            QEventLoop loop;
            if (mode == "nested-cancel") QTimer::singleShot(1, &loop, [&] {
                QMetaObject::invokeMethod(&app, "aboutToQuit", Qt::DirectConnection);
            });
            QTimer::singleShot(mode == "nested-deadline" ? 200 : 10, &loop, [&] {
                QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
                nestedHeld = probe && probe->findChild<PageOpenSession *>() && result.isEmpty();
                loop.quit();
            });
            loop.exec();
            pumping = false;
        };
    }
    int enteredReads = 0;
    bool targetHookUsed = false;
    if (mode.startsWith("getter-")) receiver->getterHook = [&] {
        auto *activeSession = integrated && probe ? probe->findChild<PageOpenSession *>() : nullptr;
        if (activeSession && activeSession->attempted() && activeSession->returned() && !targetHookUsed && mode.startsWith("getter-target-")) {
            targetHookUsed = true;
            if (mode == "getter-target-expiry") QThread::msleep(600);
            else { scene->setVisible(false); scene->setVisible(true); scene->forceActiveFocus(); }
            return;
        }
        if (mode.startsWith("getter-target-")) return;
        // The two six-page observe passes exercise sixteen native owner reads;
        // the seventeenth is the final bridge claim's getter-bearing admission.
        if (!activeSession || activeSession->attempted()) return;
        // Session/helper admission starts with observe(); count only after the
        // owned QML helper has latched entered, excluding topology/wiring reads.
        bool entered = false;
        for (QObject *child : activeSession->children())
            if (child->property("entered").metaType() == QMetaType::fromType<bool>() && child->property("entered").toBool()) entered = true;
        if (entered && ++enteredReads == 17) {
            if (mode == "getter-expiry") QThread::msleep(600);
            else if (mode == "getter-document") {
                scene->setProperty("document", QVariant::fromValue<QObject *>(&app));
                scene->setProperty("document", QVariant::fromValue<QObject *>(&document));
            }
            else { scene->setVisible(false); scene->setVisible(true); scene->forceActiveFocus(); }
        }
    };
    PageOpenSession session(&app, &engine, config, [&] { return progress; }, [&](const char *value) { result = QString::fromLatin1(value); app.quit(); });
    if (integrated) {
        probe = new Probe(&app, [&](const char *stage, bool a, bool e, bool h, bool c, const QByteArray &diagnostic) {
            assert(!pumping);
            result = QString::fromLatin1(stage);
            const auto trial = QJsonDocument::fromJson(diagnostic).object().value("page_open_trial").toObject();
            if (!a || !e || !h || (!c && mode != "timeout" && !mode.contains("expiry") && mode != "nested-deadline") || trial.value("native_api_qualified").toBool() || trial.value("render_authority").toBool()) result = "bad-integrated-receipt";
            if (mode == "already" && (trial.value("native_call_attempted").toBool() || trial.value("native_call_returned").toBool())) result = "bad-noop-receipt";
            app.quit();
        }, 3000, mode == "nested-deadline" ? 100 : 500, {}, config);
    } else QTimer::singleShot(0, &session, [&] {
        session.begin();
        if (mode == "destroy") delete scene;
        if (mode == "cancel-queued") progress = false;
        if (mode == "remap-queued") document.ids.swapItemsAt(0, 1);
        if (mode == "page-change-queued") { receiver->setProperty("currentPage", 2); receiver->setProperty("currentPage", 0); }
        if (mode == "notifications") { session.changed(); session.changed(); }
        if (mode == "focus-loss") { scene->setVisible(false); scene->setVisible(true); }
        if (mode == "document-change") { scene->setProperty("document", QVariant::fromValue<QObject *>(&app)); scene->setProperty("document", QVariant::fromValue<QObject *>(&document)); }
        if (mode == "window-destroy") { receiver = nullptr; scene = nullptr; delete window; }
        if (mode == "engine-destroy") { receiver = nullptr; scene = nullptr; ownedEngine.reset(); }
        if (mode == "document-destroy") ownedDocument.reset();
    });
    QTimer::singleShot(integrated ? 5000 : 200, &app, [&] { if (result.isEmpty()) result = "unresolved"; app.quit(); });
    app.exec();
    const int calls = receiver ? receiver->property("calls").toInt() : 0;
    const bool good = mode == "good" || mode == "already" || mode == "notifications";
    const bool ok = good ? result == "open-observed" && calls == (mode == "already" ? 0 : 1) :
        mode.startsWith("getter-target-") ? targetHookUsed && result != "open-observed" && calls == 1 :
        mode.startsWith("getter-") ? enteredReads >= 17 && result != "open-observed" && calls == 0 :
        mode == "timeout" ? result == (integrated ? "deadline" : "unresolved") && calls == 1 :
        mode == "nested-cancel" || mode == "nested-deadline" ? nestedHeld && result == (mode == "nested-cancel" ? "open-cancelled" : "deadline") && calls == 1 :
        mode == "wrong-page" || mode == "cancel-after" ? result != "open-observed" && calls == 1 : result != "open-observed" && calls == 0;
    std::printf("%s result=%s calls=%d %s\n", requested.constData(), qPrintable(result), calls, ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
#include "qt_page_open_fixture.moc"
