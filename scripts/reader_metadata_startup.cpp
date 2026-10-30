// Independent metadata-only entry; no legacy executor/configuration includes.
#ifdef READER_METADATA_TESTING
#error Fixture-only clock/publication hooks must not enter the startup artifact.
#endif
#include "reader_metadata_entry.h"
#include <atomic>
#ifndef READER_CENSUS_NONCE
#error A separately frozen READER_CENSUS_NONCE is required.
#endif
namespace {
constexpr char nonce[] = READER_CENSUS_NONCE;
constexpr bool validNonce() {
    if (sizeof nonce != 33) return false;
    for (unsigned i = 0; i < 32; ++i)
        if (!((nonce[i] >= '0' && nonce[i] <= '9') || (nonce[i] >= 'a' && nonce[i] <= 'f'))) return false;
    return true;
}
static_assert(validNonce(), "Frozen nonce must be 32 lowercase hexadecimal characters");
std::atomic_flag scheduled = ATOMIC_FLAG_INIT;
void startup() {
    if (scheduled.test_and_set(std::memory_order_relaxed)) return;
    auto *app = QCoreApplication::instance();
    if (!app) return;
    QMetaObject::invokeMethod(app, [app] {
        auto *gui = qobject_cast<QGuiApplication *>(app);
        if (!gui || QThread::currentThread() != gui->thread() || ::geteuid() != 0) return;
        auto *entry = new reader_metadata::Entry(gui,
            QStringLiteral("/run/rmb-reader-census-") + QString::fromLatin1(nonce), QByteArray(nonce));
        entry->start();
    }, Qt::QueuedConnection);
}
}
Q_COREAPP_STARTUP_FUNCTION(startup)
