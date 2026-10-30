#pragma once
#include "read_only_backend.h"
#include "resident_mutation.h"
#include "resident_selection.h"

namespace remarkable::rm2_3_28_0_172 {
// Wire identities are historical assertions. Coordinator repeats fresh source
// joins and the native conditional predicate; this conversion grants no lease.
inline std::optional<reader_insertion_entry::Config> mutationConfig(const resident::Frame &request,
        std::chrono::steady_clock::time_point deadline) {
    if(request.message!=2||request.kind!=1||!resident::validId(request.operation)||
       !resident::validFacts(request.facts)||!resident::validId(request.target)||request.facts.order.size()>16)return {};
    reader_insertion_entry::Config config;config.wantConditionalForward=true;config.absoluteDeadline=deadline;
    const auto text=[](const QByteArray&id){return QUuid::fromRfc4122(id).toString(QUuid::WithoutBraces);};
    auto &r=config.request;r.documentId=text(request.facts.document);r.sourcePageId=text(request.facts.page);r.targetPageId=text(request.target);
    for(const auto &id:request.facts.order)r.expectedPages.append(text(id));
    r.sourceIndex=r.expectedPages.indexOf(r.sourcePageId);
    return config.valid()?std::optional<reader_insertion_entry::Config>(std::move(config)):std::nullopt;
}
inline resident::Frame mutationReply(const resident::Frame &request,const reader_insertion_entry::Result &result) {
    resident::Frame reply;reply.kind=4; // Ambiguous accepted work is never relabeled a safe retry.
    const auto config=mutationConfig(request,{});if(!config||!result.finished)return reply;
    auto expected=config->request.expectedPages;expected.insert(config->request.sourceIndex+1,config->request.targetPageId);
    if(result.observed&&!result.canceled&&!result.unknown&&!result.reconciliationRequired&&result.refusal.isEmpty()&&
       result.postOrder.observed&&result.postOrder.ids==expected) {
        reply.kind=1;reply.target=request.target;reply.facts.document=request.facts.document;reply.facts.page=request.target;
        for(const auto &id:result.postOrder.ids)reply.facts.order.push_back(QUuid(id).toRfc4122());
    }else if(!result.observed&&!result.unknown&&!result.reconciliationRequired&&!result.creation.attempted&&!result.core.forwardCommitted) {
        reply.kind=3;reply.rejection=2; // Executor proved refusal before mutation attempt.
    }
    return reply;
}

// Source-integrated dispatcher, still subject to explicit profile activation.
// The resident executor owns mutation lifetime independently of this facade.
class MutationBackend final:public QObject {
    ReadOnlyBackend readOnly_;
    reader_worker_native::ResidentMutation *mutation_=nullptr;
    ResidentSelection *selection_=nullptr;
    QPointer<resident::Endpoint> endpoint_;
#ifdef RESIDENT_MUTATION_BACKEND_TESTING
public:
    using TestStart=std::function<bool(QQuickWindow*,reader_insertion_entry::Config,QByteArray,reader_insertion_entry::Permit,reader_worker_native::ResidentMutation::Completion)>;
private:
    TestStart testStart_;
    std::function<bool()> testBusy_;
#endif
public:
    explicit MutationBackend(const reader_insertion_native_profile::Facilities *profile,QObject *parent=nullptr)
        :QObject(parent),readOnly_(profile,this),mutation_(profile?reader_worker_native::ResidentMutation::create(*profile):nullptr),selection_(ResidentSelection::create(profile)){}
#ifdef RESIDENT_MUTATION_BACKEND_TESTING
    MutationBackend(TestStart start,std::function<bool()> busy,QObject *parent=nullptr)
        :QObject(parent),readOnly_(nullptr,this),testStart_(std::move(start)),testBusy_(std::move(busy)){}
#endif
    bool attach(resident::Endpoint *endpoint) {
        if(endpoint_||!readOnly_.attach(endpoint))return false;
        endpoint_=endpoint;return true;
    }
    void submit(resident::Endpoint::Lease lease) {
        const QPointer<MutationBackend> self(this);
        const QPointer<resident::Endpoint> endpoint(endpoint_);
        const QPointer<QGuiApplication> app(qobject_cast<QGuiApplication*>(QCoreApplication::instance()));
        if(!app||!endpoint||!lease||!lease->active()||QThread::currentThread()!=app->thread()||thread()!=app->thread())return;
        if(lease->request().kind==3){submitSelection(std::move(lease));return;}
        if(lease->request().kind!=1){readOnly_.submit(std::move(lease));return;}
        const auto reject=[&](quint8 reason){resident::Frame r;r.kind=3;r.rejection=reason;if(endpoint)endpoint->finish(lease,r);};
        bool supported=mutation_!=nullptr,busy=mutation_&&mutation_->busy();
#ifdef RESIDENT_MUTATION_BACKEND_TESTING
        if(testStart_){supported=true;busy=testBusy_&&testBusy_();}
#endif
        if(!supported){reject(0);return;}
        if(busy){reject(4);return;}
        const auto config=mutationConfig(lease->request(),lease->deadline());
        if(!config){reject(0);return;}
        const QPointer<QQuickWindow> window(qobject_cast<QQuickWindow*>(QGuiApplication::focusWindow()));
        const QPointer<QQmlEngine> engine(window?qmlEngine(window):nullptr);
        const auto permit=[self,endpoint,app,window,engine,lease]{
            return self&&endpoint&&app&&window&&engine&&lease->active()&&!QCoreApplication::closingDown()&&
                QThread::currentThread()==app->thread()&&window->thread()==app->thread()&&engine->thread()==app->thread()&&
                QGuiApplication::focusWindow()==window&&qmlEngine(window)==engine;
        };
        if(!permit()){reject(3);return;}
        const auto completion=[self,endpoint,lease,request=lease->request()](const reader_insertion_entry::Result &result){
            if(self&&endpoint&&lease->active())endpoint->finish(lease,mutationReply(request,result));
        };
        bool consumed=false;
#ifdef RESIDENT_MUTATION_BACKEND_TESTING
        if(testStart_)consumed=testStart_(window,*config,lease->request().operation.toHex(),permit,completion);
        else
#endif
            consumed=bool(mutation_->start(window,*config,lease->request().operation.toHex(),permit,completion));
        if(!consumed&&self&&lease->active())reject(1);
    }
private:
    void submitSelection(resident::Endpoint::Lease lease){
        const QPointer<MutationBackend> self(this);const QPointer<resident::Endpoint> endpoint(endpoint_);
        const auto reject=[&](quint8 reason){resident::Frame r;r.kind=3;r.rejection=reason;if(endpoint)endpoint->finish(lease,r);};
        if(!selection_){reject(0);return;} // Native qualification is explicitly disabled.
        if(selection_->busy()){reject(4);return;}
        const QPointer<QQuickWindow> window(qobject_cast<QQuickWindow*>(QGuiApplication::focusWindow()));
        const QPointer<QQmlEngine> engine(window?qmlEngine(window):nullptr);
        const auto permit=[self,endpoint,window,engine,lease]{
            return self&&endpoint&&window&&engine&&lease->active()&&!QCoreApplication::closingDown()&&
                QGuiApplication::focusWindow()==window&&qmlEngine(window)==engine;
        };
        const auto completion=[self,endpoint,lease](const resident::Frame &reply){if(self&&endpoint&&lease->active())endpoint->finish(lease,reply);};
        if(!selection_->start(window,lease->request(),lease->deadline(),permit,completion)&&self&&lease->active())reject(1);
    }
};
}
