/* All engines, windows and singletons here are owned synthetic objects. */
#include "qt_qml_access_probe_core.h"
#include <QQmlContext>
#include <cassert>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>
#include <QEventLoop>

int main(int argc, char **argv) {
    assert(argc == 2);
    const std::string mode = argv[1];
    QGuiApplication app(argc, argv);
    if (mode == "classifier") {
        auto error = [](const QString &description) { QQmlError e; e.setDescription(description); return e; };
        const auto missing = error(QStringLiteral("module \"fixture.absent\" is not installed"));
        assert(std::string(qml_access::errorStage({missing})) == "module-missing");
        assert(std::string(qml_access::errorStage({error(QStringLiteral("module \"fixture.versioned\" version 1.0 is not installed"))})) == "module-version");
        assert(std::string(qml_access::errorStage({error(QStringLiteral("UnknownRoot is not a type"))})) == "type-missing");
        assert(std::string(qml_access::errorStage({error(QStringLiteral("Invalid property assignment: unsupported type \"QObject*\""))})) == "property-type");
        assert(std::string(qml_access::errorStage({})) == "component-error");
        assert(std::string(qml_access::errorStage({error(QStringLiteral("unrecognized diagnostic"))})) == "component-error");
        assert(std::string(qml_access::errorStage({missing, error(QStringLiteral("UnknownRoot is not a type"))})) == "component-error");
        assert(std::string(qml_access::errorStage(QList<QQmlError>(9, missing))) == "component-error");
        assert(std::string(qml_access::errorStage(QList<QQmlError>(8, missing))) == "module-missing");
        assert(std::string(qml_access::errorStage({error(QString(257, QChar('x')))})) == "component-error");
        const QString prefix = QStringLiteral("module \"");
        const QString suffix = QStringLiteral("\" is not installed");
        assert(std::string(qml_access::errorStage({error(prefix + QString(256-prefix.size()-suffix.size(), QChar('x')) + suffix)})) == "module-missing");
        puts("classifier: fixed categories, unknown/mixed/size/count bounds passed");
        return 0;
    }
    if (mode == "compiler-errors") {
        QQmlEngine owned;
        qmlRegisterType<QObject>("fixture.versioned", 2, 0, "Owned");
        auto compile = [&](const char *text, const char *expected) {
            QQmlComponent component(&owned);
            component.setData(text, QUrl());
            if (component.isLoading()) {
                QEventLoop loop;
                QObject::connect(&component, &QQmlComponent::statusChanged, &loop, [&] {
                    if (!component.isLoading()) loop.quit();
                });
                QTimer::singleShot(3000, &loop, &QEventLoop::quit);
                loop.exec();
            }
            if (std::string(expected) == "ready") assert(component.isReady());
            else assert(component.isError() && std::string(qml_access::errorStage(component.errors())) == expected);
        };
        compile("import QtQml\nimport fixture.versioned 1.0\nQtObject {}", "module-version");
        compile("import QtQml\nimport fixture.versioned\nQtObject {}", "ready");
        compile("import QtQml\nimport fixture.absent\nQtObject {}", "module-missing");
        compile("import QtQml\nUnknownRoot {}", "type-missing");
        compile("import QtQml\nQtObject { property QtObject incompatible: 3 }", "property-type");
        puts("compiler-errors: actual owned Qt categories and versionless major passed");
        return 0;
    }
    QObject singleton;
    int deaths = 0;
    int receipts = 0;
    bool nestedEntered = false;
    QPointer<qml_access::Probe> guard;
    QObject::connect(&singleton, &QObject::destroyed, [&] { ++deaths; });
    if (mode == "nested-deadline") {
        qmlRegisterSingletonType<QObject>("xofm.libs.library", 1, 0, "DocumentController",
            [&](QQmlEngine *, QJSEngine *) -> QObject * {
                nestedEntered = true;
                QEventLoop loop;
                QTimer::singleShot(1100, &loop, &QEventLoop::quit);
                loop.exec();
                assert(guard && receipts == 0); // No deletion/receipt inside create.
                QQmlEngine::setObjectOwnership(&singleton, QQmlEngine::CppOwnership);
                return &singleton;
            });
    } else if (mode != "absent")
        qmlRegisterSingletonInstance("xofm.libs.library", mode == "major-version" ? 2 : 1, 0, "DocumentController", &singleton);
    QQmlEngine engine;
    QWindow window;
    std::unique_ptr<QQmlEngine> other;
    std::unique_ptr<QWindow> otherWindow;
    std::vector<std::unique_ptr<QWindow>> extraWindows;
    if (mode == "window-cap")
        for (int i = 0; i < 16; ++i) extraWindows.push_back(std::make_unique<QWindow>());
    if (mode == "existing") {
        QQmlComponent prepare(&engine);
        prepare.setData(qml_access::helper, QUrl());
        assert(prepare.isReady());
        std::unique_ptr<QObject> object(prepare.create());
        assert(object && object->property("controllerAvailable").toBool());
    }
    if (mode == "conflict") {
        other = std::make_unique<QQmlEngine>();
        QQmlComponent prepare(other.get());
        prepare.setData(qml_access::helper, QUrl());
        assert(prepare.isReady());
        std::unique_ptr<QObject> object(prepare.create());
        assert(object && object->property("controllerAvailable").toBool());
    }
    if (mode != "deadline" && mode != "late" && mode != "cap" && mode != "cancel" && mode != "engine-lost")
        QQmlEngine::setContextForObject(&window, engine.rootContext());
    if (mode == "multiple") {
        other = std::make_unique<QQmlEngine>();
        otherWindow = std::make_unique<QWindow>();
        QQmlEngine::setContextForObject(otherWindow.get(), other->rootContext());
    }
    if (mode == "engine-lost") {
        other = std::make_unique<QQmlEngine>();
        QQmlEngine::setContextForObject(&window, other->rootContext());
    }
    std::string stage;
    auto *probe = new qml_access::Probe(&app, [&](const char *s, bool a, bool e, bool h, bool c) {
        ++receipts;
        stage = s;
        fprintf(stderr, "stage=%s app=%d engine=%d helper=%d controller=%d\n", s, a, e, h, c);
        assert(a);
        if (mode == "live" || mode == "late" || mode == "existing" || mode == "major-version") assert(e && h && c);
        if (mode == "conflict") assert(!c);
        QTimer::singleShot(0, &app, &QCoreApplication::quit);
    }, mode == "nested-deadline" ? 1000 : mode == "deadline" || mode == "cap" ? 150 : 3000);
    guard = probe;
    if (mode == "engine-lost") QMetaObject::invokeMethod(&app, [&] { other.reset(); }, Qt::QueuedConnection);
    if (mode == "late") QTimer::singleShot(10, &app, [&] {
        QQmlEngine::setContextForObject(&window, engine.rootContext());
        window.show();
    });
    if (mode == "cap") {
        auto *events = new QTimer(&app);
        QObject::connect(events, &QTimer::timeout, &app, [&] {
            QEvent show(QEvent::Show);
            QCoreApplication::sendEvent(&window, &show);
        });
        events->start(5);
    }
    if (mode == "cancel") QTimer::singleShot(10, &app, &QCoreApplication::quit);
    QTimer::singleShot(6000, &app, [&] { assert(false && "fixture timeout"); });
    assert(app.exec() == 0);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    assert(!guard);
    assert(deaths == 0); // Helper cleanup never owns/deletes the singleton.
    if (mode == "cancel") assert(receipts == 0);
    else {
        assert(receipts == 1);
        if (mode == "live" || mode == "late" || mode == "existing" || mode == "major-version") assert(stage == "resolved");
        else if (mode == "absent") assert(stage == "module-missing");
        else if (mode == "conflict") assert(stage == "unavailable" || stage == "create-error");
        else if (mode == "multiple") assert(stage == "multiple-engines");
        else if (mode == "window-cap") assert(stage == "window-cap");
        else if (mode == "engine-lost") assert(stage == "engine-lost");
        else if (mode == "cap") assert(stage == "readiness-cap");
        else if (mode == "deadline" || mode == "nested-deadline") assert(stage == "deadline");
        else assert(false && "unknown case");
    }
    if (mode == "nested-deadline") assert(nestedEntered);
    printf("%s: %s, receipts=%d, owned teardown passed\n", argv[1], stage.c_str(), receipts);
}
