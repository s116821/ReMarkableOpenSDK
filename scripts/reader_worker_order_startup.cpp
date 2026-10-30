// Separately frozen one-request worker tuple artifact; no fixture hooks.
#if defined(READER_SELECTED_TESTING) || defined(READER_WORKER_NATIVE_TESTING) || defined(READER_WORKER_ORDER_TESTING) || defined(READER_WORKER_ORDER_ENTRY_TESTING)
#error Fixture hooks must not enter this startup artifact.
#endif
#include "reader_worker_order_entry.h"
#include <atomic>
#if !defined(READER_WORKER_ORDER_NONCE) || !defined(READER_WORKER_ORDER_DOCUMENT_ID) || !defined(READER_WORKER_ORDER_SOURCE_INDEX) || !defined(READER_WORKER_ORDER_PAGE_COUNT) || !defined(READER_WORKER_ORDER_LEFT_ID) || !defined(READER_WORKER_ORDER_SOURCE_ID) || !defined(READER_WORKER_ORDER_RIGHT_ID)
#error A separately frozen fixed worker tuple configuration is required.
#endif
#if defined(READER_WORKER_ORDER_CANCEL_RUNNING) && READER_WORKER_ORDER_CANCEL_RUNNING != 1
#error Only the fixed already-running cancellation scenario is accepted.
#endif
namespace {
std::atomic_flag scheduled=ATOMIC_FLAG_INIT;
void startup() {
    if(scheduled.test_and_set(std::memory_order_relaxed))return;
    auto *app=QCoreApplication::instance();if(!app)return;
    QMetaObject::invokeMethod(app,[app]{
        auto *gui=qobject_cast<QGuiApplication*>(app);
        if(!gui||QThread::currentThread()!=gui->thread()||::geteuid()!=0)return;
        reader_worker_order_entry::Config config;
        config.nonce=QByteArray(READER_WORKER_ORDER_NONCE);
        config.navigation={QStringLiteral(READER_WORKER_ORDER_DOCUMENT_ID),READER_WORKER_ORDER_SOURCE_INDEX,true};
        config.pageCount=READER_WORKER_ORDER_PAGE_COUNT;
        config.pageIds={{QStringLiteral(READER_WORKER_ORDER_LEFT_ID),QStringLiteral(READER_WORKER_ORDER_SOURCE_ID),QStringLiteral(READER_WORKER_ORDER_RIGHT_ID)}};
#ifdef READER_WORKER_ORDER_CANCEL_RUNNING
        config.scenario=reader_worker_order_entry::Scenario::CancelRunning;
#endif
        auto *entry=new reader_worker_order_entry::Entry(gui,QStringLiteral("/run/rmb-qt-probe-")+QString::fromLatin1(config.nonce),config);
        entry->start();
    },Qt::QueuedConnection);
}
}
Q_COREAPP_STARTUP_FUNCTION(startup)
