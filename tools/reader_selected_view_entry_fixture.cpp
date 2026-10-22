// Reuse owned interfaces and real-QML component setup; no firmware resources.
#define main readerSelectedOwnedTestsMain
#include "reader_selected_view_fixture.cpp"
#undef main
#define READER_SELECTED_TESTING
#include "reader_selected_view_entry.h"
#include <QDir>
#include <QProcess>

static QByteArray readEntry(const QString &path) {
    QFile f(path); return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}
static void putEntry(const QString &path, const QByteArray &bytes) {
    QFile f(path); require(f.open(QIODevice::WriteOnly | QIODevice::NewOnly), "exclusive entry fixture file");
    require(f.setPermissions(QFile::ReadOwner | QFile::WriteOwner), "entry file mode");
    require(f.write(bytes) == bytes.size(), "entry file write"); f.close();
}
static void pumpEntry() { for (int n = 0; n < 8; ++n) QCoreApplication::processEvents(); }
static void validReceipt(const QByteArray &bytes, const QByteArray &nonce, const QByteArray &identity, const QString &stage) {
    require(!bytes.isEmpty() && bytes.size() <= 2048, "bounded entry receipt");
    const auto json = QJsonDocument::fromJson(bytes).object();
    const auto parts = identity.trimmed().split(' ');
    require(json["stage"].toString() == stage && json["nonce"].toString() == QString::fromLatin1(nonce) &&
            json["pid"].toString() == QString::fromLatin1(parts[0]) && json["start"].toString() == QString::fromLatin1(parts[1]),
            "exact entry receipt binding");
    for (const QString &name : {QStringLiteral("native_authority"), QStringLiteral("render_authority"), QStringLiteral("durable_authority")})
        require(json[name].isBool() && !json[name].toBool(), "entry authority remains false");
}
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    qmlRegisterType<QObject>("OwnedProbe", 1, 0, "QtObject");
    qmlRegisterType<QQuickItem>("OwnedProbe", 1, 0, "Item");
    qmlRegisterType<FocusFixture>("OwnedProbe", 1, 0, "FocusScope");
    qmlRegisterType<SceneView>("OwnedProbe", 1, 0, "SceneFixture");
    registerCreationFixture();
    const QByteArray nonce(32, 'a'), identity = reader_selected::selfIdentity();
    require(!identity.isEmpty(), "entry process identity");
    Fixture f;
    require(argc == 2, "explicit owned publisher executable required");
    const QString publisher = QString::fromLocal8Bit(argv[1]);
    const QList<QByteArray> modes{"valid", "bad-binding", "cancel", "restore", "expiry", "duplicate", "pending",
        "root-replaced", "symlink", "fifo", "hardlink", "bad-mode", "request-replaced", "nested-cancel", "nested-expiry",
        "nested-restore", "postlink-cancel", "prelink-expiry", "postlink-expiry", "check-cap",
        "slow-valid", "slow-cancel", "slow-restore"};
    for (const QByteArray &mode : modes) {
        QTemporaryDir base; require(base.isValid(), "entry temporary base");
        const QString root = base.path() + "/root";
        require(QDir().mkdir(root) && ::chmod(root.toLocal8Bit().constData(), 0700) == 0, "entry root ownership/mode");
        putEntry(root + "/owner", nonce); putEntry(root + "/attempt.identity", identity);
        if (mode == "duplicate") putEntry(root + "/selected-result", "preserved");
        if (mode == "pending") putEntry(root + "/.pending-result", "preserved");
        reader_selected::Entry entry(&app, root, nonce);
        int getterReads = 0;
        f.current->pageRead = [&] { ++getterReads; };
        const bool started = entry.start();
        if (mode == "duplicate" || mode == "pending") {
            require(!started && entry.terminal() && entry.samplesForTest() == 0 && getterReads == 0, "existing output refused without reading native state");
            require(readEntry(root + (mode == "duplicate" ? "/selected-result" : "/.pending-result")) == "preserved", "existing output preserved");
            f.current->pageRead = {}; continue;
        }
        require(started && readEntry(root + "/selected-ready") == nonce + ' ' + identity, "exact ready publication");
        pumpEntry();
        require(entry.samplesForTest() == 0 && getterReads == 0 && readEntry(root + "/selected-result").isEmpty(), "no observer before exact request");
        if (mode == "check-cap") {
            for (int n = 0; n < 33; ++n) { entry.checkForTest(); pumpEntry(); }
        } else if (mode == "expiry") entry.expireForTest();
        else {
            if (mode == "cancel") putEntry(root + "/selected-cancel", "cancel");
            if (mode == "restore") putEntry(root + "/restore.claim", "restore");
            if (mode == "root-replaced") {
                require(QDir().rename(root, base.path() + "/moved") && QDir().mkdir(root), "replace entry root");
            }
            if (mode == "postlink-cancel") entry.postLinkForTest([&] { putEntry(root + "/selected-cancel", "cancel"); });
            if (mode == "prelink-expiry") entry.preLinkForTest([&] { entry.expireForTest(); });
            if (mode == "postlink-expiry") entry.postLinkForTest([&] { entry.expireForTest(); });
            if (mode == "symlink" || mode == "hardlink") {
                putEntry(base.path() + "/target", nonce + ' ' + identity);
                const QByteArray target = (base.path() + "/target").toLocal8Bit(), request = (root + "/selected-request").toLocal8Bit();
                require((mode == "symlink" ? ::symlink(target.constData(), request.constData()) : ::link(target.constData(), request.constData())) == 0,
                        "request link fixture");
            } else if (mode == "fifo") require(::mkfifo((root + "/selected-request").toLocal8Bit().constData(), 0600) == 0, "request FIFO fixture");
            else putEntry(root + "/selected-request", mode == "bad-binding" ? nonce + ' ' + identity + "extra" : nonce + ' ' + identity);
            if (mode == "bad-mode") require(::chmod((root + "/selected-request").toLocal8Bit().constData(), 0644) == 0, "bad request mode");
            if (mode == "request-replaced") f.current->pageRead = [&] {
                ++getterReads;
                require(QFile::rename(root + "/selected-request", root + "/preserved-request"), "pin original request inode");
                putEntry(root + "/selected-request", nonce + ' ' + identity);
            };
            if (mode == "nested-cancel" || mode == "nested-expiry" || mode == "nested-restore") f.current->pageRead = [&] {
                ++getterReads; require(entry.samplingForTest(), "adversarial getter is on sample stack");
                if (mode == "nested-expiry") entry.expireForTest();
                else {
                    putEntry(root + (mode == "nested-cancel" ? "/selected-cancel" : "/restore.claim"), "stop");
                    entry.checkForTest(); pumpEntry();
                }
                require(entry.samplingForTest() && entry.disarmedForTest() && !entry.terminal() &&
                        readEntry(root + "/selected-result").isEmpty(), "nested stop disarms but defers terminal publication until sample unwinds");
            };
            if (mode.startsWith("slow-")) f.current->pageRead = [&] {
                if (++getterReads != 1) return;
                std::this_thread::sleep_for(std::chrono::milliseconds(1100));
                if (mode != "slow-valid") putEntry(root + (mode == "slow-cancel" ? "/selected-cancel" : "/restore.claim"), "stop");
            };
            entry.checkForTest(); pumpEntry();
        }
        pumpEntry();
        require(entry.terminal(), "entry terminal is sticky");
        const QByteArray bytes = readEntry(root + "/selected-result");
        const bool suppressed = mode == "cancel" || mode == "restore" || mode == "root-replaced" || mode == "request-replaced" ||
            mode == "nested-cancel" || mode == "nested-restore" || mode == "slow-cancel" || mode == "slow-restore";
        if (suppressed) require(bytes.isEmpty(), "cancellation/restore/request lifetime/trust loss suppress valid output");
        else if (mode == "postlink-cancel" || mode == "postlink-expiry") {
            struct stat st{};
            require(!bytes.isEmpty() && readEntry(root + "/.pending-result") == bytes &&
                    ::lstat((root + "/selected-result").toLocal8Bit().constData(), &st) == 0 && st.st_nlink == 2,
                    "postlink stop preserves explicit invalid pending/nlink2 state");
        } else if (mode == "prelink-expiry") {
            struct stat st{};
            require(bytes.isEmpty() && !readEntry(root + "/.pending-result").isEmpty() &&
                    ::lstat((root + "/.pending-result").toLocal8Bit().constData(), &st) == 0 && st.st_nlink == 1,
                    "deadline during publication cannot commit a late observation");
        } else {
            const bool successful = mode == "valid" || mode == "slow-valid";
            const QString expected = successful ? QStringLiteral("observed") :
                (mode == "expiry" || mode == "nested-expiry") ? QStringLiteral("setup-expired") :
                mode == "check-cap" ? QStringLiteral("request-check-cap") : QStringLiteral("request-refused");
            validReceipt(bytes, nonce, identity, expected);
            const auto json = QJsonDocument::fromJson(bytes).object();
            require(!json.contains("diagnostic") && !json.contains("creation_join_observed"), "default entry wire schema remains unchanged");
            require(json["observed"].toBool() == successful, "only actual admitted observation has identities");
            if (successful) require(json["ready"].toBool() && json["document_id"].toString() == f.document.key &&
                    json["page_id"].toString() == f.document.pages[0] && getterReads > 0 && entry.samplesForTest() == 1,
                    "one real observer result through entry");
            else require(json["document_id"].toString().isEmpty() && !json["ready"].toBool(), "terminal refusal clears identity observation");
            if (mode == "slow-valid") require(json["sample_end_ms"].toInt() - json["sample_begin_ms"].toInt() >= 1100,
                    "entry permits an admitted observation beyond the former one-second cutoff");
            struct stat st{};
            require(::lstat((root + "/selected-result").toLocal8Bit().constData(), &st) == 0 && st.st_nlink == 1 &&
                    (st.st_mode & 07777) == 0600 && readEntry(root + "/.pending-result").isEmpty(), "closed single-link receipt");
        }
        const int before = entry.samplesForTest();
        putEntry(root + "/extra-event", "event"); entry.checkForTest(); pumpEntry();
        require(entry.samplesForTest() == before && readEntry(root + "/selected-result") == bytes && !entry.start(), "terminal entry cannot replay or rearm");
        f.current->pageRead = {};
    }
    int completeDirectObserved = 0, completeDirectRefused = 0;
    for (const QByteArray &mode : {QByteArray("open-gap"), QByteArray("partial-gap"), QByteArray("complete-direct"), QByteArray("atomic")}) {
        const int trials = (mode == "complete-direct" || mode == "atomic") ? 10 : 1;
        for (int trial = 0; trial < trials; ++trial) {
            QTemporaryDir base; require(base.isValid(), "interprocess temporary root");
            const QString root = base.path() + "/root";
            require(QDir().mkdir(root) && ::chmod(root.toLocal8Bit().constData(), 0700) == 0, "interprocess root mode");
            putEntry(root + "/owner", nonce); putEntry(root + "/attempt.identity", identity);
            reader_selected::Entry entry(&app, root, nonce); require(entry.start(), "interprocess entry start"); pumpEntry();
            const QByteArray request = nonce + ' ' + identity;
            QProcess writer;
            if (mode == "atomic") {
                QString program = publisher;
                QStringList arguments{root, QString::fromLatin1(nonce),
                    QString::fromLatin1(identity.trimmed().split(' ')[0]), QString::fromLatin1(identity.trimmed().split(' ')[1])};
                const QString qemu = qEnvironmentVariable("READER_SELECTED_HELPER_QEMU");
                if (!qemu.isEmpty()) { arguments.prepend(program); arguments.prepend(qEnvironmentVariable("READER_SELECTED_HELPER_SYSROOT")); arguments.prepend("-L"); program = qemu; }
                writer.start(program, arguments);
            } else {
                QString script = "umask 077; ";
                if (mode == "complete-direct") script += "printf '%s' \"$2\" > \"$1\"";
                else {
                    script += "exec 3> \"$1\"; ";
                    if (mode == "partial-gap") script += "printf '%s' \"$3\" >&3; ";
                    script += "printf 'opened\\n'; IFS= read -r signal; printf '%s' \"$2\" >&3; exec 3>&-";
                }
                const QByteArray remaining = mode == "partial-gap" ? request.mid(10) : request;
                writer.start("/bin/sh", {"-c", script, "owned-writer", root + "/selected-request", QString::fromLatin1(remaining), QString::fromLatin1(request.left(10))});
            }
            require(writer.waitForStarted(5000), "interprocess writer started");
            if (mode == "open-gap" || mode == "partial-gap") {
                require(writer.waitForReadyRead(5000) && writer.readAllStandardOutput() == "opened\n", "writer staged open/write handshake");
                QElapsedTimer wait; wait.start();
                while (!entry.terminal() && wait.elapsed() < 5000) pumpEntry();
                require(entry.terminal() && entry.samplesForTest() == 0, "visible incomplete request refuses before observation");
                require(writer.write("go\n") == 3 && writer.waitForBytesWritten(5000), "release staged writer");
            }
            QElapsedTimer wait; wait.start();
            while ((!entry.terminal() || writer.state() != QProcess::NotRunning) && wait.elapsed() < 5000) pumpEntry();
            require(entry.terminal() && writer.state() == QProcess::NotRunning && writer.exitCode() == 0, "interprocess writer/entry completed");
            require(readEntry(root + "/selected-request") == request, "final complete bytes cannot explain earlier refusal alone");
            const auto result = QJsonDocument::fromJson(readEntry(root + "/selected-result")).object();
            if (mode == "open-gap" || mode == "partial-gap") {
                require(result["stage"] == "request-refused" && result["sample_begin_ms"].toInt() == -1 && result["sample_end_ms"].toInt() == -1,
                        "interprocess incomplete visibility reproduces valid pre-observation refusal");
            } else if (mode == "complete-direct") {
                if (result["stage"] == "observed") ++completeDirectObserved;
                else { require(result["stage"] == "request-refused" && entry.samplesForTest() == 0, "direct publication outcome remains bounded"); ++completeDirectRefused; }
            } else {
                require(result["stage"] == "observed" && entry.samplesForTest() == 1 && result["ready"].toBool(), "atomic closed-file request observed exactly once");
                QProcess duplicate;
                duplicate.start(writer.program(), writer.arguments()); require(duplicate.waitForFinished(5000) && duplicate.exitCode() != 0,
                        "atomic publisher never overwrites/replays existing request");
                require(readEntry(root + "/selected-request") == request && entry.samplesForTest() == 1, "duplicate preserves committed request");
            }
        }
    }
    QTextStream(stdout) << "PASS interprocess request publication: deterministic open/partial refusal; complete-direct observed=" << completeDirectObserved
        << " refused=" << completeDirectRefused << "; atomic10/10 exact once; no overwrite\n";
    require(creationPreparations == 0, "ordinary entry never resolves creation singleton");
    for (const QByteArray &mode : {QByteArray("valid"), QByteArray("bad-binding"), QByteArray("before-cancel"), QByteArray("before-expiry"),
            QByteArray("prepare-cancel"), QByteArray("prepare-restore"), QByteArray("prepare-expiry"),
            QByteArray("prepare-request-replaced"), QByteArray("prepare-reentry"), QByteArray("postlink-cancel")}) {
        Fixture joinFixture;
        Library library;
        DocumentController creation;
        creation.setLibrary(&library); library.result = &joinFixture.document;
        joinFixture.engine.rootContext()->setContextProperty("externalCreation", &creation);
        QTemporaryDir base; require(base.isValid(), "join entry temporary base");
        const QString root = base.path() + "/root";
        require(QDir().mkdir(root) && ::chmod(root.toLocal8Bit().constData(), 0700) == 0, "join entry private root");
        putEntry(root + "/owner", nonce); putEntry(root + "/attempt.identity", identity);
        const int preparationsBefore = creationPreparations;
        reader_selected::Entry entry(&app, root, nonce, nullptr, reader_selected::Entry::Diagnostic::CreationSourceJoin);
        require(entry.start(), "join entry starts without singleton resolution");
        pumpEntry();
        require(creationPreparations == preparationsBefore && library.calls == 0 && entry.samplesForTest() == 0 &&
                readEntry(root + "/selected-result").isEmpty(), "join entry has no singleton/native lookup before exact request");
        bool hookReached = false;
        preparationHook = [&] {
            hookReached = true;
            require(entry.samplingForTest() && entry.samplesForTest() == 1 && library.calls == 0,
                    "preparation occurs on one admitted operation stack before resolver");
            if (mode == "prepare-expiry") entry.expireForTest();
            else if (mode == "prepare-request-replaced") {
                require(QFile::rename(root + "/selected-request", root + "/preserved-request"), "preserve original join request inode");
                putEntry(root + "/selected-request", nonce + ' ' + identity);
            } else if (mode == "prepare-reentry") {
                const auto nested = reader_selected::observeCreationJoin(&joinFixture.window);
                refused(nested, "preparation cannot recursively enter join");
                require(nested.refusal == "sample-reentered", "preparation reentry has fixed refusal");
            } else if (mode == "prepare-cancel" || mode == "prepare-restore") {
                putEntry(root + (mode == "prepare-cancel" ? "/selected-cancel" : "/restore.claim"), "stop");
                entry.checkForTest(); pumpEntry();
                require(entry.disarmedForTest(), "nested preparation cancellation disarms entry");
            }
            require(entry.samplingForTest() && !entry.terminal() && readEntry(root + "/selected-result").isEmpty(),
                    "preparation stop cannot publish or finish before operation stack unwinds");
        };
        if (mode == "postlink-cancel") entry.postLinkForTest([&] { putEntry(root + "/selected-cancel", "stop"); });
        if (mode == "before-cancel") putEntry(root + "/selected-cancel", "stop");
        if (mode == "before-expiry") entry.expireForTest();
        else putEntry(root + "/selected-request", nonce + ' ' + identity + (mode == "bad-binding" ? QByteArray("extra") : QByteArray()));
        entry.checkForTest(); pumpEntry();
        preparationHook = {};
        require(entry.terminal() && entry.disarmedForTest() && !entry.samplingForTest(), "join entry terminal/disarmed after unwind");
        const bool noPreparation = mode == "bad-binding" || mode == "before-cancel" || mode == "before-expiry";
        require(creationPreparations == preparationsBefore + (noPreparation ? 0 : 1) && hookReached == !noPreparation,
                "exact request controls single singleton preparation");
        const bool joined = mode == "valid" || mode == "postlink-cancel";
        require(library.calls == (joined ? 1 : 0) && entry.samplesForTest() == (noPreparation ? 0 : 1),
                "no resolver after preparation stop, refusal, or reentry");
        const QByteArray bytes = readEntry(root + "/selected-result");
        const bool suppressed = mode == "before-cancel" || mode == "prepare-cancel" || mode == "prepare-restore" || mode == "prepare-request-replaced";
        if (suppressed) require(bytes.isEmpty(), "join cancellation/restore/request replacement suppresses output");
        else {
            const QString stage = joined ? QStringLiteral("observed") :
                (mode == "before-expiry" || mode == "prepare-expiry") ? QStringLiteral("setup-expired") :
                mode == "bad-binding" ? QStringLiteral("request-refused") : QStringLiteral("observation-refused");
            validReceipt(bytes, nonce, identity, stage);
            const auto json = QJsonDocument::fromJson(bytes).object();
            require(json["diagnostic"].isString() && json["diagnostic"].toString() == "creation-source-join" &&
                    json["creation_join_observed"].isBool() && json["creation_join_observed"].toBool() == joined &&
                    json["observed"].isBool() && json["observed"].toBool() == joined,
                    "join-specific kind and scalar success are explicit");
            if (joined) require(json["document_id"].toString() == joinFixture.document.key && json["ready"].toBool(),
                    "joined entry returns selected source scalar facts");
            else require(json["document_id"].toString().isEmpty() && !json["ready"].toBool(), "join terminal refusal has no identity");
            struct stat st{};
            require(::lstat((root + "/selected-result").toLocal8Bit().constData(), &st) == 0 &&
                    (st.st_mode & 07777) == 0600 && S_ISREG(st.st_mode), "private join receipt");
            if (mode == "postlink-cancel") require(st.st_nlink == 2 && readEntry(root + "/.pending-result") == bytes,
                    "uncertain join publication retains invalid pending/single-link failure evidence");
            else require(st.st_nlink == 1 && readEntry(root + "/.pending-result").isEmpty(), "closed join receipt");
        }
        const int samplesBefore = entry.samplesForTest(), callsBefore = library.calls;
        putEntry(root + "/extra-event", "event"); entry.checkForTest(); pumpEntry();
        require(entry.samplesForTest() == samplesBefore && library.calls == callsBefore &&
                creationPreparations == preparationsBefore + (noPreparation ? 0 : 1) &&
                readEntry(root + "/selected-result") == bytes && !entry.start(),
                "join entry never retries spent request or uncertain publication");
    }
    QTextStream(stdout) << "PASS creation join entry owned Qt: no preparation before exact request, one typed join, explicit diagnostic/scalar wire, preparation cancel/restore/expiry/reentry/request replacement, uncertain publication/no retry; all authorities=false\n";
    QTextStream(stdout) << "PASS selected entry owned Qt: exact request/no early getter, binding/mode/link/FIFO refusal, pinned request, one-shot/cap/deadline, nested cancel/restore/expiry deferral, pending publication; all authorities=false\n";
}
