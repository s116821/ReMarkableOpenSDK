#pragma once
// Original one-forward decision primitive. No dispatch, clock or capability.
#include <atomic>

namespace reader_insertion_forward_decision {
enum class State { Pending, Canceled, Rejected, ForwardCommitted };
static_assert(std::atomic<State>::is_always_lock_free, "Decision requires lock-free state storage");
struct Transition { bool changed = false; State state = State::Pending; };
class Decision final {
    std::atomic<State> state_{State::Pending};
    Transition choose(State next) noexcept {
        State expected = State::Pending;
        const bool changed = state_.compare_exchange_strong(expected, next,
            std::memory_order_acq_rel, std::memory_order_acquire);
        return {changed, changed ? next : expected};
    }
public:
    Decision() = default;
    Decision(const Decision &) = delete;
    Decision &operator=(const Decision &) = delete;
    Transition forward() noexcept { return choose(State::ForwardCommitted); }
    Transition cancel() noexcept { return choose(State::Canceled); }
    Transition reject() noexcept { return choose(State::Rejected); }
    State current() const noexcept { return state_.load(std::memory_order_acquire); }
};
}
