#define READER_METADATA_TESTING
#include "reader_metadata_entry.h"
#include <QQmlComponent>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <memory>
#include <vector>
#include <cstdlib>
static int valueReads = 0;
class TrapItem : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QObject* document READ objectValue)
    Q_PROPERTY(QObject* sceneController READ objectValue)
    Q_PROPERTY(QObject* controller READ objectValue)
    Q_PROPERTY(QQuickItem* sceneView READ itemValue)
    Q_PROPERTY(QString pageId READ stringValue)
    Q_PROPERTY(QString currentPageId READ stringValue)
public:
    QObject *objectValue() { ++valueReads; return nullptr; }
    QQuickItem *itemValue() { ++valueReads; return nullptr; }
    QString stringValue() { ++valueReads; return {}; }
};
class TrapHelper : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* document READ objectValue)
public:
    QObject *objectValue() { ++valueReads; return nullptr; }
};
static void require(bool ok, const char *why) {
    if (!ok) { QTextStream(stderr) << "FAIL " << why << '\n'; std::exit(1); }
}
static void pump() { for (int i = 0; i < 8; ++i) QCoreApplication::processEvents(); }
static QByteArray read(const QString &path) {
    QFile f(path); if (!f.open(QIODevice::ReadOnly)) return {}; return f.readAll();
}
static void put(const QString &path, const QByteArray &bytes) {
    QFile f(path); require(f.open(QIODevice::WriteOnly | QIODevice::NewOnly), "exclusive fixture file");
    require(f.setPermissions(QFile::ReadOwner | QFile::WriteOwner), "fixture permissions");
    require(f.write(bytes) == bytes.size(), "fixture write"); f.close();
}
int main(int argc, char **argv) {
    const bool startupTrial = argc == 2 && QByteArray(argv[1]) == "owned-startup";
    const QString startupRoot = QStringLiteral("/run/rmb-reader-census-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    if (startupTrial) {
        require(!QDir(startupRoot).exists() && QDir().mkdir(startupRoot), "fresh owned startup root");
        require(::chmod(startupRoot.toLocal8Bit().constData(), 0700) == 0, "startup root mode");
        put(startupRoot + "/owner", QByteArray(32, 'a'));
        const auto id = reader_metadata::selfIdentity();
        require(!id.isEmpty(), "startup identity");
        put(startupRoot + "/attempt.identity", id);
        put(startupRoot + "/census-request", QByteArray(32, 'a') + ' ' + id);
    }
    QGuiApplication app(argc, argv);
    qmlRegisterType<TrapItem>("OwnedCensus", 1, 0, "TrapItem");
    qmlRegisterType<TrapHelper>("OwnedCensus", 1, 0, "TrapHelper");
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"QML(import QtQuick
import OwnedCensus 1.0
Window { width: 100; height: 100; visible: true
    TrapItem { objectName: "fixture-reader"
        TrapHelper { objectName: "DocumentView.ToolSettings" }
        TrapItem { Item { objectName: "SceneViewsRoot" } }
    }
})QML", QUrl(QStringLiteral("owned-metadata.qml")));
    if (component.isError()) {
        for (const auto &e : component.errors()) QTextStream(stderr) << e.toString() << '\n';
        return 1;
    }
    std::unique_ptr<QObject> object(component.create());
    auto *window = qobject_cast<QQuickWindow *>(object.get());
    require(window, "actual QML window"); window->requestActivate(); pump();
    auto result = reader_metadata::census([] { return true; });
    if (QByteArray(result.stage) != "metadata-observed") QTextStream(stderr) << result.stage << '\n';
    require(QByteArray(result.stage) == "metadata-observed" && result.metadata && result.ownerLinks &&
            result.pageStringDomain && valueReads == 0, "metadata without any application value read");
    require(result.visited == 6 && result.helpers == 1 && result.containers == 1,
            "pinned fixture crosslink node and anchor deduplication");
    if (startupTrial) {
        const auto bytes = read(startupRoot + "/census-result");
        const auto json = QJsonDocument::fromJson(bytes).object();
        require(json["stage"].toString() == "metadata-observed" && !json["native_authority"].toBool(true) &&
                valueReads == 0 && bytes.size() <= 1024, "actual owned preloaded startup receipt");
        require(QDir(startupRoot).removeRecursively(), "exact owned startup cleanup");
        QTextStream(stdout) << "PASS actual owned preload/startup/application-queued metadata entry\n";
        return 0;
    }
    auto *reader = window->findChild<QObject *>(QStringLiteral("fixture-reader"));
    require(reader, "fixture reader");
    {
        QObject duplicate(reader); duplicate.setObjectName(QStringLiteral("DocumentView.ToolSettings"));
        require(QByteArray(reader_metadata::census([] { return true; }).stage) == "anchor-uniqueness", "duplicate refusal");
    }
    {
        std::vector<std::unique_ptr<QObject>> extras;
        for (int i = 0; i < 65; ++i) extras.emplace_back(std::make_unique<QObject>(reader));
        require(QByteArray(reader_metadata::census([] { return true; }).stage) == "census-cap", "child cap refusal");
    }
    require(QByteArray(reader_metadata::census([] { return false; }).stage) == "canceled-or-expired", "cancellation refusal");
    window->hide(); pump();
    require(QByteArray(reader_metadata::census([] { return true; }).stage) != "metadata-observed", "lost window refusal");
    window->show(); window->requestActivate(); pump();
    const QByteArray nonce(32, 'a'), identity = reader_metadata::selfIdentity();
    require(!identity.isEmpty(), "bounded process identity");
    for (const QByteArray &mode : {QByteArray("valid"), QByteArray("binding"), QByteArray("cancel"),
            QByteArray("expiry"), QByteArray("duplicate"), QByteArray("root-replaced"), QByteArray("symlink"),
            QByteArray("postlink-cancel")}) {
        QTemporaryDir base; require(base.isValid(), "owned directory");
        const QString root = base.path() + "/root";
        require(QDir().mkdir(root) && ::chmod(root.toLocal8Bit().constData(), 0700) == 0, "owned root mode");
        put(root + "/owner", nonce); put(root + "/attempt.identity", identity);
        if (mode == "duplicate") put(root + "/census-result", "preserved\n");
        reader_metadata::Entry entry(&app, root, nonce);
        const bool started = entry.start();
        if (mode == "duplicate") {
            require(!started && read(root + "/census-result") == "preserved\n", "duplicate output preserved");
            continue;
        }
        require(started && read(root + "/census-ready") == nonce + ' ' + identity, "exact ready binding");
        if (mode == "postlink-cancel") entry.postLinkForTest([&] { put(root + "/census-cancel", "cancel"); });
        pump();
        if (mode == "cancel") put(root + "/census-cancel", "cancel");
        if (mode == "expiry") entry.expireForTest();
        if (mode == "root-replaced") {
            require(QDir().rename(root, base.path() + "/moved"), "move owned root");
            require(QDir().mkdir(root), "replacement root");
            // A directory rename need not yield a prompt watcher event. Exercise
            // the next queued deadline check; no immediate notification claim.
            entry.expireForTest();
        }
        if (mode == "symlink") {
            put(base.path() + "/request-target", nonce + ' ' + identity);
            require(::symlink((base.path() + "/request-target").toLocal8Bit().constData(),
                              (root + "/census-request").toLocal8Bit().constData()) == 0, "owned request symlink");
        } else put(root + "/census-request", mode == "binding" ? nonce + ' ' + identity + "extra" : nonce + ' ' + identity);
        pump();
        const auto bytes = read(root + "/census-result");
        if (mode == "cancel" || mode == "root-replaced") require(bytes.isEmpty(), "lost/canceled ownership suppresses output");
        else if (mode == "postlink-cancel") {
            struct stat st{};
            require(!bytes.isEmpty() && read(root + "/.pending-result") == bytes &&
                    ::lstat((root + "/census-result").toLocal8Bit().constData(), &st) == 0 && st.st_nlink == 2,
                    "fully formed postlink guard failure remains invalid and unknown");
        } else {
            const auto json = QJsonDocument::fromJson(bytes).object();
            const QString expected = mode == "valid" ? "metadata-observed" : mode == "expiry" ? "setup-expired" : "request-refused";
            require(!bytes.isEmpty() && bytes.size() <= 1024 && json["stage"].toString() == expected &&
                json["native_authority"].isBool() && !json["native_authority"].toBool(), "bounded terminal result");
            struct stat st{};
            require(::lstat((root + "/census-result").toLocal8Bit().constData(), &st) == 0 && st.st_nlink == 1,
                    "closed single-link final receipt");
            put(root + "/extra-event", "event"); pump();
            require(read(root + "/census-result") == bytes, "terminal receipt never replayed");
        }
        if (!entry.terminal()) QTextStream(stderr) << "unterminated mode=" << mode << '\n';
        require(entry.terminal(), "sticky terminal");
    }
    require(valueReads == 0, "all declared application value getters remain uncalled");
    QTextStream(stdout) << "PASS metadata census and one-shot entry; value_reads=0; native_authority=false\n";
}
#include "reader_metadata_fixture.moc"
