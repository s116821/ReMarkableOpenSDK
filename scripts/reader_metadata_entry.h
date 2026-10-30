#pragma once
#include "reader_metadata_census.h"
#include <QFileSystemWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace reader_metadata {
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
        if (!absent("census-ready") || !absent("census-result") ||
            !absent(".pending-ready") || !absent(".pending-result")) return terminal_ = true, false;
        connect(&watcher_, &QFileSystemWatcher::directoryChanged, this, [this] { queue(); });
        connect(&timer_, &QTimer::timeout, this, [this] { Result r; r.stage = "setup-expired"; finish(r); });
        connect(app_, &QCoreApplication::aboutToQuit, this, [this] { stop(); });
        if (!watcher_.addPath(path_)) return terminal_ = true, false;
        timer_.setSingleShot(true); timer_.setTimerType(Qt::PreciseTimer);
        const qint64 remaining = 60000 - now();
        if (remaining <= 0) { stop(); return false; }
        timer_.start(int(remaining));
        if (!publish("census-ready", ".pending-ready", nonce_ + ' ' + identity_)) { stop(); return false; }
        queue();
        return true;
    }
    bool terminal() const { return terminal_; }
#ifdef READER_METADATA_TESTING
    void expireForTest() { testOffset_ = 60000; queue(); }
    void postLinkForTest(std::function<void()> hook) { postLink_ = std::move(hook); }
#endif
private:
    qint64 now() const {
#ifdef READER_METADATA_TESTING
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
        return same && closed && absent("census-cancel") && absent("restore.claim") &&
            readExact("owner", nonce_, 32) && readExact("attempt.identity", identity_, 42);
    }
    bool publish(const char *name, const char *pending, const QByteArray &bytes) const {
        if (bytes.isEmpty() || bytes.size() > 1024 || !trusted()) return false;
        const int fd = ::openat(root_, pending, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
        if (fd < 0) return false;
        struct stat st{};
        const bool fileOk = ::fstat(fd, &st) == 0 && S_ISREG(st.st_mode) && st.st_uid == rootStat_.st_uid &&
            (st.st_mode & 07777) == 0600 && st.st_nlink == 1 && st.st_size == 0;
        const ssize_t n = fileOk ? ::write(fd, bytes.constData(), size_t(bytes.size())) : -1;
        const bool closed = ::close(fd) == 0;
        // Publish only after a full write and successful close. A retained pending
        // file is unknown, never a ready/result receipt and never overwritten.
        if (n != bytes.size() || !closed || !trusted()) return false;
        if (::linkat(root_, pending, root_, name, 0) != 0) return false;
#ifdef READER_METADATA_TESTING
        if (postLink_ && QByteArray(name) == "census-result") postLink_();
#endif
        // Keep pending until the post-link ownership/cancel guard passes.
        // A failed guard leaves nlink2 + pending, explicitly unknown.
        if (!trusted()) return false;
        if (::unlinkat(root_, pending, 0) != 0) return false; // nlink2 output is invalid
        // The commit follows that sampled guard, not an atomic cancellation
        // fence. The packet consumer must independently bracket the receipt.
        return true;
    }
    void stop() {
        terminal_ = true; timer_.stop();
        const auto paths = watcher_.directories();
        if (!paths.isEmpty()) watcher_.removePaths(paths);
    }
    void finish(Result r) {
        if (terminal_) return;
        stop();
        if (!trusted()) return;
        if (now() >= 60000) r.stage = "setup-expired";
        const auto identity = identity_.trimmed().split(' ');
        const QJsonObject json{{"schema", 1}, {"nonce", QString::fromLatin1(nonce_)},
            {"pid", QString::fromLatin1(identity[0])}, {"start", QString::fromLatin1(identity[1])},
            {"stage", QString::fromLatin1(r.stage)}, {"application_thread", r.applicationThread},
            {"current_window", r.currentWindow}, {"visited", r.visited},
            {"helper_count", r.helpers}, {"container_count", r.containers},
            {"owner_links", r.ownerLinks}, {"metadata", r.metadata},
            {"page_string_domain", r.pageStringDomain}, {"native_authority", false}};
        publish("census-result", ".pending-result", QJsonDocument(json).toJson(QJsonDocument::Compact) + '\n');
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
        if (terminal_) return;
        if (!trusted()) { stop(); return; }
        Result r;
        if (now() >= 60000) { r.stage = "setup-expired"; finish(r); return; }
        if (++checks_ > 32) { r.stage = "request-check-cap"; finish(r); return; }
        if (absent("census-request")) return;
        if (!readExact("census-request", nonce_ + ' ' + identity_, 75)) {
            r.stage = "request-refused"; finish(r); return;
        }
        // No nested event pumping. The entry remains alive through synchronous
        // census and terminal publication; all later watcher callbacks are inert.
        r = census([this] { return !terminal_ && now() < 60000 && trusted(); });
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
    bool started_ = false, terminal_ = false, queued_ = false;
#ifdef READER_METADATA_TESTING
    qint64 testOffset_ = 0;
    std::function<void()> postLink_;
#endif
};
}
