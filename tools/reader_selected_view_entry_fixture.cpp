// Reuse owned interfaces and real-QML component setup; no firmware resources.
#define main readerSelectedOwnedTestsMain
#include "reader_selected_view_fixture.cpp"
#undef main
#define READER_SELECTED_TESTING
#include "reader_selected_view_entry.h"
#include <QDir>

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
    const QByteArray nonce(32, 'a'), identity = reader_selected::selfIdentity();
    require(!identity.isEmpty(), "entry process identity");
    Fixture f;
    const QList<QByteArray> modes{"valid", "bad-binding", "cancel", "restore", "expiry", "duplicate", "pending",
        "root-replaced", "symlink", "fifo", "hardlink", "bad-mode", "request-replaced", "nested-cancel", "nested-expiry",
        "nested-restore", "postlink-cancel", "prelink-expiry", "postlink-expiry", "check-cap"};
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
            entry.checkForTest(); pumpEntry();
        }
        pumpEntry();
        require(entry.terminal(), "entry terminal is sticky");
        const QByteArray bytes = readEntry(root + "/selected-result");
        const bool suppressed = mode == "cancel" || mode == "restore" || mode == "root-replaced" || mode == "request-replaced" ||
            mode == "nested-cancel" || mode == "nested-restore";
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
            const QString expected = mode == "valid" ? QStringLiteral("observed") :
                (mode == "expiry" || mode == "nested-expiry") ? QStringLiteral("setup-expired") :
                mode == "check-cap" ? QStringLiteral("request-check-cap") : QStringLiteral("request-refused");
            validReceipt(bytes, nonce, identity, expected);
            const auto json = QJsonDocument::fromJson(bytes).object();
            require(json["observed"].toBool() == (mode == "valid"), "only actual admitted observation has identities");
            if (mode == "valid") require(json["ready"].toBool() && json["document_id"].toString() == f.document.key &&
                    json["page_id"].toString() == f.document.pages[0] && getterReads > 0 && entry.samplesForTest() == 1,
                    "one real observer result through entry");
            else require(json["document_id"].toString().isEmpty() && !json["ready"].toBool(), "terminal refusal clears identity observation");
            struct stat st{};
            require(::lstat((root + "/selected-result").toLocal8Bit().constData(), &st) == 0 && st.st_nlink == 1 &&
                    (st.st_mode & 07777) == 0600 && readEntry(root + "/.pending-result").isEmpty(), "closed single-link receipt");
        }
        const int before = entry.samplesForTest();
        putEntry(root + "/extra-event", "event"); entry.checkForTest(); pumpEntry();
        require(entry.samplesForTest() == before && readEntry(root + "/selected-result") == bytes && !entry.start(), "terminal entry cannot replay or rearm");
        f.current->pageRead = {};
    }
    QTextStream(stdout) << "PASS selected entry owned Qt: exact request/no early getter, binding/mode/link/FIFO refusal, pinned request, one-shot/cap/deadline, nested cancel/restore/expiry deferral, pending publication; all authorities=false\n";
}
