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
        qmlRegisterSingletonInstance("xofm.libs.library", 1, 0, "DocumentController", &singleton);
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
        if (mode == "live" || mode == "late" || mode == "existing") assert(e && h && c);
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
        if (mode == "live" || mode == "late" || mode == "existing") assert(stage == "resolved");
        else if (mode == "absent") assert(stage == "component-error");
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
