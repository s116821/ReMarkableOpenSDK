#pragma once
// Internal historical dispatch; only admitted facades may construct/submit it.
#include "../rm2_3_28_0_172/order_observation.h"
#include "../rm2_3_28_0_172/reader_source_capture.h"
#include "../rm2_3_28_0_172/window_visual_capture.h"
#include "endpoint.h"
#include <algorithm>
#include <functional>

namespace remarkable::rm2_3_28_0_172 { class ReadOnlyBackend; }
namespace remarkable::rm1_observation { class ReadOnlyBackend; }
namespace remarkable::resident {
// Historical facts only. In particular a presence result never clears an
// insertion/selection reservation or a resident effect quarantine.
inline resident::Frame observationReply(const resident::Frame &request,resident::Facts facts) {
    resident::Frame response;response.kind=3;response.rejection=3;
    if(request.message!=2||!resident::validFacts(facts))return response;
    if(request.kind==0){response.kind=0;response.facts=std::move(facts);return response;}
    if(request.kind!=2&&request.kind!=4){response.rejection=0;return response;}
    if(!resident::validFacts(request.facts)||!resident::validId(request.target)||facts.document!=request.facts.document)return response;
    if(request.kind==4&&!resident::validSelection(request.facts,request.target))return response;
    if(request.kind==2&&std::find(request.facts.order.begin(),request.facts.order.end(),request.target)!=request.facts.order.end())return response;
    response.kind=request.kind==2?2:6;response.facts=std::move(facts);
    response.targetPresent=std::find(response.facts.order.begin(),response.facts.order.end(),request.target)!=response.facts.order.end();
    return response;
}
class ReadOnlyDispatch final:public QObject {
    QPointer<resident::Endpoint> endpoint_;
    friend class remarkable::rm2_3_28_0_172::ReadOnlyBackend;
    friend class remarkable::rm1_observation::ReadOnlyBackend;
    explicit ReadOnlyDispatch(QObject *parent):QObject(parent) {}
    bool attach(resident::Endpoint *endpoint) {
        auto *app=QCoreApplication::instance();
        if(!app||endpoint_||!endpoint||QThread::currentThread()!=app->thread()||thread()!=app->thread()||endpoint->thread()!=app->thread())return false;
        endpoint_=endpoint;return true;
    }
    void submit(resident::Endpoint::Lease lease,const std::function<bool()> &admitted,
                const std::function<bool()> &keepAdmission={}) {
        const QPointer<ReadOnlyDispatch> self(this);
        const QPointer<resident::Endpoint> endpoint(endpoint_);
        const QPointer<QGuiApplication> app(qobject_cast<QGuiApplication*>(QCoreApplication::instance()));
        if(!app||!endpoint||!lease||!lease->active()||QThread::currentThread()!=app->thread()||thread()!=app->thread())return;
        const auto reject=[&](quint8 reason){resident::Frame response;response.kind=3;response.rejection=reason;if(endpoint)endpoint->finish(lease,response);};
        if(!admitted||!admitted()){reject(0);return;}
        const auto command=lease->request().kind;
        // Mutation is handled by the separately gated resident dispatcher.
        // This observation facade has no diagnostic one-shot fallback.
        if(command!=0&&command!=2&&command!=4&&command!=5&&command!=6){reject(0);return;}
        const QPointer<QQuickWindow> window(qobject_cast<QQuickWindow*>(QGuiApplication::focusWindow()));
        const QPointer<QQmlEngine> engine(window?qmlEngine(window):nullptr);
        const auto permit=[&]{
            return self&&app&&endpoint&&lease->active()&&window&&engine&&!QCoreApplication::closingDown()&&
                QThread::currentThread()==app->thread()&&window->thread()==app->thread()&&engine->thread()==app->thread()&&
                QGuiApplication::focusWindow()==window&&qmlEngine(window)==engine&&(!keepAdmission||keepAdmission());
        };
        if(!permit()){reject(3);return;}
        if(command==6) {
            const auto remaining=std::chrono::duration_cast<std::chrono::milliseconds>(lease->deadline()-std::chrono::steady_clock::now()).count();
            if(remaining<1)return;
            auto response=rm2_3_28_0_172::captureVisual(window,permit,int(std::min(qint64(20000),qint64(remaining))));
            if(permit())endpoint->finish(lease,std::move(response));
            return;
        }
        if(command==5) {
            const auto remaining=std::chrono::duration_cast<std::chrono::milliseconds>(lease->deadline()-std::chrono::steady_clock::now()).count();
            if(remaining<1)return;
            auto captured=rm2_3_28_0_172::captureSource(window,lease->request().facts,permit,int(std::min(qint64(20000),qint64(remaining))));
            if(!permit())return;
            if(!captured.captured()){reject(3);return;}
            auto response=rm2_3_28_0_172::captureReply(std::move(captured));
            // Read-only; no mutation/selection reservation is retired here.
            if(permit())endpoint->finish(lease,std::move(response));
            return;
        }
        rm2_3_28_0_172::OrderObservation order;
        const auto selected=reader_selected::observeCreationJoin(window,permit,reader_selected::MaxObservationMs,{},
            [&](QObject *,QObject *document,QObject *,const QVariant &actualId,
                const reader_selected::Observation &joined,const std::function<bool()> &guard){
                order=rm2_3_28_0_172::observeOrder(engine,document,actualId,joined,guard);
            });
        if(!permit())return; // Canceled/deadline requests cannot publish.
        if(!selected.observed||!selected.creationJoinObserved||!order.observed){reject(3);return;}
        endpoint->finish(lease,observationReply(lease->request(),std::move(order.facts)));
    }
};
}
