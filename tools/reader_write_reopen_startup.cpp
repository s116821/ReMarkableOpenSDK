// Fixed intended-target write/close/reopen research startup. No insertion hooks.
#if defined(READER_WRITE_REOPEN_TESTING) || defined(READER_FIXED_PEN_HANDOFF_TESTING) || defined(READER_ORDINARY_CLOSE_TESTING) || defined(READER_CAPTURE_FILES_TESTING) || defined(READER_SELECTED_TESTING) || defined(READER_IDENTITY_CAPTURE_TESTING) || defined(VISUAL_CAPTURE_TESTING) || defined(READER_WORKER_NATIVE_TESTING)
#error Fixture boundaries must not enter the write/reopen startup artifact.
#endif
#if defined(READER_SELECTED_NONCE) || defined(READER_SELECTED_OPEN_FIXTURE_JOIN) || defined(READER_SELECTED_CREATION_JOIN) || defined(READER_SELECTED_NAVIGATION_OWNERSHIP) || defined(READER_SELECTED_LEGACY_COMPOSITION) || defined(READER_SELECTED_WORKER_CALLBACK) || defined(READER_SELECTED_JOINED_CREATION) || defined(READER_INSERTION_NONCE) || defined(READER_INSERTION_FORWARD) || defined(READER_INSERTION_POST_REJECT_PROBE) || defined(READER_INSERTION_SELECT_TARGET)
#error Incompatible diagnostic selectors must not enter the write/reopen startup artifact.
#endif
#if !defined(READER_WRITE_REOPEN_NONCE) || !defined(READER_WRITE_REOPEN_DOCUMENT_ID) || !defined(READER_WRITE_REOPEN_PAGE_ID) || !defined(READER_WRITE_REOPEN_PAGE_INDEX) || !defined(READER_WRITE_REOPEN_PAGE_COUNT)
#error Write/reopen startup requires separately frozen nonce and complete expected identity.
#endif
#if !defined(__arm__) || !defined(__ARM_PCS_VFP) || __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error This research capture profile requires little-endian ARM32 hard-float.
#endif
#include "reader_selected_view_entry.h"
#include "reader_write_reopen_backend.h"
#include "reader_insertion_child_environment.h"
#include <cstring>
#include <ctime>
#include <cstdint>

