#pragma once
#include "visual_capture_core.h"
#include <QFileSystemWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QElapsedTimer>
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace visual_capture {
inline bool decimal(const QByteArray &s) {
    if (s.isEmpty() || s.size() > 20 || s[0] == '0') return false;
    for (char c : s) if (c < '0' || c > '9') return false;
    bool ok = false; s.toULongLong(&ok); return ok;
}
inline QByteArray selfIdentity() {
    const int fd = ::open("/proc/self/stat", O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0) return {};
    char buffer[4097]; const ssize_t n = ::read(fd, buffer, sizeof buffer);
    const bool closed = ::close(fd) == 0;
    if (!closed || n <= 0 || n > 4096) return {};
    const QByteArray stat(buffer, n);
    const int end = stat.lastIndexOf(')');
    if (end < 0) return {};
    const auto fields = stat.mid(end + 1).simplified().split(' ');
    if (fields.size() <= 19 || !decimal(fields[19])) return {};
    return QByteArray::number(::getpid()) + ' ' + fields[19] + '\n';
}
class Entry final : public QObject {
public:
    Entry(QGuiApplication *app, QString path, QByteArray nonce, QObject *parent = nullptr)
        : QObject(parent ? parent : app), app_(app), path_(std::move(path)), nonce_(std::move(nonce)) {
        elapsed_.start();
    }
    ~Entry() override { if (root_ >= 0) ::close(root_); }
    bool start() {
        if (started_) return false;
        started_ = true;
        if (!app_ || QThread::currentThread() != app_->thread() || thread() != app_->thread() || nonce_.size() != 32)
            return terminal_ = true, false;
        for (char c : nonce_) if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
            return terminal_ = true, false;
        identity_ = selfIdentity();
        if (identity_.isEmpty()) return terminal_ = true, false;
        root_ = ::open(path_.toLocal8Bit().constData(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if (root_ < 0 || ::fstat(root_, &rootStat_) != 0 || !S_ISDIR(rootStat_.st_mode) ||
            (rootStat_.st_mode & 07777) != 0700 || rootStat_.st_uid != ::geteuid() || !trusted())
            return terminal_ = true, false;
        if (!absent("visual-ready") || !absent("visual-result") ||
            !absent(".pending-ready") || !absent(".pending-result") ||
            !absent("visual-image.png") || !absent(".pending-image")) return terminal_ = true, false;
        connect(&watcher_, &QFileSystemWatcher::directoryChanged, this, [this] { queue(); });
        connect(&timer_, &QTimer::timeout, this, [this] { Result r; r.stage = "setup-expired"; finish(r); });
        connect(app_, &QCoreApplication::aboutToQuit, this, [this] { stop(); });
        if (!watcher_.addPath(path_)) return terminal_ = true, false;
        timer_.setSingleShot(true); timer_.setTimerType(Qt::PreciseTimer);
        const qint64 remaining = 60000 - now();
        if (remaining <= 0) { stop(); return false; }
        timer_.start(int(remaining));
        if (!publish("visual-ready", ".pending-ready", nonce_ + ' ' + identity_, {})) { stop(); return false; }
        queue();
        return true;
    }
    bool terminal() const { return terminal_; }
#ifdef VISUAL_CAPTURE_TESTING
    void grabForTest(std::function<QImage(QQuickWindow *)> grab) { grab_ = std::move(grab); }
    void expireForTest() { testOffset_ = 60000; queue(); }
    void postLinkForTest(std::function<void()> hook) { postLink_ = std::move(hook); }
#endif
private:
    qint64 now() const {
#ifdef VISUAL_CAPTURE_TESTING
        return elapsed_.elapsed() + testOffset_;
#else
        return elapsed_.elapsed();
#endif
    }
    bool absent(const char *name) const {
        struct stat st{};
        if (::fstatat(root_, name, &st, AT_SYMLINK_NOFOLLOW) == 0) return false;
        return errno == ENOENT;
    }
    static bool sameFile(const struct stat &a, const struct stat &b) {
        return a.st_dev == b.st_dev && a.st_ino == b.st_ino && a.st_mode == b.st_mode &&
            a.st_uid == b.st_uid && a.st_nlink == b.st_nlink && a.st_size == b.st_size &&
            a.st_mtim.tv_sec == b.st_mtim.tv_sec && a.st_mtim.tv_nsec == b.st_mtim.tv_nsec &&
            a.st_ctim.tv_sec == b.st_ctim.tv_sec && a.st_ctim.tv_nsec == b.st_ctim.tv_nsec;
    }
    bool readExact(const char *name, const QByteArray &expected, int cap) const {
        const int fd = ::openat(root_, name, O_RDONLY | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK);
        if (fd < 0) return false;
        struct stat before{}, after{};
        bool ok = ::fstat(fd, &before) == 0 && S_ISREG(before.st_mode) &&
            (before.st_mode & 07777) == 0600 && before.st_uid == rootStat_.st_uid && before.st_nlink == 1 &&
            before.st_size == expected.size() && before.st_size <= cap;
        char bytes[76];
        const ssize_t n = ok ? ::read(fd, bytes, size_t(cap + 1)) : -1;
        ok = ok && n == expected.size() && QByteArray(bytes, n) == expected &&
            ::fstat(fd, &after) == 0 && sameFile(before, after);
        const bool closed = ::close(fd) == 0;
        return ok && closed;
    }
    bool trusted() const {
        if (root_ < 0 || !app_ || QCoreApplication::closingDown()) return false;
        const int fresh = ::open(path_.toLocal8Bit().constData(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if (fresh < 0) return false;
        struct stat current{};
        const bool same = ::fstat(fresh, &current) == 0 && current.st_dev == rootStat_.st_dev &&
            current.st_ino == rootStat_.st_ino && current.st_uid == rootStat_.st_uid &&
            (current.st_mode & 07777) == 0700;
        const bool closed = ::close(fresh) == 0;
        return same && closed && absent("visual-cancel") && absent("restore.claim") &&
            readExact("owner", nonce_, 32) && readExact("attempt.identity", identity_, 42);
    }
    bool fileBound(const char *name, const struct stat &original, nlink_t links) const {
        struct stat st{};
        return ::fstatat(root_, name, &st, AT_SYMLINK_NOFOLLOW) == 0 && S_ISREG(st.st_mode) &&
            st.st_dev == original.st_dev && st.st_ino == original.st_ino && st.st_uid == rootStat_.st_uid &&
            (st.st_mode & 07777) == 0600 && st.st_nlink == links && st.st_size == original.st_size &&
            st.st_mtim.tv_sec == original.st_mtim.tv_sec && st.st_mtim.tv_nsec == original.st_mtim.tv_nsec;
    }
    bool commit(const char *name, const char *pending, const struct stat &original,
                const std::function<bool()> &context) const {
        const auto guard = [&] { return trusted() && (!context || context()); };
        if (!guard() || !fileBound(pending, original, 1)) return false;
        if (::linkat(root_, pending, root_, name, 0) != 0) return false;
#ifdef VISUAL_CAPTURE_TESTING
        if (postLink_ && QByteArray(name) != "visual-ready") postLink_();
#endif
        if (!guard() || !fileBound(pending, original, 2) || !fileBound(name, original, 2)) return false;
        return ::unlinkat(root_, pending, 0) == 0;
    }
    bool publish(const char *name, const char *pending, const QByteArray &bytes,
                 const std::function<bool()> &context) const {
        if (bytes.isEmpty() || bytes.size() > 1024 || !trusted() || (context && !context())) return false;
        const int fd = ::openat(root_, pending, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
        if (fd < 0) return false;
        struct stat original{}, final{};
        const bool fileOk = ::fstat(fd, &original) == 0 && S_ISREG(original.st_mode) &&
            original.st_uid == rootStat_.st_uid && (original.st_mode & 07777) == 0600 &&
            original.st_nlink == 1 && original.st_size == 0;
        const ssize_t n = fileOk ? ::write(fd, bytes.constData(), size_t(bytes.size())) : -1;
        const bool bound = ::fstat(fd, &final) == 0 && final.st_dev == original.st_dev &&
            final.st_ino == original.st_ino && final.st_size == bytes.size();
        const bool closed = ::close(fd) == 0;
        return n == bytes.size() && bound && closed && commit(name, pending, final, context);
    }
    Result image() {
        Result r; r.stage = "image-output-refused";
        const int fd = ::openat(root_, ".pending-image", O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
        if (fd < 0) return r;
        struct stat original{}, final{};
        const bool good = ::fstat(fd, &original) == 0 && S_ISREG(original.st_mode) &&
            original.st_uid == rootStat_.st_uid && (original.st_mode & 07777) == 0600 &&
            original.st_nlink == 1 && original.st_size == 0;
        Sink sink(fd);
        if (good) {
#ifdef VISUAL_CAPTURE_TESTING
            r = capture([this] { return !terminal_ && now() < 60000 && trusted(); }, sink, grab_);
#else
            r = capture([this] { return !terminal_ && now() < 60000 && trusted(); }, sink);
#endif
        }
        const bool bound = ::fstat(fd, &final) == 0 && final.st_dev == original.st_dev &&
            final.st_ino == original.st_ino && final.st_size == sink.count();
        sink.close();
        const bool closed = ::close(fd) == 0;
        if (QByteArray(r.stage) != "visual-image-observed") return r;
        if (!sink.valid() || !bound || !closed || !r.context || !r.context() ||
            !commit("visual-image.png", ".pending-image", final, r.context)) r.stage = "image-publication-refused";
        return r;
    }
    void stop() {
        terminal_ = true; timer_.stop();
        const auto paths = watcher_.directories();
        if (!paths.isEmpty()) watcher_.removePaths(paths);
    }
    void finish(Result r) {
        if (terminal_ || finishing_) return;
        finishing_ = true;
        if (!trusted()) { stop(); return; }
        if (now() >= 60000) r.stage = "setup-expired";
        const bool success = QByteArray(r.stage) == "visual-image-observed";
        if (success && (!r.context || !r.context())) { stop(); return; }
        const auto identity = identity_.trimmed().split(' ');
        const QJsonObject json{{"schema", 1}, {"nonce", QString::fromLatin1(nonce_)},
            {"pid", QString::fromLatin1(identity[0])}, {"start", QString::fromLatin1(identity[1])},
            {"stage", QString::fromLatin1(r.stage)}, {"application_thread", r.applicationThread},
            {"current_window", r.currentWindow}, {"width", r.width}, {"height", r.height},
            {"storage_bytes", double(r.storage)}, {"encoded_bytes", double(r.encoded)},
            {"device_pixel_ratio", std::isfinite(r.dpr) ? r.dpr : 0},
            {"png_sha256", QString::fromLatin1(r.sha)}, {"native_authority", false}};
        publish("visual-result", ".pending-result", QJsonDocument(json).toJson(QJsonDocument::Compact) + '\n',
                success ? r.context : std::function<bool()>{});
        stop();
    }
    void queue() {
        if (terminal_ || queued_) return;
        queued_ = true;
        QPointer<Entry> weak(this);
        if (!QMetaObject::invokeMethod(this, [weak] {
            if (!weak || weak->terminal_) return;
            weak->queued_ = false;
            weak->check();
        }, Qt::QueuedConnection)) stop();
    }
    void check() {
        if (terminal_ || consumed_) return;
        if (!trusted()) { stop(); return; }
        Result r;
        if (now() >= 60000) { r.stage = "setup-expired"; finish(r); return; }
        if (++checks_ > 32) { r.stage = "request-check-cap"; finish(r); return; }
        if (absent("visual-request")) return;
        if (!readExact("visual-request", nonce_ + ' ' + identity_, 75)) {
            r.stage = "request-refused"; finish(r); return;
        }
        consumed_ = true;
        r = image();
        finish(r);
    }
    QPointer<QGuiApplication> app_;
    QString path_;
    QByteArray nonce_, identity_;
    int root_ = -1, checks_ = 0;
    struct stat rootStat_{};
    QElapsedTimer elapsed_;
    QFileSystemWatcher watcher_;
    QTimer timer_;
    bool started_ = false, terminal_ = false, queued_ = false, consumed_ = false, finishing_ = false;
#ifdef VISUAL_CAPTURE_TESTING
    qint64 testOffset_ = 0;
    std::function<void()> postLink_;
    std::function<QImage(QQuickWindow *)> grab_;
#endif
};
}
