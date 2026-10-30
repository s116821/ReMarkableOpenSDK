#define VISUAL_CAPTURE_TESTING
#include "visual_capture_entry.h"
#include <QQmlComponent>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QSignalBlocker>
#include <memory>
#include <cstdlib>
#include <thread>
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
    const bool frameStartup = argc == 2 && QByteArray(argv[1]) == "owned-frame-startup";
    const bool startupTrial = frameStartup || (argc == 2 && QByteArray(argv[1]) == "owned-startup");
    const QString startupRoot = "/run/rmb-visual-capture-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    if (startupTrial) {
        require(!QDir(startupRoot).exists() && QDir().mkdir(startupRoot), "fresh owned startup root");
        require(::chmod(startupRoot.toLocal8Bit().constData(), 0700) == 0, "startup root mode");
        const QByteArray nonce(32, 'a'), id = visual_capture::selfIdentity();
        put(startupRoot + "/owner", nonce); put(startupRoot + "/attempt.identity", id);
        if (!frameStartup) put(startupRoot + "/visual-request", nonce + ' ' + id);
    }
    QGuiApplication app(argc, argv);
    QQmlEngine engine; QQmlComponent component(&engine);
    component.setData("import QtQuick\nWindow { width: 100; height: 100; visible: true; color: \"white\" }", QUrl("owned-visual.qml"));
    std::unique_ptr<QObject> object(component.create());
    auto *window = qobject_cast<QQuickWindow *>(object.get());
    require(window, "original owned QtWindow"); window->requestActivate(); pump();
    if (argc == 2 && QByteArray(argv[1]) == "fixed-signal-only") {
        // Owned-fixture-only isolation: suppress natural signals while pumping;
        // explicitly injected queued events are delivered with signals blocked.
        QSignalBlocker naturalSignals(window);
        using visual_capture::FrameSignal;
        const QByteArray nonce(32, 'a'), id = visual_capture::selfIdentity();
        for (auto profile : {FrameSignal::AfterFrameEnd, FrameSignal::FrameSwapped}) {
            for (auto mode : {"valid", "premature", "context", "cancel", "bound-cancel", "expiry"}) {
                window->show(); window->requestActivate(); pump();
                QTemporaryDir root;
                require(root.isValid() && ::chmod(root.path().toLocal8Bit().constData(), 0700) == 0, "profile root");
                put(root.path() + "/owner", nonce); put(root.path() + "/attempt.identity", id);
                visual_capture::Entry entry(&app, root.path(), nonce, nullptr, true, true, profile);
                int grabs = 0; entry.grabForTest([&](QQuickWindow *) { ++grabs; return pixels(); });
                require(entry.start() && QFile::exists(root.path() + "/visual-window-bound"), "profile witness before delivery");
                // Opposite synthetic signal cannot satisfy this fixed profile.
                naturalSignals.unblock();
                if (profile == FrameSignal::FrameSwapped) window->afterFrameEnd(); else window->frameSwapped();
                naturalSignals.reblock();
                pump(); require(!QFile::exists(root.path() + "/visual-frame-ready"), "opposite signal ignored");
                if (QByteArray(mode) == "expiry") {
                    entry.expireForTest(); pump(); require(entry.terminal() && grabs == 0, "profile original deadline");
                    continue;
                }
                naturalSignals.unblock();
                std::thread emitter([&] {
                    for (int i = 0; i < 3; ++i) {
                        if (profile == FrameSignal::FrameSwapped) window->frameSwapped(); else window->afterFrameEnd();
                    }
                }); emitter.join(); naturalSignals.reblock();
                require(!QFile::exists(root.path() + "/visual-frame-ready"), "profile queued GUI delivery");
                if (QByteArray(mode) == "premature") put(root.path() + "/visual-request", nonce + ' ' + id);
                if (QByteArray(mode) == "context") window->hide();
                if (QByteArray(mode) == "cancel") put(root.path() + "/visual-cancel", "x");
                pump();
                if (QByteArray(mode) == "premature" || QByteArray(mode) == "context" || QByteArray(mode) == "cancel") {
                    require(entry.terminal() && grabs == 0 && !QFile::exists(root.path() + "/visual-frame-ready"), "profile pre-marker guards");
                    continue;
                }
                const auto bytes = read(root.path() + "/visual-frame-ready");
                const auto j = QJsonDocument::fromJson(bytes).object();
                require(bytes.size() <= 512 && j.size() == 8 && j["stage"] == "frame-stage-observed" &&
                    j["signal"] == (profile == FrameSignal::FrameSwapped ? "frameSwapped" : "afterFrameEnd") &&
                    j["gui_delivery_observed"].toBool() && !j["native_authority"].toBool(true), "profile exact literal/domain");
                if (QByteArray(mode) == "bound-cancel") {
                    require(QFile::remove(root.path() + "/visual-window-bound"), "profile remove bound proof");
                }
                put(root.path() + "/visual-request", nonce + ' ' + id); pump();
                require(entry.terminal() && grabs == (QByteArray(mode) == "valid" ? 1 : 0), "profile one grab/bound guard");
                naturalSignals.unblock(); window->frameSwapped(); window->afterFrameEnd(); naturalSignals.reblock(); pump();
                require(grabs == (QByteArray(mode) == "valid" ? 1 : 0), "profile no rearm");
            }
        }
        for (auto profile : {FrameSignal::FrameSwapped, static_cast<FrameSignal>(99)}) {
            QTemporaryDir root;
            visual_capture::Entry entry(&app, root.path(), nonce, nullptr, false, false, profile);
            require(!entry.start() && entry.terminal() && !QFile::exists(root.path() + "/visual-ready"), "profile runtime dependency/domain refusal before ready");
        }
        QTextStream(stdout) << "PASS fixed signal: both exact literals/opposite signal/queued latch/early request/current context/cancel/bound proof/expiry/one grab/runtime refusal (synthetic only)\n";
        return 0;
    }
    if (argc == 2 && QByteArray(argv[1]) == "binding-witness-only") {
        const QByteArray nonce(32, 'a'), id = visual_capture::selfIdentity();
        const auto prepare = [&](QTemporaryDir &root) {
            require(root.isValid() && ::chmod(root.path().toLocal8Bit().constData(), 0700) == 0, "witness fixture root");
            put(root.path() + "/owner", nonce); put(root.path() + "/attempt.identity", id);
        };
        const auto marker = [&](const QString &path, const char *stage, bool bound) {
            const auto bytes = read(path); const auto j = QJsonDocument::fromJson(bytes).object();
            require(bytes.size() <= 512 && j.size() == 8 && j.value("stage").toString() == stage &&
                j["window_bound"].isBool() && j["window_bound"].toBool() == bound &&
                j["frame_observer_installed"].isBool() && j["frame_observer_installed"].toBool() == bound &&
                !j["native_authority"].toBool(true), "exact historical-only witness domain");
        };
        {
            QTemporaryDir root; prepare(root);
            visual_capture::Entry entry(&app, root.path(), nonce, nullptr, true, true); int grabs = 0;
            entry.grabForTest([&](QQuickWindow *) { ++grabs; return pixels(); });
            require(entry.start(), "initial eligible witness");
            marker(root.path() + "/visual-window-bound", "window-bound-observer-installed", true);
            require(!QFile::exists(root.path() + "/visual-binding-status"), "immediate bound no invented waiting");
            put(root.path() + "/visual-request", nonce + ' ' + id); pump();
            require(entry.terminal() && grabs == 0 && !QFile::exists(root.path() + "/visual-frame-ready") &&
                !QFile::exists(root.path() + "/visual-image.png"), "bound witness alone never admits grab");
        }
        {
            QWindow other; other.resize(20, 20); other.show(); other.requestActivate(); pump();
            require(QGuiApplication::focusWindow() == &other, "deferred witness non-Quick focus");
            QTemporaryDir root; prepare(root);
            visual_capture::Entry entry(&app, root.path(), nonce, nullptr, true, true); int grabs = 0;
            entry.grabForTest([&](QQuickWindow *) { ++grabs; return pixels(); });
            require(entry.start(), "initial deferred witness");
            marker(root.path() + "/visual-binding-status", "waiting-first-eligible-window", false);
            const auto waiting = read(root.path() + "/visual-binding-status");
            window->requestActivate(); pump();
            marker(root.path() + "/visual-window-bound", "window-bound-observer-installed", true);
            require(read(root.path() + "/visual-binding-status") == waiting, "waiting history not overwritten");
            std::thread emitter([window] { window->afterFrameEnd(); }); emitter.join(); pump();
            put(root.path() + "/visual-request", nonce + ' ' + id); pump();
            require(entry.terminal() && grabs == 1 && QFile::exists(root.path() + "/visual-frame-ready"),
                "separate frame marker remains necessary and sufficient for owned guarded grab");
        }
        window->requestActivate(); pump();
        {
            QQmlEngine secondEngine; QQmlComponent second(&secondEngine);
            second.setData("import QtQuick\nWindow { width: 20; height: 20; visible: true }", QUrl("owned-rejected-binding.qml"));
            std::unique_ptr<QObject> other(second.create()); require(bool(other), "owned second engine");
            QTemporaryDir root; prepare(root);
            visual_capture::Entry entry(&app, root.path(), nonce, nullptr, true, true);
            entry.start();
            require(entry.terminal() && !QFile::exists(root.path() + "/visual-window-bound"), "initial ambiguous binding rejected");
            marker(root.path() + "/visual-binding-status", "binding-rejected", false);
        }
        window->requestActivate(); pump();
        {
            QTemporaryDir root; prepare(root);
            visual_capture::Entry entry(&app, root.path(), nonce, nullptr, true, true);
            entry.postLinkForTest([&] { put(root.path() + "/visual-cancel", "x"); });
            entry.start();
            struct stat st{};
            require(entry.terminal() && ::stat((root.path() + "/visual-window-bound").toLocal8Bit().constData(), &st) == 0 &&
                st.st_nlink == 2 && QFile::exists(root.path() + "/.pending-window-bound") &&
                !QFile::exists(root.path() + "/visual-frame-ready"), "canceled bound witness invalid and terminal");
        }
        {
            QTemporaryDir root; prepare(root);
            visual_capture::Entry entry(&app, root.path(), nonce, nullptr, false, true);
            require(!entry.start() && entry.terminal() && !QFile::exists(root.path() + "/visual-ready"),
                "runtime witness cannot enable missing passive mode");
        }
        QTextStream(stdout) << "PASS NEW binding witness immediate/deferred/rejected/history/cancel/dependency/no-frame-authority controls\n";
        return 0;
    }
    if (argc == 2 && QByteArray(argv[1]) == "frame-interleaving-only") {
        QTemporaryDir root; require(::chmod(root.path().toLocal8Bit().constData(), 0700) == 0, "interleaving root");
        const QByteArray nonce(32, 'a'), id = visual_capture::selfIdentity();
        put(root.path() + "/owner", nonce); put(root.path() + "/attempt.identity", id);
        visual_capture::Entry entry(&app, root.path(), nonce, nullptr, true); int grabs = 0;
        entry.grabForTest([&](QQuickWindow *) { ++grabs; return pixels(); });
        require(entry.start(), "interleaving entry setup"); pump(); // initial check sees no request
        std::thread emitter([window] { window->afterFrameEnd(); }); emitter.join(); // queued before watcher
        put(root.path() + "/visual-request", nonce + ' ' + id);
        pump();
        require(entry.terminal() && grabs == 0 && !QFile::exists(root.path() + "/visual-frame-ready") &&
            !QFile::exists(root.path() + "/visual-image.png") &&
            QJsonDocument::fromJson(read(root.path() + "/visual-result")).object().value("stage").toString() == "frame-not-admitted",
            "queued signal before watcher cannot admit premature request");
        window->afterFrameEnd(); pump(); require(grabs == 0, "interleaving refusal sticky");
        QTextStream(stdout) << "PASS deterministic frame delivery queued before request watcher: premature request refused/no marker/no grab\n";
        return 0;
    }
    if (startupTrial) {
        if (frameStartup) {
            // Signal injection is fixture-only; the preloaded entry has no test hooks.
            std::thread emitter([window] { window->afterFrameEnd(); }); emitter.join(); pump();
            const auto marker = QJsonDocument::fromJson(read(startupRoot + "/visual-frame-ready")).object();
            require(marker.size() == 8 && marker["gui_delivery_observed"].toBool() &&
                !marker["native_authority"].toBool(true), "actual passive preload bounded marker");
            put(startupRoot + "/visual-request", QByteArray(32, 'a') + ' ' + visual_capture::selfIdentity()); pump();
        }
        const auto j = QJsonDocument::fromJson(read(startupRoot + "/visual-result")).object();
        const auto png = read(startupRoot + "/visual-image.png");
        require(j["stage"] == "visual-image-observed" && !j["native_authority"].toBool(true) &&
            j["png_sha256"].toString().toLatin1() == QCryptographicHash::hash(png, QCryptographicHash::Sha256).toHex(),
            "actual owned preload/startup public grab receipt");
        require(QDir(startupRoot).removeRecursively(), "exact owned startup cleanup");
        QTextStream(stdout) << (frameStartup ? "PASS actual owned passive preload/queued synthetic frame/public GUI grab\n" :
            "PASS actual owned preload/startup public GUI capture\n");
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
    const auto frameTrial = [&](const char *mode) {
        QTemporaryDir root; require(root.isValid() && ::chmod(root.path().toLocal8Bit().constData(), 0700) == 0, "frame fixture root");
        const QByteArray nonce(32, 'a'), id = visual_capture::selfIdentity();
        put(root.path() + "/owner", nonce); put(root.path() + "/attempt.identity", id);
        visual_capture::Entry entry(&app, root.path(), nonce, nullptr, true); int grabs = 0;
        entry.grabForTest([&](QQuickWindow *) { ++grabs; return pixels(); });
        if (QByteArray(mode) == "marker-cancel") entry.postLinkForTest([&] { put(root.path() + "/visual-cancel", "x"); });
        require(entry.start(), "passive frame start");
        if (QByteArray(mode) == "premature") {
            put(root.path() + "/visual-request", nonce + ' ' + id); pump();
            require(entry.terminal() && grabs == 0 &&
                QJsonDocument::fromJson(read(root.path() + "/visual-result")).object()["stage"] == "frame-not-admitted",
                "premature request consumed without grab");
            window->afterFrameEnd(); pump();
            require(!QFile::exists(root.path() + "/visual-frame-ready"), "no rearm after premature request");
            return;
        }
        if (QByteArray(mode) == "no-signal") {
            pump(); require(!QFile::exists(root.path() + "/visual-frame-ready"), "no historical flag substitute");
            entry.expireForTest(); pump(); require(entry.terminal() && grabs == 0, "original expiry without signal");
            return;
        }
        if (QByteArray(mode) == "cancel") put(root.path() + "/visual-cancel", "x");
        if (QByteArray(mode) == "context") window->hide();
        // Fixture-only signal injection from another thread; producer grabs only on GUI.
        std::thread emitter([window] { window->afterFrameEnd(); window->afterFrameEnd(); window->afterFrameEnd(); });
        emitter.join();
        require(!QFile::exists(root.path() + "/visual-frame-ready"), "queued delivery is not synchronous rendering-thread work");
        pump();
        if (QByteArray(mode) == "cancel" || QByteArray(mode) == "context" || QByteArray(mode) == "marker-cancel") {
            require(entry.terminal() && grabs == 0 && !QFile::exists(root.path() + "/visual-image.png"), "frame guard loss no grab");
            if (QByteArray(mode) == "marker-cancel") {
                struct stat st{}; require(::stat((root.path() + "/visual-frame-ready").toLocal8Bit().constData(), &st) == 0 &&
                    st.st_nlink == 2 && QFile::exists(root.path() + "/.pending-frame-ready"), "canceled marker invalid two-link output");
            }
        } else {
            const auto bytes = read(root.path() + "/visual-frame-ready");
            const auto marker = QJsonDocument::fromJson(bytes).object();
            require(bytes.size() <= 512 && marker.size() == 8 && marker["stage"] == "frame-stage-observed" &&
                marker["signal"] == "afterFrameEnd" && marker["gui_delivery_observed"].toBool() &&
                !marker["native_authority"].toBool(true), "exact bounded ordering-only frame marker");
            put(root.path() + "/visual-request", nonce + ' ' + id); pump();
            require(entry.terminal() && grabs == 1 &&
                QJsonDocument::fromJson(read(root.path() + "/visual-result")).object()["stage"] == "visual-image-observed",
                "one latched signal admits one grab");
            window->afterFrameEnd(); pump(); require(grabs == 1, "already queued/late signals cannot redispatch");
        }
        window->show(); window->requestActivate(); pump();
    };
    for (auto mode : {"no-signal", "premature", "valid", "cancel", "context", "marker-cancel"}) frameTrial(mode);
    {
        QTemporaryDir root; require(::chmod(root.path().toLocal8Bit().constData(), 0700) == 0, "late-focus root");
        const QByteArray nonce(32, 'a'), id = visual_capture::selfIdentity();
        put(root.path() + "/owner", nonce); put(root.path() + "/attempt.identity", id);
        QWindow other; other.resize(20, 20); other.show(); other.requestActivate(); pump();
        require(QGuiApplication::focusWindow() == &other, "owned non-Quick focus transition established");
        visual_capture::Entry entry(&app, root.path(), nonce, nullptr, true);
        require(entry.start(), "no initial eligible window waits for focus event");
        window->requestActivate(); pump();
        window->afterFrameEnd(); pump();
        require(QFile::exists(root.path() + "/visual-frame-ready"), "public focus event binds first eligible window");
        put(root.path() + "/visual-cancel", "x"); pump(); require(entry.terminal(), "late-focus fixture stop");
    }
    {
        QQmlComponent other(&engine); other.setData("import QtQuick\nWindow { width: 20; height: 20; visible: true }", QUrl("owned-destroyed.qml"));
        std::unique_ptr<QObject> doomed(other.create()); auto *w = qobject_cast<QQuickWindow *>(doomed.get());
        require(w, "owned disposable window"); w->requestActivate(); pump();
        QTemporaryDir root; require(::chmod(root.path().toLocal8Bit().constData(), 0700) == 0, "destruction root");
        const QByteArray nonce(32, 'a'); put(root.path() + "/owner", nonce); put(root.path() + "/attempt.identity", visual_capture::selfIdentity());
        visual_capture::Entry entry(&app, root.path(), nonce, nullptr, true);
        require(entry.start(), "bound disposable window");
        w->afterFrameEnd(); doomed.reset(); pump();
        require(entry.terminal() && !QFile::exists(root.path() + "/visual-frame-ready"), "queued callback inert after window destruction");
        window->requestActivate(); pump();
    }
    {
        QTemporaryDir root; require(::chmod(root.path().toLocal8Bit().constData(), 0700) == 0, "natural frame root");
        const QByteArray nonce(32, 'a'); put(root.path() + "/owner", nonce); put(root.path() + "/attempt.identity", visual_capture::selfIdentity());
        visual_capture::Entry entry(&app, root.path(), nonce, nullptr, true);
        require(entry.start(), "owned natural signal observation setup");
        window->setColor(Qt::black); pump(); // Only the owned fixture changes scene state.
        const bool observed = QFile::exists(root.path() + "/visual-frame-ready");
        QTextStream(stdout) << (observed ? "OBSERVED owned software natural afterFrameEnd (no native freshness claim)\n" :
            "NOT OBSERVED owned software natural afterFrameEnd; original expiry remains permitted\n");
        entry.expireForTest(); pump(); require(entry.terminal(), "natural observation original expiry");
    }
    QTextStream(stdout) << "PASS visual core/sink/original public grab and focused entry quotas/context/nested expiry/publication\n";
    QTextStream(stdout) << "PASS passive queued frame marker/early-request/expiry/context/cancel/one-grab controls (synthetic signal delivery)\n";
}
