#pragma once
// Exact-ABI primitive only. Caller must own/pin the control and independently
// validate profile/provider, control identity, alignment and physical retirement.
// No native owner is adopted or automatically released by this header.
#include <cstdint>
#include <cstddef>

namespace remarkable::rm2_3_28_0_172::owner_release {
enum class Result : unsigned { Released, RetainedLast, RetainedBusy, RefusedIdentity };
constexpr unsigned MaxAttempts=8;
namespace detail {
// Generic operations permit deterministic owned contention/spurious-failure
// tests. Production calls only the fixed Builtin operations below.
template<class Atomic> Result decrement(Atomic &atomic) noexcept {
    auto expected=atomic.load();
    for(unsigned attempt=0;attempt<MaxAttempts;++attempt) {
        if(expected<=0)return Result::RefusedIdentity;
        if(expected==1)return Result::RetainedLast;
        if(atomic.compareExchangeWeak(expected,expected-1))return Result::Released;
    }
    return Result::RetainedBusy;
}
struct Builtin {
    std::int32_t *strong;
    std::int32_t load()const noexcept{return __atomic_load_n(strong,__ATOMIC_SEQ_CST);}
    bool compareExchangeWeak(std::int32_t &expected,std::int32_t desired)const noexcept {
        return __atomic_compare_exchange_n(strong,&expected,desired,true,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST);
    }
};
}
// Success consumes exactly one strong reference while another still exists at
// the CAS linearization point. Another owner may immediately perform 1->0 and
// destroy the control. Do not access any control/raw native memory after success,
// adjust weak count, or subsequently call ordinary native release for this ref.
inline Result decrementPinnedStrong(std::int32_t *strong) noexcept {
    static_assert(sizeof(std::int32_t)==4&&__atomic_always_lock_free(4,nullptr));
    if(!strong||reinterpret_cast<std::uintptr_t>(strong)%alignof(std::int32_t))return Result::RefusedIdentity;
    detail::Builtin atomic{strong};return detail::decrement(atomic);
}
}
