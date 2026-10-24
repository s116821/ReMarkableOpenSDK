// Separate startup artifact; terminal entries remain resident until app exit.
#if defined(READER_SELECTED_TESTING) || defined(READER_WORKER_NATIVE_TESTING)
#error Fixture hooks must not enter the startup artifact.
#endif
#include "reader_selected_view_entry.h"
#include <atomic>
#ifndef READER_SELECTED_NONCE
#error A separately frozen READER_SELECTED_NONCE is required.
#endif
#if defined(READER_SELECTED_OPEN_FIXTURE_JOIN) && defined(READER_SELECTED_CREATION_JOIN)
#error Select exactly one diagnostic mode.
#endif
#if defined(READER_SELECTED_NAVIGATION_OWNERSHIP) && (defined(READER_SELECTED_OPEN_FIXTURE_JOIN) || defined(READER_SELECTED_CREATION_JOIN))
#error Select exactly one diagnostic mode.
#endif
#ifdef READER_SELECTED_OPEN_FIXTURE_JOIN
#if !defined(READER_SELECTED_DOCUMENT_ID) || !defined(READER_SELECTED_PAGE_INDEX)
#error Navigation requires separately frozen document UUID and zero-based page index.
#endif
#endif
#if defined(READER_SELECTED_LEGACY_COMPOSITION) && !defined(READER_SELECTED_OPEN_FIXTURE_JOIN)
#error Legacy composition selection requires the fixed-fixture navigation mode.
#endif
#if defined(READER_SELECTED_WORKER_CALLBACK) && (!defined(READER_SELECTED_OPEN_FIXTURE_JOIN) || !defined(READER_SELECTED_LEGACY_COMPOSITION))
#error Worker callback witness requires the fixed-fixture legacy navigation mode.
#endif
#if defined(READER_SELECTED_JOINED_CREATION) && (!defined(READER_SELECTED_OPEN_FIXTURE_JOIN) || !defined(READER_SELECTED_LEGACY_COMPOSITION) || !defined(READER_SELECTED_TARGET_PAGE_ID) || !defined(READER_SELECTED_EXPECTED_PAGES))
#error Joined creation requires fixed legacy navigation and a fresh target UUID.
#endif
#if defined(READER_SELECTED_JOINED_CREATION) && defined(READER_SELECTED_WORKER_CALLBACK)
#error Select exactly one effect diagnostic.
#endif
namespace {
#ifdef READER_SELECTED_JOINED_CREATION
reader_joined_creation::Request creationRequest() {
    reader_joined_creation::Request request;
    request.documentId = QStringLiteral(READER_SELECTED_DOCUMENT_ID);
    request.sourceIndex = READER_SELECTED_PAGE_INDEX;
    request.targetPageId = QStringLiteral(READER_SELECTED_TARGET_PAGE_ID);
    request.expectedPages = QStringLiteral(READER_SELECTED_EXPECTED_PAGES).split(QLatin1Char(','), Qt::KeepEmptyParts);
    request.sourcePageId = request.expectedPages.value(request.sourceIndex);
    return request;
}
#endif
constexpr char nonce[] = READER_SELECTED_NONCE;
constexpr bool validNonce() {
    if (sizeof nonce != 33) return false;
    for (unsigned i = 0; i < 32; ++i)
        if (!((nonce[i] >= '0' && nonce[i] <= '9') || (nonce[i] >= 'a' && nonce[i] <= 'f'))) return false;
    return true;
}
static_assert(validNonce(), "Frozen nonce must be exactly 32 lowercase hexadecimal characters");
std::atomic_flag scheduled = ATOMIC_FLAG_INIT;
void startup() {
    if (scheduled.test_and_set(std::memory_order_relaxed)) return;
    QCoreApplication *app = QCoreApplication::instance();
    if (!app) return;
    QMetaObject::invokeMethod(app, [app] {
        auto *gui = qobject_cast<QGuiApplication *>(app);
        if (!gui || QThread::currentThread() != gui->thread() || ::geteuid() != 0) return;
        auto *entry = new reader_selected::Entry(gui,
            QStringLiteral("/run/rmb-qt-probe-") + QString::fromLatin1(nonce), QByteArray(nonce), nullptr,
#ifdef READER_SELECTED_NAVIGATION_OWNERSHIP
            reader_selected::Entry::Diagnostic::NavigationOwnership
#elif defined(READER_SELECTED_OPEN_FIXTURE_JOIN)
#ifdef READER_SELECTED_JOINED_CREATION
            reader_selected::Entry::Diagnostic::NavigateJoinedCreation,
#elif defined(READER_SELECTED_WORKER_CALLBACK)
            reader_selected::Entry::Diagnostic::NavigateCreationWorkerCallback,
#else
            reader_selected::Entry::Diagnostic::NavigateCreationSourceJoin,
#endif
            {QStringLiteral(READER_SELECTED_DOCUMENT_ID), READER_SELECTED_PAGE_INDEX
#ifdef READER_SELECTED_LEGACY_COMPOSITION
                , true
#endif
            }
#ifdef READER_SELECTED_JOINED_CREATION
            , creationRequest()
#endif
#elif defined(READER_SELECTED_CREATION_JOIN)
            reader_selected::Entry::Diagnostic::CreationSourceJoin
#else
            reader_selected::Entry::Diagnostic::SelectedPage
#endif
        );
        entry->start();
    }, Qt::QueuedConnection);
}
}
Q_COREAPP_STARTUP_FUNCTION(startup)
