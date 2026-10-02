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
    const bool retry = mode.rfind("retry-", 0) == 0;
    QGuiApplication app(argc, argv);
    if (mode == "diagnostics") {
        auto error = [](const QString &text) { QQmlError e; e.setDescription(text); e.setLine(7); e.setColumn(9); e.setUrl(QUrl("file:///fixture/hidden.qml")); return e; };
        const QString actual = QStringLiteral("module \"QtQml\" is not installed");
        auto snapshot = [&](const QList<QQmlError> &errors) {
            const auto bytes = qml_access::diagnosticSnapshot(errors);
            assert(!bytes.isEmpty() && bytes.size() <= 8192);
            const auto doc = QJsonDocument::fromJson(bytes);
            assert(doc.isObject());
            return doc.object();
        };
        auto root = snapshot({error(actual)});
        auto item = root.value("errors").toArray().first().toObject();
        assert(item.value("description").toString() == actual && !item.contains("url"));
        assert(item.value("line").toInt() == 7 && item.value("column").toInt() == 9);
        for (const QString &path : {QStringLiteral("/home/person/item"), QStringLiteral("/ROOT/item"), QStringLiteral("/Users/person/item"), QStringLiteral("\\Users\\person\\item"), QStringLiteral("C:\\private\\item")}) {
            item = snapshot({error(path)}).value("errors").toArray().first().toObject();
            assert(item.value("description").toString() == QStringLiteral("[redacted personal path]") && item.value("description_redacted").toBool());
        }
        const QString system = QStringLiteral("Cannot load plugin /usr/lib/fixture.so");
        item = snapshot({error(system)}).value("errors").toArray().first().toObject();
        assert(item.value("description").toString() == system && !item.value("description_redacted").toBool());
        root = snapshot(QList<QQmlError>(9, error(QString(257, QChar('x')))));
        assert(root.value("reported_error_count").toInt() == 9 && root.value("retained_error_count").toInt() == 8 && root.value("count_truncated").toBool());
        item = root.value("errors").toArray().first().toObject();
        assert(item.value("description").toString().size() == 256 && item.value("description_truncated").toBool());
        root = snapshot(QList<QQmlError>(8, error(QString(256, QChar(1)))));
        assert(root.value("output_overflow").toBool() && root.value("errors").toArray().isEmpty() && root.value("retained_error_count").toInt() == 0);
        puts("diagnostics: bounded actual text, redaction, URL omission and serialization overflow passed");
        return 0;
    }
    if (mode == "classifier") {
        auto error = [](const QString &description) { QQmlError e; e.setDescription(description); return e; };
        const auto missing = error(QStringLiteral("module \"fixture.absent\" is not installed"));
        assert(std::string(qml_access::errorStage({missing})) == "module-missing");
        const std::pair<const char *, const char *> knownModules[] = {
            {"QtQml", "missing-qtqml"}, {"QML", "missing-qml"},
            {"QtQml.Models", "missing-models"}, {"QtQml.WorkerScript", "missing-worker"},
            {"xofm.libs.library", "missing-library"}
        };
        for (const auto &known : knownModules) {
            const auto knownError = error(QStringLiteral("module \"") + QString::fromLatin1(known.first) + QStringLiteral("\" is not installed"));
            assert(std::string(qml_access::errorStage({knownError})) == known.second);
            assert(std::string(qml_access::errorStage({knownError, knownError})) == known.second);
            assert(std::string(qml_access::errorStage({knownError, missing})) == "component-error");
        }
        assert(std::string(qml_access::errorStage({error(QStringLiteral("module \"qtqml\" is not installed"))})) == "module-missing");
        assert(std::string(qml_access::errorStage({error(QStringLiteral("module \"QtQml\" version 1.0 is not installed"))})) == "module-version");
        assert(std::string(qml_access::errorStage({error(QStringLiteral("module \"QtQml\" is not installed")), error(QStringLiteral("module \"QML\" is not installed"))})) == "component-error");
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
    } else if (mode != "absent" && !retry)
        qmlRegisterSingletonInstance("xofm.libs.library", mode == "major-version" ? 2 : 1, 0, "DocumentController", &singleton);
    QQmlEngine engine;
    if (mode == "wrong-error") engine.setImportPathList({});
    QWindow window;
    std::unique_ptr<QQmlContext> fixtureContext;
    if (mode == "retry-changed") fixtureContext = std::make_unique<QQmlContext>(&engine);
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
        QQmlEngine::setContextForObject(&window, fixtureContext ? fixtureContext.get() : engine.rootContext());
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
    auto *probe = new qml_access::Probe(&app, [&](const char *s, bool a, bool e, bool h, bool c, const QByteArray &diagnostic) {
        ++receipts;
        stage = s;
        fprintf(stderr, "stage=%s app=%d engine=%d helper=%d controller=%d\n", s, a, e, h, c);
        assert(a);
        assert(!diagnostic.isEmpty() && diagnostic.size() <= 8192);
        const auto summary = QJsonDocument::fromJson(diagnostic).object();
        assert(summary.value("terminal_stage").toString() == QString::fromLatin1(s));
        const int attempts = summary.value("attempts").toInt();
        const int compiles = summary.value("compile_attempts").toInt();
        const int admittedEvents = summary.value("admitted_post_failure_events").toInt();
        assert(attempts >= 0 && attempts <= 8 && compiles >= 0 && compiles <= attempts && admittedEvents >= 0 && admittedEvents <= 8);
        const qint64 terminalElapsed = summary.value("elapsed_ms").toInteger();
        assert(terminalElapsed >= 0);
        if (mode == "retry-late") {
            assert(attempts == 2 && compiles == 2 && admittedEvents == 1);
            assert(summary.value("component_status").toString() == QStringLiteral("ready"));
        }
        if (mode == "retry-no-event" || mode == "retry-stale" || mode == "absent") {
            assert(attempts == 1 && compiles == 1 && admittedEvents == 0 && terminalElapsed >= 2000);
            assert(summary.value("component_status").toString() == QStringLiteral("absent"));
        }
        if (mode == "retry-cap") assert(attempts == 8 && compiles == 8 && admittedEvents == 8);
        if (mode == "nested-deadline") {
            assert(attempts == 1 && compiles == 1 && admittedEvents == 0 && terminalElapsed >= 1000);
            assert(summary.value("component_status").toString() == QStringLiteral("ready"));
        }
        if (mode == "absent" || retry) {
            assert(!diagnostic.isEmpty() && diagnostic.size() <= 8192);
            const auto parsed = QJsonDocument::fromJson(diagnostic).object();
            assert(parsed.value("context").toString() == QStringLiteral("last-compile-failure"));
            assert(parsed.value("reported_error_count").toInt() > 0);
            assert(parsed.value("errors").toArray().first().toObject().value("description").toString() == QStringLiteral("module \"xofm.libs.library\" is not installed"));
            assert(parsed.value("failed_attempt").toInt() == (mode == "retry-cap" ? 8 : 1));
        } else if (mode == "wrong-error") assert(summary.value("context").toString() == QStringLiteral("last-compile-failure"));
        else {
            assert(summary.value("context").toString() == QStringLiteral("runtime-only"));
            assert(summary.value("failed_attempt").toInt() == 0 && summary.value("errors").toArray().isEmpty());
        }
        if (mode == "live" || mode == "late" || mode == "existing" || mode == "major-version" || mode == "retry-late") assert(e && h && c);
        if (mode == "conflict") assert(!c);
        QTimer::singleShot(0, &app, &QCoreApplication::quit);
    }, mode == "nested-deadline" ? 1000 : mode == "deadline" || mode == "cap" ? 150 : retry || mode == "absent" ? 2000 : 3000);
    guard = probe;
    int failedCleanups = 0;
    std::function<void()> observeComponent;
    observeComponent = [&] {
        if (!guard) return;
        auto *component = guard->findChild<QQmlComponent *>();
        if (!component) { assert(receipts == 1); return; }
        QObject::connect(component, &QObject::destroyed, &app, [&] {
            ++failedCleanups;
            if (mode == "retry-stale") {
                QEvent stale(QEvent::Show);
                QCoreApplication::sendEvent(&window, &stale); // During cleanup: ignored.
            }
            QMetaObject::invokeMethod(&app, [&] {
                if (mode == "retry-late" || mode == "retry-no-event" || mode == "retry-stale")
                    qmlRegisterSingletonInstance("xofm.libs.library", 1, 0, "DocumentController", &singleton);
                if (mode == "retry-changed" || mode == "retry-ambiguous") {
                    if (mode == "retry-changed") fixtureContext.reset();
                    other = std::make_unique<QQmlEngine>();
                    otherWindow = std::make_unique<QWindow>();
                    QQmlEngine::setContextForObject(otherWindow.get(), other->rootContext());
                }
                if (mode != "retry-no-event" && mode != "retry-stale") {
                    QEvent fresh(QEvent::Show);
                    QCoreApplication::sendEvent(otherWindow ? otherWindow.get() : &window, &fresh);
                    if (mode == "retry-cap") QMetaObject::invokeMethod(&app, observeComponent, Qt::QueuedConnection);
                }
            }, Qt::QueuedConnection);
        });
        if (mode == "retry-stale") {
            QEvent early(QEvent::Show);
            QCoreApplication::sendEvent(&window, &early); // Component not yet refused.
        }
    };
    if (retry) QMetaObject::invokeMethod(&app, observeComponent, Qt::QueuedConnection);
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
        if (mode == "live" || mode == "late" || mode == "existing" || mode == "major-version" || mode == "retry-late") assert(stage == "resolved");
        else if (mode == "absent" || mode == "retry-no-event" || mode == "retry-stale") assert(stage == "deadline");
        else if (mode == "retry-changed") assert(stage == "engine-changed");
        else if (mode == "retry-ambiguous") assert(stage == "multiple-engines");
        else if (mode == "retry-cap") assert(stage == "readiness-cap");
        else if (mode == "wrong-error") assert(stage == "missing-qtqml" || stage == "component-error");
        else if (mode == "conflict") assert(stage == "unavailable" || stage == "create-error");
        else if (mode == "multiple") assert(stage == "multiple-engines");
        else if (mode == "window-cap") assert(stage == "window-cap");
        else if (mode == "engine-lost") assert(stage == "engine-lost");
        else if (mode == "cap") assert(stage == "readiness-cap");
        else if (mode == "deadline" || mode == "nested-deadline") assert(stage == "deadline");
        else assert(false && "unknown case");
    }
    if (mode == "nested-deadline") assert(nestedEntered);
    if (retry) assert(failedCleanups == (mode == "retry-cap" ? 8 : 1));
    printf("%s: %s, receipts=%d, owned teardown passed\n", argv[1], stage.c_str(), receipts);
}
