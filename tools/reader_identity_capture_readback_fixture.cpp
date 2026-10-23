// Own-process admission and original framing only. Target/provider facts are not
// supplied by this fixture and no device or target application is contacted.
#define READER_CAPTURE_READBACK_OWNED_HELPERS_ONLY
#include "reader_identity_capture_readback.cpp"
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <sys/resource.h>
#include <sys/wait.h>
#include <signal.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
namespace core = reader_capture_pair_read;
using capture_process_readback::Admission;
unsigned scenarios = 0, checks = 0;
void check(bool value, const char *label) { ++checks; if (!value) throw std::runtime_error(label); }
void scenario(const char *name, const std::function<void()> &body) { ++scenarios; try { body(); } catch (...) { std::cerr << "FAIL " << name << '\n'; throw; } }
quint64 ownClockMs() { struct timespec t{}; check(::clock_gettime(CLOCK_BOOTTIME, &t) == 0, "independent boot clock"); return quint64(t.tv_sec) * 1000 + quint64(t.tv_nsec / 1000000); }
QByteArray original(const QString &path) { QFile file(path); check(file.open(QIODevice::ReadOnly), "original read"); return file.readAll(); }
QByteArray ownStart(pid_t pid) {
    const auto raw = original(QStringLiteral("/proc/%1/stat").arg(pid));
    const auto end = raw.lastIndexOf(')'); check(end >= 0, "proc comm delimiter");
    const auto fields = raw.mid(end + 2).trimmed().split(' '); check(fields.size() > 19, "proc start index"); return fields[19];
}
QByteArray ownBoot() { return original(QStringLiteral("/proc/sys/kernel/random/boot_id")).trimmed(); }
int fdCount() { int n = 0; for (int fd = 0; fd < 1024; ++fd) if (::fcntl(fd, F_GETFD) >= 0) ++n; return n; }
struct Root {
    QTemporaryDir temporary;
    QString path;
    core::Expected expected{QByteArray(32, 'd'), QByteArray::number(::getpid()), ownStart(::getpid())};
    QByteArray boot = ownBoot();
    quint64 before = ownClockMs() - 1, armed = before / 1000 - 3, end = armed + 360, began = armed + 1, ends = began + 90;
    QByteArray scalar = QByteArray("original\0scalar\xff", 16), image = QByteArray("png\0original\xff", 13);
    Root() { check(temporary.isValid(), "temporary root"); path = temporary.path() + QStringLiteral("/owned"); check(::mkdir(QFile::encodeName(path).constData(), 0700) == 0, "private owned root"); controls(); pair(); }
    QByteArray file(const char *name) const { return QFile::encodeName(path + '/' + QString::fromLatin1(name)); }
    void put(const char *name, const QByteArray &bytes, mode_t mode = 0600) const {
        const int fd = ::open(file(name).constData(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_NOFOLLOW, mode);
        check(fd >= 0, "write open"); const auto n = ::write(fd, bytes.constData(), size_t(bytes.size())); const bool closed = ::close(fd) == 0; check(n == bytes.size() && closed, "complete private write");
    }
    void controls() { put("deadline", QByteArray::number(armed) + ' ' + QByteArray::number(end) + '\n'); put("candidate.window", expected.nonce + ' ' + QByteArray::number(armed) + ' ' + QByteArray::number(end) + ' ' + QByteArray::number(began) + ' ' + QByteArray::number(ends) + '\n'); }
    void pair() { const auto id = expected.pid + ' ' + expected.start + '\n'; put("owner", expected.nonce); put("attempt.identity", id); put("selected-ready", expected.nonce + ' ' + id); put("selected-request", expected.nonce + ' ' + id); put("selected-result", scalar); put("visual-image.png", image); }
    std::unique_ptr<Admission> make() const { return std::make_unique<Admission>(path, expected, boot, before); }
};
struct Child {
    pid_t pid = -1;
    Child() {
        int fds[2]; check(::pipe(fds) == 0, "child ready pipe"); pid = ::fork(); check(pid >= 0, "actual fork");
        if (pid == 0) { ::close(fds[0]); const char byte = 'r'; const auto n = ::write(fds[1], &byte, 1); if (n != 1) ::_Exit(100); ::close(fds[1]); for (;;) ::pause(); }
        ::close(fds[1]); char byte = 0; check(::read(fds[0], &byte, 1) == 1 && byte == 'r', "child alive"); check(::close(fds[0]) == 0, "ready pipe close");
    }
    void stop() { check(pid > 0 && ::kill(pid, SIGKILL) == 0, "stop only owned child"); int status = 0; check(::waitpid(pid, &status, 0) == pid && WIFSIGNALED(status), "reap child"); pid = -1; }
    ~Child() { if (pid > 0) { ::kill(pid, SIGKILL); int status = 0; ::waitpid(pid, &status, 0); } }
};
void refused(Admission &admission) { check(!admission.permit() && !admission.refusal().isEmpty(), "sticky admission refusal"); }
}

