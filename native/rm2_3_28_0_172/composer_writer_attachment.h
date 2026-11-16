#pragma once
// Private one-request transaction borrow; publication belongs to the exact job
// completion witness. No persistent document observer or public Scene callback.
#include "composer_owned_edit.h"
#include <mutex>

namespace remarkable::composer_scene { class Delivery; }
namespace remarkable::composer_writer {
class Attachment final {
    friend class Issuer;
    friend class remarkable::composer_scene::Delivery;
#ifdef COMPOSER_WRITER_TESTING
    friend struct testing::Access;
#endif
    using Clock=composer_text::Clock;
    enum class Command { Provision, Observe, Clear };
    struct Result {
        bool entered=false,finished=false;
        EditReceipt edit;
        OwnedEdit::ObservedDraft observed;
        std::shared_ptr<OwnedEdit::Range> range;
    };
    struct State {
        std::mutex borrow;
        bool closed=false,claimed=false;
        const Clock::time_point deadline;
        const Command command;
        const std::shared_ptr<OwnedEdit> engine;
        const std::shared_ptr<std::atomic<bool>> canceled,visitRevoked;
        const std::shared_ptr<const std::atomic<bool>> leaseRevoked;
        const std::shared_ptr<OwnedEdit::Range> range;
        const int revision;
        const QString expected;
        Result result;
        State(Clock::time_point end,Command kind,std::shared_ptr<OwnedEdit> owner,
                std::shared_ptr<std::atomic<bool>> cancel,std::shared_ptr<std::atomic<bool>> visit,
                std::shared_ptr<OwnedEdit::Range> existing,int rev,QString text,std::shared_ptr<const std::atomic<bool>> lease)
            :deadline(end),command(kind),engine(std::move(owner)),canceled(std::move(cancel)),
             visitRevoked(std::move(visit)),leaseRevoked(std::move(lease)),range(std::move(existing)),revision(rev),expected(std::move(text)){}
    };
    const std::shared_ptr<State> state_;
    explicit Attachment(std::shared_ptr<State> state):state_(std::move(state)){}
    static std::shared_ptr<Attachment> prepare(Command command,std::shared_ptr<OwnedEdit> engine,
            Clock::time_point deadline,std::shared_ptr<std::atomic<bool>> canceled,
            std::shared_ptr<std::atomic<bool>> visit,std::shared_ptr<OwnedEdit::Range> range={},
            int revision=0,QString expected={},std::shared_ptr<const std::atomic<bool>> lease={}) {
        if(!engine||!canceled||!visit||!lease||deadline<=Clock::now()||canceled->load(std::memory_order_acquire)||
                visit->load(std::memory_order_acquire)||(command!=Command::Provision&&!range))return {};
        return std::shared_ptr<Attachment>(new Attachment(std::make_shared<State>(deadline,command,
            std::move(engine),std::move(canceled),std::move(visit),std::move(range),revision,std::move(expected),std::move(lease))));
    }
    Clock::time_point deadline()const{return state_->deadline;}
    bool mutation()const{return state_->command!=Command::Observe;}
    void revoke()noexcept{state_->canceled->store(true,std::memory_order_release);}
    composer_scene::Callback callback()const {
        return [s=state_](::Scene *scene)noexcept {
            std::unique_lock<std::mutex> lock(s->borrow,std::try_to_lock);
            if(!lock.owns_lock()||s->closed||s->claimed)return false;
            s->claimed=true;
            const composer_text::detail::Guard guard{*s->leaseRevoked,s->canceled.get(),s->deadline};
            if(s->visitRevoked->load(std::memory_order_acquire))return false;
            if(guard.check()!=composer_text::Failure::None)return false;
            s->result.entered=true;
            try {
                switch(s->command) {
                case Command::Provision: {
                    auto value=s->engine->provision(scene,guard,s->visitRevoked);
                    s->result.edit=value.receipt;s->result.range=std::move(value.range);break;
                }
                case Command::Observe:
                    s->result.observed=s->engine->observe(scene,*s->range,guard);break;
                case Command::Clear:
                    s->result.edit=s->engine->clear(scene,*s->range,s->revision,s->expected,guard);break;
                }
                s->result.finished=true;
            }catch(...) {
                // OwnedEdit records native attempts internally. A surrounding
                // failure cannot promote a partly copied result to success.
                s->result.edit.status=EditStatus::Unknown;
                s->result.edit.failure=EditFailure::Exception;
            }
            // The native true branch flushes pending Scene damage. It is
            // required even when an attempted edit's verification is unknown.
            return s->result.edit.action.submissionAttempted;
        };
    }
    bool closeBorrow() {
        std::unique_lock<std::mutex> lock(state_->borrow,std::try_to_lock);
        if(!lock.owns_lock())return false;
        state_->closed=true;return true;
    }
    std::optional<Result> snapshot()const {
        std::unique_lock<std::mutex> lock(state_->borrow,std::try_to_lock);
        if(!lock.owns_lock())return {};
        return state_->result;
    }
};
} // namespace remarkable::composer_writer
