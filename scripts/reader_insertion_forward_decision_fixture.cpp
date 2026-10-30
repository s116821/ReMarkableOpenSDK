// Owned concurrent decision controls only; no native profile or insertion.
#include "reader_insertion_forward_decision.h"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <thread>
using namespace reader_insertion_forward_decision;
namespace {
unsigned checks = 0;
void require(bool value) { ++checks; if (!value) std::abort(); }
void later(Decision &d, State state) {
    for (auto result : {d.forward(), d.cancel(), d.reject()}) {
        require(!result.changed); require(result.state == state);
    }
    require(d.current() == state);
}
void competing(unsigned mode) {
    for (unsigned round = 0; round != 128; ++round) {
        Decision d;
        std::atomic<unsigned> ready{0}, dispatches{0};
        std::atomic<bool> start{false};
        Transition a, b;
        auto wait = [&] {
            ready.fetch_add(1, std::memory_order_release);
            while (!start.load(std::memory_order_acquire)) std::this_thread::yield();
        };
        std::thread first([&] { wait(); a=d.forward(); if (a.changed) dispatches.fetch_add(1); });
        std::thread second([&] { wait(); b=mode == 0 ? d.cancel() : mode == 1 ? d.reject() : d.forward();
            if (mode == 2 && b.changed) dispatches.fetch_add(1); });
        while (ready.load(std::memory_order_acquire) != 2) std::this_thread::yield();
        start.store(true, std::memory_order_release); first.join(); second.join();
        require(a.changed != b.changed);
        const State selected = a.changed ? State::ForwardCommitted :
            mode == 0 ? State::Canceled : mode == 1 ? State::Rejected : State::ForwardCommitted;
        require(d.current() == selected);
        require(dispatches.load() == unsigned(selected == State::ForwardCommitted));
        require(a.state == (a.changed ? State::ForwardCommitted : selected));
        require(b.state == (b.changed ? (mode == 0 ? State::Canceled : mode == 1 ? State::Rejected : State::ForwardCommitted) : selected));
        later(d, selected);
    }
}
}
int main() {
    { Decision d; require(d.current() == State::Pending); const auto r=d.forward();
      require(r.changed && r.state == State::ForwardCommitted); later(d,State::ForwardCommitted); }
    { Decision d; const auto r=d.cancel(); require(r.changed && r.state == State::Canceled); later(d,State::Canceled); }
    { Decision d; const auto r=d.reject(); require(r.changed && r.state == State::Rejected); later(d,State::Rejected); }
    competing(0); competing(1); competing(2);
    std::printf("PASS owned decision: 3 ordered cases, 384 real thread races, %u checks; no native dispatch\n",checks);
}
