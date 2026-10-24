#pragma once
#include "order_observation.h"
#include "reader_insertion_guard_native_profile.h"
#include "../resident/endpoint.h"
#include <algorithm>

namespace remarkable::rm2_3_28_0_172 {
// Constructed only by the explicit native bootstrap after installBeforeMain.
// Facilities has a private constructor/destructor and remains process-resident.
// This facade retains no DocumentWorker owners and advertises no capability.
class ReadOnlyBackend final:public QObject {
    const reader_insertion_native_profile::Facilities *profile_;
    QPointer<resident::Endpoint> endpoint_;
public:
    explicit ReadOnlyBackend(const reader_insertion_native_profile::Facilities *profile,QObject *parent=nullptr)
        :QObject(parent),profile_(profile) {}
    bool attach(resident::Endpoint *endpoint) {
        auto *app=QCoreApplication::instance();
        if(!app||endpoint_||!endpoint||QThread::currentThread()!=app->thread()||thread()!=app->thread()||endpoint->thread()!=app->thread())return false;
        endpoint_=endpoint;return true;
    }
    void submit(resident::Endpoint::Lease lease) {
        const QPointer<ReadOnlyBackend> self(this);
        const QPointer<resident::Endpoint> endpoint(endpoint_);
        const QPointer<QGuiApplication> app(qobject_cast<QGuiApplication*>(QCoreApplication::instance()));
        if(!app||!endpoint||!lease||!lease->active()||QThread::currentThread()!=app->thread()||thread()!=app->thread())return;
        const auto reject=[&](quint8 reason){resident::Frame response;response.kind=3;response.rejection=reason;if(endpoint)endpoint->finish(lease,response);};
        if(!profile_||!profile_->core||!profile_->applicationAbsent||!profile_->baseAdmission.profileAndProviders){reject(0);return;}
        const auto command=lease->request().kind;
        // Mutation is handled by the separately gated resident dispatcher.
        // This observation facade has no diagnostic one-shot fallback.
        if(command!=0&&command!=2){reject(0);return;}
        const QPointer<QQuickWindow> window(qobject_cast<QQuickWindow*>(QGuiApplication::focusWindow()));
        const QPointer<QQmlEngine> engine(window?qmlEngine(window):nullptr);
        const auto permit=[&]{
            return self&&app&&endpoint&&lease->active()&&window&&engine&&!QCoreApplication::closingDown()&&
                QThread::currentThread()==app->thread()&&window->thread()==app->thread()&&engine->thread()==app->thread()&&
                QGuiApplication::focusWindow()==window&&qmlEngine(window)==engine;
        };
        if(!permit()){reject(3);return;}
        OrderObservation order;
        const auto selected=reader_selected::observeCreationJoin(window,permit,reader_selected::MaxObservationMs,{},
            [&](QObject *,QObject *document,QObject *,const QVariant &actualId,
                const reader_selected::Observation &joined,const std::function<bool()> &guard){
                order=observeOrder(engine,document,actualId,joined,guard);
            });
        if(!permit())return; // Canceled/deadline requests cannot publish.
        if(!selected.observed||!selected.creationJoinObserved||!order.observed){reject(3);return;}
        resident::Frame response;response.facts=std::move(order.facts);
        if(command==0)response.kind=0;
        else {
            const auto &original=lease->request();
            if(response.facts.document!=original.facts.document){reject(3);return;}
            response.kind=2;
            response.targetPresent=std::find(response.facts.order.begin(),response.facts.order.end(),original.target)!=response.facts.order.end();
        }
        endpoint->finish(lease,std::move(response));
    }
};
}
