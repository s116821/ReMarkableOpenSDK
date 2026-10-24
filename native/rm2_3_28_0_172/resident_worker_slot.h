#pragma once
// Process-resident single acquisition slot. Physical retirement is deliberately
// private until the executor supplies its exact submitted-Task lifetime proof.
#include "reader_native_worker_bridge.h"
#include "reader_insertion_guard_native_profile.h"
#include <atomic>

namespace reader_worker_native {
class ResidentMutation;
#ifdef RESIDENT_WORKER_SLOT_TESTING
namespace testing { struct SlotAccess; }
#endif
class ResidentSlot final {
    using ReleaseResult=reader_insertion_native_profile::OwnerReleaseResult;
    using Kind=reader_insertion_native_profile::NativeOwnerKind;
    enum class Ownership { Empty, Acquiring, Owned, Released, Unknown };
    struct Owner {
        alignas(std::uintptr_t) std::array<std::uintptr_t,2> words{};
        Ownership ownership=Ownership::Empty;
        ReleaseResult last=ReleaseResult::RefusedIdentity;
        void begin() noexcept { ownership=Ownership::Acquiring; }
        void returned() noexcept {
            ownership=(!words[0]&&!words[1])?Ownership::Empty:
                words[0]&&words[1]?Ownership::Owned:Ownership::Unknown;
        }
        void uncertain() noexcept {ownership=Ownership::Unknown;}
        bool settled()const noexcept {return ownership==Ownership::Empty||ownership==Ownership::Released;}
        // No destructor invokes ordinary native release, even on partial failure.
    };
    struct State {
        Owner document,worker;
        QPointer<QObject> controller,publicWorker;
        bool ready=false,consumed=false,canceled=false,unknown=false,physicalRetired=false;
        bool preparing=true,submitting=false;
        bool live()const {
            return !canceled&&!physicalRetired&&controller&&publicWorker&&detail::gui()&&
                controller->thread()==QThread::currentThread()&&publicWorker->thread()==QThread::currentThread();
        }
    };
public:
    class Token final {
        std::weak_ptr<State> state_;
        explicit Token(const std::shared_ptr<State> &state):state_(state){}
        friend class ResidentSlot;
    public:
        Token()=default;
    };
    struct Preparation {Token token;bool ready=false,unknown=false;QString refusal;};
    struct Retirement {
        bool matched=false,retired=false,reclaimed=false,unknown=false;
        bool documentRetained=false,workerRetained=false;
        ReleaseResult document=ReleaseResult::RefusedIdentity,worker=ReleaseResult::RefusedIdentity;
    };
private:
    detail::Functions fn_;
    const reader_insertion_native_profile::Facilities *facilities_;
    std::shared_ptr<State> current_;
    static inline std::atomic<bool> created_{false};
    ResidentSlot(detail::Functions fn,const reader_insertion_native_profile::Facilities *facilities):fn_(fn),facilities_(facilities){}
    ~ResidentSlot()=default; // Only process-resident construction is public.
    friend class ResidentMutation;
#ifdef RESIDENT_WORKER_SLOT_TESTING
    friend struct testing::SlotAccess;
    ReleaseResult (*testRelease_)(Kind,std::uintptr_t,std::uintptr_t)=nullptr;
#endif
    std::shared_ptr<State> match(const Token &token)const {
        auto value=token.state_.lock();return value&&value==current_?value:nullptr;
    }
    void release(Owner &owner,Kind kind) noexcept {
        if(owner.ownership!=Ownership::Owned)return;
#ifdef RESIDENT_WORKER_SLOT_TESTING
        owner.last=testRelease_?testRelease_(kind,owner.words[0],owner.words[1]):ReleaseResult::RefusedIdentity;
#else
        owner.last=facilities_->tryRelinquishNonLast(kind,owner.words[0],owner.words[1]);
#endif
        if(owner.last==ReleaseResult::Released) {
            // No callout or native dereference between success and consuming our
            // local words. A competing owner may already have destroyed both.
            owner.words={};owner.ownership=Ownership::Released;
        }
    }
    // Caller must prove no future invocation for every Task using these owners,
    // no active native/adapter stacks, and a later non-reentrant GUI turn. This is
    // not made public by a boolean assertion or a forgeable raw-pointer token.
    Retirement retirePhysically(const Token &token) {
        Retirement result;
        if(!detail::gui()||detail::busy)return result;
        detail::Gate gate;auto state=match(token);
        if(!state||state->preparing||state->submitting)return result;
        result.matched=true;result.unknown=state->unknown;
        state->canceled=true;
        if(!state->unknown) {
            state->physicalRetired=true;result.retired=true;
            release(state->worker,Kind::Worker);
            release(state->document,Kind::Document);
        }
        result.worker=state->worker.last;result.document=state->document.last;
        result.workerRetained=!state->worker.settled();result.documentRetained=!state->document.settled();
        if(!state->unknown&&!result.workerRetained&&!result.documentRetained) {
            // Only adapter-owned fields remain. Stale weak tokens never bind to
            // a replacement allocation, even if its address is later reused.
            current_.reset();result.reclaimed=true;
        }
        return result;
    }
public:
    ResidentSlot(const ResidentSlot &)=delete;
    ResidentSlot &operator=(const ResidentSlot &)=delete;
    static ResidentSlot *create(Bridge &bridge,const reader_insertion_native_profile::Facilities &facilities) {
        if(!detail::gui()||detail::busy||bridge.preparedOnce)return nullptr;
        auto *slot=new ResidentSlot(bridge.fn,&facilities);
        bool expected=false;
        if(!created_.compare_exchange_strong(expected,true)){delete slot;return nullptr;}
        bridge.preparedOnce=true;return slot;
    }
    bool occupied()const {return !detail::gui()||bool(current_);}
    bool cancel(const Token &token) {
        if(!detail::gui())return false;
        auto state=match(token);if(!state)return false;
        state->canceled=true;return true;
    }
    QObject *worker(const Token &token)const {
        if(!detail::gui())return nullptr;
        auto state=match(token);
        return state&&state->ready&&state->live()?state->publicWorker.data():nullptr;
    }
    Preparation prepare(QObject *controller,const QVariant &actualId,QObject *expectedWorker,const Permit &permit) {
        Preparation result;detail::Gate gate;
        if(!gate.entered||!detail::gui()){result.refusal=QStringLiteral("resident-reentered-or-thread");return result;}
        if(current_){result.refusal=QStringLiteral("resident-capacity");return result;}
        const auto type=actualId.metaType();
        if(type.iface()!=fn_.idInterface||type.sizeOf()!=12||type.alignOf()!=4||!actualId.constData()) {
            result.refusal=QStringLiteral("resident-input-contract");return result;
        }
        QVariant id=actualId;
        auto state=std::make_shared<State>();current_=state;result.token=Token(state);
        struct Finished {State &state;~Finished(){state.preparing=false;}} finished{*state};
        state->controller=controller;state->publicWorker=expectedWorker;
        if(!state->live()||!detail::ancestry(state->controller.data(),"xofm::libs::library::DocumentController")||
           !detail::ancestry(state->publicWorker.data(),"DocumentWorker",fn_.workerMeta)||!qobject_cast<QThread*>(state->publicWorker.data())||
           !detail::allowed(permit)||!state->live()) {
            result.refusal=QStringLiteral("resident-preparation-refused");return result;
        }
        state->document.begin();
        try {fn_.resolve(state->document.words.data(),state->controller.data(),id.constData());}
        catch(...) {state->document.uncertain();state->unknown=true;result.unknown=true;result.refusal=QStringLiteral("resident-resolver-unknown");return result;}
        state->document.returned();
        if(state->document.ownership==Ownership::Unknown)state->unknown=true;
        result.unknown=state->unknown;
        if(state->document.ownership!=Ownership::Owned||!detail::allowed(permit)||!state->live()) {
            result.refusal=QStringLiteral("resident-document-refused");return result;
        }
        state->worker.begin();
        try {fn_.acquire(state->worker.words.data(),reinterpret_cast<void*>(state->document.words[0]),1);}
        catch(...) {state->worker.uncertain();state->unknown=true;result.unknown=true;result.refusal=QStringLiteral("resident-acquire-unknown");return result;}
        state->worker.returned();
        if(state->worker.ownership==Ownership::Unknown)state->unknown=true;
        result.unknown=state->unknown;
        if(state->worker.ownership!=Ownership::Owned||state->worker.words[0]!=reinterpret_cast<std::uintptr_t>(state->publicWorker.data())||
           !detail::allowed(permit)||!state->live()) {
            result.refusal=QStringLiteral("resident-worker-refused");return result;
        }
        state->ready=true;result.ready=true;return result;
    }
    Submission submit(const Token &token,const std::function<void()> &callback,const Permit &permit) {
        Submission result;detail::Gate gate;
        if(!gate.entered||!detail::gui()){result.refusal=QStringLiteral("resident-reentered-or-thread");return result;}
        auto state=match(token);
        if(!state||!state->ready||state->consumed||!state->live()) {result.refusal=QStringLiteral("resident-submission-refused");return result;}
        state->consumed=true;state->submitting=true;
        struct Finished {State &state;~Finished(){state.submitting=false;}} finished{*state};
        std::function<void()> retained;
        try {retained=callback;}catch(...){result.refusal=QStringLiteral("resident-callback-copy-refused");return result;}
        if(!retained||!detail::allowed(permit)||!state->live()){result.refusal=QStringLiteral("resident-submission-refused");return result;}
        result.attempted=true;
        try {result.jobId=fn_.request(reinterpret_cast<void*>(state->worker.words[0]),retained);}
        catch(...) {state->unknown=true;result.unknown=true;result.refusal=QStringLiteral("resident-request-unknown");return result;}
        if(!result.jobId||state->canceled||detail::reentered) {
            state->unknown=true;result.unknown=true;result.refusal=QStringLiteral("resident-request-unknown");return result;
        }
        result.accepted=true;return result;
    }
};
}
