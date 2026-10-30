#pragma once
#include "reader_request_retirement.h"
#include "reader_request_scope.h"

namespace reader_retirement {
namespace detail {
struct ScopeLease final : reader_request_scope::Lease {
    Frame frame;
    ScopeLease(Fence &fence,const Token &token):frame(fence.enter(token)) {}
};
}
// Internal GUI-only adapter. Frame construction uses C++17 guaranteed elision;
// no movable Frame, extra QObject, native callback or borrowed Sample is needed.
inline reader_request_scope::Binding binding(Fence *fence,Token token) {
    const QPointer<Fence> weak(fence);
    return reader_request_scope::Binding([weak,token]() -> std::unique_ptr<reader_request_scope::Lease> {
        if(!weak)return {};
        auto lease=std::make_unique<detail::ScopeLease>(*weak,token);
        if(!lease->frame)return {};
        return lease;
    },[weak,token] { return weak && weak->accepts(token); });
}
}
