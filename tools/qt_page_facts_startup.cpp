// Native compilation requires a separately reviewed, private frozen config.
// This file is not included in the owned fixture executable or an SO build here.
#include "qt_page_facts_entry.h"
#include <atomic>
#ifndef QT_FACTS_ENTRY_CONFIG
#error A separately reviewed frozen native facts config is required.
#endif
#include QT_FACTS_ENTRY_CONFIG
namespace {
std::atomic_flag factsScheduled=ATOMIC_FLAG_INIT;
void startFactsEntry() {
    if (factsScheduled.test_and_set(std::memory_order_relaxed)) return;
    auto *app=QCoreApplication::instance();
    if (!app) return;
    QMetaObject::invokeMethod(app,[app]{
        auto *gui=qobject_cast<QGuiApplication *>(app);
        if (!gui || QThread::currentThread()!=gui->thread() || ::geteuid()!=0) return;
        auto config=pageFactsEntryConfig();
        if (config.directory!=QStringLiteral("/run/rmb-qt-probe-")+config.nonce ||
            !config.developmentSetup120 || !config.setupValid() || config.facts.budgetMs!=5000 ||
            config.facts.pageCap!=6 || config.facts.expectedOrder.size()!=6) return;
        // No parent/lifetime signal deletes the entry during a getter. Only its
        // deferred completion schedules teardown; the extension remains loaded.
        auto holder=std::make_shared<QPointer<qml_access::FactsEntry>>();
        *holder=new qml_access::FactsEntry(gui,std::move(config),[holder](qml_access::FactsEntryResult){
            if (*holder) (*holder)->deleteLater();
        });
        (*holder)->start();
    },Qt::QueuedConnection);
}
}
Q_COREAPP_STARTUP_FUNCTION(startFactsEntry)
