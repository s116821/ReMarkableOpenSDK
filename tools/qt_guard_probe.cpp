/* Separate development payload. The earlier nonce/thread probe is unchanged. */
#include "qt_guard_probe_core.h"
#include <QCoreApplication>
#include <QThread>
#include <atomic>
#include <cstdio>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#if !defined(QT_PROBE_NONCE) || !defined(QT_GUARD_MODULE) || !defined(QT_GUARD_NAME) || !defined(QT_GUARD_META_OBJECT)
#error Provide a reviewed frozen nonce and exact registration configuration.
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
std::atomic_flag scheduled = ATOMIC_FLAG_INIT;
const char *boolean(bool value) { return value ? "true" : "false"; }

void record(bool applicationThread, const guard_probe::Candidate &candidate) {
    char output[256];
    const int length = std::snprintf(output, sizeof output,
        "{\"nonce\":\"" QT_PROBE_NONCE "\",\"application_thread\":%s,"
        "\"registration_match\":%s,\"typed_target_match\":%s,"
        "\"guard_not_cleared\":%s,\"uniqueness\":\"unproven\"}\n",
        boolean(applicationThread), boolean(candidate.registrationMatch),
        boolean(candidate.typedTargetMatch), boolean(!candidate.guard.isNull()));
    if (length <= 0 || static_cast<size_t>(length) >= sizeof output) return;
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
    const ssize_t written = write(fd, output, static_cast<size_t>(length));
    close(fd);
    if (written != length) return; // Partial/error output is not a valid receipt.
}

void startup() {
    if (scheduled.test_and_set(std::memory_order_relaxed)) return;
    QCoreApplication *app = QCoreApplication::instance();
    if (!app) return;
    (void)QMetaObject::invokeMethod(app, [app] {
        const bool sameThread = QCoreApplication::instance() == app &&
                                QThread::currentThread() == app->thread();
        if (!sameThread) { record(false, {}); return; }
        const auto candidate = guard_probe::lookup(QStringLiteral(QT_GUARD_MODULE),
            QStringLiteral(QT_GUARD_NAME), QTypeRevision::fromVersion(1, 0), QT_GUARD_META_OBJECT);
        record(true, candidate);
    }, Qt::QueuedConnection);
}
}
Q_COREAPP_STARTUP_FUNCTION(startup)
