// Owned Entry/Reader/worker model; no native ABI calls or tablet resources.
#define READER_ENTRY_OWNED_HELPERS_ONLY
#include "reader_selected_view_entry_fixture.cpp"
#include <QSemaphore>
#include <stdexcept>

class DocumentWorker final : public QThread {
    Q_OBJECT
public:
    std::function<void()> callback;
    qulonglong job = 18446744073709551614ULL;
    bool hold = false, invoke = true, complete = true, inlineGui = false;
    QSemaphore gate;
    ~DocumentWorker() override { gate.release(); wait(); }
    qulonglong submit(const std::function<void()> &value) {
        callback = value;
        if (inlineGui) { if (invoke) callback(); if (complete) emit jobCompleted(job); }
        else start();
        return job;
    }
    void run() override {
        if (hold) gate.acquire();
        if (invoke) callback();
        if (complete) emit jobCompleted(job); // after the copied callback returns
    }
signals:
    void jobCompleted(qulonglong);
};

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    qmlRegisterType<QObject>("OwnedProbe", 1, 0, "QtObject");
    qmlRegisterType<QQuickItem>("OwnedProbe", 1, 0, "Item");
    qmlRegisterType<FocusFixture>("OwnedProbe", 1, 0, "FocusScope");
    qmlRegisterType<SceneView>("OwnedProbe", 1, 0, "SceneFixture");
    qmlRegisterType<OwnershipOwner>("OwnedLegacyEntry", 1, 0, "Owner");
    registerCreationFixture();
    const QByteArray nonce(32, 'b'), identity = reader_selected::selfIdentity();
    for (const QByteArray &mode : {QByteArray("valid"), QByteArray("inline"), QByteArray("skipped"), QByteArray("zero"),
            QByteArray("before-cancel"), QByteArray("cancel-submit"), QByteArray("restore-submit"),
            QByteArray("expire-submit"), QByteArray("replace-submit"), QByteArray("budget-submit"),
            QByteArray("throw-submit"), QByteArray("wrong-target"), QByteArray("worker-missing"), QByteArray("postlink-cancel")}) {
        Fixture f;
        Library library; DocumentController creation; creation.setLibrary(&library); library.result = &f.document;
        f.engine.rootContext()->setContextProperty("externalCreation", &creation);
        DocumentWorker worker;
        f.controller.workerValue = mode == "worker-missing" ? nullptr : &worker;
        worker.inlineGui = mode == "inline";
        worker.invoke = mode != "skipped";
        worker.hold = mode.endsWith("submit") && mode != "budget-submit";
        if (mode == "zero") worker.job = 0;
        xofm::bindings::qt::WindowNavigatorWrapper navigator;
        xofm::bindings::qt::WindowLayout layout; layout.activeValue = QStringLiteral("legacydevice/window/main");
        f.engine.rootContext()->setContextProperty("entryNavigator", &navigator);
        f.engine.rootContext()->setContextProperty("entryLayout", &layout);
        QQmlComponent composition(&f.engine);
        composition.setData(R"QML(
import OwnedLegacyEntry 1.0
Owner {
    id: root; width: 100; height: 100
    windowNavigator: entryNavigator; windowLayout: entryLayout; contentItem: mainView
    Owner { id: mainView; width: 100; height: 100; windowNavigator: root.windowNavigator; windowLayout: root.windowLayout }
}
)QML", QUrl(QStringLiteral("qrc:/qt/qml/xofm/modules/legacydevice/qml/LegacyMainWindow.qml")));
        require(composition.isReady(), "worker Entry composition compiles");
        std::unique_ptr<QObject> root(composition.create());
        auto *visual = qobject_cast<QQuickItem *>(root.get()); require(visual, "worker Entry composition creates");
        visual->setParentItem(f.window.contentItem()); visual->setParent(f.window.contentItem());
        f.reader->setVisible(false);
        QTemporaryDir base; require(base.isValid(), "worker Entry temporary base");
        const QString path = base.path() + "/root";
        require(QDir().mkdir(path) && ::chmod(path.toLocal8Bit().constData(), 0700) == 0, "worker Entry private root");
        putEntry(path + "/owner", nonce); putEntry(path + "/attempt.identity", identity);
        reader_selected::Entry entry(&app, path, nonce, nullptr,
            reader_selected::Entry::Diagnostic::NavigateCreationWorkerCallback, {f.document.key, 1, true});
        int submissions = 0;
        entry.workerSubmitForTest([&](QObject *source, const std::function<void()> &callback) {
            ++submissions;
            require(source == &worker && entry.samplingForTest(), "qualified source submits only on admitted sample stack");
            require(readEntry(path + "/selected-result").isEmpty(), "no worker result on submit stack");
            const auto job = worker.submit(callback);
            if (mode == "budget-submit") entry.advanceForTest(20001);
            if (mode == "cancel-submit" || mode == "restore-submit") {
                putEntry(path + (mode == "cancel-submit" ? "/selected-cancel" : "/restore.claim"), "stop");
                entry.checkForTest(); pumpEntry();
                require(entry.disarmedForTest() && !entry.terminal(), "nested worker stop defers publication");
            }
            if (mode == "expire-submit") entry.expireForTest();
            if (mode == "replace-submit") {
                require(QFile::rename(path + "/selected-request", path + "/preserved-request"), "preserve worker request");
                putEntry(path + "/selected-request", nonce + ' ' + identity);
                entry.checkForTest(); pumpEntry();
            }
            if (mode == "throw-submit") throw std::runtime_error("owned queued submission outcome unknown");
            require(readEntry(path + "/selected-result").isEmpty(), "still no result before submit return");
            return job;
        });
        navigator.onOpen = [&] {
            f.reader->setVisible(true);
            if (mode != "wrong-target") {
                require(f.reader->setProperty("currentPage", 1), "worker Entry fixture selects page");
                f.controller.setPageId(f.document.pages[1]);
            }
        };
        if (mode == "postlink-cancel") entry.postLinkForTest([&] { putEntry(path + "/selected-cancel", "stop"); });
        require(entry.start(), "worker Entry starts"); pumpEntry();
        require(submissions == 0 && navigator.calls == 0, "no worker/navigation before request");
        if (mode == "before-cancel") putEntry(path + "/selected-cancel", "stop");
        putEntry(path + "/selected-request", nonce + ' ' + identity); entry.checkForTest(); pumpEntry();
        if (entry.navigationPendingForTest()) entry.afterNavigationForTest();
        QElapsedTimer wait; wait.start();
        while (!entry.terminal() && wait.elapsed() < 3000) { pumpEntry(); QThread::msleep(1); }
        require(entry.terminal(), "worker Entry reaches terminal without resampling");
        const auto bytes = readEntry(path + "/selected-result");
        const bool successful = mode == "valid" || mode == "postlink-cancel";
        if (successful) {
            validReceipt(bytes, nonce, identity, "worker-witness-observed");
            struct stat committed{};
            require(::stat((path + "/selected-result").toLocal8Bit().constData(), &committed) == 0 &&
                committed.st_nlink == (mode == "postlink-cancel" ? 2U : 1U), "post-link cancellation retains an uncertain two-link receipt");
            require(QFile::exists(path + "/.pending-result") == (mode == "postlink-cancel"), "uncertain publication retains pending name");
            const auto json = QJsonDocument::fromJson(bytes).object();
            require(json["diagnostic"] == "worker-callback-witness" && json["observation_phase"] == "before-worker-submit" &&
                json["navigation_scope"] == "legacy-main-composition", "explicit worker/prequeue scope");
            require(json["observed"].toBool() && json["creation_join_observed"].toBool() && json["ready"].toBool() &&
                json["worker_finished"].toBool() && json["worker_submission_returned"].toBool() &&
                json["worker_native_attempted"].toBool() && json["worker_native_accepted"].toBool() &&
                json["worker_delivery_observed"].toBool() && json["worker_matching_completion"].toBool() &&
                json["worker_owners_released"].toBool(), "owned delivery/identity facts remain separate");
            require(json["worker_job_id"] == "18446744073709551614" && json["worker_entered_count"] == 1 &&
                json["worker_returning_count"] == 1 && json["worker_off_gui_count"] == 1,
                "lossless large job ID and offGUI facts");
            require(json["worker_refusal"].toString().isEmpty(), "successful owned worker refusal empty");
        } else if (!bytes.isEmpty()) {
            const auto json = QJsonDocument::fromJson(bytes).object();
            require(json["stage"] != "worker-witness-observed" && !json["worker_delivery_observed"].toBool(), "failure never worker positive");
        }
        const int expected = (mode == "before-cancel" || mode == "wrong-target" || mode == "worker-missing") ? 0 : 1;
        require(submissions == expected, "at most one modeled native submission");
        const auto saved = bytes;
        worker.gate.release(); worker.wait();
        entry.checkForTest(); pumpEntry();
        require(submissions == expected && readEntry(path + "/selected-result") == saved && !entry.start(),
            "late copied closures and completion cannot retry or republish");
    }
    QTextStream(stdout) << "PASS owned worker Entry: prequeue source join, offGUI/matching completion, explicit phase, original budget, cancel/replacement/exception, lossless IDs and no retry; no native authority\n";
}
#include "reader_worker_callback_entry_fixture.moc"
