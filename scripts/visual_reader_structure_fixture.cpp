#define VISUAL_CAPTURE_TESTING
#include "visual_capture_entry.h"
#include <QQmlComponent>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <memory>
#include <stdexcept>
static int valueReads = 0;
class ReaderTrap : public QQuickItem {
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
class ReaderIntPage : public ReaderTrap {
    Q_OBJECT
    Q_PROPERTY(int pageId READ integerValue)
    Q_PROPERTY(int currentPageId READ integerValue)
public:
    int integerValue() { ++valueReads; return 7; }
};
class ReaderHelper : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* document READ objectValue)
public:
    QObject *objectValue() { ++valueReads; return nullptr; }
};
static void require(bool ok, const char *why) {
    if (!ok) { QTextStream(stderr) << "FAIL " << why << '\n'; std::exit(1); }
}
static void pump() { for (int i = 0; i < 8; ++i) QCoreApplication::processEvents(); }
static void focus(QQuickWindow *window) {
    window->show(); window->requestActivate(); pump();
    require(QGuiApplication::focusWindow() == window && window->isActive(), "owned focus established");
}
static QByteArray read(const QString &p) {
    QFile file(p); if (!file.open(QIODevice::ReadOnly)) return {}; return file.readAll();
}
static void put(const QString &p, const QByteArray &b) {
    QFile file(p); require(file.open(QIODevice::WriteOnly | QIODevice::NewOnly), "exclusive fixture input");
    require(file.setPermissions(QFile::ReadOwner | QFile::WriteOwner) && file.write(b) == b.size(), "fixture bytes/mode");
}
static QImage pixels() { QImage image(100, 100, QImage::Format_RGB32); image.fill(Qt::white); return image; }
static bool exists(const QString &root, const char *name) { return QFile::exists(root + '/' + name); }
static bool noOwnedFd(const QString &root) {
    const QDir fds("/proc/self/fd");
    for (const auto &name : fds.entryList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
        const auto target = QFileInfo(fds.filePath(name)).symLinkTarget();
        if (target == root || target.startsWith(root + '/')) return false;
    }
    return true;
}
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv); app.setQuitOnLastWindowClosed(false);
    qmlRegisterType<ReaderTrap>("ReaderOwned", 1, 0, "ReaderTrap");
    qmlRegisterType<ReaderIntPage>("ReaderOwned", 1, 0, "ReaderIntPage");
    qmlRegisterType<ReaderHelper>("ReaderOwned", 1, 0, "ReaderHelper");
    QQmlEngine engine; QQmlComponent component(&engine);
    const QByteArray source = R"QML(import QtQuick
