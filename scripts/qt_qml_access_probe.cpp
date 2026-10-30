#include "qt_qml_access_probe_core.h"
#ifdef QT_PROBE_CREATION_CONFIG
#include QT_PROBE_CREATION_CONFIG
#endif
#ifdef QT_PROBE_PAGE_OPEN_CONFIG
#include QT_PROBE_PAGE_OPEN_CONFIG
#endif
#include <atomic>
#include <cstdio>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#ifndef QT_PROBE_NONCE
#error Define a fresh frozen QT_PROBE_NONCE.
#endif
namespace {
constexpr char nonce[] = QT_PROBE_NONCE;
constexpr bool validNonce() {
    if (sizeof nonce != 33) return false;
    for (unsigned i = 0; i < 32; ++i)
        if (!((nonce[i] >= '0' && nonce[i] <= '9') || (nonce[i] >= 'a' && nonce[i] <= 'f'))) return false;
    return true;
}
static_assert(validNonce(), "Invalid nonce");
std::atomic_flag scheduled = ATOMIC_FLAG_INIT;
void record(const char *stage, bool app, bool engine, bool helper, bool controller, const QByteArray &diagnostic = {}) {
    char buffer[256];
    const int length = snprintf(buffer, sizeof buffer,
        "{\"nonce\":\"%s\",\"stage\":\"%s\",\"application_thread\":%s,\"engine_thread\":%s,\"helper_available\":%s,\"controller_available\":%s}\n",
        nonce, stage, app ? "true" : "false", engine ? "true" : "false",
        helper ? "true" : "false", controller ? "true" : "false");
    if (length <= 0 || length >= static_cast<int>(sizeof buffer)) return;
    const int directory = open("/run/rmb-qt-probe-" QT_PROBE_NONCE, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (directory < 0) return;
    struct stat st{};
    if (fstat(directory, &st) != 0 || st.st_uid != geteuid() || (st.st_mode & 0777) != 0700) { close(directory); return; }
    if (!diagnostic.isEmpty() && diagnostic.size() <= 8192) {
        const int diagnosticFd = openat(directory, "diagnostics.json", O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
        if (diagnosticFd < 0) { close(directory); return; }
        if (diagnosticFd >= 0) {
            const ssize_t diagnosticWritten = write(diagnosticFd, diagnostic.constData(), static_cast<size_t>(diagnostic.size()));
            close(diagnosticFd);
            // A partial/error diagnostic remains invalid and is never retried.
            if (diagnosticWritten != diagnostic.size()) { close(directory); return; }
        }
    }
    const int fd = openat(directory, "callback.json", O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
    close(directory);
    if (fd < 0) return;
    const ssize_t written = write(fd, buffer, static_cast<size_t>(length));
    close(fd);
    if (written != length) return; // Partial write is invalid, never retried.
}
void startup() {
    if (scheduled.test_and_set(std::memory_order_relaxed)) return;
    auto *app = QCoreApplication::instance();
    if (!app) return;
    QMetaObject::invokeMethod(app, [app] {
        if (QThread::currentThread() != app->thread()) { record("application-thread", false, false, false, false); return; }
        auto *gui = qobject_cast<QGuiApplication *>(app);
        if (!gui) { record("no-gui", true, false, false, false); return; }
#ifdef QT_PROBE_PAGE_OPEN_CONFIG
#ifdef QT_PROBE_CREATION_CONFIG
#error Page open and creation configurations are mutually exclusive.
#endif
        auto openConfig = pageOpenConfig();
        if (openConfig.developmentSetupGate) {
            openConfig.setupNonce = QString::fromLatin1(nonce);
            openConfig.setupDirectory = QStringLiteral("/run/rmb-qt-probe-") + openConfig.setupNonce;
        }
        new qml_access::Probe(gui, record, 20000, 5000, {}, openConfig);
#elif defined(QT_PROBE_CREATION_CONFIG)
        auto config = creationConfig();
        if (config.developmentAfterOpen) {
            config.setupNonce = QString::fromLatin1(nonce);
            config.setupDirectory = QStringLiteral("/run/rmb-qt-probe-") + config.setupNonce;
        }
        new qml_access::Probe(gui, record, 20000, 5000, config);
#else
        new qml_access::Probe(gui, record);
#endif
    }, Qt::QueuedConnection);
}
}
Q_COREAPP_STARTUP_FUNCTION(startup)
