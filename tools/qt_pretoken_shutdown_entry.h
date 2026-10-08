#pragma once
#if !defined(QT_FACTS_PRETOKEN_DIAGNOSTIC) || QT_FACTS_PRETOKEN_DIAGNOSTIC != 1
#error This helper requires the separately selected pre-token diagnostic build.
#endif
#include "qt_page_facts_entry.h"
#include "qt_shutdown_trace.h"

namespace pretoken_shutdown {
inline qml_access::FactsEntryConfig config(const char *nonce) {
    qml_access::FactsEntryConfig c;
    c.nonce=QString::fromLatin1(nonce);
    c.directory=QStringLiteral("/run/rmb-qt-shutdown-")+c.nonce;
    // Synthetic identifiers satisfy config validation only. No native identity read.
    c.facts.documentId=QStringLiteral("00000000-0000-4000-8000-000000000001");
    c.facts.pageCap=6;c.facts.budgetMs=5000;
    for(int i=2;i<=7;++i)c.facts.expectedOrder.append(
        QStringLiteral("00000000-0000-4000-8000-%1").arg(i,12,16,QLatin1Char('0')));
    c.setupBudgetMs=120000;c.developmentSetup120=true;
    c.setupSelection=QStringLiteral("main-dev-facts-120s");
    c.developmentCaptureObservation=true;
    c.developmentReceiverSubtreeCapture=true;
    c.developmentReceiverSubtreeItemCap=4096;c.developmentReceiverSubtreeDepthCap=32;
    c.developmentReceiverSubtreeCaptureAllowUnfocusedArea=true;
    c.developmentReceiverSubtreeCaptureDetailedIdentityDiagnostics=true;
    c.developmentReceiverSubtreeCaptureDocumentIdTypeDiagnostics=true;
    c.developmentReceiverSubtreeCaptureEntryIdConversion=true;
    c.developmentReceiverSourceFacts=true;
    return c;
}
inline QPointer<qml_access::FactsEntry> start(QGuiApplication *gui,
        qml_access::FactsEntryConfig c,shutdown_trace::Recorder *recorder) {
    if(!gui || QThread::currentThread()!=gui->thread() || ::geteuid()!=0 || !recorder ||
       c.directory!=QStringLiteral("/run/rmb-qt-shutdown-")+c.nonce ||
       !c.developmentSetup120 || !c.setupValid() || c.facts.budgetMs!=5000 ||
       c.facts.pageCap!=6 || c.facts.expectedOrder.size()!=6)return {};
    // Same parentless lifetime and queued completion -> deleteLater as original
    // qt_page_facts_startup.cpp. No manual cancel, deletion or application quit.
    auto holder=std::make_shared<QPointer<qml_access::FactsEntry>>();
    *holder=new qml_access::FactsEntry(gui,std::move(c),[holder](qml_access::FactsEntryResult){
        if(*holder)(*holder)->deleteLater();
    });
    (*holder)->start();
    if(*holder && (*holder)->pretokenInstalled())
        recorder->record(shutdown_trace::Event::EntryInstalled);
    return *holder;
}
}
