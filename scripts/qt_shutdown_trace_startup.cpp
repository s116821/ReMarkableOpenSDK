#include "qt_shutdown_trace.h"
#include <QGuiApplication>
#include <QQuickWindow>
#include <QThread>
#ifdef QT_FACTS_PRETOKEN_DIAGNOSTIC
#include "qt_pretoken_shutdown_entry.h"
#endif
#ifndef QT_SHUTDOWN_NONCE
#error Supply a separately selected private frozen QT_SHUTDOWN_NONCE.
#endif
namespace {
constexpr char nonce[]=QT_SHUTDOWN_NONCE;
constexpr bool validNonce(){
    if(sizeof nonce!=33)return false;
    for(unsigned i=0;i<32;++i)if(!((nonce[i]>='0'&&nonce[i]<='9')||(nonce[i]>='a'&&nonce[i]<='f')))return false;
    return true;
}
static_assert(validNonce());
std::atomic_flag scheduled=ATOMIC_FLAG_INIT;
void startup(){
    if(scheduled.test_and_set())return;
    auto *app=QCoreApplication::instance();if(!app)return;
    QMetaObject::invokeMethod(app,[app]{
        auto *gui=qobject_cast<QGuiApplication *>(app);
        if(!gui || QThread::currentThread()!=gui->thread())return;
        auto *r=shutdown_trace::Recorder::open("/run/rmb-qt-shutdown-" QT_SHUTDOWN_NONCE,nonce);
        if(!r)return;
        r->record(shutdown_trace::Event::Startup);
        QObject::connect(gui,&QCoreApplication::aboutToQuit,gui,[r]{r->aboutToQuit();},Qt::DirectConnection);
        QObject::connect(gui,&QObject::destroyed,[r]{r->record(shutdown_trace::Event::ApplicationDestroyed);});
        // Only one naturally focused Quick window. No event filter, polling,
        // focus change, show/hide, engine lookup, getter or forced render.
        auto *bound=new bool(false); // process-resident, used only on GUI thread
        auto bind=[r,bound,gui](QWindow *window){
            if(*bound || !window)return;
            auto *quick=qobject_cast<QQuickWindow *>(window);if(!quick)return;
            *bound=true;r->record(shutdown_trace::Event::Window);
            QObject::connect(quick,&QQuickWindow::beforeRendering,gui,[r]{r->beforeRender();},Qt::DirectConnection);
            QObject::connect(quick,&QQuickWindow::afterRendering,gui,[r]{r->afterRender();},Qt::DirectConnection);
            QObject::connect(quick,&QObject::destroyed,[r]{r->record(shutdown_trace::Event::WindowDestroyed);});
        };
        QObject::connect(gui,&QGuiApplication::focusWindowChanged,gui,bind);
        bind(QGuiApplication::focusWindow());
#ifdef QT_FACTS_PRETOKEN_DIAGNOSTIC
        // Preserve ordinary signal/binding order; do not reset frame counters.
        (void)pretoken_shutdown::start(gui,pretoken_shutdown::config(nonce),r);
#endif
    },Qt::QueuedConnection);
}
}
Q_COREAPP_STARTUP_FUNCTION(startup)