import ReaderOwned 1.0
Window { width: 100; height: 100; visible: true
    ReaderTrap { ReaderHelper { objectName: "DocumentView.ToolSettings" }
        ReaderTrap { Item { objectName: "SceneViewsRoot" } }
    }
})QML";
    component.setData(source, QUrl("reader-owned.qml"));
    std::unique_ptr<QObject> owner(component.create()); auto *window = qobject_cast<QQuickWindow *>(owner.get());
    require(window, "owned original QML window"); focus(window);
    const QByteArray nonce(32, 'a'), identity = visual_capture::selfIdentity();
    const auto prepare = [&](QTemporaryDir &root) {
        require(root.isValid() && ::chmod(root.path().toLocal8Bit().constData(), 0700) == 0, "owned root");
        put(root.path() + "/owner", nonce); put(root.path() + "/attempt.identity", identity);
    };
    using visual_capture::SamplePhase;
    using visual_capture::FrameSignal;
    // The maximum typed-width envelope deliberately is not an admissible live observation.
    {
        reader_metadata::Result maximum; maximum.stage = "canceled-or-expired";
        maximum.visited = maximum.helpers = maximum.containers = 128;
        const auto bytes = visual_capture::readerStructureBytes(QByteArray(32, 'f'),
            QByteArray(20, '9') + ' ' + QByteArray(20, '9') + '\n', QByteArray(64, 'f'), maximum, maximum);
        require(bytes.size() == 721 && QJsonDocument::fromJson(bytes).object().size() == 10,
            "actual Qt serializer matches721-byte conservative envelope");
    }
    for (const QByteArray &mode : {QByteArray("valid"), QByteArray("before-refuse"), QByteArray("after-refuse"),
            QByteArray("before-context"), QByteArray("after-context"), QByteArray("permit-reentry"), QByteArray("throw"), QByteArray("missing-observer"), QByteArray("throw-permit"),
            QByteArray("before-census-once-false"), QByteArray("after-census-once-false"), QByteArray("before-census-once-throw"), QByteArray("after-census-once-throw")}) {
        focus(window); QTemporaryDir root;
        const int fd = ::open((root.path() + "/sink").toLocal8Bit().constData(), O_WRONLY | O_CREAT | O_EXCL, 0600);
        require(fd >= 0, "owned core sink"); visual_capture::Sink sink(fd);
        int grabs = 0, before = 0, after = 0; bool changeInPermit = false, insideCensus = false, originalFailed = false;
        const auto permit = [&] {
            if (mode == "throw-permit") throw std::runtime_error("owned permit failure");
            if (insideCensus && !originalFailed) {
                originalFailed = true;
                if (mode.endsWith("throw")) throw std::runtime_error("one-time original permit failure inside helper");
                return false;
            }
            if (changeInPermit) { changeInPermit = false; window->hide(); }
            return true;
        };
        const visual_capture::WindowObserver observer = [&](SamplePhase phase, QPointer<QQuickWindow> actual,
                QPointer<QQuickItem> actualRoot, const std::function<bool()> &guard) {
            require(actual == window && actualRoot == window->contentItem(), "observer receives actual core window/root");
            if ((phase == SamplePhase::BeforeGrab && mode.startsWith("before-census")) ||
                (phase == SamplePhase::AfterGrab && mode.startsWith("after-census"))) {
                insideCensus = true;
                const auto refusedSample = reader_metadata::censusWindow(actual, actualRoot, guard);
                insideCensus = false;
                require(QByteArray(refusedSample.stage) != "metadata-observed", "one-time original guard loss reaches helper refusal");
            }
            if (phase == SamplePhase::BeforeGrab) {
                ++before; require(grabs == 0 && after == 0, "before phase ordering");
                if (mode == "throw") throw std::runtime_error("owned observer refusal");
                if (mode == "before-context") window->hide();
                if (mode == "permit-reentry") changeInPermit = true;
                return mode != "before-refuse";
            }
            ++after; require(before == 1 && grabs == 1, "after phase ordering");
            if (mode == "after-context") window->hide();
            return mode != "after-refuse";
        };
        const auto result = visual_capture::captureObserved(permit, sink,
            mode == "missing-observer" ? visual_capture::WindowObserver{} : observer,
            [&](QQuickWindow *actual) { require(actual == window, "same actual one-grab window"); ++grabs; return pixels(); });
        if (mode == "valid") require(QByteArray(result.stage) == "visual-image-observed" && grabs == 1 &&
            before == 1 && after == 1 && sink.valid(), "two samples bracket exactly one grab");
        else require(QByteArray(result.stage) != "visual-image-observed" &&
            grabs == (mode.startsWith("after") ? 1 : 0) && !sink.valid(), "callback/context refusal no encode/retry");
        if (mode.contains("census-once")) require(originalFailed && result.context && !result.context(),
            "original guard loss stays latched after completed refused helper, no later true re-admission");
        sink.close(); require(::close(fd) == 0, "core local descriptor closed");
    }
    focus(window);
    {
        QTemporaryDir root; const int fd = ::open((root.path() + "/default").toLocal8Bit().constData(), O_WRONLY | O_CREAT, 0600);
        require(fd >= 0, "default sink"); visual_capture::Sink sink(fd); int grabs = 0;
        const auto result = visual_capture::capture([] { return true; }, sink, [&](QQuickWindow *) { ++grabs; return pixels(); });
        require(QByteArray(result.stage) == "visual-image-observed" && grabs == 1, "default core API compatible");
        sink.close(); require(::close(fd) == 0, "default sink close");
    }
    // New profile refuses missing runtime dependencies before creating readiness.
    for (int dependency = 0; dependency < 3; ++dependency) {
        QTemporaryDir root; prepare(root);
        visual_capture::Entry entry(&app, root.path(), nonce, nullptr, dependency != 0, dependency != 1,
            dependency == 2 ? FrameSignal::AfterFrameEnd : FrameSignal::FrameSwapped, true);
        require(!entry.start() && !exists(root.path(), "visual-ready"), "runtime profile dependency refusal before ready");
    }
    for (const QByteArray &mode : {QByteArray("valid"), QByteArray("structural-refusal"), QByteArray("reader-link-cancel"),
            QByteArray("replace-reader"), QByteArray("reader-name-collision"), QByteArray("permit-throw"), QByteArray("permit-throw-finish"), QByteArray("postlink-throw-reader"), QByteArray("grab-throw"), QByteArray("grab-delete-throw"), QByteArray("delete-before"), QByteArray("delete-after"),
            QByteArray("permit-delete-before"), QByteArray("permit-delete-after"), QByteArray("delete-in-grab"),
            QByteArray("delete-finish-context"), QByteArray("delete-image-link"), QByteArray("delete-reader-link"),
            QByteArray("delete-normal-link")}) {
        focus(window); QTemporaryDir root; prepare(root);
        auto entry = std::make_unique<visual_capture::Entry>(&app, root.path(), nonce, nullptr, true, true, FrameSignal::FrameSwapped, true);
        QPointer<visual_capture::Entry> weak(entry.get()); int grabs = 0, samples = 0; bool deleteInPermit = false;
        QPointer<QQuickItem> moved = window->findChild<QQuickItem *>(QStringLiteral("SceneViewsRoot"));
        QPointer<QQuickItem> originalParent(moved->parentItem());
        entry->readerSampleForTest([&](SamplePhase phase) {
            ++samples;
            const bool before = phase == SamplePhase::BeforeGrab;
            if ((mode == "delete-before" && before) || (mode == "delete-after" && !before)) entry.reset();
            if ((mode == "permit-delete-before" && before) || (mode == "permit-delete-after" && !before)) deleteInPermit = true;
        });
        entry->readerPermitForTest([&] {
            if (mode == "permit-throw" || (mode == "permit-throw-finish" && exists(root.path(), "visual-image.png") &&
                !exists(root.path(), ".pending-image"))) throw std::runtime_error("owned Entry permit failure");
            if (deleteInPermit || (mode == "delete-finish-context" && exists(root.path(), "visual-image.png") &&
                    !exists(root.path(), ".pending-image"))) entry.reset();
            if (mode == "replace-reader" && exists(root.path(), "visual-reader-structure") &&
                !exists(root.path(), ".pending-reader-structure") && !exists(root.path(), "visual-result")) {
                const auto bytes = read(root.path() + "/visual-reader-structure");
                require(QFile::rename(root.path() + "/visual-reader-structure", root.path() + "/old-reader"), "retained old reader inode");
                put(root.path() + "/visual-reader-structure", bytes);
            }
        });
        entry->postLinkForTest([&] {
            if (mode == "postlink-throw-reader" && exists(root.path(), "visual-reader-structure"))
                throw std::runtime_error("owned new-mode postLink failure");
            if (mode == "reader-name-collision" && exists(root.path(), "visual-image.png") &&
                !exists(root.path(), "visual-reader-structure")) put(root.path() + "/visual-reader-structure", QByteArray(1025, 'x'));
            if (mode == "reader-link-cancel" && exists(root.path(), "visual-reader-structure"))
                put(root.path() + "/visual-cancel", "x");
            if ((mode == "delete-image-link" && exists(root.path(), "visual-image.png")) ||
                (mode == "delete-reader-link" && exists(root.path(), "visual-reader-structure")) ||
                (mode == "delete-normal-link" && exists(root.path(), "visual-result"))) entry.reset();
        });
        entry->grabForTest([&](QQuickWindow *) {
            ++grabs;
            if (mode == "structural-refusal") moved->setParentItem(window->contentItem());
            if (mode == "delete-in-grab") entry.reset();
            if (mode == "grab-delete-throw") entry.reset();
            if (mode == "grab-throw" || mode == "grab-delete-throw") throw std::runtime_error("owned test grab failure");
            return pixels();
        });
        require(entry->start(), "new owned profile start"); window->frameSwapped(); pump();
        require(exists(root.path(), "visual-frame-ready"), "synthetic queued frame marker");
        put(root.path() + "/visual-request", nonce + ' ' + identity); pump();
        if (mode == "valid" || mode == "structural-refusal") {
            require(weak && weak->terminal() && grabs == 1 && samples == 2, "one request/grab and two completed samples");
            const auto reader = QJsonDocument::fromJson(read(root.path() + "/visual-reader-structure")).object();
            const auto normal = QJsonDocument::fromJson(read(root.path() + "/visual-result")).object();
            const auto png = read(root.path() + "/visual-image.png");
            require(reader.size() == 10 && reader["before_grab"].toObject().size() == 9 && reader["after_grab"].toObject().size() == 9 &&
                read(root.path() + "/visual-reader-structure").size() <= 1024 && normal["stage"] == "visual-image-observed" &&
                reader["png_sha256"] == normal["png_sha256"] && reader["png_sha256"].toString().toLatin1() ==
                QCryptographicHash::hash(png, QCryptographicHash::Sha256).toHex() && !reader["native_authority"].toBool(true),
                "exact bounded reader scalar joins normal scalar and actual encoded bytes");
            require(!exists(root.path(), ".pending-reader-structure") && !exists(root.path(), ".pending-result"), "final pending-free triple");
            if (mode == "structural-refusal") require(reader["before_grab"].toObject()["stage"] == "metadata-observed" &&
                reader["after_grab"].toObject()["stage"] == "owner-links", "completed refused snapshot remains diagnostic, not absence");
        } else if (mode == "permit-throw" || mode == "grab-throw") {
            const auto failed = QJsonDocument::fromJson(read(root.path() + "/visual-result")).object();
            require(weak && weak->terminal() && grabs == (mode == "grab-throw" ? 1 : 0) && !exists(root.path(), "visual-reader-structure") &&
                !exists(root.path(), "visual-image.png") && failed["stage"] != "visual-image-observed",
                "throwing initial permit refuses without escape/grab/reader success");
        } else if (mode.startsWith("delete") || mode.startsWith("permit-delete") || mode == "grab-delete-throw") {
            require(!weak && !entry && grabs <= 1, "Entry deletion fenced through complete call stack");
            if (mode == "delete-normal-link") require(exists(root.path(), ".pending-result"), "postLink normal output remains unaccepted pending");
            else require(!exists(root.path(), "visual-result"), "deleted Entry never reaches normal completion");
        } else {
            require(weak && weak->terminal() && grabs == 1 && !exists(root.path(), "visual-result"), "invalid new publication prevents normal success");
            if (mode == "reader-link-cancel") require(exists(root.path(), ".pending-reader-structure"), "two-link reader evidence retained");
            if (mode == "reader-name-collision") require(read(root.path() + "/visual-reader-structure") == QByteArray(1025, 'x'),
                "exclusive reader publication never overwrites preexisting bytes");
        }
        if (moved && originalParent) moved->setParentItem(originalParent);
        entry.reset(); pump();
        require(noOwnedFd(root.path()), "no owned root/image descriptor leak after Entry loss or completion");
    }
    {
        QQmlComponent integerPage(&engine); auto text = source; text.replace("ReaderTrap", "ReaderIntPage");
        integerPage.setData(text, QUrl("int-page-owned.qml"));
        std::unique_ptr<QObject> other(integerPage.create()); auto *w = qobject_cast<QQuickWindow *>(other.get());
        require(w, "owned nonQString page types"); focus(w);
        const auto result = reader_metadata::censusWindow(w, w->contentItem(), [] { return true; });
        require(QByteArray(result.stage) == "metadata-observed" && result.metadata && !result.pageStringDomain,
            "metadata-observed permits independently false page-string domain");
        const auto snapshot = visual_capture::readerSnapshot(result);
        require(snapshot["page_string_domain"].isBool() && !snapshot["page_string_domain"].toBool(true), "serializer preserves false page-string measurement");
    }
    require(valueReads == 0, "new source path invokes zero application VALUE getters");
    QTextStream(stdout) << "PASS same-grab core/Entry: actual inputs, phases, context, full-stack deletion/FD cleanup, normal triple, refused snapshots, publication/inode,721-byte Qt envelope; value_reads=0 (owned synthetic only)\n";
}
#include "visual_reader_structure_fixture.moc"
