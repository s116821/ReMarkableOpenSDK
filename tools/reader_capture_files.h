#pragma once
// Fixed research capture files only. No request protocol or publication authority.
#include <QByteArray>
#include <QString>
#include <atomic>
#include <functional>
#include <memory>
#include <utility>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>

namespace reader_capture_files {
using Permit = std::function<bool()>;
enum class ScalarKind { Diagnostic, AcceptedPair };
struct Result {
    bool ok = false, attempted = false, linked = false, committed = false;
    qint64 bytes = 0;
    QString refusal;
};
namespace detail {
struct Fd {
    int value = -1;
    explicit Fd(int fd = -1) : value(fd) {}
    ~Fd() { if (value >= 0) ::close(value); }
    bool close() { const int fd = std::exchange(value, -1); return fd >= 0 && ::close(fd) == 0; }
};
inline bool same(const struct stat &a, const struct stat &b) {
    return a.st_dev == b.st_dev && a.st_ino == b.st_ino && a.st_mode == b.st_mode &&
        a.st_uid == b.st_uid && a.st_nlink == b.st_nlink && a.st_size == b.st_size &&
        a.st_mtim.tv_sec == b.st_mtim.tv_sec && a.st_mtim.tv_nsec == b.st_mtim.tv_nsec &&
        a.st_ctim.tv_sec == b.st_ctim.tv_sec && a.st_ctim.tv_nsec == b.st_ctim.tv_nsec;
}
inline bool expected(const struct stat &a, const struct stat &written, nlink_t links) {
    // link/unlink legitimately changes ctime. At each stable link count both
    // names are compared with full metadata, including their current ctime.
    return a.st_dev == written.st_dev && a.st_ino == written.st_ino && a.st_mode == written.st_mode &&
        a.st_uid == written.st_uid && a.st_size == written.st_size && a.st_nlink == links &&
        a.st_mtim.tv_sec == written.st_mtim.tv_sec && a.st_mtim.tv_nsec == written.st_mtim.tv_nsec;
}
inline thread_local bool creating = false, createReentered = false;
struct CreateGate {
    bool entered = false;
    CreateGate() { if (creating) createReentered = true; else { creating = true; createReentered = false; entered = true; } }
    ~CreateGate() { if (entered) creating = false; }
};
}
class Files final : public std::enable_shared_from_this<Files> {
public:
    static constexpr qint64 ImageCap = 1048576, ScalarCap = 4096;
    Files(const Files &) = delete;
    Files &operator=(const Files &) = delete;
    ~Files() { if (imageFd_ >= 0) ::close(imageFd_); if (imagePin_ >= 0) ::close(imagePin_); if (root_ >= 0) ::close(root_); }
    static std::shared_ptr<Files> create(int borrowedRootFd, const Permit &inputPermit) {
        detail::CreateGate gate;
        if (!gate.entered) return {};
        // Own the directory before the first permission callout can destroy an
        // Entry and close its original descriptor. No path is retained here.
        detail::Fd duplicate(::fcntl(borrowedRootFd, F_DUPFD_CLOEXEC, 0));
        if (duplicate.value < 0) return {};
        try {
            auto self = std::shared_ptr<Files>(new Files(std::exchange(duplicate.value, -1)));
            const Permit permit = inputPermit;
            if (::fstat(self->root_, &self->rootStat_) || !self->rootOkay() ||
                !self->allowed(permit, false) || detail::createReentered) return {};
            return self;
        } catch (...) { return {}; }
    }
    // Borrow only while the caller retains shared Files. Seal closes this FD;
    // neither a Sink nor the caller owns it, and it must not escape that scope.
    int imageFd() const { return imageFd_; }
    Result openImage(const Permit &inputPermit) {
        const auto self = shared_from_this(); Operation operation(*self);
        Result result;
        if (!operation.entered) return refused(result, "reentered");
        if (opened_) return refused(result, "image-open-consumed");
        opened_ = true; result.attempted = true;
        try {
            const Permit permit = inputPermit;
            if (!allowed(permit, false)) return refused(result, "permit");
            if (!absent(".pending-image") || !absent("visual-image.png")) return refused(result, "image-exists");
            imageFd_ = ::openat(root_, ".pending-image", O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
            if (imageFd_ < 0) return refused(result, "image-open");
            if (::fstat(imageFd_, &openedStat_) || !regular(openedStat_, 0, 1) || !allowed(permit, false))
                return refused(result, "image-open-binding");
            result.ok = true; return result;
        } catch (...) { revoked_.store(true); return refused(result, "exception"); }
    }
    Result sealImage(qint64 exactBytes, const Permit &inputPermit) {
        const auto self = shared_from_this(); Operation operation(*self);
        Result result;
        if (!operation.entered) return refused(result, "reentered");
        if (sealedAttempt_) return refused(result, "image-seal-consumed");
        sealedAttempt_ = true; result.attempted = true;
        // The local close obligation survives any permission/lifetime failure.
        detail::Fd fd(std::exchange(imageFd_, -1));
        try {
            const Permit permit = inputPermit;
            if (fd.value < 0 || exactBytes < 1 || exactBytes > ImageCap || !allowed(permit, false))
                return refused(result, "image-seal-request");
            struct stat written{}, named{};
            const bool valid = !::fstat(fd.value, &written) && regular(written, exactBytes, 1) &&
                written.st_dev == openedStat_.st_dev && written.st_ino == openedStat_.st_ino &&
                written.st_mode == openedStat_.st_mode && written.st_uid == openedStat_.st_uid;
            // Keep a non-writable inode pin before closing the writer. Stat
            // equality alone cannot prevent immediate inode-number reuse.
            detail::Fd pin(valid ? ::openat(root_, ".pending-image", O_PATH | O_NOFOLLOW | O_CLOEXEC) : -1);
            struct stat pinned{}, again{};
            const bool pinOkay = pin.value >= 0 && !::fstat(pin.value, &pinned) && detail::same(written, pinned) &&
                !::fstat(fd.value, &again) && detail::same(written, again);
            const bool closed = fd.close();
            if (!valid || !pinOkay || !closed || !allowed(permit, false) ||
                ::fstatat(root_, ".pending-image", &named, AT_SYMLINK_NOFOLLOW) || !detail::same(written, named))
                return refused(result, "image-seal-binding");
            imagePin_ = std::exchange(pin.value, -1);
            sealedStat_ = written; sealed_ = true; result.bytes = exactBytes; result.ok = true; return result;
        } catch (...) { revoked_.store(true); return refused(result, "exception"); }
    }
    Result commitImage(const Permit &inputPermit) {
        const auto self = shared_from_this(); Operation operation(*self);
        Result result;
        if (!operation.entered) return refused(result, "reentered");
        if (committedAttempt_) return refused(result, "image-commit-consumed");
        committedAttempt_ = true; result.attempted = true;
        try {
            const Permit permit = inputPermit;
            if (!sealed_) return refused(result, "image-not-sealed");
            result = commit(".pending-image", "visual-image.png", sealedStat_, permit, false);
            if (result.committed) { imageCommitted_ = true; imageFinal_ = finalStat_; }
            return result;
        } catch (...) { revoked_.store(true); return refused(result, "exception"); }
    }
    Result publishScalar(const QByteArray &inputBytes, ScalarKind kind, const Permit &inputPermit) {
        const auto self = shared_from_this(); Operation operation(*self);
        Result result;
        if (!operation.entered) return refused(result, "reentered");
        if (scalarAttempt_) return refused(result, "scalar-consumed");
        scalarAttempt_ = true; result.attempted = true;
        try {
            const QByteArray bytes = inputBytes;
            const Permit permit = inputPermit;
            if (bytes.isEmpty() || bytes.size() > ScalarCap ||
                (kind != ScalarKind::Diagnostic && kind != ScalarKind::AcceptedPair)) return refused(result, "scalar-request");
            const bool pair = kind == ScalarKind::AcceptedPair;
            if (!allowed(permit, pair)) return refused(result, "permit-or-image");
            if (!absent(".pending-result") || !absent("selected-result")) return refused(result, "scalar-exists");
            detail::Fd fd(::openat(root_, ".pending-result", O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600));
            if (fd.value < 0) return refused(result, "scalar-open");
            struct stat initial{}, written{}, named{};
            const bool initialOkay = !::fstat(fd.value, &initial) && regular(initial, 0, 1);
            const ssize_t n = initialOkay ? ::write(fd.value, bytes.constData(), size_t(bytes.size())) : -1;
            if (n > 0) result.bytes = n;
            const bool valid = n == bytes.size() && !::fstat(fd.value, &written) && regular(written, bytes.size(), 1) &&
                written.st_dev == initial.st_dev && written.st_ino == initial.st_ino &&
                written.st_mode == initial.st_mode && written.st_uid == initial.st_uid;
            detail::Fd pin(valid ? ::openat(root_, ".pending-result", O_PATH | O_NOFOLLOW | O_CLOEXEC) : -1);
            struct stat pinned{}, again{};
            const bool pinOkay = pin.value >= 0 && !::fstat(pin.value, &pinned) && detail::same(written, pinned) &&
                !::fstat(fd.value, &again) && detail::same(written, again);
            const bool closed = fd.close();
            if (!valid || !pinOkay || !closed || !allowed(permit, pair) ||
                ::fstatat(root_, ".pending-result", &named, AT_SYMLINK_NOFOLLOW) || !detail::same(written, named))
                return refused(result, "scalar-write-binding");
            return commit(".pending-result", "selected-result", written, permit, pair);
        } catch (...) { revoked_.store(true); return refused(result, "exception"); }
    }
private:
    explicit Files(int root) : root_(root) {}
    struct Operation {
        Files &files;
        bool entered;
        explicit Operation(Files &f) : files(f), entered(!f.busy_.exchange(true)) {
            if (!entered) files.revoked_.store(true);
        }
        ~Operation() { if (entered) files.busy_.store(false); }
    };
    static Result refused(Result r, const char *reason) { r.ok = false; r.committed = false; r.refusal = QString::fromLatin1(reason); return r; }
    bool rootOkay() const {
        struct stat fresh{};
        return root_ >= 0 && !::fstat(root_, &fresh) && S_ISDIR(fresh.st_mode) &&
            fresh.st_dev == rootStat_.st_dev && fresh.st_ino == rootStat_.st_ino &&
            fresh.st_uid == ::geteuid() && fresh.st_uid == rootStat_.st_uid &&
            (fresh.st_mode & 07777) == 0700 && (rootStat_.st_mode & 07777) == 0700;
    }
    bool absent(const char *name) const {
        struct stat s{}; if (!::fstatat(root_, name, &s, AT_SYMLINK_NOFOLLOW)) return false;
        return errno == ENOENT;
    }
    bool regular(const struct stat &s, qint64 size, nlink_t links) const {
        return S_ISREG(s.st_mode) && (s.st_mode & 07777) == 0600 && s.st_uid == rootStat_.st_uid &&
            s.st_nlink == links && s.st_size == size;
    }
    bool imageOkay() const {
        struct stat image{};
        return imageCommitted_ && absent(".pending-image") &&
            !::fstatat(root_, "visual-image.png", &image, AT_SYMLINK_NOFOLLOW) && detail::same(imageFinal_, image);
    }
    bool allowed(const Permit &permit, bool pair) {
        if (revoked_.load()) return false;
        try {
            if (!rootOkay() || !permit || !permit() || !rootOkay() || revoked_.load() || (pair && !imageOkay())) {
                revoked_.store(true); return false;
            }
            return true;
        } catch (...) { revoked_.store(true); return false; }
    }
    Result commit(const char *pending, const char *name, const struct stat &written, const Permit &permit, bool pair) {
        Result r; r.attempted = true; r.bytes = written.st_size;
        struct stat named{};
        if (!allowed(permit, pair) || ::fstatat(root_, pending, &named, AT_SYMLINK_NOFOLLOW) ||
            !detail::same(written, named) || !absent(name)) return refused(r, "commit-binding");
        // No callback separates the last named-inode check from exclusive link.
        if (::linkat(root_, pending, root_, name, 0)) return refused(r, "commit-link");
        r.linked = true;
        struct stat linked{}, final{};
        if (!allowed(permit, pair) || ::fstatat(root_, pending, &linked, AT_SYMLINK_NOFOLLOW) ||
            !detail::expected(linked, written, 2) || ::fstatat(root_, name, &final, AT_SYMLINK_NOFOLLOW) ||
            !detail::same(linked, final)) return refused(r, "commit-linked-binding");
        if (!allowed(permit, pair) || ::fstatat(root_, pending, &named, AT_SYMLINK_NOFOLLOW) || !detail::same(linked, named) ||
            ::fstatat(root_, name, &final, AT_SYMLINK_NOFOLLOW) || !detail::same(named, final)) return refused(r, "commit-before-unlink");
        if (::unlinkat(root_, pending, 0)) return refused(r, "commit-unlink");
        struct stat after{};
        if (::fstatat(root_, name, &after, AT_SYMLINK_NOFOLLOW) || !detail::expected(after, written, 1) ||
            !absent(pending) || !allowed(permit, pair) || ::fstatat(root_, name, &final, AT_SYMLINK_NOFOLLOW) ||
            !detail::same(after, final) || !absent(pending)) return refused(r, "commit-final-binding");
        finalStat_ = final; r.ok = r.committed = true; return r;
    }
    int root_ = -1, imageFd_ = -1, imagePin_ = -1;
    struct stat rootStat_{}, openedStat_{}, sealedStat_{}, imageFinal_{}, finalStat_{};
    std::atomic<bool> busy_{false}, revoked_{false};
    bool opened_ = false, sealedAttempt_ = false, sealed_ = false, committedAttempt_ = false,
        imageCommitted_ = false, scalarAttempt_ = false;
};
} // namespace reader_capture_files
