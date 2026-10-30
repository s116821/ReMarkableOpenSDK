#pragma once
#include "reader_adjacent_selection.h"
#include "resident_effect_gate.h"
#include "reader_insertion_guard_native_profile.h"
#include <QTimer>
#include <chrono>

namespace remarkable::rm2_3_28_0_172 {
#ifdef RESIDENT_SELECTION_TESTING
namespace testing {struct SelectionAccess;}
#endif
// Process-resident request executor, independent of the endpoint facade.
// Its production factory remains closed pending native transition settlement
// qualification. Owned joins exercise this state machine without native access.
class ResidentSelection final:private QObject {
    using Clock=std::chrono::steady_clock;
    using Permit=std::function<bool()>;
    using Join=std::function<reader_selected::Observation(QQuickWindow*,const Permit&,int,const reader_selected::ValidatedSourceAction&)>;
    enum class Phase {Before,After,Finishing};
    struct State {
        resident::Frame request,result;
        Clock::time_point deadline;
        Permit permit;
        std::function<void(const resident::Frame&)> completion;
        QPointer<QQuickWindow> window;
        QPointer<QQmlEngine> engine;
        QPointer<QObject> reader,document;
        ResidentEffectGate::Token effect;
        std::shared_ptr<reader_insertion_target_acquisition::Claim> claim=std::make_shared<reader_insertion_target_acquisition::Claim>();
        Phase phase=Phase::Before;
        unsigned observations=0;
        bool attempted=false,observed=false,failed=false;
    };
    ResidentEffectGate &effects_;
    const Join join_;
    std::shared_ptr<State> active_;
    static constexpr bool NativeSelectionQualified=false;
public:
    using Completion=std::function<void(const resident::Frame&)>;
    static ResidentSelection *create(const reader_insertion_native_profile::Facilities *profile) {
        if(!gui()||!NativeSelectionQualified||!profile||!profile->conditionQualified||!profile->applicationAbsent||
           !profile->core||!profile->baseAdmission.profileAndProviders)return nullptr;
        return new ResidentSelection(ResidentEffectGate::process(),
            [](QQuickWindow *window,const Permit &permit,int budget,const reader_selected::ValidatedSourceAction &action){
                return reader_selected::observeCreationJoin(window,permit,budget,{},action);
            });
    }
    bool busy()const{return !gui()||bool(active_)||effects_.busy();}
    bool start(QQuickWindow *window,resident::Frame request,Clock::time_point deadline,Permit permit,Completion completion) {
        if(busy()||!window||window->thread()!=QCoreApplication::instance()->thread()||QGuiApplication::focusWindow()!=window||
           !permit||!completion||request.message!=2||request.kind!=3||
           !resident::validId(request.operation)||!resident::validSelection(request.facts,request.target)||
           request.facts.order.size()>17||deadline<=Clock::now())return false;
        auto state=std::make_shared<State>();state->request=std::move(request);state->deadline=deadline;
        state->window=window;state->engine=qmlEngine(window);state->permit=std::move(permit);state->completion=std::move(completion);
        if(!state->engine)return false;
        state->effect=effects_.acquire();if(!state->effect)return false;
        active_=state;schedule(state,0);return true;
    }
private:
    explicit ResidentSelection(ResidentEffectGate &effects,Join join):effects_(effects),join_(std::move(join)){}
    ~ResidentSelection()override=default;
#ifdef RESIDENT_SELECTION_TESTING
    friend struct testing::SelectionAccess;
    static ResidentEffectGate *ownedGate(){return new ResidentEffectGate;}
    static void destroyOwnedGate(ResidentEffectGate *gate){delete gate;}
#endif
    static bool gui(){
        auto *app=qobject_cast<QGuiApplication*>(QCoreApplication::instance());
        return app&&!QCoreApplication::closingDown()&&QThread::currentThread()==app->thread();
    }
    bool baseAllowed(const std::shared_ptr<State> &state)const {
        auto *app=QCoreApplication::instance();
        return gui()&&!state->failed&&state==active_&&state->window&&state->engine&&Clock::now()<state->deadline&&
            state->window->thread()==app->thread()&&state->engine->thread()==app->thread()&&
            QGuiApplication::focusWindow()==state->window&&qmlEngine(state->window)==state->engine;
    }
    bool allowed(const std::shared_ptr<State> &state)const {
        if(!baseAllowed(state))return false;
        try {const auto permission=state->permit;return permission&&permission()&&baseAllowed(state);}catch(...){return false;}
    }
    void schedule(const std::shared_ptr<State> &state,int ms){
        const std::weak_ptr<State> weak=state;
        QTimer::singleShot(ms,this,[this,weak]{if(auto original=weak.lock();original&&original==active_)step(original);});
    }
    void fail(const std::shared_ptr<State> &state){
        state->failed=true;
        state->observed=false;state->result={};state->result.kind=state->attempted?4:3;state->result.rejection=3;
        state->phase=Phase::Finishing;
    }
    void success(const std::shared_ptr<State> &state,resident::Facts facts){
        if(state->failed){fail(state);return;}
        state->observed=true;state->result={};state->result.kind=5;state->result.facts=std::move(facts);
        state->phase=Phase::Finishing;
    }
    void finish(const std::shared_ptr<State> &state){
        // This callback is queued only after the synchronous Sample and its
        // borrowed guard have completely unwound. Completion captures retire
        // while the shared effect reservation is still held.
        if(!allowed(state)){state->observed=false;state->result={};state->result.kind=state->attempted?4:3;state->result.rejection=3;}
        const bool release=!state->attempted||state->observed;
        auto completion=std::move(state->completion);state->permit={};
        try {if(completion)completion(state->result);}catch(...){}
        completion={};active_.reset();
        if(release)effects_.release(state->effect);
        // Otherwise the process gate retains uncertainty even after State dies.
    }
    void step(const std::shared_ptr<State> &state){
        if(!gui()||state!=active_)return;
        if(state->phase==Phase::Finishing){finish(state);return;}
        if(!allowed(state)||++state->observations>128){fail(state);schedule(state,0);return;}
        const auto remaining=std::chrono::duration_cast<std::chrono::milliseconds>(state->deadline-Clock::now()).count();
        if(remaining<=0){fail(state);schedule(state,0);return;}
        bool called=false,waiting=false;
        const bool after=state->phase==Phase::After;
        try {
            const auto observed=join_(state->window,[this,state]{return allowed(state);},
                int(std::min<decltype(remaining)>(remaining,reader_selected::MaxObservationMs)),
                [this,state,after,&called,&waiting](QObject *reader,QObject *document,QObject *,const QVariant &id,
                        const reader_selected::Observation &joined,const Permit &guard){
                    if(called){fail(state);return;}called=true;
                    if(!reader||!document||(after&&(state->reader!=reader||state->document!=document))){fail(state);return;}
                    const QPointer<QObject> heldReader(reader),heldDocument(document);
                    if((!joined.ready||joined.loading||joined.working||joined.awaitingTiles||joined.pendingTiles)&&
                       !joined.pageError&&!joined.documentError){waiting=true;return;}
                    const auto order=observeOrder(state->engine,document,id,joined,guard);
                    const auto &expected=state->request.facts;
                    if(state->failed||!heldReader||!heldDocument||!order.observed||order.facts.document!=expected.document||order.facts.order!=expected.order){fail(state);return;}
                    if(after){
                        if(order.facts.page!=state->request.target){fail(state);return;}
                        success(state,order.facts);return;
                    }
                    if(order.facts.page!=expected.page){fail(state);return;}
                    state->reader=heldReader;state->document=heldDocument;
                    reader_insertion_target_acquisition::MethodInfo method;
                    if(state->request.target!=expected.page)method=reader_insertion_target_acquisition::inspect(state->engine,heldReader,guard);
                    if(state->failed||!heldReader||!heldDocument){fail(state);return;}
                    const auto navigation=dispatchAdjacent(state->engine,heldReader,heldDocument,expected,state->request.target,
                        order,method,state->claim,guard);
                    // Only owned results are inspected after the old epoch ends.
                    state->attempted=state->attempted||navigation.attempted;
                    if(state->failed||!navigation.admitted||navigation.unknown||!navigation.refusal.isEmpty()||
                       (!navigation.alreadySelected&&!navigation.dispatched)){fail(state);return;}
                    if(navigation.alreadySelected)success(state,order.facts);
                    else state->phase=Phase::After;
                });
            // The join may revoke its callback after Sample/helper unwind.
            // Inspect only its owned result; never revisit the old epoch guard.
            if(called&&(!observed.observed||!observed.refusal.isEmpty()))fail(state);
            if(state->phase==Phase::Finishing){schedule(state,0);return;}
            if(!after&&state->phase==Phase::After){schedule(state,0);return;}
            const bool readiness=(!called&&observed.refusal==QStringLiteral("worker-source-unready"))||
                (called&&waiting&&observed.observed);
            if(!readiness||!allowed(state)){fail(state);schedule(state,0);return;}
            schedule(state,int(std::min<decltype(remaining)>(remaining,100)));
        }catch(...){fail(state);schedule(state,0);}
    }
};
}
