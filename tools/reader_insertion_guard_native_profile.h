#pragma once
// Exact-build research facilities. No constructor, admission file, or capability.
#include "reader_insertion_guard_hooks.h"
#include "reader_insertion_job_snapshot.h"
#include <QObject>

namespace reader_insertion_native_profile {
struct AttributionParts {
    void *map=nullptr;
    reader_insertion_job_snapshot::Snapshot *context=nullptr;
    bool (*read64)(void *,std::uint64_t &) noexcept=nullptr;
    explicit operator bool() const {return map && context && read64;}
};
struct State;
class Facilities final {
    State *state_;
    explicit Facilities(State *);
    ~Facilities()=default; // Process-resident; no native owner release.
    friend Facilities *installBeforeMain();
public:
    reader_insertion_hooks::Core *const core;
    const reader_insertion_predicate::Calls passiveCalls;
    // Only profileAndProviders is established here. Other flags remain false.
    const reader_insertion_predicate::CallerAdmission baseAdmission;
    const std::uintptr_t outerReturnPC,innerReturnPC;
    const unsigned observedTaskCount;
    const bool applicationAbsent;
    // GUI/outside-critical construction only. Caller retains actual strong owners
    // through candidate exit; a QObject pointer does not supply that ownership.
    AttributionParts attributionParts(QObject *stronglyRetainedWorker) const;
};
// Exact unsupported/uncertain/partial states terminate119, without an unhook or
// logger. Caller owns nonce/root admission and actual linked init ordering.
Facilities *installBeforeMain();
}
