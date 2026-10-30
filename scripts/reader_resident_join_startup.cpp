// Fixed resident read-only pair, separately built. No fixture hooks in payload.
#if defined(READER_SELECTED_TESTING) || defined(READER_WORKER_NATIVE_TESTING) || defined(READER_RESIDENT_TESTING)
#error Fixture hooks must not enter this startup artifact.
#endif
#include "reader_resident_join_pair.h"
#include <atomic>
#if !defined(READER_RESIDENT_ACTIVATION_NONCE) || !defined(READER_RESIDENT_FIRST_NONCE) || !defined(READER_RESIDENT_SECOND_NONCE) || !defined(READER_RESIDENT_DOCUMENT_ID) || !defined(READER_RESIDENT_PAGE_ID) || !defined(READER_RESIDENT_PAGE_INDEX) || !defined(READER_RESIDENT_PAGE_COUNT)
#error A separately frozen fixed pair configuration is required.
#endif
namespace {
std::atomic_flag scheduled=ATOMIC_FLAG_INIT;
void startup() {
    if(scheduled.test_and_set(std::memory_order_relaxed))return;
    auto *app=QCoreApplication::instance();if(!app)return;
    QMetaObject::invokeMethod(app,[app]{
        auto *gui=qobject_cast<QGuiApplication*>(app);
        if(!gui||QThread::currentThread()!=gui->thread()||::geteuid()!=0)return;
        reader_resident::Config config;
        config.activation=QByteArray(READER_RESIDENT_ACTIVATION_NONCE);
        config.first=QByteArray(READER_RESIDENT_FIRST_NONCE);config.second=QByteArray(READER_RESIDENT_SECOND_NONCE);
        config.navigation={QStringLiteral(READER_RESIDENT_DOCUMENT_ID),READER_RESIDENT_PAGE_INDEX,true};
        config.pageId=QStringLiteral(READER_RESIDENT_PAGE_ID);config.pageCount=READER_RESIDENT_PAGE_COUNT;
        auto *pair=new reader_resident::Pair(gui,QStringLiteral("/run/rmb-qt-probe-")+QString::fromLatin1(config.activation),config);
        pair->start();
    },Qt::QueuedConnection);
}
}
Q_COREAPP_STARTUP_FUNCTION(startup)
