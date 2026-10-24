#pragma once
#include "reader_insertion_guard_entry.h"
#include "reader_retirement_scope.h"

namespace reader_worker_native {
#ifdef RESIDENT_MUTATION_TESTING
namespace testing {struct MutationAccess;}
#endif
// One process-resident executor. Normal completion may reclaim its single slot;
// uncertain work retains that slot, independently of read-only observations.
// Private QObject inheritance prevents external deletion/parenting/deleteLater.
class ResidentMutation final:private QObject {
    using Entry=reader_insertion_entry::Coordinator;
    using Result=reader_insertion_entry::Result;
    struct Active;
public:
    using Completion=std::function<void(const Result &)>;
    class Token final {
        std::weak_ptr<Active> state_;
        explicit Token(const std::shared_ptr<Active> &state):state_(state){}
        friend class ResidentMutation;
#ifdef RESIDENT_MUTATION_TESTING
        friend struct testing::MutationAccess;
#endif
    public:
        Token()=default;
        explicit operator bool()const{return !state_.expired();}
    };
    static ResidentMutation *create(const reader_insertion_native_profile::Facilities &);
    bool busy()const{return !gui()||dispatching_||bool(active_);}
    Token start(QQuickWindow *window,reader_insertion_entry::Config config,QByteArray nonce,
                reader_insertion_entry::Permit permit,Completion completion);
    bool cancel(const Token &token) {
        if(!gui())return false;
        const auto state=token.state_.lock();
        if(!state||state!=active_||state->terminal||!state->entry)return false;
        state->entry->cancel();return true;
    }
private:
    struct Active {
        std::unique_ptr<reader_retirement::Fence> fence;
        reader_retirement::Token fenceToken;
        reader_request_scope::Binding binding;
        QPointer<Entry> entry;
        Entry::Resident *record=nullptr;
        reader_insertion_hooks::Core::Token coreToken;
        ResidentSlot::Token ownerToken;
        QPointer<QObject> worker;
        Completion completion;
        Result result;
        bool terminal=false,normal=false,cleaned=false,failed=false,published=false,coreRetired=false;
        unsigned retirementAttempts=0;
    };
    const reader_insertion_entry::Runtime runtime_;
    ResidentSlot *const owners_;
    reader_insertion_hooks::Core *const core_;
    std::shared_ptr<Active> active_;
    QTimer maintenance_;
    bool dispatching_=false;
    explicit ResidentMutation(reader_insertion_entry::Runtime runtime)
        :runtime_(std::move(runtime)),owners_(runtime_.owners),core_(runtime_.core) {
        maintenance_.setSingleShot(true);
        connect(&maintenance_,&QTimer::timeout,this,[this]{maintain();});
    }
    ~ResidentMutation()override=default;
#ifdef RESIDENT_MUTATION_TESTING
    friend struct testing::MutationAccess;
    std::function<void(Entry &)> testEntry_;
    std::function<bool()> testCoreRetire_;
    std::function<reader_insertion_hooks::Outcome()> testCoreSnapshot_;
#endif
    bool gui()const {
        auto *app=qobject_cast<QGuiApplication*>(QCoreApplication::instance());
        return app&&!QCoreApplication::closingDown()&&QThread::currentThread()==app->thread()&&thread()==app->thread();
    }
    static bool normal(const Result &r) {
        const auto &p=r.probe;const auto &c=r.creation;
        return r.finished&&r.observed&&r.ownersRetained&&r.registrationAccepted&&!r.canceled&&!r.unknown&&
            !r.reconciliationRequired&&r.refusal.isEmpty()&&r.probeJob&&r.job&&r.probeJob!=r.job&&r.job==r.core.jobId&&
            p.finished&&p.attempted&&p.submissionReturned&&p.deliveryObserved&&p.matchingCompletion&&
            p.matchingCompletionCount==1&&p.enteredCount==1&&p.returningCount==1&&p.offGuiCount==1&&
            !p.canceledCount&&!p.canceled&&p.refusal.isEmpty()&&p.jobId==QString::number(r.probeJob)&&
            Entry::forwardGood(r.core)&&r.matchingCompletions==1&&r.pageAdded==1&&r.postOrder.observed&&
            c.finished&&c.attempted&&c.returnKnown&&c.accepted&&c.callbackObserved&&c.callbackCount==1&&
            !c.unknown&&!c.duplicate&&!c.canceled&&c.refusal.isEmpty();
    }
    void terminal(const std::shared_ptr<Active> &,bool exception=false);
    static void classify(const std::shared_ptr<Active> &state,bool exception) {
        state->normal=!exception&&normal(state->result);
        if(!state->normal&&state->result.observed)quarantine(state,"terminal-proof-refused");
    }
    void cleanup(const std::shared_ptr<Active> &);
    void maintain();
    static void quarantine(const std::shared_ptr<Active> &state,const char *reason) {
        state->failed=true;state->normal=false;
        state->result.observed=false;state->result.unknown=true;state->result.reconciliationRequired=true;
        state->result.refusal=QString::fromLatin1(reason);
        state->result.stage=QStringLiteral("resident-retirement-refused");
    }
    void publish(const std::shared_ptr<Active> &state) {
        if(state->published)return;
        state->published=true;dispatching_=true;
        auto completion=std::move(state->completion);
        try {if(completion)completion(state->result);}catch(...){}
        completion={}; // Finish capture destruction before any new admission.
        dispatching_=false;
    }
};
}
