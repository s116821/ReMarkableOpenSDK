#define VISUAL_CAPTURE_TESTING
#include "visual_capture_entry.h"
#include <QQmlComponent>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <memory>
#include <cstdlib>
static void require(bool ok, const char *why) {
    if (!ok) { QTextStream(stderr) << "FAIL " << why << '\n'; std::exit(1); }
}
static void pump() { for (int i = 0; i < 8; ++i) QCoreApplication::processEvents(); }
static void put(const QString &p, const QByteArray &b) {
    QFile f(p); require(f.open(QIODevice::WriteOnly | QIODevice::NewOnly), "exclusive fixture input");
    require(f.setPermissions(QFile::ReadOwner | QFile::WriteOwner), "fixture input mode");
    require(f.write(b) == b.size(), "fixture write"); f.close();
}
static QByteArray read(const QString &p) { QFile f(p); require(f.open(QIODevice::ReadOnly), "fixture read"); return f.readAll(); }
static QImage pixels() { QImage i(100, 100, QImage::Format_RGB32); i.fill(Qt::white); return i; }
int main(int argc, char **argv) {
    const bool startupTrial = argc == 2 && QByteArray(argv[1]) == "owned-startup";
    const QString startupRoot = "/run/rmb-visual-capture-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    if (startupTrial) {
        require(!QDir(startupRoot).exists() && QDir().mkdir(startupRoot), "fresh owned startup root");
        require(::chmod(startupRoot.toLocal8Bit().constData(), 0700) == 0, "startup root mode");
        const QByteArray nonce(32, 'a'), id = visual_capture::selfIdentity();
        put(startupRoot + "/owner", nonce); put(startupRoot + "/attempt.identity", id);
        put(startupRoot + "/visual-request", nonce + ' ' + id);
    }
    QGuiApplication app(argc, argv);
    QQmlEngine engine; QQmlComponent component(&engine);
    component.setData("import QtQuick\nWindow { width: 100; height: 100; visible: true; color: \"white\" }", QUrl("owned-visual.qml"));
    std::unique_ptr<QObject> object(component.create());
    auto *window = qobject_cast<QQuickWindow *>(object.get());
    require(window, "original owned QtWindow"); window->requestActivate(); pump();
    if (startupTrial) {
        const auto j = QJsonDocument::fromJson(read(startupRoot + "/visual-result")).object();
        const auto png = read(startupRoot + "/visual-image.png");
        require(j["stage"] == "visual-image-observed" && !j["native_authority"].toBool(true) &&
            j["png_sha256"].toString().toLatin1() == QCryptographicHash::hash(png, QCryptographicHash::Sha256).toHex(),
            "actual owned preload/startup public grab receipt");
        require(QDir(startupRoot).removeRecursively(), "exact owned startup cleanup");
        QTextStream(stdout) << "PASS actual owned preload/startup public GUI capture\n";
        return 0;
    }
    QTemporaryDir sinkRoot;
    {
        const QByteArray p = (sinkRoot.path() + "/cap").toLocal8Bit();
        int fd = ::open(p.constData(), O_WRONLY | O_CREAT | O_EXCL, 0600); require(fd >= 0, "sink fixture fd");
        visual_capture::Sink sink(fd);
        QByteArray bytes(visual_capture::PngCap, 'x');
        require(sink.write(bytes) == bytes.size() && sink.valid(), "exact encoded cap admitted");
        require(sink.write("x", 1) == -1 && !sink.valid() && sink.write("", 0) <= 0, "overflow sticky refusal");
        require(sink.count() == visual_capture::PngCap, "no bytes beyond cap"); ::close(fd);
    }
    {
        int fds[2]; require(::pipe(fds) == 0 && ::fcntl(fds[1], F_SETFL, O_NONBLOCK) == 0, "short-write pipe");
        require(::fcntl(fds[1], F_SETPIPE_SZ, 4096) == 4096, "bounded short-write pipe capacity");
        visual_capture::Sink sink(fds[1]); QByteArray bytes(65536, 'x');
        require(sink.write(bytes) == -1 && !sink.valid() && sink.write("x", 1) == -1, "short write sticky");
        ::close(fds[0]); ::close(fds[1]);
        visual_capture::Sink bad(-1); require(bad.write("x", 1) == -1 && !bad.valid(), "IO error sticky");
    }
    {
        QFile file(sinkRoot.path() + "/actual.png"); require(file.open(QIODevice::WriteOnly), "actual public grab fd");
        visual_capture::Sink sink(file.handle());
        auto r = visual_capture::capture([] { return true; }, sink);
        require(QByteArray(r.stage) == "visual-image-observed" && r.applicationThread && r.currentWindow, "original public GUI grab");
        require(r.encoded <= visual_capture::PngCap && r.width == 100 && r.height == 100, "public owned dimensions");
        file.close();
        require(QImage(sinkRoot.path() + "/actual.png").size() == QSize(100, 100), "owned PNG decode");
    }
    const auto trial = [&](const char *mode) {
        QTemporaryDir root; require(root.isValid(), "fresh entry root");
        require(::chmod(root.path().toLocal8Bit().constData(), 0700) == 0, "root mode");
        const QByteArray nonce(32, 'a'), id = visual_capture::selfIdentity();
        put(root.path() + "/owner", nonce); put(root.path() + "/attempt.identity", id);
        visual_capture::Entry entry(&app, root.path(), nonce); int grabs = 0;
        int links = 0;
        entry.grabForTest([&](QQuickWindow *) {
            ++grabs;
            if (QByteArray(mode) == "null") return QImage{};
            if (QByteArray(mode) == "dimensions") return QImage(1873, 1, QImage::Format_RGB32);
            if (QByteArray(mode) == "storage") return QImage(1000, 1400, QImage::Format_RGBA64);
            if (QByteArray(mode) == "pixel-cap") return QImage(1872, 1872, QImage::Format_RGB32);
            if (QByteArray(mode) == "encoded-cap") {
                QImage image(1000, 1000, QImage::Format_RGB32); quint32 seed = 1;
                for (int y = 0; y < image.height(); ++y) {
                    auto *row = reinterpret_cast<QRgb *>(image.scanLine(y));
                    for (int x = 0; x < image.width(); ++x) { seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; row[x] = seed | 0xff000000; }
                }
                return image;
            }
            if (QByteArray(mode) == "context") { window->hide(); pump(); }
            if (QByteArray(mode) == "expiry") { entry.expireForTest(); pump(); }
            pump(); // nested watcher callbacks must not redispatch
            return pixels();
        });
        if (QByteArray(mode) == "post-link-cancel") entry.postLinkForTest([&] { put(root.path() + "/visual-cancel", "x"); });
        if (QByteArray(mode) == "scalar-link-cancel") entry.postLinkForTest([&] {
            if (++links == 2) put(root.path() + "/visual-cancel", "x");
        });
        if (QByteArray(mode) == "path-substitution") entry.postLinkForTest([&] {
            if (++links == 1) {
                require(::unlink((root.path() + "/.pending-image").toLocal8Bit().constData()) == 0, "fixture pending replacement");
                put(root.path() + "/.pending-image", "replaced");
            }
        });
        require(entry.start(), "entry setup"); pump();
        if (QByteArray(mode) == "bad-request") put(root.path() + "/visual-request", "wrong");
        else put(root.path() + "/visual-request", nonce + ' ' + id);
        pump(); require(entry.terminal(), "sticky terminal result");
        if (QByteArray(mode) == "post-link-cancel") {
            require(grabs == 1 && !QFile::exists(root.path() + "/visual-result"), "cancel blocks successful scalar");
            struct stat a{}, b{};
            require(::stat((root.path() + "/visual-image.png").toLocal8Bit().constData(), &a) == 0 &&
                ::stat((root.path() + "/.pending-image").toLocal8Bit().constData(), &b) == 0 &&
                a.st_ino == b.st_ino && a.st_nlink == 2, "complete-looking canceled image invalid");
        } else if (QByteArray(mode) == "scalar-link-cancel") {
            struct stat a{}, b{}, image{};
            require(grabs == 1 && ::stat((root.path() + "/visual-result").toLocal8Bit().constData(), &a) == 0 &&
                ::stat((root.path() + "/.pending-result").toLocal8Bit().constData(), &b) == 0 &&
                ::stat((root.path() + "/visual-image.png").toLocal8Bit().constData(), &image) == 0 &&
                a.st_ino == b.st_ino && a.st_nlink == 2 && image.st_nlink == 1,
                "non-atomic pair with canceled scalar remains invalid");
        } else if (QByteArray(mode) == "path-substitution") {
            struct stat a{}, b{};
            const auto j = QJsonDocument::fromJson(read(root.path() + "/visual-result")).object();
            require(j["stage"] == "image-publication-refused" && grabs == 1 &&
                ::stat((root.path() + "/visual-image.png").toLocal8Bit().constData(), &a) == 0 &&
                ::stat((root.path() + "/.pending-image").toLocal8Bit().constData(), &b) == 0 &&
                a.st_ino != b.st_ino && read(root.path() + "/.pending-image") == "replaced",
                "substituted pending never unlinked or accepted");
        } else {
            const auto bytes = read(root.path() + "/visual-result");
            const auto j = QJsonDocument::fromJson(bytes).object();
            require(bytes.size() <= 1024 && !j["native_authority"].toBool(true), "bounded nonauthoritative scalar");
            if (QByteArray(mode) == "valid") {
                const auto png = read(root.path() + "/visual-image.png");
                require(j["stage"] == "visual-image-observed" && grabs == 1 &&
                    j["png_sha256"].toString().toLatin1() == QCryptographicHash::hash(png, QCryptographicHash::Sha256).toHex() &&
                    j["encoded_bytes"].toInt() == png.size() && !QFile::exists(root.path() + "/.pending-image") &&
                    !QFile::exists(root.path() + "/.pending-result"), "matching image/scalar publication");
            } else require(j["stage"] != "visual-image-observed" && !QFile::exists(root.path() + "/visual-image.png"), "refusal no final image");
        }
        const int before = grabs; pump(); require(grabs == before && grabs <= 1, "no second grab after terminal");
        window->show(); window->requestActivate(); pump();
    };
    for (auto mode : {"valid", "null", "dimensions", "storage", "pixel-cap", "encoded-cap", "context", "expiry", "bad-request", "post-link-cancel", "scalar-link-cancel", "path-substitution"}) trial(mode);
    QTextStream(stdout) << "PASS visual core/sink/original public grab and focused entry quotas/context/nested expiry/publication\n";
}
