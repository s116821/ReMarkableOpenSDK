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
        bool mergeOverflowFound = false;
        for (int controls = 0; controls <= 256; ++controls) {
            const auto prior = qml_access::diagnosticSnapshot(QList<QQmlError>(8, error(QString(controls, QChar(1)) + QString(256-controls, QChar('x')))), 1);
            if (QJsonDocument::fromJson(prior).object().value("output_overflow").toBool()) continue;
            const auto merged = qml_access::runtimeSnapshot(prior, 2, 2, 1, "ready", "resolved", 1234);
            assert(merged.size() <= 8192);
            const auto summary = QJsonDocument::fromJson(merged).object();
            if (summary.value("output_overflow").toBool()) {
                assert(summary.value("attempts").toInt() == 2 && summary.value("compile_attempts").toInt() == 2 && summary.value("admitted_post_failure_events").toInt() == 1);
                assert(summary.value("terminal_stage").toString() == QStringLiteral("resolved") && summary.value("failed_attempt").toInt() == 1 && summary.value("errors").toArray().isEmpty());
                mergeOverflowFound = true;
                break;
            }
        }
        assert(mergeOverflowFound); // Prior compiler JSON fit; runtime metadata crossed cap.
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
    if (mode.rfind("phase-", 0) == 0) {
        const int readinessBudget = mode == "phase-late-ready" ? 500 : 2000;
        const int accessBudget = mode == "phase-boundary" ? 1500 : 500;
        QObject singleton;
        QPointer<qml_access::Probe> guard;
        int receipts = 0;
        int creates = 0;
        if (mode == "phase-boundary") qmlRegisterSingletonType<QObject>("xofm.libs.library", 1, 0, "DocumentController",
            [&](QQmlEngine *, QJSEngine *) -> QObject * {
                ++creates;
                QEventLoop nested;
                QTimer::singleShot(900, &nested, &QEventLoop::quit);
                nested.exec();
                assert(guard && receipts == 0);
                QQmlEngine::setObjectOwnership(&singleton, QQmlEngine::CppOwnership);
                return &singleton;
            });
        else if (mode == "phase-ready-changed" || mode == "phase-ready-ambiguous")
            qmlRegisterSingletonType<QObject>("xofm.libs.library", 1, 0, "DocumentController",
                [&](QQmlEngine *, QJSEngine *) -> QObject * {
                    ++creates; QQmlEngine::setObjectOwnership(&singleton, QQmlEngine::CppOwnership); return &singleton;
                });
        else qmlRegisterSingletonInstance("xofm.libs.library", 1, 0, "DocumentController", &singleton);
        QQmlApplicationEngine engine;
        engine.loadData("import QtQml\nQtObject {}", QUrl());
        QWindow window;
        std::unique_ptr<QQmlContext> initialContext;
        if (mode == "phase-ready-changed") initialContext = std::make_unique<QQmlContext>(&engine);
        QQmlEngine::setContextForObject(&window, initialContext ? initialContext.get() : engine.rootContext());
        QQmlEngine otherEngine;
        std::unique_ptr<QWindow> otherWindow;
        QJsonObject summary;
        std::string stage;
        guard = new qml_access::Probe(&app,
            [&](const char *s, bool a, bool e, bool h, bool c, const QByteArray &bytes) {
                ++receipts; stage = s; summary = QJsonDocument::fromJson(bytes).object();
                fprintf(stderr, "phase-stage=%s root_ms=%lld ready_ms=%lld elapsed_ms=%lld compiles=%d rechecks=%d\n", s,
                    static_cast<long long>(summary.value("root_witness_at_ms").toInteger(-1)),
                    static_cast<long long>(summary.value("component_ready_at_ms").toInteger(-1)),
                    static_cast<long long>(summary.value("elapsed_ms").toInteger(-1)),
                    summary.value("compile_attempts").toInt(), summary.value("timer_rechecks_admitted").toInt());
                assert(a && e && bytes.size() <= 8192);
                assert(summary.value("access_anchor").toString() == QStringLiteral("component-ready-observation"));
                assert(summary.value("compile_attempts").toInt() == 1 && summary.value("timer_rechecks_admitted").toInt() == 0);
                if (mode == "phase-late-ready") assert(!h && !c && summary.value("component_ready_at_ms").isNull());
                else if (mode == "phase-ready-changed" || mode == "phase-ready-ambiguous")
                    assert(!h && !c && creates == 0 && !summary.value("component_ready_at_ms").isNull());
                else {
                    assert(h && c);
                    const auto ready = summary.value("component_ready_at_ms").toInteger();
                    const auto total = summary.value("elapsed_ms").toInteger();
                    assert(ready < readinessBudget && ready >= summary.value("root_witness_at_ms").toInteger() + 500);
                    assert(total < ready + accessBudget);
                    if (mode == "phase-boundary") assert(total >= readinessBudget && creates == 1);
                }
                QTimer::singleShot(0, &app, &QCoreApplication::quit);
            }, readinessBudget, accessBudget);
        // The first queued callback follows acquisition; its child callback
        // follows root continuation but precedes queued Ready processing.
        QMetaObject::invokeMethod(&app, [&] {
            QMetaObject::invokeMethod(&app, [&] {
                auto *component = guard ? guard->findChild<QQmlComponent *>() : nullptr;
                assert(component && component->isReady());
                if (mode == "phase-ready-changed" || mode == "phase-ready-ambiguous") {
                    if (mode == "phase-ready-changed") initialContext.reset();
                    otherWindow = std::make_unique<QWindow>();
                    QQmlEngine::setContextForObject(otherWindow.get(), otherEngine.rootContext());
                    return;
                }
                QThread::msleep(mode == "phase-boundary" ? 1300 : 600);
            }, Qt::QueuedConnection);
        }, Qt::QueuedConnection);
        QTimer::singleShot(8000, &app, [&] { assert(false && "phase fixture timeout"); });
        assert(app.exec() == 0);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        assert(!guard && receipts == 1);
        assert(stage == (mode == "phase-late-ready" ? "readiness-deadline" :
            mode == "phase-ready-changed" ? "engine-changed" : mode == "phase-ready-ambiguous" ? "multiple-engines" : "resolved"));
        printf("%s: %s, observed Ready phase passed\n", argv[1], stage.c_str());
        return 0;
    }
    if (mode.rfind("root-", 0) == 0) {
        const int readinessBudget = mode == "root-queued-readiness-deadline" ? 100 : mode == "root-wait" || mode == "root-late-witness" ? 200 : 3000;
        const int accessBudget = mode == "root-wait" || mode == "root-late-witness" || mode == "root-queued-readiness-deadline" ? 100 : 3000;
        QObject singleton;
        qmlRegisterSingletonInstance("xofm.libs.library", 1, 0, "DocumentController", &singleton);
        auto engine = std::make_unique<QQmlApplicationEngine>();
        QQmlEngine unsupported;
        QWindow window;
        std::unique_ptr<QWindow> extra;
        std::unique_ptr<QQmlContext> initialContext;
        if (mode == "root-changed") initialContext = std::make_unique<QQmlContext>(engine.get());
        QQmlEngine::setContextForObject(&window, mode == "root-unsupported" ? unsupported.rootContext() : initialContext ? initialContext.get() : engine->rootContext());
        const auto loadRoot = [&] { engine->loadData("import QtQml\nQtObject {}", QUrl()); };
        if (mode == "root-queued-readiness-deadline") { loadRoot(); delete engine->rootObjects().first(); }
        if (mode == "root-preexisting") loadRoot();
        if (mode == "root-cap") for (int i = 0; i < 17; ++i) loadRoot();
        // These handlers run before the probe's signal handler, but defer the
        // destructive/blocking fixture action until after it captures witness.
        if (mode == "root-lost") QObject::connect(engine.get(), &QQmlApplicationEngine::objectCreated, &app,
            [&](QObject *root, const QUrl &) {
                const QPointer<QObject> weak = root;
                QMetaObject::invokeMethod(&app, [weak] { delete weak.data(); }, Qt::QueuedConnection);
            });
        if (mode == "root-queued-readiness-deadline") QObject::connect(engine.get(), &QQmlApplicationEngine::objectCreated, &app,
            [&](QObject *, const QUrl &) {
                QMetaObject::invokeMethod(&app, [] { QThread::msleep(150); }, Qt::QueuedConnection);
            });
        if (mode == "root-late-witness") QObject::connect(engine.get(), &QQmlApplicationEngine::objectCreated, &app,
            [](QObject *, const QUrl &) { QThread::msleep(250); });
        if (mode == "root-cancel-queued") QObject::connect(engine.get(), &QQmlApplicationEngine::objectCreated, &app,
            [&](QObject *, const QUrl &) { QMetaObject::invokeMethod(&app, &QCoreApplication::quit, Qt::QueuedConnection); });
        QJsonObject summary;
        std::string stage;
        int receipts = 0;
        QPointer<qml_access::Probe> guard = new qml_access::Probe(&app,
            [&](const char *s, bool a, bool e, bool h, bool c, const QByteArray &bytes) {
                ++receipts; stage = s;
                summary = QJsonDocument::fromJson(bytes).object();
                fprintf(stderr, "root-stage=%s attempts=%d compiles=%d roots=%d witness_ms=%lld elapsed_ms=%lld begin_ms=%lld return_ms=%lld\n",
                    s, summary.value("attempts").toInt(), summary.value("compile_attempts").toInt(), summary.value("root_count").toInt(),
                    static_cast<long long>(summary.value("root_witness_at_ms").toInteger(-1)),
                    static_cast<long long>(summary.value("elapsed_ms").toInteger(-1)),
                    static_cast<long long>(summary.value("first_setData_begin_ms").toInteger(-1)),
                    static_cast<long long>(summary.value("first_setData_return_ms").toInteger(-1)));
                assert(a && e && bytes.size() <= 8192);
                assert(summary.value("terminal_stage").toString() == QString::fromLatin1(s));
                if (stage == "resolved") assert(h && c);
                else assert(!h && !c && summary.value("compile_attempts").toInt() == 0);
                QTimer::singleShot(0, &app, &QCoreApplication::quit);
            }, readinessBudget, accessBudget);
        if (mode != "root-preexisting" && mode != "root-cap" && mode != "root-unsupported")
            QTimer::singleShot(20, &app, [&] {
                assert(guard && !guard->findChild<QQmlComponent *>() && receipts == 0);
                // Window activity while waiting cannot authorize compilation.
                QEvent event(QEvent::Show); QCoreApplication::sendEvent(&window, &event);
                if (mode == "root-wait") return;
                if (mode == "root-cancel") { app.quit(); return; }
                if (mode == "root-engine-lost") { engine.reset(); return; }
                if (mode == "root-wrong-thread") {
                    auto *worker = QThread::create([raw = engine.get()] { raw->objectCreated(nullptr, QUrl()); });
                    worker->start(); worker->wait(); delete worker;
                    return;
                }
                if (mode == "root-failed") engine->loadData("import QtQml\nMissingFixtureRoot {}", QUrl());
                else {
                    if (mode == "root-changed" || mode == "root-ambiguous") {
                        if (mode == "root-changed") initialContext.reset();
                        extra = std::make_unique<QWindow>(); QQmlEngine::setContextForObject(extra.get(), unsupported.rootContext());
                    }
                    loadRoot();
                    if (mode == "root-duplicate") loadRoot();
                    if (mode == "root-stale-failure") engine->loadData("import QtQml\nMissingFixtureRoot {}", QUrl());
                }
            });
        QTimer::singleShot(5000, &app, [&] { assert(false && "root fixture timeout"); });
        assert(app.exec() == 0);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        assert(!guard);
        if (mode == "root-cancel" || mode == "root-cancel-queued") assert(receipts == 0);
        else {
            assert(receipts == 1 && summary.value("attempts").toInt() == 1);
            assert(summary.value("admitted_post_failure_events").toInt() == 0);
            assert(summary.value("readiness_budget_ms").toInt() == readinessBudget && summary.value("access_budget_ms").toInt() == accessBudget);
            if (mode == "root-preexisting" || mode == "root-signal" || mode == "root-duplicate" || mode == "root-stale-failure") {
                assert(stage == "resolved" && summary.value("compile_attempts").toInt() == 1);
                assert(summary.value("root_gate").toString() == QStringLiteral("consumed"));
                assert(summary.value("root_witness_kind").toString() == (mode == "root-preexisting" ? QStringLiteral("preexisting") : QStringLiteral("signal")));
                const auto witness = summary.value("root_witness_at_ms").toInteger();
                assert(witness >= 0 && witness < readinessBudget);
                assert(summary.value("first_setData_begin_ms").toInteger() >= witness);
                assert(summary.value("first_setData_return_ms").toInteger() >= summary.value("first_setData_begin_ms").toInteger());
                assert(summary.value("first_error_handled_ms").isNull());
            } else {
                assert(summary.value("first_setData_begin_ms").isNull() && summary.value("first_setData_return_ms").isNull());
                if (mode == "root-wait") assert(stage == "root-readiness-deadline" && summary.value("root_witness_at_ms").isNull());
                else if (mode == "root-failed") assert(stage == "root-load-failed" && summary.value("root_witness_at_ms").isNull());
                else if (mode == "root-cap") assert(stage == "root-cap" && summary.value("root_count").toInt() == 17);
                else if (mode == "root-unsupported") assert(stage == "unsupported-engine" && summary.value("gate_engine_kind").toString() == QStringLiteral("unsupported"));
                else if (mode == "root-lost") assert(stage == "root-witness-lost");
                else if (mode == "root-engine-lost") assert(stage == "engine-lost");
                else if (mode == "root-changed") assert(stage == "engine-changed");
                else if (mode == "root-ambiguous") assert(stage == "multiple-engines");
                else if (mode == "root-wrong-thread") assert(stage == "engine-thread" && summary.value("root_witness_at_ms").isNull());
                else if (mode == "root-late-witness") assert(stage == "root-readiness-deadline" && summary.value("root_witness_at_ms").isNull());
                else if (mode == "root-queued-readiness-deadline") {
                    assert(stage == "readiness-deadline" && summary.value("root_witness_at_ms").toInteger() < readinessBudget);
                    assert(summary.value("component_ready_at_ms").isNull() && summary.value("elapsed_ms").toInteger() >= readinessBudget);
                } else assert(false && "unknown root case");
            }
        }
        printf("%s: %s, receipts=%d, root gate passed\n", argv[1], stage.c_str(), receipts);
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
    QQmlApplicationEngine engine;
    engine.loadData("import QtQml\nQtObject {}", QUrl());
    assert(engine.rootObjects().size() == 1);
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
    if (mode != "deadline" && mode != "late" && mode != "cap" && mode != "cancel" && mode != "engine-lost" && mode != "retry-engine-lost")
        QQmlEngine::setContextForObject(&window, fixtureContext ? fixtureContext.get() : engine.rootContext());
    if (mode == "multiple") {
        other = std::make_unique<QQmlEngine>();
        otherWindow = std::make_unique<QWindow>();
        QQmlEngine::setContextForObject(otherWindow.get(), other->rootContext());
    }
    if (mode == "engine-lost" || mode == "retry-engine-lost") {
        auto application = std::make_unique<QQmlApplicationEngine>();
        application->loadData("import QtQml\nQtObject {}", QUrl());
        other = std::move(application);
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
        const int rechecks = summary.value("timer_rechecks_admitted").toInt();
        fprintf(stderr, "attempts=%d compiles=%d rechecks=%d ready_ms=%lld elapsed_ms=%lld\n", attempts, compiles, rechecks,
            static_cast<long long>(summary.value("component_ready_at_ms").toInteger(-1)),
            static_cast<long long>(summary.value("elapsed_ms").toInteger(-1)));
        assert(attempts >= 0 && attempts <= 8 && compiles >= 0 && compiles <= attempts && admittedEvents == 0 && rechecks >= 0 && rechecks <= 7);
        assert(summary.value("access_anchor").toString() == QStringLiteral("component-ready-observation"));
        assert(summary.value("retry_interval_ms").toInt() == 200);
        if (compiles) {
            assert(summary.value("last_setData_begin_ms").toInteger() >= summary.value("first_setData_begin_ms").toInteger());
            assert(summary.value("last_setData_return_ms").toInteger() >= summary.value("last_setData_begin_ms").toInteger());
            if (compiles == 1) assert(summary.value("last_setData_begin_ms") == summary.value("first_setData_begin_ms") &&
                summary.value("last_setData_return_ms") == summary.value("first_setData_return_ms"));
        } else assert(summary.value("last_setData_begin_ms").isNull() && summary.value("last_setData_return_ms").isNull());
        const qint64 terminalElapsed = summary.value("elapsed_ms").toInteger();
        assert(terminalElapsed >= 0);
        if (mode == "retry-late" || mode == "retry-no-event" || mode == "retry-stale" || mode == "retry-delayed") {
            assert(attempts == 2 && compiles == 2 && rechecks == 1);
            assert(summary.value("component_status").toString() == QStringLiteral("ready"));
            assert(summary.value("component_ready_at_ms").toInteger() >= summary.value("first_error_handled_ms").toInteger() + 200);
            if (mode == "retry-delayed") assert(summary.value("component_ready_at_ms").toInteger() >= summary.value("root_witness_at_ms").toInteger() + 500);
        }
        if (mode == "absent") {
            assert(compiles >= 1 && compiles < 8 && rechecks == compiles - 1 && terminalElapsed >= 500);
            assert(summary.value("component_status").toString() == QStringLiteral("absent"));
            assert(summary.value("component_ready_at_ms").isNull());
        }
        if (mode == "retry-cap") assert(attempts == 8 && compiles == 8 && rechecks == 7);
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
            assert(parsed.value("failed_attempt").toInt() == (mode == "retry-cap" || mode == "absent" ? compiles : 1));
        } else if (mode == "wrong-error") assert(summary.value("context").toString() == QStringLiteral("last-compile-failure"));
        else {
            assert(summary.value("context").toString() == QStringLiteral("runtime-only"));
            assert(summary.value("failed_attempt").toInt() == 0 && summary.value("errors").toArray().isEmpty());
        }
        if (mode == "live" || mode == "late" || mode == "existing" || mode == "major-version" || mode == "retry-late" || mode == "retry-no-event" || mode == "retry-stale" || mode == "retry-delayed") assert(e && h && c);
        if (mode == "conflict") assert(!c);
        QTimer::singleShot(0, &app, &QCoreApplication::quit);
    }, mode == "deadline" || mode == "cap" ? 150 : mode == "absent" ? 500 : 3000,
       mode == "nested-deadline" ? 1000 : mode == "retry-delayed" ? 500 : 3000);
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
                if (mode == "retry-delayed") QThread::msleep(600);
                if (mode == "retry-late" || mode == "retry-no-event" || mode == "retry-stale" || mode == "retry-delayed")
                    qmlRegisterSingletonInstance("xofm.libs.library", 1, 0, "DocumentController", &singleton);
                if (mode == "retry-cancel") {
                    assert(QMetaObject::invokeMethod(&app, "aboutToQuit", Qt::DirectConnection));
                    QEventLoop staleTimer;
                    QTimer::singleShot(250, &staleTimer, &QEventLoop::quit);
                    staleTimer.exec();
                    assert(receipts == 0); // Stale retry cannot emit after cancellation.
                    if (guard) assert(!guard->findChild<QQmlComponent *>());
                    app.quit();
                    return;
                }
                if (mode == "retry-engine-lost") { other.reset(); return; }
                if (mode == "retry-changed" || mode == "retry-ambiguous") {
                    if (mode == "retry-changed") fixtureContext.reset();
                    other = std::make_unique<QQmlEngine>();
                    otherWindow = std::make_unique<QWindow>();
                    QQmlEngine::setContextForObject(otherWindow.get(), other->rootContext());
                }
                if (mode != "retry-no-event" && mode != "retry-stale" && mode != "retry-delayed") {
                    QEvent fresh(QEvent::Show);
                    QCoreApplication::sendEvent(otherWindow ? otherWindow.get() : &window, &fresh);
                }
            }, Qt::QueuedConnection);
        });
        if (mode == "retry-stale") {
            QEvent early(QEvent::Show);
            QCoreApplication::sendEvent(&window, &early); // Component not yet refused.
        }
    };
    if (retry) QMetaObject::invokeMethod(&app, [&] {
        QMetaObject::invokeMethod(&app, observeComponent, Qt::QueuedConnection);
    }, Qt::QueuedConnection);
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
    if (mode == "cancel" || mode == "retry-cancel") assert(receipts == 0);
    else {
        assert(receipts == 1);
        if (mode == "live" || mode == "late" || mode == "existing" || mode == "major-version" || mode == "retry-late" || mode == "retry-no-event" || mode == "retry-stale" || mode == "retry-delayed") assert(stage == "resolved");
        else if (mode == "absent") assert(stage == "readiness-deadline");
        else if (mode == "retry-changed") assert(stage == "engine-changed");
        else if (mode == "retry-ambiguous") assert(stage == "multiple-engines");
        else if (mode == "retry-cap") assert(stage == "readiness-cap");
        // The owned preloaded root may already cache QtQml before paths clear.
        else if (mode == "wrong-error") assert(stage == "missing-qtqml" || stage == "type-missing" || stage == "component-error");
        else if (mode == "conflict") assert(stage == "unavailable" || stage == "create-error");
        else if (mode == "multiple") assert(stage == "multiple-engines");
        else if (mode == "window-cap") assert(stage == "window-cap");
        else if (mode == "engine-lost") assert(stage == "engine-lost");
        else if (mode == "retry-engine-lost") assert(stage == "engine-lost");
        else if (mode == "cap") assert(stage == "readiness-cap");
        else if (mode == "deadline") assert(stage == "root-readiness-deadline");
        else if (mode == "nested-deadline") assert(stage == "deadline");
        else assert(false && "unknown case");
    }
    if (mode == "nested-deadline") assert(nestedEntered);
    if (retry) assert(failedCleanups == 1); // First component observed; counters cover all rechecks.
    printf("%s: %s, receipts=%d, owned teardown passed\n", argv[1], stage.c_str(), receipts);
}
