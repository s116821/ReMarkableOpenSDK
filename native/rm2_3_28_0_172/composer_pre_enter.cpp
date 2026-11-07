#include "composer_pre_enter.h"

namespace remarkable::composer_pre_enter {
std::unique_ptr<Capture> Capture::prepare(const composer_scene::Profile &,
        composer_text::ReadRange range,std::chrono::milliseconds budget) {
    return armRequest(range,budget);
}

std::unique_ptr<Capture> Capture::armRequest(composer_text::ReadRange range,
                                             std::chrono::milliseconds budget) {
    using detail::Route;
    auto *route=Route::current();
    if(!route||route->state()||!composer_text::detail::validRequest(range)||
       budget.count()<1||budget>std::chrono::milliseconds(120000))return {};
    auto handle=std::unique_ptr<Capture>(new Capture(std::make_shared<detail::State>(range,budget)));
    auto *holder=new Route::Armed{handle->state_};
    const Route::Armed *expected=nullptr;
    if(route->armed_.compare_exchange_strong(expected,holder,std::memory_order_release,
                                              std::memory_order_relaxed))
        return handle; // Ownership belongs to the process-resident route now.
    delete holder;
    return {};
}
}

namespace remarkable::composer_pre_enter::detail {
std::atomic<Route *> Route::current_{nullptr};
#if defined(__linux__)
namespace { void *codePin=nullptr; }
#endif
bool pinPreparedRouteCode() noexcept {
#if defined(__linux__)
    if (codePin) return true; // Serialized preparation, before interception.
    Dl_info info{};
    if (!::dladdr(reinterpret_cast<void *>(&remarkable_pre_enter_shim),&info) || !info.dli_fname)
        return false;
    codePin=::dlopen(info.dli_fname,RTLD_NOW|RTLD_NOLOAD|RTLD_NODELETE);
    return codePin!=nullptr; // Deliberately never dlclose this resident reference.
#else
    return false;
#endif
}
// This function MUST permit original exceptions to unwind through its compiled
// EHABI frame. Only the bounded capture attempt contains exceptions.
extern "C" __attribute__((noinline)) void remarkable_pre_enter_shim(
        void *participant,const StockPair *owner,const QString *text) {
    auto *const route=Route::current();
    auto *const state=route ? route->state() : nullptr;
    const auto original=reinterpret_cast<Insert>(Original);
    const bool claimed=state && !state->claimed.exchange(true,std::memory_order_acq_rel);
    composer_text::Result candidate;
    if (claimed) candidate=state->attempt(participant,owner);
    try {
        original(participant,owner,text);
    } catch (...) {
        if (claimed) state->publish(candidate,false);
        throw;
    }
    if (claimed) state->publish(candidate,true);
}
} // namespace remarkable::composer_pre_enter::detail
