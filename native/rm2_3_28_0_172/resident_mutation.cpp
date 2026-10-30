#include "resident_mutation.h"

namespace reader_worker_native {
ResidentMutation *ResidentMutation::create(const reader_insertion_native_profile::Facilities &profile) {
#ifdef RESIDENT_MUTATION_TESTING
    Q_UNUSED(profile);return nullptr; // Owned fixture cannot install a native executor.
#else
    if(!detail::gui()||!profile.core||!profile.applicationAbsent||!profile.conditionQualified||
       !profile.baseAdmission.profileAndProviders||!profile.baseConditionAdmission.profileAndProviders)return nullptr;
    auto loaded=loadPinned([]{return detail::gui();});
    if(!loaded.bridge)return nullptr;
    auto *owners=ResidentSlot::create(*loaded.bridge,profile);
    if(!owners)return nullptr;
    reader_insertion_entry::Runtime runtime;
    runtime.core=profile.core;runtime.owners=owners;runtime.calls=profile.passiveCalls;
    // Coordinator discharges the remaining callsite, request, worker, owner and
    // original-final-lock requirements before it can register a conditional Task.
    runtime.admission={true,true,true,true,true,true};
    runtime.conditionAdmission={true,true,true,true,true,true};
    runtime.outerReturnPC=profile.outerReturnPC;runtime.innerReturnPC=profile.innerReturnPC;
    runtime.makeAttribution=[owners](QObject *worker,const ResidentSlot::Token &token) {
        if(owners->worker(token)!=worker)return reader_insertion_entry::Attribution{};
        const auto parts=owners->attribution(token,true);
        return reader_insertion_entry::Attribution{parts.map,parts.context,parts.read64,parts.postRejectProbe,parts.conditionCalls};
    };
    return new ResidentMutation(std::move(runtime),remarkable::rm2_3_28_0_172::ResidentEffectGate::process());
#endif
}

ResidentMutation::Token ResidentMutation::start(QQuickWindow *window,reader_insertion_entry::Config config,QByteArray nonce,
        reader_insertion_entry::Permit permit,Completion completion) {
    if(busy()||!owners_||owners_->occupied()||!config.valid()||!config.wantConditionalForward||!permit||!completion)return {};
    auto state=std::make_shared<Active>();
    state->fence=std::make_unique<reader_retirement::Fence>(static_cast<QObject*>(this));
    state->fenceToken=state->fence->begin(nonce,reader_retirement::Kind::Mutation);
    if(!state->fenceToken)return {};
    state->binding=reader_retirement::binding(state->fence.get(),state->fenceToken);
    state->effect=effects_.acquire();
    if(!state->effect)return {};
    active_=state;
    const reader_request_scope::Scope frame(state->binding); // Covers construction and every copied capture.
    state->completion=std::move(completion);completion={};
    try {
        auto *entry=new Entry(runtime_,nullptr,state->binding);
        state->entry=entry;state->record=entry->resident_;
#ifdef RESIDENT_MUTATION_TESTING
        if(testEntry_)testEntry_(*entry);
#endif
        const std::weak_ptr<Active> weak=state;
        connect(entry,&Entry::finished,this,[this,weak]{if(auto value=weak.lock();value&&value==active_)terminal(value);});
        connect(entry,&QObject::destroyed,this,[this,weak]{if(auto value=weak.lock();value&&value==active_)terminal(value,true);});
        connect(state->fence.get(),&reader_retirement::Fence::retirementFailed,this,[this,weak]{
            if(auto value=weak.lock();value&&value==active_){value->failed=true;maintenance_.start(10);}
        });
        entry->start(window,std::move(config),std::move(permit));
        permit={};
    }catch(...) {
        permit={};completion={};terminal(state,true);
    }
    return Token(state); // Consumed request even when its Coordinator refused.
}

void ResidentMutation::terminal(const std::shared_ptr<Active> &state,bool exception) {
    const reader_request_scope::Scope frame(state->binding);
    if(!frame.active()||state!=active_||state->terminal)return;
    state->terminal=true;
    if(state->entry)state->result=state->entry->snapshot();
    if(exception){state->result.finished=true;state->result.unknown=true;state->result.refusal=QStringLiteral("executor-exception");}
    classify(state,exception);
    // Before acquisition there can be no submitted SDK Task or native owner.
    // A known preflight refusal must not consume process capacity permanently.
    const auto &r=state->result;
    state->noEffect=!exception&&!state->failed&&!r.observed&&!r.unknown&&!r.reconciliationRequired&&
        !r.ownersRetained&&!r.registrationAccepted&&!r.probe.attempted&&!r.creation.attempted&&!owners_->occupied();
    if(state->record) {
        state->coreToken=state->record->registration;state->ownerToken=state->record->owners;state->worker=state->record->worker;
    }
    const std::weak_ptr<Active> weak=state;
    if(!state->fence->requestRetire(state->fenceToken,[this,weak]{
        if(auto value=weak.lock();value&&value==active_)cleanup(value);
    },state->noEffect?reader_retirement::Effect::NoMutationAttempted:reader_retirement::Effect::ReconciliationRequired))state->failed=true;
    maintenance_.start(10);
}

void ResidentMutation::cleanup(const std::shared_ptr<Active> &state) {
    const reader_request_scope::Scope frame(state->binding);
    if(!frame.entered()||frame.active()||state!=active_||state->cleaned){state->failed=true;return;}
    if(state->normal&&(!state->entry||!state->worker||owners_->worker(state->ownerToken)!=state->worker||
       !state->entry->disarmForRetirement()))quarantine(state,"retirement-disarm-refused");
    delete state->entry.data();state->entry=nullptr;
    // Native callbacks retain only independent plain shared state. Core's marker
    // is the owner slot; its attribution context is retained by that slot too.
    delete state->record;state->record=nullptr;
    state->cleaned=true;
}

void ResidentMutation::maintain() {
    if(!gui()||dispatching_||!active_)return;
    const auto state=active_;
    if(!state->terminal)return;
    if(state->failed) {
        if(state->fence->snapshot(state->fenceToken).depth){maintenance_.start(10);return;}
        if(state->result.refusal.isEmpty())quarantine(state,"retirement-cleanup-refused");
        else {state->result.observed=false;state->result.unknown=true;state->result.reconciliationRequired=true;}
        publish(state);return; // Single bounded quarantine, never admit another mutation.
    }
    if(!state->cleaned||!state->fence->handoffReady(state->fenceToken)){maintenance_.start(10);return;}
    if(!state->normal){
        const bool reusable=state->noEffect&&!owners_->occupied();
        publish(state);if(reusable){active_.reset();effects_.release(state->effect);}return;
    }
    bool coreRetired=state->coreRetired;
    if(!coreRetired) {
#ifdef RESIDENT_MUTATION_TESTING
        const auto current=testCoreSnapshot_?testCoreSnapshot_():core_->snapshot(state->coreToken);
#else
        const auto current=core_->snapshot(state->coreToken);
#endif
        if(!Entry::forwardGood(current)||current.jobId!=state->result.job){
            state->result.core=current;quarantine(state,"retirement-core-changed");publish(state);return;
        }
#ifdef RESIDENT_MUTATION_TESTING
        coreRetired=testCoreRetire_?testCoreRetire_():core_->retirePhysically(state->coreToken);
#else
        coreRetired=core_->retirePhysically(state->coreToken);
#endif
        state->coreRetired=coreRetired;
    }
    bool reclaimed=false;
    if(coreRetired) {
        const auto result=owners_->retirePhysically(state->ownerToken);
        if(!result.matched||result.unknown){quarantine(state,"retirement-owner-refused");publish(state);return;}
        reclaimed=result.reclaimed;
    }
    if(reclaimed) {
        publish(state);active_.reset();effects_.release(state->effect);return;
    }
    if(++state->retirementAttempts<8){maintenance_.start(100);return;}
    publish(state); // Capacity remains occupied; no ordinary-release fallback.
}
}
