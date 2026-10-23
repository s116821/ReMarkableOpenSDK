// Real owned file/FD controls; the external admission is modeled, never native.
#include "reader_capture_pair_read.h"
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
using namespace reader_capture_pair_read;
unsigned scenarios = 0, checks = 0;
void check(bool value, const char *label) { ++checks; if (!value) throw std::runtime_error(label); }
void scenario(const char *name, const std::function<void()> &body) {
    ++scenarios; try { body(); } catch (...) { std::cerr << "FAIL " << name << '\n'; throw; }
}
const Permit yes = [] { return true; };
Expected binding() { return {QByteArray(32, 'a'), QByteArrayLiteral("123"), QByteArrayLiteral("456")}; }
int fdCount() { int n = 0; for (int fd = 0; fd < 1024; ++fd) if (::fcntl(fd, F_GETFD) >= 0) ++n; return n; }
struct Root {
    QTemporaryDir temp;
    QByteArray path;
    int fd = -1;
    Expected expected = binding();
    QByteArray scalar = QByteArray("scalar\0original\xff", 16), image = QByteArray(8201, char(0xa7));
    Root() {
        check(temp.isValid(), "temporary directory");
        path = QFile::encodeName(temp.path() + QStringLiteral("/owned"));
        check(::mkdir(path.constData(), 0700) == 0, "owned root");
        fd = ::open(path.constData(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        check(fd >= 0, "root descriptor"); reset();
    }
    ~Root() { if (fd >= 0) ::close(fd); }
    QByteArray file(const char *name) const { return path + '/' + name; }
    void put(const char *name, const QByteArray &bytes, mode_t mode = 0600) const {
        const int f = ::open(file(name).constData(), O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW | O_CLOEXEC, mode);
        check(f >= 0, "write open");
        const auto n = ::write(f, bytes.constData(), size_t(bytes.size()));
        const bool close = ::close(f) == 0; check(n == bytes.size() && close, "write complete");
    }
    void reset() {
        const auto identity = expected.pid + ' ' + expected.start + '\n';
        put("owner", expected.nonce); put("attempt.identity", identity);
        put("selected-ready", expected.nonce + ' ' + identity); put("selected-request", expected.nonce + ' ' + identity);
        put("selected-result", scalar); put("visual-image.png", image);
    }
    QByteArray read(const char *name) const { QFile f(QString::fromLocal8Bit(file(name))); check(f.open(QIODevice::ReadOnly), "fixture read"); return f.readAll(); }
};
void refused(const Result &r) { check(!r.observed && !r.refusal.isEmpty() && r.scalar.isEmpty() && r.image.isEmpty(), "no partial output on refusal"); }
void accepted(const Result &r, const Root &root) {
    check(r.observed && r.attempted && r.refusal.isEmpty(), "observed conjunction");
    check(r.scalar == root.scalar && r.image == root.image, "original unparsed bytes");
    check(!Result::liveProcessAuthority, "no process authority");
}
struct DestructorState { bool armed = false, fired = false; std::function<void()> action; };
struct DestructorPermit {
    std::shared_ptr<DestructorState> state;
    bool operator()() const { state->armed = true; return true; }
    ~DestructorPermit() { if (state && state->armed && !state->fired) { state->fired = true; state->action(); } }
};
}
int main() {
    try {
        scenario("original-binary-pair", [] { Root r; const int n = fdCount(); const auto result = readPair(r.fd, r.expected, yes); accepted(result, r); check(fdCount() == n, "all owned FDs closed"); check(r.read("selected-result") == r.scalar && r.read("visual-image.png") == r.image, "read only"); });
        for (const int size : {1, 4096, 4097, 1048576}) scenario("bounded-chunk-images", [=] { Root r; r.image = QByteArray(size, char(0xee)); r.scalar = QByteArray(4096, '\0'); r.reset(); accepted(readPair(r.fd, r.expected, yes), r); });
        scenario("canonical-u64-max", [] { Root r; r.expected.pid = "18446744073709551615"; r.expected.start = r.expected.pid; r.reset(); accepted(readPair(r.fd, r.expected, yes), r); });
        scenario("owned-duplicate-survives-source-close-reuse", [] { Root r; const int old = r.fd; int replacement = -1; bool first = true; const auto result = readPair(r.fd, r.expected, [&] { if (first) { first = false; check(::close(r.fd) == 0, "close borrowed FD"); r.fd = -1; replacement = ::open("/dev/null", O_RDONLY | O_CLOEXEC); check(replacement == old, "reuse caller descriptor"); } return true; }); accepted(result, r); check(::close(replacement) == 0, "close replacement"); });
        scenario("copied-inputs-survive-owner-death", [] { Root r; struct Input { Expected e; Permit p; }; auto owner = std::make_unique<Input>(); owner->e = r.expected; owner->p = [&] { owner.reset(); return true; }; const auto result = readPair(r.fd, owner->e, owner->p); accepted(result, r); check(!owner, "caller destroyed"); });
        scenario("all-six-pins-and-flags", [] { Root r; bool sawAll = false; const auto result = readPair(r.fd, r.expected, [&] { int found = 0; for (const char *name : detail::Names) { struct stat named{}; check(::lstat(r.file(name).constData(), &named) == 0, "named stat"); for (int fd = 0; fd < 128; ++fd) { struct stat st{}; if (::fstat(fd, &st) == 0 && st.st_dev == named.st_dev && st.st_ino == named.st_ino) { const int flags = ::fcntl(fd, F_GETFL); check((flags & O_ACCMODE) == O_RDONLY && (flags & O_NONBLOCK), "read-only nonblock pin"); check(::fcntl(fd, F_GETFD) & FD_CLOEXEC, "closeexec pin"); ++found; } } } if (found == 6) sawAll = true; return true; }); accepted(result, r); check(sawAll, "all six retained"); });
        for (const char *name : detail::Names) {
            scenario("missing-file", [=] { Root r; check(::unlink(r.file(name).constData()) == 0, "unlink"); refused(readPair(r.fd, r.expected, yes)); });
            scenario("file-mode", [=] { Root r; check(::chmod(r.file(name).constData(), 0640) == 0, "chmod"); refused(readPair(r.fd, r.expected, yes)); });
            scenario("file-hardlink", [=] { Root r; check(::link(r.file(name).constData(), r.file("extra-link").constData()) == 0, "hardlink"); refused(readPair(r.fd, r.expected, yes)); });
            scenario("file-symlink", [=] { Root r; check(::rename(r.file(name).constData(), r.file("original").constData()) == 0 && ::symlink("original", r.file(name).constData()) == 0, "symlink"); refused(readPair(r.fd, r.expected, yes)); });
            scenario("file-fifo", [=] { Root r; check(::unlink(r.file(name).constData()) == 0 && ::mkfifo(r.file(name).constData(), 0600) == 0, "fifo"); refused(readPair(r.fd, r.expected, yes)); });
            scenario("file-directory", [=] { Root r; check(::unlink(r.file(name).constData()) == 0 && ::mkdir(r.file(name).constData(), 0600) == 0, "directory"); refused(readPair(r.fd, r.expected, yes)); });
            scenario("file-foreign-owner", [=] { Root r; check(::geteuid() == 0, "owned Docker root supports chown control"); check(::chown(r.file(name).constData(), 1, gid_t(-1)) == 0, "foreign owner"); refused(readPair(r.fd, r.expected, yes)); });
        }
        for (const char *name : detail::Absent) {
            scenario("blocker-file", [=] { Root r; r.put(name, "x"); refused(readPair(r.fd, r.expected, yes)); });
            scenario("blocker-dangling-symlink", [=] { Root r; check(::symlink("missing", r.file(name).constData()) == 0, "dangling blocker"); refused(readPair(r.fd, r.expected, yes)); });
        }
        for (const char *name : {"owner", "attempt.identity", "selected-ready", "selected-request"}) {
            scenario("binding-wrong-same-size", [=] { Root r; auto bytes = r.read(name); bytes[0] = '9'; r.put(name, bytes); refused(readPair(r.fd, r.expected, yes)); });
            scenario("binding-no-newline-or-short", [=] { Root r; auto bytes = r.read(name); bytes.chop(1); r.put(name, bytes); refused(readPair(r.fd, r.expected, yes)); });
            scenario("binding-extra-byte", [=] { Root r; r.put(name, r.read(name) + '\n'); refused(readPair(r.fd, r.expected, yes)); });
        }
        for (const auto &bad : std::vector<QByteArray>{"", "0", "01", "-1", "+1", "1\n", "18446744073709551616", "999999999999999999999"}) for (bool pid : {false, true}) scenario("invalid-expected-number", [=] { Root r; auto e = r.expected; if (pid) e.pid = bad; else e.start = bad; const auto result = readPair(r.fd, e, yes); refused(result); check(!result.attempted, "invalid binding no read attempt"); });
        for (const auto &bad : std::vector<QByteArray>{QByteArray(), QByteArray(31, 'a'), QByteArray(33, 'a'), QByteArray(32, 'A'), QByteArray(32, 'g'), QByteArray(32, '\0')}) scenario("invalid-nonce", [=] { Root r; auto e = r.expected; e.nonce = bad; refused(readPair(r.fd, e, yes)); });
        scenario("bad-root-fd", [] { Root r; const auto result = readPair(-1, r.expected, yes); refused(result); check(!result.attempted, "bad root no attempt"); });
        scenario("root-mode", [] { Root r; check(::chmod(r.path.constData(), 0750) == 0, "root mode change"); refused(readPair(r.fd, r.expected, yes)); });
        scenario("root-owner", [] { Root r; check(::chown(r.path.constData(), 1, gid_t(-1)) == 0, "root uid change"); refused(readPair(r.fd, r.expected, yes)); });
        scenario("root-not-directory", [] { Root r; const int fd = ::open(r.file("owner").constData(), O_RDONLY); check(fd >= 0, "file fd"); const auto result = readPair(fd, r.expected, yes); check(::close(fd) == 0, "close test fd"); refused(result); });
        for (const char *name : {"selected-result", "visual-image.png"}) for (bool large : {false, true}) scenario("pair-size", [=] { Root r; const int size = !large ? 0 : (QByteArray(name) == "selected-result" ? int(ScalarCap + 1) : int(ImageCap + 1)); r.put(name, QByteArray(size, 'x')); refused(readPair(r.fd, r.expected, yes)); });
        int permits = 0;
        scenario("measure-fresh-boundaries", [&] { Root r; accepted(readPair(r.fd, r.expected, [&] { ++permits; return true; }), r); check(permits > 30 && permits < 100, "bounded fresh brackets"); });
        for (int at = 1; at <= permits; ++at) scenario("permission-loss-every-boundary", [=] { Root r; int calls = 0; const int old = fdCount(); const auto result = readPair(r.fd, r.expected, [&] { return ++calls != at; }); refused(result); check(calls == at && fdCount() == old, "sticky revoke closes all and no later permit"); });
        for (const int at : {1, 15, permits}) {
            scenario("throwing-permission", [=] { Root r; int calls = 0; refused(readPair(r.fd, r.expected, [&] { if (++calls == at) throw std::runtime_error("owned permit"); return true; })); check(calls == at, "throw sticky"); });
            scenario("permission-reentry", [=] { Root r; int calls = 0; const auto result = readPair(r.fd, r.expected, [&] { if (++calls == at) { const auto nested = readPair(r.fd, r.expected, yes); refused(nested); check(!nested.attempted && nested.refusal == "reentered", "nested refused"); } return true; }); refused(result); check(calls == at, "reentry poisons outer"); });
        }
        for (const char *name : detail::Names) scenario("late-unlink-recreate-pinned-inode", [=] { Root r; int calls = 0; const auto original = r.read(name); const auto result = readPair(r.fd, r.expected, [&] { if (++calls == 14) { struct stat before{}, after{}; check(::lstat(r.file(name).constData(), &before) == 0, "before pinned inode"); check(::unlink(r.file(name).constData()) == 0, "late unlink"); r.put(name, original); check(::lstat(r.file(name).constData(), &after) == 0 && before.st_ino != after.st_ino, "open pin prevents inode reuse"); } return true; }); refused(result); });
        for (const char *name : {"selected-result", "visual-image.png"}) {
            scenario("late-rename-replacement", [=] { Root r; int calls = 0; const auto original = r.read(name); refused(readPair(r.fd, r.expected, [&] { if (++calls == 14) { check(::rename(r.file(name).constData(), r.file("retained-original").constData()) == 0, "retain old file"); r.put(name, original); } return true; })); });
            scenario("late-inplace-metadata", [=] { Root r; int calls = 0; refused(readPair(r.fd, r.expected, [&] { if (++calls == 14) check(::chmod(r.file(name).constData(), 0400) == 0, "mutate mode"); return true; })); });
            scenario("late-hardlink", [=] { Root r; int calls = 0; refused(readPair(r.fd, r.expected, [&] { if (++calls == 14) check(::link(r.file(name).constData(), r.file("extra").constData()) == 0, "late link"); return true; })); });
        }
        scenario("partial-image-truncate-after-first-chunk", [] { Root r; int calls = 0; bool truncated = false; const auto result = readPair(r.fd, r.expected, [&] { if (++calls == 36) { check(::truncate(r.file("visual-image.png").constData(), 4096) == 0, "partial image"); truncated = true; } return true; }); refused(result); check(truncated, "read stage reached"); });
        scenario("late-directory-metadata-mutation", [] { Root r; int calls = 0; refused(readPair(r.fd, r.expected, [&] { if (++calls == 14) { r.put("unrelated-new-file", "x"); const struct timespec times[2]{{1, 0}, {1, 0}}; check(::utimensat(AT_FDCWD, r.path.constData(), times, AT_SYMLINK_NOFOLLOW) == 0, "different directory mtime despite coarse clocks"); } return true; })); });
        for (const char *name : detail::Absent) scenario("late-cancel-pending-restoration-stop", [=] { Root r; int calls = 0; refused(readPair(r.fd, r.expected, [&] { if (++calls == 14) r.put(name, "late"); return true; })); check(calls == 14, "late blocker stops before bytes"); });
        scenario("empty-permission", [] { Root r; refused(readPair(r.fd, r.expected, {})); });
        scenario("permit-capture-destructor-reentry", [] { Root r; auto state = std::make_shared<DestructorState>(); bool nestedRefused = false; state->action = [&] { const auto nested = readPair(r.fd, r.expected, yes); nestedRefused = !nested.observed && nested.refusal == "reentered"; }; const Permit permit = DestructorPermit{state}; const auto result = readPair(r.fd, r.expected, permit); refused(result); check(state->fired && nestedRefused && result.refusal == "permit-unwind-binding", "destructor reentry seen before byte exposure"); });
        scenario("permit-capture-destructor-file-replacement", [] { Root r; auto state = std::make_shared<DestructorState>(); state->action = [&] { check(::rename(r.file("selected-result").constData(), r.file("retained").constData()) == 0, "destructor preserves old scalar"); r.put("selected-result", r.scalar); }; const Permit permit = DestructorPermit{state}; const auto result = readPair(r.fd, r.expected, permit); refused(result); check(state->fired && result.refusal == "permit-unwind-binding", "destructor replacement bracketed"); });
        scenario("permit-capture-destructor-source-close", [] { Root r; auto state = std::make_shared<DestructorState>(); state->action = [&] { check(::close(r.fd) == 0, "destructor closes caller FD"); r.fd = -1; }; const Permit permit = DestructorPermit{state}; const auto result = readPair(r.fd, r.expected, permit); accepted(result, r); check(state->fired, "source FD lifetime independent"); });
        scenario("retained-read-descriptor-lost", [] { Root r; int calls = 0; int closed = -1; const auto result = readPair(r.fd, r.expected, [&] { if (++calls == 14) { struct stat named{}; check(::lstat(r.file("visual-image.png").constData(), &named) == 0, "image named stat"); for (int fd = 0; fd < 128; ++fd) { struct stat st{}; if (::fstat(fd, &st) == 0 && st.st_dev == named.st_dev && st.st_ino == named.st_ino) { closed = fd; check(::close(fd) == 0, "injected descriptor loss"); break; } } check(closed >= 0, "owned retained descriptor found"); } return true; }); refused(result); check(calls == 14, "bad FD stops read"); });
        scenario("external-path-permit-refuses-root-replacement", [] { Root r; int calls = 0; struct stat original{}; check(::fstat(r.fd, &original) == 0, "original root"); refused(readPair(r.fd, r.expected, [&] { if (++calls == 14) { check(::rename(r.path.constData(), (r.path + "-old").constData()) == 0 && ::mkdir(r.path.constData(), 0700) == 0, "replace root path"); } struct stat named{}; return ::lstat(r.path.constData(), &named) == 0 && named.st_dev == original.st_dev && named.st_ino == original.st_ino; })); });
        scenario("fresh-result-owned-after-collection", [] { Root r; auto result = readPair(r.fd, r.expected, yes); accepted(result, r); r.put("selected-result", "changed"); r.put("visual-image.png", "changed"); check(result.scalar == r.scalar && result.image == r.image, "owned arrays survive later writers without live claim"); });
        std::cout << "PASS capture pair read owned scenarios=" << scenarios << " checks=" << checks << " original_bytes=true modeled_external_permit=true live_process_authority=false native_execution=false\n";
        return 0;
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
