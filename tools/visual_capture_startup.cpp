// Independent metadata-only entry; no legacy executor/configuration includes.
#ifdef VISUAL_CAPTURE_TESTING
#error Fixture-only clock/publication hooks must not enter the startup artifact.
#endif
#include "visual_capture_entry.h"
#include <atomic>
#ifndef VISUAL_CAPTURE_NONCE
#error A separately frozen VISUAL_CAPTURE_NONCE is required.
#endif
namespace {
constexpr char nonce[] = VISUAL_CAPTURE_NONCE;
constexpr bool validNonce() {
    if (sizeof nonce != 33) return false;
    for (unsigned i = 0; i < 32; ++i)
        if (!((nonce[i] >= '0' && nonce[i] <= '9') || (nonce[i] >= 'a' && nonce[i] <= 'f'))) return false;
    return true;
}
static_assert(validNonce(), "Frozen nonce must be 32 lowercase hexadecimal characters");
#ifdef VISUAL_CAPTURE_PASSIVE_FRAME_READY
#if VISUAL_CAPTURE_PASSIVE_FRAME_READY != 1
#error Passive frame readiness must be explicitly enabled with value 1.
#endif
constexpr bool passiveFrame = true;
#else
constexpr bool passiveFrame = false;
#endif
#ifdef VISUAL_CAPTURE_BINDING_WITNESS
#if VISUAL_CAPTURE_BINDING_WITNESS != 1 || !defined(VISUAL_CAPTURE_PASSIVE_FRAME_READY) || VISUAL_CAPTURE_PASSIVE_FRAME_READY != 1
#error Binding witness requires explicit value 1 and passive frame readiness value 1.
#endif
constexpr bool bindingWitness = true;
#else
constexpr bool bindingWitness = false;
#endif
std::atomic_flag scheduled = ATOMIC_FLAG_INIT;
void startup() {
    if (scheduled.test_and_set(std::memory_order_relaxed)) return;
    auto *app = QCoreApplication::instance();
    if (!app) return;
    QMetaObject::invokeMethod(app, [app] {
        auto *gui = qobject_cast<QGuiApplication *>(app);
        if (!gui || QThread::currentThread() != gui->thread() || ::geteuid() != 0) return;
        auto *entry = new visual_capture::Entry(gui,
            QStringLiteral("/run/rmb-visual-capture-") + QString::fromLatin1(nonce), QByteArray(nonce), nullptr, passiveFrame, bindingWitness);
        entry->start();
    }, Qt::QueuedConnection);
}
}
Q_COREAPP_STARTUP_FUNCTION(startup)
