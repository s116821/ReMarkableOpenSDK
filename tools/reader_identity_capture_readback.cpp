// Standalone read-only research collector. Full target/provider/actor admission
// belongs to the separately frozen operator, before AND after this process.
#include "reader_capture_pair_read.h"
#include <QElapsedTimer>
#include <QRegularExpression>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <climits>
#include <limits>
#include <poll.h>
#include <signal.h>

namespace capture_process_readback {
using Expected = reader_capture_pair_read::Expected;
using Result = reader_capture_pair_read::Result;
using Fd = reader_capture_pair_read::detail::Fd;
inline constexpr qint64 CollectionMs = 10000;
inline bool decimal(const QByteArray &value, quint64 &number) {
    if (!reader_capture_pair_read::detail::decimal(value)) return false;
    bool okay = false;
    number = value.toULongLong(&okay);
    return okay;
}
inline bool validBoot(const QByteArray &boot) {
    static const QRegularExpression pattern(QStringLiteral("^[0-9a-f]{8}(-[0-9a-f]{4}){3}-[0-9a-f]{12}$"));
    return boot.size() == 36 && pattern.match(QString::fromLatin1(boot)).hasMatch() &&
        boot != QByteArrayLiteral("00000000-0000-0000-0000-000000000000");
}
inline bool validExpected(const Expected &expected) {
    quint64 pid = 0, start = 0;
    return reader_capture_pair_read::detail::valid(expected) && decimal(expected.pid, pid) &&
        pid > 1 && pid <= INT_MAX && decimal(expected.start, start);
}
inline bool bootMilliseconds(quint64 &value) {
    struct timespec now{};
    if (::clock_gettime(CLOCK_BOOTTIME, &now) || now.tv_sec < 0 || now.tv_nsec < 0 || now.tv_nsec >= 1000000000 ||
        quint64(now.tv_sec) > (std::numeric_limits<quint64>::max() - 999) / 1000) return false;
    value = quint64(now.tv_sec) * 1000 + quint64(now.tv_nsec) / 1000000;
    return true;
}
inline QByteArray procBytes(const QByteArray &path, int cap) {
    Fd fd;
    fd.value = ::open(path.constData(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd.value < 0 || cap < 1 || cap > 4096) return {};
    char bytes[4097], extra;
    const auto n = ::read(fd.value, bytes, size_t(cap + 1));
    const bool okay = n > 0 && n <= cap && ::read(fd.value, &extra, 1) == 0;
    const bool closed = fd.close();
    return okay && closed ? QByteArray(bytes, int(n)) : QByteArray{};
}
struct Control {
    Fd fd;
    QByteArray name, bytes;
    struct stat snapshot{};
    bool unchanged(int root, uid_t owner) const {
        struct stat current{}, named{};
        if (fd.value < 0 || ::fstat(fd.value, &current) || !reader_capture_pair_read::detail::same(current, snapshot) ||
            ::fstatat(root, name.constData(), &named, AT_SYMLINK_NOFOLLOW) ||
            !reader_capture_pair_read::detail::same(named, snapshot) || snapshot.st_uid != owner) return false;
        char data[129];
        const auto n = ::pread(fd.value, data, sizeof data, 0);
        return n == bytes.size() && QByteArray(data, int(n)) == bytes &&
            ::fstat(fd.value, &current) == 0 && reader_capture_pair_read::detail::same(current, snapshot) &&
            ::fstatat(root, name.constData(), &named, AT_SYMLINK_NOFOLLOW) == 0 &&
            reader_capture_pair_read::detail::same(named, snapshot);
    }
    bool bind(int root, const char *inputName, uid_t owner) {
        name = QByteArray(inputName);
        fd.value = ::openat(root, inputName, O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC);
        if (fd.value < 0 || ::fstat(fd.value, &snapshot) || !S_ISREG(snapshot.st_mode) ||
            snapshot.st_uid != owner || (snapshot.st_mode & 07777) != 0600 || snapshot.st_nlink != 1 ||
            snapshot.st_size < 1 || snapshot.st_size > 128) return false;
        char data[129];
        const auto n = ::pread(fd.value, data, sizeof data, 0);
        if (n != snapshot.st_size) return false;
        bytes = QByteArray(data, int(n));
        return unchanged(root, owner);
    }
};
class Admission final {
public:
    Admission(QString rootPath, Expected expected, QByteArray expectedBoot, quint64 beforeRequestMs)
        : path_(std::move(rootPath)), expected_(std::move(expected)), boot_(std::move(expectedBoot)), before_(beforeRequestMs) {
        elapsed_.start();
    }
    Admission(const Admission &) = delete;
    Admission &operator=(const Admission &) = delete;
    bool start() {
        if (started_) return refuse("already-started");
        started_ = true;
        try {
            if (!validExpected(expected_) || !validBoot(boot_) || before_ == 0 ||
                before_ > std::numeric_limits<quint64>::max() - 30000) return refuse("arguments");
            root_.value = ::open(path_.toLocal8Bit().constData(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
            if (root_.value < 0 || ::fstat(root_.value, &rootStat_) || !S_ISDIR(rootStat_.st_mode) ||
                rootStat_.st_uid != ::geteuid() || (rootStat_.st_mode & 07777) != 0700 ||
                !deadline_.bind(root_.value, "deadline", rootStat_.st_uid) ||
                !window_.bind(root_.value, "candidate.window", rootStat_.st_uid)) return refuse("control-binding");
            if (!window_.bytes.endsWith('\n') || !deadline_.bytes.endsWith('\n')) return refuse("control-format");
            const auto parts = window_.bytes.chopped(1).split(' ');
            quint64 armed = 0, recovery = 0, begin = 0, end = 0;
            if (parts.size() != 5 || parts[0] != expected_.nonce || !decimal(parts[1], armed) || !decimal(parts[2], recovery) ||
                !decimal(parts[3], begin) || !decimal(parts[4], end) ||
                armed > std::numeric_limits<quint64>::max() - 360 || recovery != armed + 360 ||
                begin < armed || begin > std::numeric_limits<quint64>::max() - 90 || end != begin + 90 ||
                recovery < 185 || end > recovery - 185 ||
                end > std::numeric_limits<quint64>::max() / 1000 ||
                deadline_.bytes != parts[1] + ' ' + parts[2] + '\n') return refuse("control-clocks");
            begin_ = begin * 1000; end_ = end * 1000;
            if (before_ < begin_ || before_ >= end_) return refuse("request-clock");
            initialized_ = true;
            return permit();
        } catch (...) { return refuse("exception"); }
    }
    bool permit() {
        if (revoked_ || !initialized_) return false;
        try {
            quint64 now = 0;
            if (!clockOkay(now)) return refuse("deadline");
            struct stat current{};
            if (::lstat(path_.toLocal8Bit().constData(), &current) || !S_ISDIR(current.st_mode) ||
                current.st_dev != rootStat_.st_dev || current.st_ino != rootStat_.st_ino ||
                current.st_uid != rootStat_.st_uid || current.st_mode != rootStat_.st_mode ||
                !deadline_.unchanged(root_.value, rootStat_.st_uid) || !window_.unchanged(root_.value, rootStat_.st_uid))
                return refuse("control-changed");
            for (const char *name : {"selected-cancel", "restore.claim", "stop.request"}) {
                if (::fstatat(root_.value, name, &current, AT_SYMLINK_NOFOLLOW) == 0 || errno != ENOENT)
                    return refuse("stopped");
            }
            if (procBytes(QByteArrayLiteral("/proc/sys/kernel/random/boot_id"), 64) != boot_ + '\n') return refuse("boot");
            const auto stat = procBytes(QByteArrayLiteral("/proc/") + expected_.pid + "/stat", 4096);
            const int open = stat.indexOf('('), close = stat.lastIndexOf(')');
            if (open < 1 || close <= open || stat.left(open).trimmed() != expected_.pid) return refuse("process");
            const auto fields = stat.mid(close + 1).simplified().split(' ');
            if (fields.size() <= 19 || fields[0].size() != 1 || fields[0] == "Z" || fields[0] == "X" || fields[0] == "x" ||
                fields[19] != expected_.start) return refuse("process");
            if (!clockOkay(now)) return refuse("deadline");
            return true;
        } catch (...) { return refuse("exception"); }
    }
    int rootFd() const { return root_.value; }
    QString refusal() const { return refusal_; }
private:
    bool clockOkay(quint64 &now) const {
        return elapsed_.isValid() && elapsed_.elapsed() >= 0 && elapsed_.elapsed() < CollectionMs &&
            bootMilliseconds(now) && now >= before_ && now >= begin_ && now < end_ && now - before_ < 30000;
    }
    bool refuse(const char *reason) { revoked_ = true; if (refusal_.isEmpty()) refusal_ = QString::fromLatin1(reason); return false; }
    QString path_, refusal_;
    Expected expected_;
    QByteArray boot_;
    quint64 before_ = 0, begin_ = 0, end_ = 0;
    Fd root_;
    Control deadline_, window_;
    struct stat rootStat_{};
    QElapsedTimer elapsed_;
    bool started_ = false, initialized_ = false, revoked_ = false;
};
inline QByteArray makeFrame(const Result &result, const Expected &expected, const QByteArray &boot) {
    if (!result.observed || !result.attempted || !result.refusal.isEmpty() || !validExpected(expected) || !validBoot(boot) ||
        result.scalar.isEmpty() || result.scalar.size() > reader_capture_pair_read::ScalarCap ||
        result.image.isEmpty() || result.image.size() > reader_capture_pair_read::ImageCap) return {};
    QByteArray frame = QByteArrayLiteral("RMB_CAPTURE_PAIR_V1 ") + expected.nonce + ' ' + expected.pid + ' ' + expected.start +
        ' ' + boot + ' ' + QByteArray::number(result.scalar.size()) + ' ' + QByteArray::number(result.image.size()) + '\n';
    frame += result.scalar; frame += result.image; frame += QByteArrayLiteral("\nRMB_CAPTURE_PAIR_END\n");
    return frame;
}
inline bool writeFrameOnce(int fd, const QByteArray &frame) {
    return !frame.isEmpty() && frame.size() <= reader_capture_pair_read::ScalarCap + reader_capture_pair_read::ImageCap + 256 &&
        ::write(fd, frame.constData(), size_t(frame.size())) == frame.size();
}
// Trusted serialized admission and owned output FD, not an arbitrary caller grant.
inline bool armCollectionAlarm() {
    struct sigaction action{}; action.sa_handler = SIG_DFL;
    sigset_t unblocked{};
    if (::sigemptyset(&action.sa_mask) || ::sigaction(SIGALRM, &action, nullptr) ||
        ::sigemptyset(&unblocked) || ::sigaddset(&unblocked, SIGALRM) ||
        ::sigprocmask(SIG_UNBLOCK, &unblocked, nullptr)) return false;
    ::alarm(12); return true;
}
template<class Permit> bool writeFrameBounded(int fd, const QByteArray &frame, Permit &permit) {
    if (frame.isEmpty() || frame.size() > reader_capture_pair_read::ScalarCap + reader_capture_pair_read::ImageCap + 256 || !permit()) return false;
    const int flags = ::fcntl(fd, F_GETFL);
    if (flags < 0 || ::fcntl(fd, F_SETFL, flags | O_NONBLOCK)) return false;
    qsizetype sent = 0;
    while (sent < frame.size()) {
        if (!permit()) return false;
        const auto count = ::write(fd, frame.constData() + sent, size_t(frame.size() - sent));
        if (count > 0) { sent += count; continue; }
        if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
            pollfd output{fd, POLLOUT, 0};
            if (::poll(&output, 1, 2) < 0 && errno != EINTR) return false;
            continue;
        }
        return false;
    }
    return permit();
}
} // namespace capture_process_readback

#ifndef READER_CAPTURE_READBACK_OWNED_HELPERS_ONLY
#if !defined(__arm__) || !defined(__ARM_PCS_VFP) || __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error The production readback profile is ARM32 little-endian hard-float only.
#endif
int main(int argc, char **argv) {
    using namespace capture_process_readback;
    try {
        if (!armCollectionAlarm()) return 90;
        if (argc != 6 || ::geteuid() != 0 || QByteArray(qVersion()) != "6.10.3" ||
            std::getenv("LD_PRELOAD") || std::getenv("LD_BIND_NOW")) return 90;
        const Expected expected{QByteArray(argv[1]), QByteArray(argv[2]), QByteArray(argv[3])};
        const QByteArray boot(argv[4]);
        quint64 before = 0;
        if (!validExpected(expected) || !validBoot(boot) || !decimal(QByteArray(argv[5]), before)) return 90;
        Admission admission(QStringLiteral("/run/rmb-qt-probe-") + QString::fromLatin1(expected.nonce), expected, boot, before);
        if (!admission.start()) return 90;
        const auto result = reader_capture_pair_read::readPair(admission.rootFd(), expected, [&] { return admission.permit(); });
        if (!result.observed || !admission.permit()) return 90; // After copied Permit destruction.
        const auto frame = makeFrame(result, expected, boot);
        auto permit = [&] { return admission.permit(); };
        if (!writeFrameBounded(STDOUT_FILENO, frame, permit)) return 90;
        return admission.permit() ? 0 : 90;
    } catch (...) { return 90; }
}
#endif
