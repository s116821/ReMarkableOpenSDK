#pragma once
#include "read_only_backend.h"
#include "resident_mutation.h"
#include "resident_selection.h"
#ifdef RSDK_DEVELOPMENT_ORDINARY_CLOSE_V1
#include "reader_ordinary_close.h"
#include "reader_closed_view.h"
#endif

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
#ifdef RSDK_DEVELOPMENT_ORDINARY_CLOSE_V1
    bool closeConsumed_=false,closeDispatched_=false;
#ifdef RSDK_DEVELOPMENT_FIXTURE_REOPEN_V1
    bool closedObserved_=false,reopenConsumed_=false;
    resident::Facts closeFacts_;
#endif
    std::shared_ptr<reader_ordinary_close::Claim> closeClaim_=std::make_shared<reader_ordinary_close::Claim>();
#endif
#ifdef RSDK_DEVELOPMENT_EFFECT_PROBE_V1
    unsigned insertionRequests_=0;
    bool selectionConsumed_=false;
#ifdef RSDK_DEVELOPMENT_REPEAT_INSERT_V1
    static constexpr unsigned maxInsertionRequests=2;
#else
    static constexpr unsigned maxInsertionRequests=1;
#endif
#endif
#ifdef RESIDENT_MUTATION_BACKEND_TESTING
public:
    using TestStart=std::function<bool(QQuickWindow*,reader_insertion_entry::Config,QByteArray,reader_insertion_entry::Permit,reader_worker_native::ResidentMutation::Completion)>;
private:
    TestStart testStart_;
    std::function<bool()> testBusy_;
#endif
public:
    explicit MutationBackend(const reader_insertion_native_profile::Facilities *profile,QObject *parent=nullptr)
        :QObject(parent),readOnly_(profile,this),mutation_(profile?reader_worker_native::ResidentMutation::create(*profile):nullptr),selection_(
#ifdef RSDK_DEVELOPMENT_EFFECT_PROBE_V1
        ResidentSelection::createDevelopmentProbe(profile)
#else
        ResidentSelection::create(profile)
#endif
        ){}
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
#ifdef RSDK_DEVELOPMENT_ORDINARY_CLOSE_V1
#ifdef RSDK_DEVELOPMENT_FIXTURE_REOPEN_V1
        if(lease->request().kind==9){submitFixtureReopen(std::move(lease));return;}
#endif
        if(lease->request().kind==7||lease->request().kind==8){submitOrdinaryClose(std::move(lease));return;}
        if(closeConsumed_&&(lease->request().kind==1||lease->request().kind==3)) {resident::Frame r;r.kind=3;r.rejection=4;endpoint->finish(lease,r);return;}
#endif
        if(lease->request().kind==3){submitSelection(std::move(lease));return;}
        if(lease->request().kind!=1){readOnly_.submit(std::move(lease));return;}
        const auto reject=[&](quint8 reason){resident::Frame r;r.kind=3;r.rejection=reason;if(endpoint)endpoint->finish(lease,r);};
#ifdef RSDK_DEVELOPMENT_EFFECT_PROBE_V1
        if(insertionRequests_>=maxInsertionRequests){reject(4);return;}
        ++insertionRequests_;
#endif
        bool supported=mutation_!=nullptr,busy=mutation_&&mutation_->busy();
#ifdef RESIDENT_MUTATION_BACKEND_TESTING
        if(testStart_){supported=true;busy=testBusy_&&testBusy_();}
