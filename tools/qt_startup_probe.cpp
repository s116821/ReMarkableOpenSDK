/* One development-only queued Qt receipt. No interposition or native page API. */
#include <QCoreApplication>
#include <QMetaObject>
#include <QThread>
#include <atomic>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef QT_PROBE_NONCE
#error Define QT_PROBE_NONCE as the frozen 32-character lowercase hex string.
#endif

namespace {
constexpr char nonce[] = QT_PROBE_NONCE;
constexpr bool validNonce() {
    if (sizeof nonce != 33) return false;
    for (unsigned i = 0; i < 32; ++i)
        if (!((nonce[i] >= '0' && nonce[i] <= '9') ||
              (nonce[i] >= 'a' && nonce[i] <= 'f'))) return false;
    return true;
}
static_assert(validNonce(), "Invalid frozen nonce");
constexpr char directory[] = "/run/rmb-qt-probe-" QT_PROBE_NONCE;
constexpr char yes[] = "{\"nonce\":\"" QT_PROBE_NONCE "\",\"application_thread\":true}\n";
constexpr char no[] = "{\"nonce\":\"" QT_PROBE_NONCE "\",\"application_thread\":false}\n";
std::atomic_flag scheduled = ATOMIC_FLAG_INIT;

void record(bool sameThread) {
    // The operator precreates this unique private tmpfs directory. Never create
    // parents, follow a final symlink, overwrite a receipt or retry an I/O error.
    const int dir = open(directory, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (dir < 0) return;
    struct stat st{};
    if (fstat(dir, &st) != 0 || st.st_uid != geteuid() || (st.st_mode & 0777) != 0700) {
        close(dir);
        return;
    }
    const int fd = openat(dir, "callback.json", O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
    close(dir);
    if (fd < 0) return;
    const char *data = sameThread ? yes : no;
    const size_t size = sameThread ? sizeof yes - 1 : sizeof no - 1;
    // A partial/error write leaves an invalid receipt, never fabricated success.
    const ssize_t written = write(fd, data, size);
    close(fd);
    if (written != static_cast<ssize_t>(size)) return;
}

void startup() {
    if (scheduled.test_and_set(std::memory_order_relaxed)) return;
    QCoreApplication *app = QCoreApplication::instance();
    if (!app) return;
    (void)QMetaObject::invokeMethod(app, [app] {
        record(QCoreApplication::instance() == app && QThread::currentThread() == app->thread());
    }, Qt::QueuedConnection);
}
}

Q_COREAPP_STARTUP_FUNCTION(startup)
