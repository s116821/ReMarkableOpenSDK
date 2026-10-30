// Research-only closed-file publication. No observer, input, or navigation calls.
#include <cerrno>
#include <climits>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <linux/fs.h>
#include <string>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace {
bool decimal(const char *s) {
    const size_t n = std::strlen(s);
    if (!n || n > 20 || s[0] == '0') return false;
    for (size_t i = 0; i < n; ++i) if (s[i] < '0' || s[i] > '9') return false;
    return true;
}
bool same(const struct stat &a, const struct stat &b) {
    return a.st_dev == b.st_dev && a.st_ino == b.st_ino && a.st_uid == b.st_uid && a.st_mode == b.st_mode &&
        a.st_nlink == b.st_nlink && a.st_size == b.st_size &&
        a.st_mtim.tv_sec == b.st_mtim.tv_sec && a.st_mtim.tv_nsec == b.st_mtim.tv_nsec &&
        a.st_ctim.tv_sec == b.st_ctim.tv_sec && a.st_ctim.tv_nsec == b.st_ctim.tv_nsec;
}
bool absent(int root, const char *name) {
    struct stat st{};
    return ::fstatat(root, name, &st, AT_SYMLINK_NOFOLLOW) != 0 && errno == ENOENT;
}
bool exact(int root, const char *name, const std::string &expected, struct stat *identity = nullptr) {
    const int fd = ::openat(root, name, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return false;
    struct stat before{}, after{}, named{};
    bool ok = expected.size() <= 75 && ::fstat(fd, &before) == 0 && S_ISREG(before.st_mode) &&
        before.st_uid == 0 && (before.st_mode & 07777) == 0600 && before.st_nlink == 1 &&
        before.st_size == static_cast<off_t>(expected.size());
    char bytes[76]; const ssize_t n = ok ? ::read(fd, bytes, sizeof bytes) : -1;
    ok = ok && n == static_cast<ssize_t>(expected.size()) && std::memcmp(bytes, expected.data(), expected.size()) == 0 &&
        ::fstat(fd, &after) == 0 && same(before, after) &&
        ::fstatat(root, name, &named, AT_SYMLINK_NOFOLLOW) == 0 && same(after, named);
    const bool closed = ::close(fd) == 0;
    if (ok && closed && identity) *identity = after;
    return ok && closed;
}
} // namespace

int main(int argc, char **argv) {
    // Bound this one-shot process independently of transport/PID discovery.
    // A signal exit is an unknown publication outcome and must never be retried.
    if (::signal(SIGALRM, SIG_DFL) == SIG_ERR) return 90;
    ::alarm(5);
    if (argc != 5 || ::geteuid() != 0 || argv[1][0] != '/' || std::strlen(argv[1]) > 512 ||
        std::strlen(argv[2]) != 32 || !decimal(argv[3]) || !decimal(argv[4])) return 90;
    for (const char *c = argv[2]; *c; ++c) if (!((*c >= '0' && *c <= '9') || (*c >= 'a' && *c <= 'f'))) return 90;
    const std::string nonce(argv[2]), identity = std::string(argv[3]) + ' ' + argv[4] + '\n';
    const std::string request = nonce + ' ' + identity;
    if (identity.size() > 42 || request.size() > 75) return 90;
    const int root = ::open(argv[1], O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (root < 0) return 90;
    struct stat initial{};
    if (::fstat(root, &initial) != 0 || !S_ISDIR(initial.st_mode) || initial.st_uid != 0 || (initial.st_mode & 07777) != 0700) {
        ::close(root); return 90;
    }
    const auto trusted = [&] {
        const int fresh = ::open(argv[1], O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        if (fresh < 0) return false;
        struct stat st{};
        const bool ok = ::fstat(fresh, &st) == 0 && st.st_dev == initial.st_dev && st.st_ino == initial.st_ino &&
            st.st_uid == 0 && (st.st_mode & 07777) == 0700;
        const bool closed = ::close(fresh) == 0;
        return ok && closed && exact(root, "owner", nonce) && exact(root, "attempt.identity", identity) &&
            absent(root, "restore.claim") && absent(root, "selected-cancel");
    };
    if (!trusted() || !absent(root, "selected-request") || !absent(root, "selected-result") || !absent(root, ".pending-result")) {
        ::close(root); return 90;
    }
    const int pending = ::openat(root, ".pending-request", O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
    if (pending < 0) { ::close(root); return 90; }
    struct stat st{};
    bool ok = ::fstat(pending, &st) == 0 && S_ISREG(st.st_mode) && st.st_uid == 0 &&
        (st.st_mode & 07777) == 0600 && st.st_nlink == 1 && st.st_size == 0;
    size_t written = 0;
    while (ok && written < request.size()) {
        const ssize_t n = ::write(pending, request.data() + written, request.size() - written);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) ok = false;
        else written += static_cast<size_t>(n);
    }
    const bool closed = ::close(pending) == 0;
    struct stat pendingIdentity{};
    if (!ok || !closed || !trusted() || !exact(root, ".pending-request", request, &pendingIdentity)) { ::close(root); return 90; }
    // No link/unlink window (which would expose nlink2), no replacing rename,
    // and no fallback on kernels/filesystems without RENAME_NOREPLACE support.
#ifdef SYS_renameat2
    const long renamed = ::syscall(SYS_renameat2, root, ".pending-request", root, "selected-request", RENAME_NOREPLACE);
#else
    const long renamed = -1;
#endif
    if (renamed != 0) { ::close(root); return 90; }
    struct stat finalIdentity{};
    // Rename changes ctime, so compare the immutable inode identity and bytes.
    const bool committed = trusted() && exact(root, "selected-request", request, &finalIdentity) &&
        finalIdentity.st_dev == pendingIdentity.st_dev && finalIdentity.st_ino == pendingIdentity.st_ino &&
        absent(root, ".pending-request");
    const bool rootClosed = ::close(root) == 0;
    if (!committed || !rootClosed) return 90; // May already be published; never retry.
    std::printf("request-published-once %s %s\n", argv[3], argv[4]);
    return 0;
}
