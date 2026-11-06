#include "composer_pre_enter.h"

namespace remarkable::composer_pre_enter::detail {
// This function MUST permit original exceptions to unwind through its compiled
// EHABI frame. Only the bounded capture attempt contains exceptions.
extern "C" __attribute__((noinline)) void remarkable_pre_enter_shim(
        void *participant,const StockPair *owner,const QString *text) {
    auto *const state=Route::instance().retained_.get();
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