int main() {
    try {
        scenario("actual-self-admission-and-original-pair", [] { Root root; const int old = fdCount(); { auto admission = root.make(); check(admission->start() && admission->permit() && admission->rootFd() >= 0, "real own proc/boot/time accepted"); const auto result = core::readPair(admission->rootFd(), root.expected, [&] { return admission->permit(); }); check(result.observed && result.scalar == root.scalar && result.image == root.image && admission->permit(), "core original bytes bracketed with actual process"); } check(fdCount() == old, "wrapper and core FDs close"); });
        scenario("admission-scope-unwind-closes-pins", [] { Root root; const int old = fdCount(); bool caught = false; try { auto admission = root.make(); check(admission->start() && fdCount() == old + 3, "root plus two control pins retained"); throw std::runtime_error("owned caller unwind"); } catch (const std::runtime_error &e) { if (QByteArray(e.what()) != "owned caller unwind") throw; caught = true; } check(caught && fdCount() == old, "actual exception unwind closes pins"); });
        scenario("start-once-sticky", [] { Root root; auto admission = root.make(); check(admission->start(), "first admission"); check(!admission->start(), "second start refused"); refused(*admission); });
        scenario("permit-before-start-no-authority", [] { Root root; auto admission = root.make(); check(!admission->permit(), "not initialized"); check(admission->start(), "single explicit start still possible"); });
        for (const char *name : {"deadline", "candidate.window"}) {
            scenario("missing-control", [=] { Root root; check(::unlink(root.file(name).constData()) == 0, "missing control"); auto a = root.make(); check(!a->start(), "start refused"); refused(*a); });
            scenario("control-mode", [=] { Root root; check(::chmod(root.file(name).constData(), 0640) == 0, "control mode"); auto a = root.make(); check(!a->start(), "start refused"); });
            scenario("control-hardlink", [=] { Root root; check(::link(root.file(name).constData(), root.file("extra").constData()) == 0, "control hardlink"); auto a = root.make(); check(!a->start(), "start refused"); });
            scenario("control-symlink", [=] { Root root; check(::rename(root.file(name).constData(), root.file("saved").constData()) == 0 && ::symlink("saved", root.file(name).constData()) == 0, "control symlink"); auto a = root.make(); check(!a->start(), "start refused"); });
            scenario("control-fifo", [=] { Root root; check(::unlink(root.file(name).constData()) == 0 && ::mkfifo(root.file(name).constData(), 0600) == 0, "nonblocking fifo"); auto a = root.make(); check(!a->start(), "start refused without block"); });
            scenario("control-foreign-owner", [=] { Root root; check(::geteuid() == 0 && ::chown(root.file(name).constData(), 1, gid_t(-1)) == 0, "different owner"); auto a = root.make(); check(!a->start(), "start refused"); });
            scenario("control-no-newline", [=] { Root root; auto bytes = original(QString::fromLocal8Bit(root.file(name))); bytes.chop(1); root.put(name, bytes); auto a = root.make(); check(!a->start(), "format refused"); });
            scenario("control-oversize", [=] { Root root; root.put(name, QByteArray(129, '1')); auto a = root.make(); check(!a->start(), "cap refused"); });
            scenario("pinned-control-replaced", [=] { Root root; auto a = root.make(); check(a->start(), "initially live"); auto bytes = original(QString::fromLocal8Bit(root.file(name))); check(::rename(root.file(name).constData(), root.file("saved").constData()) == 0, "rename retained control"); root.put(name, bytes); refused(*a); });
            scenario("pinned-control-bytes-changed", [=] { Root root; auto a = root.make(); check(a->start(), "initially live"); auto bytes = original(QString::fromLocal8Bit(root.file(name))); bytes[0] = bytes[0] == '1' ? '2' : '1'; root.put(name, bytes); refused(*a); });
            scenario("pinned-control-mode-changed", [=] { Root root; auto a = root.make(); check(a->start(), "initially live"); check(::chmod(root.file(name).constData(), 0400) == 0, "late mode"); refused(*a); });
        }
        const std::vector<QByteArray> badNumbers{"", "0", "01", "-1", "+2", "2\n", "18446744073709551616"};
        for (const auto &bad : badNumbers) for (const bool pid : {false, true}) scenario("canonical-process-arguments", [=] { Root root; if (pid) root.expected.pid = bad; else root.expected.start = bad; auto a = root.make(); check(!a->start(), "argument refused"); });
        for (const auto &bad : std::vector<QByteArray>{"1", "2147483648", "18446744073709551615"}) scenario("pid-bounds", [=] { Root root; root.expected.pid = bad; auto a = root.make(); check(!a->start(), "PID rejected"); });
        for (const auto &bad : std::vector<QByteArray>{QByteArray(), QByteArray(31, 'a'), QByteArray(32, 'A'), QByteArray(32, 'z')}) scenario("nonce-arguments", [=] { Root root; root.expected.nonce = bad; auto a = root.make(); check(!a->start(), "nonce rejected"); });
        for (const auto &bad : std::vector<QByteArray>{"", "00000000-0000-0000-0000-000000000000", "AAAAAAAA-0000-0000-0000-000000000001", "00000000_0000-0000-0000-000000000001", "00000000-0000-0000-0000-000000000001\n"}) scenario("boot-arguments", [=] { Root root; root.boot = bad; auto a = root.make(); check(!a->start(), "boot rejected"); });
        scenario("actual-boot-mismatch", [] { Root root; root.boot[0] = root.boot[0] == 'a' ? 'b' : 'a'; auto a = root.make(); check(!a->start() && a->refusal() == "boot", "different actual boot refused"); });
        scenario("actual-start-mismatch", [] { Root root; root.expected.start = QByteArray::number(root.expected.start.toULongLong() + 1); auto a = root.make(); check(!a->start() && a->refusal() == "process", "same PID wrong start refused"); });
        for (int which = 0; which < 11; ++which) scenario("control-clock-shape", [=] { Root root;
            switch (which) {
            case 0: root.put("deadline", "1 2\n"); break;
            case 1: root.end = root.armed + 359; root.controls(); break;
            case 2: root.ends = root.began + 89; root.controls(); break;
            case 3: root.began = root.armed - 1; root.ends = root.began + 90; root.controls(); break;
            case 4: root.began = root.armed + 100; root.ends = root.began + 90; root.controls(); break;
            case 5: root.put("candidate.window", QByteArray(32, 'e') + ' ' + QByteArray::number(root.armed) + ' ' + QByteArray::number(root.end) + ' ' + QByteArray::number(root.began) + ' ' + QByteArray::number(root.ends) + '\n'); break;
            case 6: root.put("candidate.window", root.expected.nonce + " 01 361 2 92\n"); break;
            case 7: root.put("candidate.window", root.expected.nonce + " 18446744073709551615 1 2 92\n"); break;
            case 8: root.put("candidate.window", root.expected.nonce + " 1 361 18446744073709551615 89\n"); break;
            case 9: root.put("candidate.window", root.expected.nonce + " 1 361 2 92 extra\n"); break;
            default: root.put("deadline", QByteArray::number(root.armed) + "  " + QByteArray::number(root.end) + '\n'); break;
            } auto a = root.make(); check(!a->start(), "clock shape refused"); });
        scenario("zero-before", [] { Root root; root.before = 0; auto a = root.make(); check(!a->start(), "zero timestamp"); });
        scenario("overflow-before", [] { Root root; root.before = std::numeric_limits<quint64>::max() - 29999; auto a = root.make(); check(!a->start(), "checked addition"); });
        scenario("future-before", [] { Root root; root.before = ownClockMs() + 10000; auto a = root.make(); check(!a->start() && a->refusal() == "deadline", "future lower bound refused"); });
        scenario("before-candidate-begin", [] { Root root; root.before = root.began * 1000 - 1; auto a = root.make(); check(!a->start() && a->refusal() == "request-clock", "request lower bound before window"); });
        scenario("before-at-candidate-end", [] { Root root; root.before = root.ends * 1000; auto a = root.make(); check(!a->start() && a->refusal() == "request-clock", "window end excluded"); });
        scenario("expired-original-operation", [] { Root root; const auto now = ownClockMs(); root.armed = now / 1000 - 41; root.end = root.armed + 360; root.began = root.armed + 1; root.ends = root.began + 90; root.before = now - 30001; root.controls(); auto a = root.make(); check(!a->start() && a->refusal() == "deadline", "original30s not renewed"); });
        scenario("candidate-expiry-with-live-operation", [] { Root root; root.ends = ownClockMs() / 1000 + 2; root.began = root.ends - 90; root.armed = root.began - 1; root.end = root.armed + 360; root.controls(); auto a = root.make(); check(a->start(), "at least one second remaining candidate headroom"); ::usleep(2100000); refused(*a); check(a->refusal() == "deadline", "original90s not renewed"); });
        scenario("request-expiry-after-start", [] { Root root; const auto now = ownClockMs(); root.armed = now / 1000 - 41; root.end = root.armed + 360; root.began = root.armed + 1; root.ends = root.began + 90; root.before = now - 29500; root.controls(); auto a = root.make(); check(a->start(), "original remaining operation"); ::usleep(600000); refused(*a); check(a->refusal() == "deadline", "operation bound survives repeated permit"); });
        scenario("local10s-includes-constructor", [] { Root root; auto a = root.make(); const auto before = ownClockMs(); ::usleep(10050000); check(ownClockMs() - before >= 10000, "actual elapsed boot-clock wait"); check(!a->start() && a->refusal() == "deadline", "constructor clock not rebased at start"); });
        for (const char *name : {"selected-cancel", "restore.claim", "stop.request"}) {
            scenario("stop-sticky", [=] { Root root; auto a = root.make(); check(a->start(), "initially live"); root.put(name, "x"); refused(*a); check(::unlink(root.file(name).constData()) == 0, "remove fixture marker"); refused(*a); });
            scenario("stop-dangling-symlink", [=] { Root root; auto a = root.make(); check(a->start(), "initially live"); check(::symlink("absent", root.file(name).constData()) == 0, "dangling stop"); refused(*a); });
        }
        scenario("root-mode-loss", [] { Root root; auto a = root.make(); check(a->start(), "initially live"); check(::chmod(QFile::encodeName(root.path).constData(), 0750) == 0, "root mode"); refused(*a); });
        scenario("root-path-replacement", [] { Root root; auto a = root.make(); check(a->start(), "initially live"); const auto path = QFile::encodeName(root.path); check(::rename(path.constData(), (path + "-old").constData()) == 0 && ::mkdir(path.constData(), 0700) == 0, "new named root"); refused(*a); });
        scenario("root-path-symlink", [] { Root root; auto a = root.make(); check(a->start(), "initially live"); const auto path = QFile::encodeName(root.path); check(::rename(path.constData(), (path + "-old").constData()) == 0 && ::symlink((path + "-old").constData(), path.constData()) == 0, "root symlink"); refused(*a); });
        scenario("root-fd-retained-after-rename", [] { Root root; auto a = root.make(); check(a->start(), "initially live"); struct stat fdBefore{}, fdAfter{}; check(::fstat(a->rootFd(), &fdBefore) == 0, "root pin"); const auto path = QFile::encodeName(root.path); check(::rename(path.constData(), (path + "-old").constData()) == 0 && ::fstat(a->rootFd(), &fdAfter) == 0 && fdBefore.st_ino == fdAfter.st_ino, "pin survives rename"); refused(*a); });
        scenario("actual-child-process-loss", [] { Root root; Child child; root.expected.pid = QByteArray::number(child.pid); root.expected.start = ownStart(child.pid); root.pair(); auto a = root.make(); check(a->start(), "actual child admitted"); child.stop(); refused(*a); check(a->refusal() == "process", "missing proc refused"); });
        scenario("actual-zombie-process-no-live-authority", [] { Root root; Child child; root.expected.pid = QByteArray::number(child.pid); root.expected.start = ownStart(child.pid); root.pair(); auto a = root.make(); check(a->start(), "actual child admitted"); check(::kill(child.pid, SIGKILL) == 0, "kill only owned child without reaping"); bool zombie = false; for (int i = 0; i < 200 && !zombie; ++i) { const auto raw = original(QStringLiteral("/proc/%1/stat").arg(child.pid)); zombie = raw.mid(raw.lastIndexOf(')') + 2, 1) == "Z"; if (!zombie) ::usleep(5000); } check(zombie && ownStart(child.pid) == root.expected.start, "actual PID/start remains but state is zombie"); refused(*a); check(a->refusal() == "process", "zombie is not live"); int status = 0; check(::waitpid(child.pid, &status, 0) == child.pid && WIFSIGNALED(status), "reap owned zombie"); child.pid = -1; });
        scenario("post-core-fresh-admission", [] { Root root; auto a = root.make(); check(a->start(), "initial admission"); const auto result = core::readPair(a->rootFd(), root.expected, [&] { return a->permit(); }); check(result.observed, "core sampled originals"); root.put("stop.request", "stop"); check(!a->permit(), "separate after-core bracket refuses"); });
        scenario("binary-frame-originals", [] { Root root; auto a = root.make(); check(a->start(), "admission"); const auto result = core::readPair(a->rootFd(), root.expected, [&] { return a->permit(); }); check(result.observed && a->permit(), "post-core admission"); const auto frame = capture_process_readback::makeFrame(result, root.expected, root.boot); const auto expected = QByteArray("RMB_CAPTURE_PAIR_V1 ") + root.expected.nonce + ' ' + root.expected.pid + ' ' + root.expected.start + ' ' + root.boot + ' ' + QByteArray::number(root.scalar.size()) + ' ' + QByteArray::number(root.image.size()) + '\n' + root.scalar + root.image + "\nRMB_CAPTURE_PAIR_END\n"; check(frame == expected, "independent exact framing oracle including NUL/high bytes"); });
        scenario("shipped-original-pair-frame-compatibility-only", [] {
            const QString assets = qEnvironmentVariable("READER_CAPTURE_READBACK_ASSETS"); check(!assets.isEmpty(), "explicit shipped asset path");
            const QByteArray expectation = original(assets + QStringLiteral("/expectation.json"));
            const auto facts = QJsonDocument::fromJson(expectation).object();
            check(facts.value(QStringLiteral("source")).toString() == QStringLiteral("owned-injected-capture-producer"), "explicit synthetic source");
            core::Expected expected{facts.value(QStringLiteral("nonce")).toString().toLatin1(), facts.value(QStringLiteral("pid")).toString().toLatin1(), facts.value(QStringLiteral("start")).toString().toLatin1()};
            core::Result result; result.observed = result.attempted = true;
            result.scalar = original(assets + QStringLiteral("/selected-result")); result.image = original(assets + QStringLiteral("/visual-image.png"));
            check(result.scalar.size() == facts.value(QStringLiteral("scalar_bytes")).toInt() && result.image.size() == facts.value(QStringLiteral("image_bytes")).toInt(), "shipped original sizes");
            check(QCryptographicHash::hash(result.scalar, QCryptographicHash::Sha256).toHex() == facts.value(QStringLiteral("scalar_sha256")).toString().toLatin1() && QCryptographicHash::hash(result.image, QCryptographicHash::Sha256).toHex() == facts.value(QStringLiteral("image_sha256")).toString().toLatin1(), "shipped original hashes");
            const QByteArray boot = ownBoot(); const auto frame = capture_process_readback::makeFrame(result, expected, boot);
            const auto header = QByteArray("RMB_CAPTURE_PAIR_V1 ") + expected.nonce + ' ' + expected.pid + ' ' + expected.start + ' ' + boot + ' ' + QByteArray::number(result.scalar.size()) + ' ' + QByteArray::number(result.image.size()) + '\n';
            check(frame == header + result.scalar + result.image + "\nRMB_CAPTURE_PAIR_END\n", "original C++ frame exact, no live admission");
            const QString output = qEnvironmentVariable("READER_CAPTURE_READBACK_EXPORT");
            if (!output.isEmpty()) {
                check(QDir().mkpath(output), "owned export directory");
                const auto write = [&](const char *name, const QByteArray &bytes) { const auto path = QFile::encodeName(output + '/' + QString::fromLatin1(name)); const int fd = ::open(path.constData(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600); check(fd >= 0, "exclusive original export"); const bool written = capture_process_readback::writeFrameOnce(fd, bytes); const bool closed = ::close(fd) == 0; check(written && closed, "single exact export write"); };
                write("original-frame.bin", frame); write("expectation.json", expectation); write("selected-result", result.scalar); write("visual-image.png", result.image);
                QJsonObject companion = facts;
                companion.insert(QStringLiteral("source"), QStringLiteral("owned-original-format-compatibility-only"));
                companion.insert(QStringLiteral("boot"), QString::fromLatin1(boot));
                companion.insert(QStringLiteral("live_admission"), false);
                companion.insert(QStringLiteral("target_provider_authority"), false);
                write("frame-facts.json", QJsonDocument(companion).toJson(QJsonDocument::Compact));
            }
        });
        scenario("max-bounded-frame-and-embedded-marker", [] { Root root; core::Result result; result.observed = result.attempted = true; result.scalar = QByteArray(4096, '\0'); result.image = QByteArray(1048576, char(0xff)); result.image.replace(10, 22, "\nRMB_CAPTURE_PAIR_END\n"); const auto frame = capture_process_readback::makeFrame(result, root.expected, root.boot); check(!frame.isEmpty() && frame.size() <= core::ImageCap + core::ScalarCap + 256 && frame.contains(result.scalar) && frame.contains(result.image), "bounded opaque bytes not marker-split"); });
        for (int which = 0; which < 10; ++which) scenario("frame-refuses-contradiction", [=] { Root root; core::Result result; result.observed = result.attempted = true; result.scalar = root.scalar; result.image = root.image; switch (which) { case 0: result.observed = false; break; case 1: result.attempted = false; break; case 2: result.refusal = "refused"; break; case 3: result.scalar.clear(); break; case 4: result.image.clear(); break; case 5: result.scalar = QByteArray(4097, 'x'); break; case 6: result.image = QByteArray(1048577, 'x'); break; case 7: root.expected.nonce = "bad"; break; case 8: root.expected.pid = "1"; break; default: root.boot = "bad"; break; } check(capture_process_readback::makeFrame(result, root.expected, root.boot).isEmpty(), "invalid pair no frame"); });
        scenario("single-original-frame-write", [] { Root root; const QByteArray frame("frame\0bytes\xff", 12); const int fd = ::open(root.file("frame.out").constData(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600); check(fd >= 0 && capture_process_readback::writeFrameOnce(fd, frame) && ::close(fd) == 0, "one actual write"); check(original(QString::fromLocal8Bit(root.file("frame.out"))) == frame, "original stream unchanged"); });
        scenario("frame-write-bad-or-empty", [] { check(!capture_process_readback::writeFrameOnce(-1, "x"), "bad FD"); check(!capture_process_readback::writeFrameOnce(STDOUT_FILENO, {}), "empty no output"); check(!capture_process_readback::writeFrameOnce(STDOUT_FILENO, QByteArray(int(core::ImageCap + core::ScalarCap + 257), 'x')), "overcap no output"); });
        for (const rlim_t limit : {rlim_t(0), rlim_t(7)}) scenario("actual-short-frame-write-no-retry", [=] { Root root; const QByteArray frame(512, 'f'); const int fd = ::open(root.file("short.out").constData(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600); check(fd >= 0, "short output"); const pid_t pid = ::fork(); check(pid >= 0, "short-write fork"); if (pid == 0) { ::signal(SIGXFSZ, SIG_IGN); const struct rlimit cap{limit, limit}; if (::setrlimit(RLIMIT_FSIZE, &cap)) ::_Exit(100); const bool okay = capture_process_readback::writeFrameOnce(fd, frame); struct stat st{}; const bool shape = ::fstat(fd, &st) == 0 && st.st_size == off_t(limit); ::_Exit(!okay && shape ? 0 : 101); } check(::close(fd) == 0, "parent close"); int status = 0; check(::waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0, "actual error/partial refused"); check(original(QString::fromLocal8Bit(root.file("short.out"))) == frame.left(int(limit)), "forensic partial bytes unchanged"); });
        std::cout << "PASS capture process readback owned scenarios=" << scenarios << " checks=" << checks << " actual_own_proc_boot_clock=true target_provider_authority=false native_execution=false\n";
        return 0;
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
