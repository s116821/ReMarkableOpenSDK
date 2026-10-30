#pragma once
#include "capture_gate.h"
#include "visual_capture_core.h"
#include "../resident/wire.h"
#include <chrono>

namespace remarkable::rm2_3_28_0_172 {
// No Reader lookup or native action. Partial bytes and borrowed callbacks stay
// within this synchronous call. Returned pixels carry no page/write authority.
inline resident::Frame captureVisual(QQuickWindow *inputWindow,
        const std::function<bool()> &inputPermit, int budgetMs
#ifdef VISUAL_CAPTURE_TESTING
        , const std::function<QImage(QQuickWindow *)> &grab = {}
#endif
) {
    resident::Frame refusal;refusal.kind=3;refusal.rejection=3;
    capture_detail::Gate gate;
    if(!gate.entered)return refusal;
    try {
        using Clock=std::chrono::steady_clock;
        if(budgetMs<1||budgetMs>20000)return refusal;
        const auto deadline=Clock::now()+std::chrono::milliseconds(budgetMs);
        const auto permit=inputPermit;
        const QPointer<QGuiApplication> app(qobject_cast<QGuiApplication*>(QCoreApplication::instance()));
        if(!app||QCoreApplication::closingDown()||QThread::currentThread()!=app->thread())return refusal;
        const QPointer<QQuickWindow> window(inputWindow);
        if(!window||window->thread()!=app->thread())return refusal;
        const QPointer<QQmlEngine> engine(qmlEngine(window));
        const QPointer<QQuickItem> root(window->contentItem());
        bool lost=false;
        const auto context=[&] {
            return !capture_detail::reentered&&app&&window&&engine&&root&&!QCoreApplication::closingDown()&&
                QThread::currentThread()==app->thread()&&window->thread()==app->thread()&&
                root->thread()==app->thread()&&engine->thread()==app->thread()&&window->contentItem()==root&&
                qmlEngine(window)==engine&&QGuiApplication::focusWindow()==window&&window->isVisible()&&window->isActive();
        };
        const std::function<bool()> current=[&] {
            if(lost)return false;
            if(Clock::now()>=deadline||!context()||!permit||!permit()||!context()||Clock::now()>=deadline)lost=true;
            return !lost;
        };
        if(!current())return refusal;
        visual_capture::Sink sink;
        const auto observed=visual_capture::capture(current,sink
#ifdef VISUAL_CAPTURE_TESTING
            ,grab
#endif
        );
        if(QByteArray(observed.stage)!="visual-image-observed"||!current())return refusal;
        auto bytes=sink.takeOwned();
        if(bytes.isEmpty()||bytes.size()!=observed.encoded||!current())return refusal;
        resident::Frame response;response.kind=8;
        auto &image=response.capture;
        image.width=quint32(observed.width);image.height=quint32(observed.height);
        std::memcpy(&image.dprBits,&observed.dpr,sizeof image.dprBits);
        image.procedure=2;image.sha256=QByteArray::fromHex(observed.sha);image.png=std::move(bytes);
        if(!resident::validImage(image,2)||!current())return refusal;
        return response;
    } catch (...) {return refusal;}
}
}