#endif
        if(!supported){reject(0);return;}
        if(busy){
#ifdef RSDK_DEVELOPMENT_REPEAT_INSERT_V1
            qInfo("resident repeat-insert request %u: executor busy",insertionRequests_);
#endif
            reject(4);return;
        }
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
#ifdef RSDK_DEVELOPMENT_ORDINARY_CLOSE_V1
    void submitOrdinaryClose(resident::Endpoint::Lease lease) {
        const QPointer<MutationBackend> self(this);
        const QPointer<resident::Endpoint> endpoint(endpoint_);
        const QPointer<QGuiApplication> app(qobject_cast<QGuiApplication*>(QCoreApplication::instance()));
        const auto reject=[&](quint8 reason){resident::Frame r;r.kind=3;r.rejection=reason;if(endpoint)endpoint->finish(lease,r);};
        if(!self||!app||!endpoint||!lease||!lease->active())return;
        if(!mutation_||!selection_){reject(0);return;}
        if(mutation_->busy()||selection_->busy()){reject(4);return;} // no debt override
        const QPointer<QQuickWindow> window(qobject_cast<QQuickWindow*>(QGuiApplication::focusWindow()));
        const QPointer<QQmlEngine> engine(window?qmlEngine(window):nullptr);
        const auto permit=[self,app,endpoint,window,engine,lease]{return self&&app&&endpoint&&window&&engine&&lease->active()&&!QCoreApplication::closingDown()&&QThread::currentThread()==app->thread()&&QGuiApplication::focusWindow()==window&&qmlEngine(window)==engine;};
        if(!permit()){reject(3);return;}
        const auto left=[&](){return std::chrono::duration_cast<std::chrono::milliseconds>(lease->deadline()-std::chrono::steady_clock::now()).count();};
        if(left()<1)return;
        if(lease->request().kind==8) {
            if(!closeConsumed_||!closeDispatched_){reject(3);return;}
            const auto closed=reader_closed_view::observe(window,permit,int(std::min(qint64(20000),qint64(left()))));
            if(!permit())return;
            if(!closed.observed||!closed.compositionObserved||!closed.loaderReady||!closed.documentNull||!closed.refusal.isEmpty()){reject(3);return;}
#ifdef RSDK_DEVELOPMENT_FIXTURE_REOPEN_V1
            closedObserved_=true;
#endif
            resident::Frame r;r.kind=10;endpoint->finish(lease,r);return;
        }
        if(closeConsumed_){reject(4);return;}
        closeConsumed_=true; // terminal intent; no repeat even on later refusal
        const auto expected=lease->request().facts;
#ifdef RSDK_DEVELOPMENT_FIXTURE_REOPEN_V1
        closeFacts_=expected;
#endif
        if(!resident::validFacts(expected)||expected.order.size()>16){reject(0);return;}
        OrderObservation order;
        const auto selected=reader_selected::observeCreationJoin(window,permit,int(std::min(qint64(20000),qint64(left()))),{},
            [&](QObject *,QObject *document,QObject *,const QVariant &actualId,const reader_selected::Observation &joined,const std::function<bool()> &guard){order=observeOrder(engine,document,actualId,joined,guard);});
        if(!permit())return;
        if(!selected.observed||!selected.creationJoinObserved||!order.observed||!sameFacts(order.facts,expected)){reject(3);return;}
        reader_ordinary_close::Expected config;
        config.documentId=QUuid::fromRfc4122(expected.document).toString(QUuid::WithoutBraces);
        config.pageId=QUuid::fromRfc4122(expected.page).toString(QUuid::WithoutBraces);
        config.pageCount=int(expected.order.size());config.pageIndex=int(std::find(expected.order.begin(),expected.order.end(),expected.page)-expected.order.begin());
        if(left()<1)return;
        const auto result=reader_ordinary_close::run(window,config,closeClaim_,permit,int(std::min(qint64(20000),qint64(left()))));
        closeDispatched_=result.dispatched;
        if(!permit())return;
        resident::Frame response;response.kind=(result.claimed&&result.matched&&result.attempted&&result.dispatched&&result.refusal.isEmpty())?9:4;
        endpoint->finish(lease,response); // dispatch only; never a save receipt
    }
#endif
#ifdef RSDK_DEVELOPMENT_FIXTURE_REOPEN_V1
    void submitFixtureReopen(resident::Endpoint::Lease lease) {
        const QPointer<MutationBackend> self(this);const QPointer<resident::Endpoint> endpoint(endpoint_);
        const QPointer<QGuiApplication> app(qobject_cast<QGuiApplication*>(QCoreApplication::instance()));
        const auto reject=[&](quint8 reason){resident::Frame r;r.kind=3;r.rejection=reason;if(endpoint)endpoint->finish(lease,r);};
        if(!self||!app||!endpoint||!lease||!lease->active())return;
        if(reopenConsumed_){reject(4);return;}
        reopenConsumed_=true; // one separately journaled intent, never retry
        if(!closeConsumed_||!closeDispatched_||!closedObserved_||!sameFacts(lease->request().facts,closeFacts_)){reject(3);return;}
        if(!mutation_||!selection_||mutation_->busy()||selection_->busy()){reject(4);return;}
        const QPointer<QQuickWindow> window(qobject_cast<QQuickWindow*>(QGuiApplication::focusWindow()));
        const QPointer<QQmlEngine> engine(window?qmlEngine(window):nullptr);
        const auto permit=[self,app,endpoint,window,engine,lease]{return self&&app&&endpoint&&window&&engine&&lease->active()&&!QCoreApplication::closingDown()&&QThread::currentThread()==app->thread()&&QGuiApplication::focusWindow()==window&&qmlEngine(window)==engine;};
        if(!permit()){reject(3);return;}
        const auto left=std::chrono::duration_cast<std::chrono::milliseconds>(lease->deadline()-std::chrono::steady_clock::now()).count();
        if(left<1)return;
        const auto &facts=lease->request().facts;
        reader_selected::FixtureNavigation config{QUuid::fromRfc4122(facts.document).toString(QUuid::WithoutBraces),int(std::find(facts.order.begin(),facts.order.end(),facts.page)-facts.order.begin()),true};
        const auto result=reader_selected::openFixture(window,config,permit,int(std::min(qint64(20000),qint64(left))));
        if(!permit())return;
        resident::Frame response;response.kind=(result.attempted&&result.dispatched&&result.refusal.isEmpty())?11:4;
        endpoint->finish(lease,response); // not readiness, selected-page, or save proof
    }
#endif
    void submitSelection(resident::Endpoint::Lease lease){
        const QPointer<MutationBackend> self(this);const QPointer<resident::Endpoint> endpoint(endpoint_);
        const auto reject=[&](quint8 reason){resident::Frame r;r.kind=3;r.rejection=reason;if(endpoint)endpoint->finish(lease,r);};
#ifdef RSDK_DEVELOPMENT_EFFECT_PROBE_V1
        if(selectionConsumed_){reject(4);return;}
        selectionConsumed_=true;
#endif
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
