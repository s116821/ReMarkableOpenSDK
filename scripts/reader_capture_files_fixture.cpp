// Original owned file/permission controls. No IPC/native request or device.
#include "reader_capture_files.h"
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <sys/resource.h>
#include <sys/wait.h>
#include <signal.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
using namespace reader_capture_files;
unsigned checks = 0, scenarios = 0;
void check(bool ok, const char *message) { ++checks; if (!ok) throw std::runtime_error(message); }
void scenario(const char *name, const std::function<void()> &body) {
    const auto only = qEnvironmentVariable("READER_CAPTURE_FILES_ONLY");
    if (!only.isEmpty() && only != QLatin1String(name)) return;
    ++scenarios;
    try { body(); } catch (const std::exception &e) { std::cerr << name << ": " << e.what() << '\n'; throw; }
}
const Permit yes = [] { return true; };
const QByteArray image("owned-window-image-bytes"), scalar("{\"owned\":true}\n");
struct Root {
    QTemporaryDir temporary;
    QByteArray path;
    int fd = -1;
    Root() {
        check(temporary.isValid(), "temporary root"); path = QFile::encodeName(temporary.path() + QStringLiteral("/owned"));
        check(::mkdir(path.constData(), 0700) == 0, "owned directory created");
        fd = ::open(path.constData(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        check(fd >= 0, "owned root descriptor");
    }
    ~Root() { if (fd >= 0) ::close(fd); }
    QByteArray file(const char *name) const { return path + '/' + name; }
    bool stat(const char *name, struct stat &s) const { return ::lstat(file(name).constData(), &s) == 0; }
    bool exists(const char *name) const { struct stat s{}; return stat(name, s); }
    QByteArray read(const char *name) const { QFile f(QString::fromLocal8Bit(file(name))); check(f.open(QIODevice::ReadOnly), "fixture read"); return f.readAll(); }
    void put(const char *name, const QByteArray &bytes, mode_t mode = 0600) const {
        const int f = ::open(file(name).constData(), O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW | O_CLOEXEC, mode);
        check(f >= 0, "fixture write open"); const auto n = ::write(f, bytes.constData(), size_t(bytes.size()));
        const bool closed = ::close(f) == 0; check(n == bytes.size() && closed, "fixture write complete");
    }
    bool privateFile(const char *name, size_t bytes, nlink_t links = 1) const {
        struct stat s{}; return stat(name, s) && S_ISREG(s.st_mode) && (s.st_mode & 07777) == 0600 &&
            s.st_uid == ::geteuid() && s.st_nlink == links && s.st_size == off_t(bytes);
    }
};
std::shared_ptr<Files> opened(Root &root) {
    auto files = Files::create(root.fd, yes); check(bool(files), "root admitted");
    const auto r = files->openImage(yes);
    check(r.ok && r.attempted && !r.linked && !r.committed && r.bytes == 0 && r.refusal.isEmpty(), "pending image opened");
    check(root.privateFile(".pending-image", 0) && files->imageFd() >= 0, "private pending FD");
    return files;
}
void writeImage(const std::shared_ptr<Files> &files, const QByteArray &bytes = image) {
    check(::write(files->imageFd(), bytes.constData(), size_t(bytes.size())) == bytes.size(), "caller writes bounded image bytes");
}
void sealed(const std::shared_ptr<Files> &files, const QByteArray &bytes = image) {
    writeImage(files, bytes); const int oldFd = files->imageFd(); const auto r = files->sealImage(bytes.size(), yes);
    check(r.ok && r.attempted && !r.committed && !r.linked && r.bytes == bytes.size(), "image sealed before commit");
    errno = 0; check(files->imageFd() == -1 && ::fcntl(oldFd, F_GETFD) == -1 && errno == EBADF, "seal closes owned image FD");
}
void refused(const Result &r) { check(!r.ok && !r.committed && !r.refusal.isEmpty(), "explicit refusal"); }
void committed(const Result &r, size_t bytes) {
    check(r.ok && r.attempted && r.linked && r.committed && r.refusal.isEmpty() && r.bytes == qint64(bytes), "commit conjunction");
}
int fdCount() {
    int count = 0;
    for (int fd = 0; fd < 1024; ++fd) if (::fcntl(fd, F_GETFD) >= 0) ++count;
    return count;
}
}

int main() {
    try {
        scenario("image-first-scalar-last", [] {
            Root root; auto files = opened(root); sealed(files);
            // Cross a full second so this exercises legitimate ctime changes
            // even when the filesystem clock has coarse timestamp resolution.
            struct stat before{}; check(root.stat(".pending-image", before), "sealed stat"); ::usleep(1100000);
            std::vector<nlink_t> transitions;
            const Permit permit = [&] { struct stat s{}; if (root.stat("visual-image.png", s)) transitions.push_back(s.st_nlink); return true; };
            const auto imageResult = files->commitImage(permit); committed(imageResult, image.size());
            check(!root.exists(".pending-image") && root.privateFile("visual-image.png", image.size()), "single-link image final");
            check(root.read("visual-image.png") == image && !root.exists("selected-result"), "image alone precedes acceptance scalar");
            check(std::find(transitions.begin(), transitions.end(), 2) != transitions.end() && transitions.back() == 1, "real hardlink 2 then 1 transitions");
            struct stat after{}; check(root.stat("visual-image.png", after), "committed stat");
            check(before.st_ino == after.st_ino && before.st_dev == after.st_dev &&
                  (before.st_ctim.tv_sec != after.st_ctim.tv_sec || before.st_ctim.tv_nsec != after.st_ctim.tv_nsec), "legitimate ctime changes accepted");
            const auto result = files->publishScalar(scalar, ScalarKind::AcceptedPair, yes); committed(result, scalar.size());
            check(root.privateFile("selected-result", scalar.size()) && !root.exists(".pending-result") && root.read("selected-result") == scalar, "closed scalar final bytes");
            refused(files->commitImage(yes)); refused(files->openImage(yes)); refused(files->sealImage(image.size(), yes));
            refused(files->publishScalar("new", ScalarKind::AcceptedPair, yes));
            check(root.read("selected-result") == scalar && root.read("visual-image.png") == image, "no operation retries/overwrites");
        });
        scenario("diagnostic-without-image", [] {
            Root root; auto files = Files::create(root.fd, yes); check(bool(files), "root only");
            committed(files->publishScalar(scalar, ScalarKind::Diagnostic, yes), scalar.size());
            check(!root.exists("visual-image.png") && !root.exists(".pending-image"), "diagnostic does not certify image");
        });
        scenario("ordinary-image-failure-diagnostic", [] {
            Root root; auto files = opened(root); writeImage(files, "short");
            refused(files->sealImage(image.size(), yes));
            committed(files->publishScalar(scalar, ScalarKind::Diagnostic, yes), scalar.size());
            check(root.read(".pending-image") == "short" && !root.exists("visual-image.png"), "orphan image retained by ordinary refusal");
        });
        scenario("accepted-pair-needs-image", [] {
            Root root; auto files = Files::create(root.fd, yes);
            refused(files->publishScalar(scalar, ScalarKind::AcceptedPair, yes));
            check(!root.exists("selected-result") && !root.exists(".pending-result"), "missing image cannot mint pair");
        });
        scenario("duplicated-root-survives-source-close", [] {
            Root root; auto files = Files::create(root.fd, [&] { check(::close(root.fd) == 0, "permission closes Entry source descriptor"); root.fd = -1; return true; });
            check(bool(files), "owned duplicate existed before callout"); check(files->openImage(yes).ok, "stage with only retained root fd");
            sealed(files); committed(files->commitImage(yes), image.size());
            committed(files->publishScalar(scalar, ScalarKind::AcceptedPair, yes), scalar.size());
        });
        for (int mode : {0755, 0777, 0000}) scenario("root-mode", [mode] {
            Root root; check(::chmod(root.path.constData(), mode_t(mode)) == 0, "fixture root mode");
            check(!Files::create(root.fd, yes), "bad root mode rejected before files");
        });
        scenario("invalid-root-fd", [] { check(!Files::create(-1, yes), "invalid fd fails closed"); });
        scenario("non-directory-root", [] { Root root; root.put("not-directory", "x"); const int fd = ::open(root.file("not-directory").constData(), O_RDONLY); check(fd >= 0, "regular root model"); auto files = Files::create(fd, yes); check(::close(fd) == 0 && !files, "regular file is no root"); });
        scenario("create-false-or-throw", [] {
            Root root; check(!Files::create(root.fd, [] { return false; }), "create denied");
            check(!Files::create(root.fd, []() -> bool { throw std::runtime_error("owned"); }), "create throwing permit");
            check(!root.exists(".pending-image") && !root.exists(".pending-result"), "no files from refused create");
        });
        scenario("create-reentry", [] {
            Root root; bool nested = false; auto outer = Files::create(root.fd, [&] {
                nested = true; check(!Files::create(root.fd, yes), "nested create refused"); return true;
            }); check(nested && !outer, "create reentry poisons outer");
        });
        for (const char *name : {".pending-image", "visual-image.png"}) scenario("existing-image-name", [name] {
            Root root; root.put(name, "old"); auto files = Files::create(root.fd, yes);
            refused(files->openImage(yes)); check(root.read(name) == "old", "stale file never overwritten");
            refused(files->openImage(yes)); committed(files->publishScalar(scalar, ScalarKind::Diagnostic, yes), scalar.size());
        });
        for (const char *name : {".pending-image", "visual-image.png", ".pending-result", "selected-result"}) scenario("symlink-name", [name] {
            Root root; root.put("outside", "outside-preserved"); check(::symlink("outside", root.file(name).constData()) == 0, "owned symlink model");
            auto files = Files::create(root.fd, yes);
            const auto r = QByteArray(name).contains("image") ? files->openImage(yes) : files->publishScalar(scalar, ScalarKind::Diagnostic, yes);
            refused(r); check(root.read("outside") == "outside-preserved", "no symlink traversal/write");
        });
        for (const char *name : {".pending-result", "selected-result"}) scenario("existing-scalar-name", [name] {
            Root root; root.put(name, "old"); auto files = Files::create(root.fd, yes);
            refused(files->publishScalar(scalar, ScalarKind::Diagnostic, yes)); check(root.read(name) == "old", "scalar existing bytes preserved");
        });
        for (int change = 0; change < 4; ++change) scenario("seal-malformed-file", [change] {
            Root root; auto files = opened(root); writeImage(files);
            if (change == 0) check(::chmod(root.file(".pending-image").constData(), 0644) == 0, "mode altered");
            if (change == 1) check(::link(root.file(".pending-image").constData(), root.file("extra").constData()) == 0, "extra hardlink");
            if (change == 2) { check(::unlink(root.file(".pending-image").constData()) == 0, "remove name only in fixture"); root.put(".pending-image", image); }
            if (change == 3) check(::ftruncate(files->imageFd(), 1) == 0, "partial image model");
            refused(files->sealImage(image.size(), yes)); check(files->imageFd() == -1 && !root.exists("visual-image.png"), "bad file closed not published");
            refused(files->commitImage(yes));
        });
        scenario("close-before-named-validation", [] {
            Root root; auto files = opened(root); writeImage(files); const int fd = files->imageFd(); int calls = 0;
            const auto r = files->sealImage(image.size(), [&] {
                if (++calls == 2) {
                    errno = 0; check(::fcntl(fd, F_GETFD) == -1 && errno == EBADF, "fd already closed before named check permit");
                    struct stat old{}; check(root.stat(".pending-image", old), "old sealed inode");
                    check(::unlink(root.file(".pending-image").constData()) == 0, "fixture unlinks closed original writer name");
                    root.put(".pending-image", image);
                    struct stat replacement{}; check(root.stat(".pending-image", replacement) && replacement.st_ino != old.st_ino, "private O_PATH pin prevents original inode reuse");
                } return true;
            }); refused(r); check(calls == 2 && !root.exists("visual-image.png"), "replacement rejected despite equal bytes");
        });
        for (qint64 size : {qint64(0), qint64(-1), Files::ImageCap + 1}) scenario("image-size-request", [size] {
            Root root; auto files = opened(root); writeImage(files); refused(files->sealImage(size, yes));
            check(files->imageFd() == -1 && root.exists(".pending-image"), "invalid seal size closes FD and preserves pending");
        });
        scenario("image-cap", [] {
            Root root; auto files = opened(root); const QByteArray bytes(int(Files::ImageCap), 'i'); sealed(files, bytes);
            committed(files->commitImage(yes), bytes.size()); check(root.read("visual-image.png") == bytes, "exact image cap accepted");
        });
        for (int size : {0, int(Files::ScalarCap + 1)}) scenario("scalar-size-refusal", [size] {
            Root root; auto files = Files::create(root.fd, yes); refused(files->publishScalar(QByteArray(size, 's'), ScalarKind::Diagnostic, yes));
            check(!root.exists(".pending-result"), "invalid scalar never opened");
        });
        scenario("scalar-cap", [] { Root root; auto files = Files::create(root.fd, yes); QByteArray bytes(int(Files::ScalarCap), 's'); committed(files->publishScalar(bytes, ScalarKind::Diagnostic, yes), bytes.size()); check(root.read("selected-result") == bytes, "exact scalar cap bytes"); });
        scenario("unknown-kind", [] { Root root; auto files = Files::create(root.fd, yes); refused(files->publishScalar(scalar, static_cast<ScalarKind>(99), yes)); check(!root.exists(".pending-result"), "unknown kind before write"); });
        for (int limit : {0, 7}) scenario("actual-short-write", [limit] {
            Root root; const pid_t child = ::fork(); check(child >= 0, "owned child fork");
            if (child == 0) {
                try {
                    ::signal(SIGXFSZ, SIG_IGN); struct rlimit old{}; check(::getrlimit(RLIMIT_FSIZE, &old) == 0, "read child limit");
                    struct rlimit bounded{rlim_t(limit), old.rlim_max}; check(::setrlimit(RLIMIT_FSIZE, &bounded) == 0, "set real child short write limit");
                    auto files = Files::create(root.fd, yes); const auto r = files->publishScalar(scalar, ScalarKind::Diagnostic, yes);
                    refused(r); check(root.privateFile(".pending-result", size_t(limit)) && !root.exists("selected-result"), "kernel partial/error leaves bounded forensic pending");
                    refused(files->publishScalar(scalar, ScalarKind::Diagnostic, yes)); ::_exit(0);
                } catch (const std::exception &e) { std::cerr << "short child: " << e.what() << '\n'; ::_exit(1); }
            }
            int status = 0; check(::waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0, "actual child write control passed");
            check(root.privateFile(".pending-result", size_t(limit)) && !root.exists("selected-result"), "parent verifies exact partial file/no retry");
        });
        for (int at = 1; at <= 4; ++at) scenario("image-commit-permit-boundaries", [at] {
            Root root; auto files = opened(root); sealed(files); int calls = 0;
            const auto r = files->commitImage([&] { return ++calls != at; }); refused(r);
            check(calls == at, "first false permit permanently stops commit");
            if (at == 1) check(!r.linked && root.privateFile(".pending-image", image.size()) && !root.exists("visual-image.png"), "prelink evidence intact");
            if (at == 2 || at == 3) check(r.linked && root.privateFile(".pending-image", image.size(), 2) && root.privateFile("visual-image.png", image.size(), 2), "lost permission preserves both nlink2 names");
            if (at == 4) check(r.linked && !root.exists(".pending-image") && root.privateFile("visual-image.png", image.size()), "after unlink refusal leaves final forensic image");
            refused(files->publishScalar(scalar, ScalarKind::Diagnostic, yes)); refused(files->commitImage(yes));
            check(!root.exists("selected-result") && !root.exists(".pending-result"), "later true cannot revive publication");
        });
        for (int at = 1; at <= 6; ++at) scenario("scalar-permit-boundaries", [at] {
            Root root; auto files = Files::create(root.fd, yes); int calls = 0;
            const auto r = files->publishScalar(scalar, ScalarKind::Diagnostic, [&] { return ++calls != at; });
            refused(r); check(calls == at, "scalar fresh permit stage stop");
            if (at <= 3) check(!root.exists("selected-result"), "no scalar link before permission");
            if (at == 4 || at == 5) check(root.privateFile(".pending-result", scalar.size(), 2) && root.privateFile("selected-result", scalar.size(), 2), "scalar lost permit preserves hardlinks");
            if (at == 6) check(!root.exists(".pending-result") && root.privateFile("selected-result", scalar.size()), "postunlink scalar remains forensic on lost permission");
            refused(files->openImage(yes));
        });
        for (int kind = 0; kind < 4; ++kind) scenario("linked-name-interference", [kind] {
            Root root; auto files = opened(root); sealed(files); bool changed = false;
            const auto r = files->commitImage([&] {
                struct stat s{};
                if (!changed && root.stat("visual-image.png", s) && s.st_nlink == 2) {
                    changed = true;
                    if (kind == 0) check(::chmod(root.file(".pending-image").constData(), 0644) == 0, "linked mode change");
                    if (kind == 1) check(::link(root.file("visual-image.png").constData(), root.file("third-link").constData()) == 0, "third hardlink");
                    if (kind == 2) { check(::unlink(root.file("visual-image.png").constData()) == 0, "replace linked final fixture"); root.put("visual-image.png", "foreign"); }
                    if (kind == 3) { check(::rename(root.file(".pending-image").constData(), root.file("renamed").constData()) == 0, "rename pending fixture"); root.put(".pending-image", image); }
                } return true;
            }); refused(r); check(changed && r.linked && root.exists(".pending-image"), "interference never causes helper to delete wrong pending");
        });
        scenario("sealed-file-replacement", [] { Root root; auto files = opened(root); sealed(files); check(::rename(root.file(".pending-image").constData(), root.file("retained-old-inode").constData()) == 0, "fixture retain sealed inode before replace"); root.put(".pending-image", image); refused(files->commitImage(yes)); check(!root.exists("visual-image.png"), "distinct sealed inode replacement refused"); });
        scenario("scalar-close-replacement", [] {
            Root root; auto files = Files::create(root.fd, yes); int calls = 0;
            const auto r = files->publishScalar(scalar, ScalarKind::Diagnostic, [&] { if (++calls == 2) { check(::unlink(root.file(".pending-result").constData()) == 0, "fixture unlinks closed scalar name"); root.put(".pending-result", scalar); } return true; });
            refused(r); check(calls == 2 && !root.exists("selected-result"), "scalar named inode validated after close");
        });
        scenario("accepted-image-altered-during-scalar", [] {
            Root root; auto files = opened(root); sealed(files); committed(files->commitImage(yes), image.size()); bool changed = false;
            const auto r = files->publishScalar(scalar, ScalarKind::AcceptedPair, [&] {
                if (!changed && root.exists(".pending-result")) {
                    changed = true; root.put("visual-image.png", QByteArray(image.size(), 'x'));
                    const struct timespec changedTimes[2]{{1, 0}, {1, 0}};
                    check(::utimensat(AT_FDCWD, root.file("visual-image.png").constData(), changedTimes, 0) == 0, "make image metadata mutation explicit");
                } return true;
            }); refused(r); check(changed && !root.exists("selected-result") && root.exists(".pending-result"), "image binding rechecked during scalar commit");
        });
        scenario("root-rename-caller-trust", [] {
            Root root; struct stat original{}; check(::fstat(root.fd, &original) == 0, "original root inode"); auto files = opened(root); sealed(files);
            const auto old = root.path + "-retained"; check(::rename(root.path.constData(), old.constData()) == 0 && ::mkdir(root.path.constData(), 0700) == 0, "fixture root path replacement");
            const Permit trust = [&] { struct stat current{}; return !::lstat(root.path.constData(), &current) && current.st_ino == original.st_ino && current.st_dev == original.st_dev; };
            refused(files->commitImage(trust)); check(QFile::exists(QString::fromLocal8Bit(old + "/.pending-image")) && !root.exists("visual-image.png"), "caller pathname trust separately required");
        });
        scenario("deadline-after-link", [] {
            Root root; auto files = opened(root); sealed(files); int clock = 0; const int deadline = 10;
            const auto r = files->commitImage([&] { struct stat s{}; if (root.stat("visual-image.png", s)) clock = deadline; return clock < deadline; });
            refused(r); check(r.linked && root.privateFile(".pending-image", image.size(), 2), "expired caller deadline cannot unlink pending");
        });
        for (bool throwing : {false, true}) scenario("sticky-permission-revocation", [throwing] {
            Root root; auto files = Files::create(root.fd, yes);
            const auto r = files->openImage([&]() -> bool { if (throwing) throw std::runtime_error("owned"); return false; }); refused(r);
            refused(files->publishScalar(scalar, ScalarKind::Diagnostic, yes)); check(!root.exists(".pending-image") && !root.exists("selected-result"), "false/throw never revives");
        });
        scenario("reentry-before-open", [] {
            Root root; auto files = Files::create(root.fd, yes); bool nested = false;
            const auto r = files->openImage([&] { nested = true; refused(files->publishScalar(scalar, ScalarKind::Diagnostic, yes)); return true; });
            refused(r); refused(files->publishScalar(scalar, ScalarKind::Diagnostic, yes)); check(nested && !root.exists(".pending-image") && !root.exists(".pending-result"), "nested operation sticky revokes outer");
        });
        scenario("reentry-after-link", [] {
            Root root; auto files = opened(root); sealed(files); bool nested = false;
            const auto r = files->commitImage([&] { if (!nested && root.exists("visual-image.png")) { nested = true; refused(files->commitImage(yes)); } return true; });
            refused(r); check(nested && r.linked && root.privateFile(".pending-image", image.size(), 2), "reentry preserves nlink2 evidence");
        });
        scenario("shared-owner-dies-in-permit", [] {
            Root root; auto files = opened(root); sealed(files); std::weak_ptr<Files> weak = files; Files *raw = files.get();
            const auto r = raw->commitImage([&] { files.reset(); return false; }); refused(r);
            check(weak.expired() && root.privateFile(".pending-image", image.size()), "method strong self closes safely after caller dies");
        });
        scenario("source-owner-dies-after-link", [] {
            Root root; auto files = opened(root); sealed(files); auto owner = std::make_shared<bool>(true);
            const auto r = files->commitImage([&] { if (root.exists("visual-image.png")) owner.reset(); return bool(owner); });
            refused(r); check(!owner && root.privateFile(".pending-image", image.size(), 2), "Entry death retains root and linked evidence");
        });
        scenario("ordinary-orphan-still-diagnostic", [] {
            Root root; auto files = opened(root); sealed(files); root.put("visual-image.png", "stale-final");
            refused(files->commitImage(yes)); committed(files->publishScalar(scalar, ScalarKind::Diagnostic, yes), scalar.size());
            check(root.read("visual-image.png") == "stale-final" && root.read(".pending-image") == image, "ordinary stale image doesn't revoke independent diagnostic");
        });
        scenario("unexpected-bad-image-fd", [] {
            Root root; auto files = opened(root); writeImage(files);
            // Deliberately violate the borrow-only rule as an owned robustness
            // control. No descriptor-opening call occurs before seal consumes it.
            check(::close(files->imageFd()) == 0, "owned fault closes borrowed writer");
            const auto r = files->sealImage(image.size(), yes); refused(r);
            check(files->imageFd() == -1 && root.privateFile(".pending-image", image.size()) && !root.exists("visual-image.png"), "bad FD cannot seal or publish");
        });
        scenario("owned-fd-pin-lifetime", [] {
            Root root; const int before = fdCount(); auto files = opened(root); sealed(files);
            check(fdCount() == before + 2, "only duplicated root and nonwritable image inode pin retained");
            committed(files->commitImage(yes), image.size());
            committed(files->publishScalar(scalar, ScalarKind::AcceptedPair, yes), scalar.size());
            check(fdCount() == before + 2, "scalar local writer/pin closed after commit");
            files.reset(); check(fdCount() == before, "last helper owner closes all retained root/image pins");
        });
        std::cout << "PASS capture files owned scenarios=" << scenarios << " checks=" << checks
                  << " fixed_names=true actual_link_transitions=true native_execution=false\n";
    } catch (const std::exception &) { return 1; }
    return 0;
}