namespace {
static_assert(sizeof(void *) == 4, "ARM32 capture profile");
constexpr char nonce[] = READER_WRITE_REOPEN_NONCE;
constexpr char rootPath[] = "/run/rmb-qt-probe-" READER_WRITE_REOPEN_NONCE;
constexpr char executableSha[] = "071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df";
constexpr qint64 executableBytes = 22046100;
constexpr bool validNonce() {
    if (sizeof nonce != 33) return false;
    for (unsigned i = 0; i < 32; ++i)
        if (!((nonce[i] >= '0' && nonce[i] <= '9') || (nonce[i] >= 'a' && nonce[i] <= 'f'))) return false;
    return true;
}
static_assert(validNonce(), "Frozen capture nonce must be32 lowercase hexadecimal characters");
static_assert(READER_WRITE_REOPEN_PAGE_COUNT >= 1 && READER_WRITE_REOPEN_PAGE_COUNT <= 16 &&
              READER_WRITE_REOPEN_PAGE_INDEX >= 0 && READER_WRITE_REOPEN_PAGE_INDEX < READER_WRITE_REOPEN_PAGE_COUNT, "Fixed capture indices");
std::atomic<bool> fingerprinted{false};
std::atomic_flag scheduled = ATOMIC_FLAG_INIT;
[[noreturn]] void stop() noexcept { std::_Exit(119); }

bool readPrivate(int root, const char *name, QByteArray &bytes) {
    reader_capture_files::detail::Fd fd(::openat(root, name, O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC));
    if (fd.value < 0) return false;
    struct stat before{}, after{}, named{}; char buffer[128];
    const bool regular = !::fstat(fd.value, &before) && S_ISREG(before.st_mode) && (before.st_mode & 07777) == 0600 &&
        before.st_uid == 0 && before.st_nlink == 1 && before.st_size > 0 && before.st_size < 128;
    const ssize_t count = regular ? ::read(fd.value, buffer, sizeof buffer) : -1;
    const bool valid = regular && count == before.st_size && !::fstat(fd.value, &after) && reader_capture_files::detail::same(before, after) &&
        !::fstatat(root, name, &named, AT_SYMLINK_NOFOLLOW) && reader_capture_files::detail::same(after, named);
    const bool closed = fd.close();
    if (valid && closed) bytes = QByteArray(buffer, count);
    return valid && closed;
}
bool actorAdmission() {
    if (::geteuid() != 0) return false;
    reader_capture_files::detail::Fd root(::open(rootPath, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
    if (root.value < 0) return false;
    struct stat before{}, after{}, named{};
    bool valid = !::fstat(root.value, &before) && S_ISDIR(before.st_mode) && (before.st_mode & 07777) == 0700 && before.st_uid == 0;
    const auto exact = [&](const char *name, const QByteArray &expected) { QByteArray actual; return readPrivate(root.value, name, actual) && actual == expected; };
    const auto absent = [&](const char *name) { struct stat st{}; return ::fstatat(root.value, name, &st, AT_SYMLINK_NOFOLLOW) && errno == ENOENT; };
    const QByteArray token(nonce);
    valid = valid && exact("owner", token) && exact("attempt.identity", reader_selected::selfIdentity());
    for (const char *name : {"actor.claim", "actor.ready", "activation.claim", "candidate-start.claim", "start.request", "attempt.claim"})
        valid = valid && exact(name, token + '\n');
    for (const char *name : {"restore.claim", "selected-cancel", "stop.request"}) valid = valid && absent(name);
    QByteArray deadline;
    valid = valid && readPrivate(root.value, "deadline", deadline) && deadline.endsWith('\n');
    const auto parts = deadline.chopped(1).split(' ');
    if (valid && parts.size() == 2 && reader_selected::decimal(parts[0]) && reader_selected::decimal(parts[1])) {
        const auto begin = parts[0].toULongLong(), end = parts[1].toULongLong(); struct timespec now{};
        valid = begin <= UINT64_MAX - 360 && end == begin + 360 && ::clock_gettime(CLOCK_BOOTTIME, &now) == 0 && now.tv_sec >= 0 &&
            std::uint64_t(now.tv_sec) >= begin && std::uint64_t(now.tv_sec) < end && end - std::uint64_t(now.tv_sec) > 185;
    } else valid = false;
    valid = valid && !::fstat(root.value, &after) && after.st_dev == before.st_dev && after.st_ino == before.st_ino &&
        after.st_uid == 0 && (after.st_mode & 07777) == 0700 && !::lstat(rootPath, &named) &&
        named.st_dev == before.st_dev && named.st_ino == before.st_ino && named.st_mode == before.st_mode && named.st_uid == 0;
    const bool closed = root.close(); return valid && closed;
}
bool executableAdmission() {
    char path[256];
    const auto length = ::readlink("/proc/self/exe", path, sizeof path);
    if (length != ssize_t(std::strlen("/usr/bin/xochitl")) || std::memcmp(path, "/usr/bin/xochitl", size_t(length))) return false;
    reader_capture_files::detail::Fd fd(::open("/proc/self/exe", O_RDONLY | O_CLOEXEC));
    if (fd.value < 0) return false;
    struct stat before{}, after{}, named{};
    if (::fstat(fd.value, &before) || !S_ISREG(before.st_mode) || before.st_uid != 0 || (before.st_mode & 022) ||
        before.st_size != executableBytes) return false;
    QCryptographicHash digest(QCryptographicHash::Sha256);
    char bytes[65536]; qint64 total = 0;
    while (total < executableBytes) {
        const auto n = ::read(fd.value, bytes, size_t(qMin<qint64>(sizeof bytes, executableBytes - total)));
        if (n <= 0) return false;
        digest.addData(QByteArrayView(bytes, n)); total += n;
    }
    char extra;
    const bool valid = ::read(fd.value, &extra, 1) == 0 && !::fstat(fd.value, &after) && reader_capture_files::detail::same(before, after) &&
        !::lstat("/usr/bin/xochitl", &named) && reader_capture_files::detail::same(after, named) && digest.result().toHex() == executableSha;
    const bool closed = fd.close(); return valid && closed;
}
__attribute__((constructor)) void admitBeforeMain() {
    try {
        const char *bind = std::getenv("LD_BIND_NOW"), *preload = std::getenv("LD_PRELOAD");
        if (QCoreApplication::instance() || !bind || std::strcmp(bind, "1") || !preload ||
            QByteArray(preload) != QByteArray(rootPath) + "/payload.so" || std::strcmp(qVersion(), "6.10.3") ||
            !actorAdmission() || !executableAdmission() || !actorAdmission() || !reader_child_environment::clearInheritedInjection()) stop();
        fingerprinted.store(true, std::memory_order_release);
    } catch (...) { stop(); }
}

class Controller final:public QObject {
public:
    explicit Controller(QGuiApplication *app):QObject(app),app_(app){}
    bool start(){
        if(!app_||QThread::currentThread()!=app_->thread()||!actorAdmission())return false;
        reader_capture_files::detail::Fd root(::open(rootPath,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));
        QByteArray window;if(root.value<0||!readPrivate(root.value,"candidate.window",window)||!root.close()||!window.endsWith('\n'))return false;
        const auto parts=window.chopped(1).split(' '),identity=reader_selected::selfIdentity().chopped(1).split(' ');
        if(parts.size()!=5||parts[0]!=nonce||identity.size()!=2||!reader_selected::decimal(parts[4]))return false;
        bool ok=false;const auto ends=parts[4].toULongLong(&ok);
        if(!ok||ends>std::numeric_limits<quint64>::max()/1000)return false;
        config_={{QStringLiteral(READER_WRITE_REOPEN_DOCUMENT_ID),QStringLiteral(READER_WRITE_REOPEN_PAGE_ID),READER_WRITE_REOPEN_PAGE_INDEX,READER_WRITE_REOPEN_PAGE_COUNT},QByteArray(nonce),identity[0],identity[1],ends*1000};
        QFile bootFile(QStringLiteral("/proc/sys/kernel/random/boot_id"));
        if(!bootFile.open(QIODevice::ReadOnly))return false;
        const auto boot=bootFile.read(64);bootFile.close();
        if(boot.size()!=37||!boot.endsWith('\n'))return false;
        io_=reader_write_reopen::NativeFiles::create(config_,boot.chopped(1));
        if(!io_)return false;
        queue();return true;
    }
private:
    void refuse(const char *why){
        if(done_)return;
        done_=true;
        if(io_){reader_write_reopen::Result r;r.finished=true;r.refusal=QString::fromLatin1(why);r.finishedMs=io_->nowMs();io_->report(r);}
    }
    void queue(){const QPointer<Controller> weak(this);QTimer::singleShot(250,this,[weak]{
        if(!weak)return;
        try{weak->poll();}catch(...){if(weak){try{weak->refuse("startup-exception");}catch(...){stop();}}}
    });}
    void poll(){
        if(done_)return;
        if(++polls_>200||!app_||QThread::currentThread()!=app_->thread()||!io_||!actorAdmission()||!io_->admitted()){refuse("startup-admission-or-cap");return;}
        const QPointer<Controller> weak(this);const auto io=io_;
        const auto allowed=[weak,io]{return weak&&weak->app_&&!QCoreApplication::closingDown()&&actorAdmission()&&io->admitted();};
        if(!ready_){
            if(!io->readyForRequest()){queue();return;}
            auto *window=qobject_cast<QQuickWindow *>(QGuiApplication::focusWindow());
            const auto composition=reader_selected::inspectLegacyFixture(window,
                [](QQuickItem *,QQmlContext *,const std::function<bool()> &guard){return guard&&guard();},allowed,1000);
            if(!weak)return;
            if(!composition.compositionObserved||!composition.refusal.isEmpty()){queue();return;}
            const auto early=io->request();
            if(!early.missing||!io->publishReady()){refuse("startup-ready-refused");return;}
            ready_=true;queue();return;
        }
        const auto request=io->request();
        if(request.missing){queue();return;}
        if(!request.observed||request.bytes!=config_.nonce+' '+config_.pid+' '+config_.start+'\n'||!io->consumeRequest()||!actorAdmission()){
            refuse("startup-request-refused");return;
        }
        done_=true;
        auto *sequence=new reader_write_reopen::Sequence(app_,config_,io,app_);
        if(!sequence->start()){reader_write_reopen::Result r;r.finished=true;r.refusal=QStringLiteral("startup-sequence-refused");r.finishedMs=io->nowMs();io->report(r);}
        // Native owners, controller/backend and sequence stay resident. No unload.
    }
    QPointer<QGuiApplication> app_;reader_write_reopen::Config config_;
    std::shared_ptr<reader_write_reopen::NativeFiles> io_;
    unsigned polls_=0;bool ready_=false,done_=false;
};
void startup(){
    if(scheduled.test_and_set(std::memory_order_acq_rel)||!fingerprinted.load(std::memory_order_acquire)||
       std::getenv("LD_PRELOAD")||std::getenv("LD_BIND_NOW")||!actorAdmission())stop();
    auto *app=QCoreApplication::instance();if(!app)stop();
    if(!QMetaObject::invokeMethod(app,[app]{
        try{
            auto *gui=qobject_cast<QGuiApplication *>(app);
            if(!gui||QThread::currentThread()!=gui->thread()||!actorAdmission())stop();
            auto *controller=new Controller(gui);if(!controller->start())stop();
        }catch(...){stop();}
    },Qt::QueuedConnection))stop();
}
}
Q_COREAPP_STARTUP_FUNCTION(startup)
