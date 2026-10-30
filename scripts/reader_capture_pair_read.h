#pragma once
// Fixed original-byte research readback. External admission and byte decoding
// are separate obligations; this helper never certifies a live target process.
#include <QByteArray>
#include <QString>
#include <array>
#include <functional>
#include <utility>
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace reader_capture_pair_read {
using Permit = std::function<bool()>;
struct Expected { QByteArray nonce, pid, start; };
struct Result {
    bool observed = false, attempted = false;
    QString refusal;
    QByteArray scalar, image;
    static constexpr bool liveProcessAuthority = false;
};
inline constexpr qint64 ScalarCap = 4096, ImageCap = 1048576;
namespace detail {
struct Fd {
    int value = -1;
    Fd() = default;
    Fd(const Fd &) = delete;
    Fd &operator=(const Fd &) = delete;
    ~Fd() { if (value >= 0) ::close(value); }
    bool close() { const int fd = std::exchange(value, -1); return fd >= 0 && ::close(fd) == 0; }
};
inline thread_local bool *active = nullptr;
struct Gate {
    bool poisoned = false, entered = false;
    Gate() { if (active) *active = true; else { active = &poisoned; entered = true; } }
    ~Gate() { if (entered) active = nullptr; }
};
inline bool same(const struct stat &a, const struct stat &b) {
    return a.st_dev == b.st_dev && a.st_ino == b.st_ino && a.st_mode == b.st_mode &&
        a.st_uid == b.st_uid && a.st_nlink == b.st_nlink && a.st_size == b.st_size &&
        a.st_mtim.tv_sec == b.st_mtim.tv_sec && a.st_mtim.tv_nsec == b.st_mtim.tv_nsec &&
        a.st_ctim.tv_sec == b.st_ctim.tv_sec && a.st_ctim.tv_nsec == b.st_ctim.tv_nsec;
}
inline bool decimal(const QByteArray &value) {
    if (value.isEmpty() || value.size() > 20 || value[0] < '1' || value[0] > '9') return false;
    for (const char c : value) if (c < '0' || c > '9') return false;
    return value.size() < 20 || value <= QByteArrayLiteral("18446744073709551615");
}
inline bool valid(const Expected &e) {
    if (e.nonce.size() != 32 || !decimal(e.pid) || !decimal(e.start)) return false;
    for (const char c : e.nonce) if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    return true;
}
inline constexpr std::array<const char *, 6> Names{{"owner", "attempt.identity", "selected-ready", "selected-request", "selected-result", "visual-image.png"}};
inline constexpr std::array<const char *, 6> Absent{{".pending-ready", ".pending-image", ".pending-result", "selected-cancel", "restore.claim", "stop.request"}};
struct State {
    Fd root;
    std::array<Fd, 6> files;
    std::array<struct stat, 6> stats{};
    struct stat rootStat{};
    size_t opened = 0;
    bool revoked = false;
    Gate &gate;
    explicit State(Gate &g) : gate(g) {}
    bool bracket() const {
        struct stat fresh{};
        if (::fstat(root.value, &fresh) || !same(fresh, rootStat)) return false;
        for (const char *name : Absent) {
            if (::fstatat(root.value, name, &fresh, AT_SYMLINK_NOFOLLOW) == 0 || errno != ENOENT) return false;
        }
        for (size_t i = 0; i < opened; ++i) {
            if (::fstat(files[i].value, &fresh) || !same(fresh, stats[i]) ||
                ::fstatat(root.value, Names[i], &fresh, AT_SYMLINK_NOFOLLOW) || !same(fresh, stats[i])) return false;
        }
        return true;
    }
    bool allowed(const Permit &permit) {
        if (revoked || gate.poisoned || !bracket()) { revoked = true; return false; }
        try {
            if (!permit || !permit()) revoked = true;
        } catch (...) { revoked = true; }
        if (gate.poisoned || !bracket()) revoked = true;
        return !revoked;
    }
    bool read(size_t index, QByteArray &bytes, const Permit &permit) {
        char chunk[4096];
        off_t remaining = stats[index].st_size;
        while (remaining > 0) {
            if (!allowed(permit)) return false;
            const auto want = size_t(remaining < off_t(sizeof(chunk)) ? remaining : off_t(sizeof(chunk)));
            const auto n = ::read(files[index].value, chunk, want);
            if (n <= 0 || size_t(n) > want || !allowed(permit)) return false;
            bytes.append(chunk, int(n)); remaining -= n;
        }
        if (!allowed(permit)) return false;
        const auto n = ::read(files[index].value, chunk, 1);
        return n == 0 && allowed(permit);
    }
};
}

inline Result readPair(int borrowedRootFd, const Expected &inputExpected, const Permit &inputPermit) {
    Result result;
    detail::Gate gate;
    if (!gate.entered) { result.refusal = QStringLiteral("reentered"); return result; }
    detail::State state(gate);
    // Pin the root before copied callables or a permission can destroy its owner.
    state.root.value = ::fcntl(borrowedRootFd, F_DUPFD_CLOEXEC, 0);
    const auto fail = [&](const char *reason) { result.refusal = QString::fromLatin1(reason); return result; };
    if (state.root.value < 0) return fail("root-duplicate");
    try {
        const Expected expected = inputExpected;
        Permit permit = inputPermit;
        if (!detail::valid(expected)) return fail("expected-binding");
        if (::fstat(state.root.value, &state.rootStat) || !S_ISDIR(state.rootStat.st_mode) ||
            state.rootStat.st_uid != ::geteuid() || (state.rootStat.st_mode & 07777) != 0700) return fail("root-binding");
        result.attempted = true;
        if (!state.allowed(permit)) return fail("permit-or-binding");
        const QByteArray identity = expected.pid + ' ' + expected.start + '\n';
        const QByteArray request = expected.nonce + ' ' + identity;
        const std::array<qint64, 6> limits{{32, 42, 75, 75, ScalarCap, ImageCap}};
        const std::array<qint64, 4> exact{{expected.nonce.size(), identity.size(), request.size(), request.size()}};
        for (size_t i = 0; i < detail::Names.size(); ++i) {
            if (!state.allowed(permit)) return fail("permit-or-binding");
            state.files[i].value = ::openat(state.root.value, detail::Names[i], O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC);
            auto &st = state.stats[i];
            if (state.files[i].value < 0 || ::fstat(state.files[i].value, &st) || !S_ISREG(st.st_mode) ||
                st.st_uid != state.rootStat.st_uid || (st.st_mode & 07777) != 0600 || st.st_nlink != 1 ||
                st.st_size < 1 || st.st_size > limits[i] || (i < exact.size() && st.st_size != exact[i])) return fail("file-binding");
            ++state.opened;
            if (!state.allowed(permit)) return fail("permit-or-binding");
        }
        std::array<QByteArray, 6> bytes;
        for (size_t i = 0; i < bytes.size(); ++i) {
            if (!state.read(i, bytes[i], permit)) return fail("read-or-binding");
        }
        if (bytes[0] != expected.nonce || bytes[1] != identity || bytes[2] != request || bytes[3] != request)
            return fail("identity-bytes");
        if (!state.allowed(permit)) return fail("permit-or-binding");
        // Releasing a copied callable is itself a callout. Do so with every
        // inode still pinned, then validate without consulting borrowed input.
        permit = nullptr;
        if (state.revoked || gate.poisoned || !state.bracket()) return fail("permit-unwind-binding");
        // No more callouts after this sampled bracket. Close each owned FD
        // exactly once before exposing bytes; a close error cannot be retried.
        bool closed = true;
        for (auto &fd : state.files) if (!fd.close()) closed = false;
        if (!state.root.close()) closed = false;
        if (!closed) return fail("close-unknown");
        result.scalar = std::move(bytes[4]); result.image = std::move(bytes[5]);
        result.observed = true; return result;
    } catch (...) { return fail("exception"); }
}
}
