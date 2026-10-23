#pragma once
// Exact-build research facilities. No constructor, admission file, or capability.
#include "reader_insertion_guard_hooks.h"
#include "reader_insertion_job_snapshot.h"
#include "reader_insertion_mutex_probe.h"
#include "reader_insertion_condition.h"
#include <QObject>

namespace reader_insertion_native_profile {
struct AttributionParts {
    void *map=nullptr;
    reader_insertion_job_snapshot::Snapshot *context=nullptr;
    bool (*read64)(void *,std::uint64_t &) noexcept=nullptr;
    reader_insertion_mutex_probe::Probe *postRejectProbe=nullptr;
    reader_insertion_condition::Calls conditionCalls;
    explicit operator bool() const {return map && context && read64;}
};
struct State;
class Facilities final {
    State *state_;
    explicit Facilities(State *);
    ~Facilities()=default; // Process-resident; no native owner release.
    friend Facilities *installBeforeMain(bool);
public:
    reader_insertion_hooks::Core *const core;
    const reader_insertion_predicate::Calls passiveCalls;
    // Only profileAndProviders is established here. Other flags remain false.
    const reader_insertion_predicate::CallerAdmission baseAdmission;
    const reader_insertion_condition::CallerAdmission baseConditionAdmission;
    const bool conditionQualified;
    const std::uintptr_t outerReturnPC,innerReturnPC;
    const unsigned observedTaskCount;
    const bool applicationAbsent;
    // GUI/outside-critical construction only. Caller retains actual strong owners
    // through candidate exit; a QObject pointer does not supply that ownership.
    AttributionParts attributionParts(QObject *stronglyRetainedWorker, bool wantPostRejectProbe=false,
                                     bool wantCondition=false) const;
};
// Exact unsupported/uncertain/partial states terminate119, without an unhook or
// logger. Caller owns nonce/root admission and actual linked init ordering.
Facilities *installBeforeMain(bool wantCondition=false);
}
