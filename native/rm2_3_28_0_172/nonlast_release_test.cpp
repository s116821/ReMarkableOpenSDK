#include "nonlast_release.h"
#include <atomic>
#include <array>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <thread>
using namespace remarkable::rm2_3_28_0_172::owner_release;
namespace {
unsigned checks=0;
void check(bool yes){++checks;if(!yes){std::fprintf(stderr,"nonlast check %u failed\n",checks);std::abort();}}
struct Mock {
    std::int32_t count=2,next=2;
    unsigned loads=0,attempts=0,failures=0;
    bool released=false,spurious=false;
    std::int32_t load()noexcept {check(!released);++loads;return count;}
    bool compareExchangeWeak(std::int32_t &expected,std::int32_t desired)noexcept {
        check(!released&&expected>1&&desired==expected-1);++attempts;
        if(failures){--failures;if(!spurious)count=next;expected=count;return false;}
        check(expected==count);count=desired;released=true;return true;
    }
};
}
int main() {
    for(const auto value:std::array<std::int32_t,6>{{INT_MIN,-1,0,1,2,INT_MAX}}) {
        struct Control{std::uint32_t vptr=0x1234;std::int32_t strong;std::int32_t weak=7;}control{0x1234,value,7};
        const auto result=decrementPinnedStrong(&control.strong);
        check(result==(value<=0?Result::RefusedIdentity:value==1?Result::RetainedLast:Result::Released));
        check(control.strong==(value>1?value-1:value)&&control.weak==7&&control.vptr==0x1234);
    }
    check(decrementPinnedStrong(nullptr)==Result::RefusedIdentity);
    alignas(4) std::array<unsigned char,8> bytes{};
    check(decrementPinnedStrong(reinterpret_cast<std::int32_t*>(bytes.data()+1))==Result::RefusedIdentity);
    for(unsigned failures=0;failures<=MaxAttempts+1;++failures) {
        Mock atomic;atomic.failures=failures;atomic.spurious=true;
        const auto result=detail::decrement(atomic);
        check(atomic.loads==1&&atomic.attempts<=MaxAttempts);
        check(result==(failures<MaxAttempts?Result::Released:Result::RetainedBusy));
        check(atomic.count==(result==Result::Released?1:2));
    }
    for(const auto next:std::array<std::int32_t,5>{{-1,0,1,3,INT_MAX}}) {
        Mock atomic;atomic.failures=1;atomic.next=next;
        const auto result=detail::decrement(atomic);
        check(result==(next<=0?Result::RefusedIdentity:next==1?Result::RetainedLast:Result::Released));
        check(atomic.attempts==(next>1?2U:1U));
    }
    // A different owner can free the control immediately after our success.
    // Only the other owner touches it after observing our 2->1 transition.
    // ASan therefore detects an accidental post-success access in the primitive.
    for(unsigned iteration=0;iteration<256;++iteration) {
        auto *control=new std::int32_t(2);
        std::atomic<bool> ready{false};
        std::thread lastOwner([&]{
            ready.store(true,std::memory_order_release);
            while(__atomic_load_n(control,__ATOMIC_SEQ_CST)!=1)std::this_thread::yield();
            const auto previous=__atomic_fetch_sub(control,1,__ATOMIC_SEQ_CST);
            if(previous!=1)std::abort();
            delete control;
        });
        while(!ready.load(std::memory_order_acquire))std::this_thread::yield();
        check(decrementPinnedStrong(control)==Result::Released);
        lastOwner.join();
    }
    std::printf("{\"passed\":true,\"checks\":%u,\"native_release\":false}\n",checks);
}
