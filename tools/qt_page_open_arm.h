#pragma once
#include <QObject>
#include <QFileSystemWatcher>
#include <QFile>
#include <QDir>
#include <QPointer>
#include <QRegularExpression>
#include <QByteArrayView>
#include <QTimer>
#include <functional>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cerrno>

namespace qml_access {
// One development transaction's setup permission, never page ownership.
class PageOpenArmGate final : public QObject {
public:
    PageOpenArmGate(QObject *parent, QString directory, QString nonce,
                    std::function<bool()> progress, std::function<void(const char *)> completed)
        : QObject(parent), directory_(std::move(directory)), nonce_(std::move(nonce)),
          progress_(std::move(progress)), completed_(std::move(completed)), watcher_(this) {}
    ~PageOpenArmGate() override { disarm(); if (directoryFd_ >= 0) ::close(directoryFd_); }
    static QByteArray processIdentity() {
        QFile stat(QStringLiteral("/proc/self/stat"));
        if (!stat.open(QIODevice::ReadOnly)) return {};
        const QByteArray bytes = stat.read(4097);
        if (bytes.size() > 4096) return {};
        const int end = bytes.lastIndexOf(')');
        if (end < 0) return {};
        const auto fields = bytes.mid(end + 1).simplified().split(' ');
        if (fields.size() <= 19) return {};
        const QByteArray start = fields[19];
        if (start.isEmpty() || start.size() > 20) return {};
        for (char c : start) if (c < '0' || c > '9') return {};
        return QByteArray::number(::getpid()) + ' ' + start;
    }
    QByteArray waitingBytes() const { return nonce_.toLatin1() + ' ' + identity_ + " waiting\n"; }
    QByteArray armBytes() const { return nonce_.toLatin1() + ' ' + identity_ + " open\n"; }
    void disarm() {
        armed_ = false;
        accepted_ = false;
        QObject::disconnect(&watcher_, nullptr, this, nullptr);
        const auto paths = watcher_.directories();
        if (!paths.isEmpty()) watcher_.removePaths(paths);
    }
    bool releaseAllowed() const {
        if (!accepted_ || !progress_() || !sameDirectory() || !admissionOpen()) return false;
        return accepted_ && progress_();
    }
    void begin() {
        if (begun_) return;
        begun_ = true;
        static const QRegularExpression noncePattern(QStringLiteral("^[0-9a-f]{32}$"));
        if (!noncePattern.match(nonce_).hasMatch() || !directory_.startsWith('/') ||
            QDir::cleanPath(directory_) != directory_ || !progress_()) { finish("open-arm-context-refused"); return; }
        identity_ = processIdentity();
        if (identity_.isEmpty()) { finish("open-arm-identity-refused"); return; }
        directoryFd_ = ::open(QFile::encodeName(directory_).constData(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if (directoryFd_ < 0 || ::fstat(directoryFd_, &directoryStat_) != 0 ||
            !S_ISDIR(directoryStat_.st_mode) || directoryStat_.st_uid != ::geteuid() ||
            (directoryStat_.st_mode & 07777) != 0700) { finish("open-arm-directory-refused"); return; }
        struct stat existing{};
        for (const char *name : {"open-arm", "open-waiting", "open-arm.tmp"}) {
            if (::fstatat(directoryFd_, name, &existing, AT_SYMLINK_NOFOLLOW) == 0 || errno != ENOENT) {
                finish("open-arm-stale-refused"); return;
            }
        }
        if (!admissionOpen()) { finish("open-arm-closed-refused"); return; }
        connect(&watcher_, &QFileSystemWatcher::directoryChanged, this, [this](const QString &path) {
            if (armed_ && path == directory_) queueCheck();
        });
        if (!watcher_.addPath(directory_)) { finish("open-arm-observer-refused"); return; }
        armed_ = true;
        if (!progress_() || !sameDirectory() || !admissionOpen()) { finish("open-arm-context-refused"); return; }
        // Observer precedes the marker; immediate check closes its publish race.
        const int marker = ::openat(directoryFd_, "open-waiting", O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
        const QByteArray bytes = waitingBytes();
        if (marker < 0) { finish("open-arm-marker-refused"); return; }
        const bool markerWritten = ::fchmod(marker, 0600) == 0 &&
            ::write(marker, bytes.constData(), static_cast<size_t>(bytes.size())) == bytes.size();
        ::close(marker);
        if (!markerWritten || !progress_()) { finish("open-arm-marker-refused"); return; }
        queueCheck();
    }
private:
    bool admissionOpen() const {
        if (directoryFd_ < 0) return false;
        struct stat guard{};
        for (const char *name : {"entry.closed", "restore.claim"})
            if (::fstatat(directoryFd_, name, &guard, AT_SYMLINK_NOFOLLOW) == 0 || errno != ENOENT) return false;
        return true;
    }
    bool sameDirectory() const {
        struct stat current{};
        const int fd = ::open(QFile::encodeName(directory_).constData(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if (fd < 0) return false;
        const bool same = ::fstat(fd, &current) == 0 && current.st_dev == directoryStat_.st_dev &&
            current.st_ino == directoryStat_.st_ino && current.st_uid == ::geteuid() && (current.st_mode & 07777) == 0700;
        ::close(fd);
        return same;
    }
    void queueCheck() {
        if (!armed_ || queued_) return;
        queued_ = true;
        if (!QMetaObject::invokeMethod(this, [this] {
            queued_ = false;
            if (!armed_) return;
            if (!progress_() || !sameDirectory() || !admissionOpen()) { finish("open-arm-context-refused"); return; }
            struct stat before{}, after{};
            if (::fstatat(directoryFd_, "open-arm", &before, AT_SYMLINK_NOFOLLOW) != 0) {
                if (errno != ENOENT) finish("open-arm-token-refused");
                return;
            }
            const QByteArray expected = armBytes();
            if (!S_ISREG(before.st_mode) || before.st_uid != ::geteuid() ||
                (before.st_mode & 07777) != 0600 || before.st_size != expected.size()) { finish("open-arm-token-refused"); return; }
            const int fd = ::openat(directoryFd_, "open-arm", O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
            if (fd < 0 || ::fstat(fd, &after) != 0 || before.st_dev != after.st_dev || before.st_ino != after.st_ino) {
                if (fd >= 0) ::close(fd);
                finish("open-arm-token-refused"); return;
            }
            char buffer[128];
            const ssize_t count = ::read(fd, buffer, sizeof buffer);
            struct stat finalStat{};
            const bool stable = ::fstat(fd, &finalStat) == 0 && after.st_size == finalStat.st_size &&
                after.st_mtim.tv_sec == finalStat.st_mtim.tv_sec && after.st_mtim.tv_nsec == finalStat.st_mtim.tv_nsec &&
                finalStat.st_uid == ::geteuid() && (finalStat.st_mode & 07777) == 0600;
            ::close(fd);
            if (!stable || count != expected.size() || QByteArray(buffer, static_cast<int>(count)) != expected) {
                finish("open-arm-token-refused"); return;
            }
            if (!progress_() || !sameDirectory() || !admissionOpen()) { finish("open-arm-context-refused"); return; }
            finish("open-arm-accepted");
        }, Qt::QueuedConnection)) finish("open-arm-dispatch-refused");
    }
    void finish(const char *stage) {
        if (finished_) return;
        finished_ = true;
        disarm();
        accepted_ = QByteArrayView(stage) == QByteArrayView("open-arm-accepted");
        QTimer::singleShot(0, this, [this, stage] { completed_(stage); });
    }
    QString directory_, nonce_;
    QByteArray identity_;
    std::function<bool()> progress_;
    std::function<void(const char *)> completed_;
    QFileSystemWatcher watcher_;
    int directoryFd_ = -1;
    struct stat directoryStat_{};
    bool begun_ = false, armed_ = false, queued_ = false, finished_ = false, accepted_ = false;
};
}
