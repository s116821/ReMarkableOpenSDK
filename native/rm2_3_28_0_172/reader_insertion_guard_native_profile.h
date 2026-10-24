#pragma once
// Exact-build research facilities. No constructor, admission file, or capability.
#include "reader_insertion_guard_hooks.h"
#include "reader_insertion_job_snapshot.h"
#include "reader_insertion_mutex_probe.h"
#include "reader_insertion_condition.h"
#include "nonlast_release.h"
#include <QObject>

namespace reader_worker_native { class ResidentSlot; }
namespace reader_insertion_native_profile {
enum class NativeOwnerKind { Document, Worker };
using OwnerReleaseResult=remarkable::rm2_3_28_0_172::owner_release::Result;
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
private:
    friend class reader_worker_native::ResidentSlot;
    // Only for a normally returned, still-owned control after separately proved
    // physical retirement. Success consumes that reference; caller clears local
    // words without touching native memory or invoking ordinary release again.
    // Every refusal retains the reference. This does not prove Task quiescence.
    OwnerReleaseResult tryRelinquishNonLast(NativeOwnerKind,std::uintptr_t raw,std::uintptr_t control) const noexcept;
};
// Exact unsupported/uncertain/partial states terminate119, without an unhook or
// logger. Caller owns nonce/root admission and actual linked init ordering.
Facilities *installBeforeMain(bool wantCondition=false);
}
