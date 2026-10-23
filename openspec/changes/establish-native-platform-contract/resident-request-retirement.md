# Proposed resident request retirement

Status: pre-code design within the existing [proposal](proposal.md) and
[design](design.md). This selects no implementation, transport or device run.
The purpose is to retire one request while keeping the admitted adapter, stock
Reader process, engine and window available for a later request. It does not
qualify a native capability or change the existing runtime recovery contract.

The [worker delivery result](../../../docs/experiments/native-worker-callback-result.md)
and [ordinary creation/selection result](../../../docs/experiments/joined-creation-selection-result.md)
have separate evidence boundaries. They used one request and final process
restoration. Neither establishes repeated request retirement in a resident
stock Reader. The existing owned logical-close fixture retained an engine and
window across two closes, but admitted no native Reader getters.

## Scope and identities

Keep one registration and its executable code resident for the admitted process
lifetime. Create a fresh one-shot Entry/Witness for each fresh request nonce;
never reset a consumed Entry, replace its original deadline or reinterpret its
terminal receipt as admission for another operation. The current startup's
once flag and compiled single nonce are a diagnostic boundary, not an existing
multi-request service.

Keep activation identity, process start identity and each request identity
distinct. Each request binds the current admitted process and its own immutable
operation fingerprint, deadline and publication scope. Reacquire document,
controller, engine/window relationships and native owners for that request.
Do not cache a borrowed document/worker, prior Sample permission or Prepared
native owner as authority for the next request. Worker job IDs remain local to
their exact receiver incarnation.

This amendment fits inside the existing Ready runtime state in the
[mechanism comparison](../../../docs/research/runtime-mechanism-comparison.md).
It does not select a new IPC framework, product conversation journal, persistent
boot injection, per-conversation restart or private stock module teardown.
SDK owns request/runtime semantics; Buddy retains Supervisor policy and product
intent ownership under the existing cross-repository boundary.

## Terminal is distinct from retired

| Request state | Meaning |
| --- | --- |
| Active | One admitted nonce may perform its bounded operation. |
| Terminal, retirement pending | Outcome is frozen or indeterminate; no additional operation/phase may be dispatched. Previous call frames or owned cleanup may still exist. |
| Retiring | Callbacks and publication are disarmed; guarded owned cleanup is in progress. New request admission remains closed. |
| Retired | No old GUI call frame, deferred phase or publisher can act for this nonce; owned request resources have been retired. Native work or native callable copies may still exist. |

A terminal result alone does not admit another request. Retirement must stop
request timers and watchers, invalidate permission and queued phase delivery,
disarm the owned callback bridge and disconnect completion observers. Freeze the
bounded result once. An already queued callback must check its old context and
cannot publish for, complete or refresh the next nonce.

Finish publication once or retain its explicit unknown/failure state, including
partial-link evidence. Retire pending writer activity before closing the owned
request directory/file descriptors. Do not reuse the old path, inode snapshot,
callback bridge or writer for a new request. Retired does not mean historical
receipts or failed publication artifacts were deleted. A retirement indication
belongs to the resident registration's own bounded handoff after request cleanup;
its concrete wire mechanism is a later implementation/review choice.

## Explicit GUI stack and reentry fence

A queued invocation is insufficient: a nested event loop can deliver it before
the original Sample, native invocation, argument destructor or finish frame
returns. The resident owner must track a GUI call-depth fence across all entry
points that can admit, cancel, sample, invoke, deliver callbacks, publish or
release request ownership. Increment before any external getter, permission,
Qt/JS/native call or destructor that can reenter. Track synchronous nested
delivery too. Cross-request admission stays closed while retiring.

Only the outermost frame's final non-reentrant epilogue may decrement the last
depth to zero and schedule a retirement check. All potentially reentrant local
destructors must have completed before that epilogue; no callout, event pump or
user callback may follow its decrement. Never run retirement inline from that
decrement. A nested queued check that sees nonzero depth records pending work;
it must wait for the outermost exit rather than repeatedly rescheduling itself
inside the nested loop.

The later GUI check must verify zero prior call depth and the same request
generation. Owned cleanup runs with admission closed and its own reentry fence.
In particular, native strong-owner release can call a deleter or notifications:
disarm first, keep a weak lifetime check around release, and wait for that
cleanup stack to unwind before marking Retired. The same rule covers QJSValue,
helper/component and publisher destruction. New Entry construction is a later
admission step after this boundary, never a side effect of an old finished slot.

The caller must declare a frame before its reentrant locals and destroy it last.
This is a caller discipline, not arbitrary C++ stack introspection. By-value
parameters can outlive local frames: explicitly release their reentrant captures
under the frame, as the retirement API does for its cleanup parameter, or keep a
covering outer frame. Queued cleanup captures owned resources or guarded resident
state only, never a borrowed Sample guard or raw request/native pointer. A valid
frame tracks a terminal stack too; callers still check token acceptance before
any borrowed getter or action.

## Native ownership and uncertain effects

Request disarm does not retract a queued native command. The
[worker contract](../../../docs/research/native-worker-callback-contract.md)
permits a native value copy to retain a benign closure after its observer is
gone. Such a closure captures only shared plain atomic state and immutable
values; it has no old Entry, QObject, borrowed guard, file writer or GUI action.
Matching completion precedes destruction of remaining command captures.
Retirement therefore neither drains the native queue nor permits payload unload.
The executable code, callable managers and required dependencies remain resident
until process exit.

For the ordinary controller callback, retain the existing-engine callable and
owned/disarmable bridge contract. Late calls after cancel or bridge destruction
must be harmless; they cannot schedule Next or authorize another creation.
Per-request strong native owners cover only their established acquisition/use
boundary and are released once after the tracked synchronous stack unwinds.

If mutation might have started, cancellation, false return, lost callback,
timeout or publication loss leaves its effect indeterminate. Retiring that
observer must inhibit subsequent mutations until fresh read-only reconciliation
of the original operation establishes an allowed continuation. It never grants
retry, a new target UUID or compensating deletion. Read-only observation may
continue under a fresh admission while the original mutation remains unresolved.

Application closing stops new admission and disarms requests. It is a separate
process-lifecycle boundary, not request retirement. Do not invoke private stock
deinit, quit the engine/window, stop xochitl or unload the adapter to close a
request. The [0404 comparison](../../../docs/research/shutdown-entrypoint-comparison.md)
and [backing-store limits](../../../docs/research/shutdown-core-extent-limits.md)
do not establish that logical entry cleanup caused the historical shutdown
fault, or that resident request retirement repairs it.

## Observable scenarios

- WHEN A finishes while a nested event loop still holds its Sample/finish or
  cleanup frame, THEN A may be terminal but is not Retired, and B is not admitted.
- WHEN A's final tracked frame and owned cleanup have unwound, THEN a later GUI
  check may record retirement and admit fresh B with fresh source qualification.
- WHEN canceled A delivers a late worker or JS callback during B, THEN it cannot
  satisfy B, publish for B, refresh a deadline or dispatch a GUI/native phase.
- WHEN A's creation effect is unknown, THEN fresh read-only reconciliation is
  permitted but another mutation remains inhibited; no replay follows retirement.
- WHEN the process, engine/window or admitted runtime generation changes, THEN
  old operational handles are invalidated; matching UUID text does not restore
  authority or renew the original recovery deadline.

## Minimal implementation and experiment gates

- [ ] Implement only the resident registration/per-request retirement boundary,
  explicit GUI depth fence, bounded resource teardown and distinct retired state.
  Keep any transport amendment separately reviewable and narrowly typed.
- [ ] Owned Qt fixtures: complete A then B with the same engine/window; nested
  loops during Sample, finish, publication and owner-release cleanup; canceled A
  with delayed worker and JS delivery during B; stale nonce/source replacement;
  no extra getter/dispatch/publication and no retained request FD/watch growth.
  A timer heartbeat alone is insufficient Reader readiness evidence.
- [ ] Freeze and independently review one two-request read-only native profile.
  Both joins use the same admitted PID/start and code image, same live engine and
  window, fresh request nonces and fresh selected document/controller joins.
  Prepare the existing Reader once; neither request creates a page, navigates,
  submits a worker closure or changes a service. Observe A retirement before B.
- [ ] Keep one original activation/actor deadline and recovery cutoff for the
  whole trial. Retain the original setup window; each request cap is clipped to
  the remaining original window. B, a heartbeat, callback or retirement must not
  renew that window, the independent 360-second actor deadline or its 185-second
  recovery reserve. Refuse B if insufficient original time remains.
- [ ] Perform one final independent stock recovery after the whole trial, not
  between A and B. Preserve runtime evidence and compare complete document,
  provider and service state. Treat refusal/partial recovery honestly; no replay.

These gates are proposed work, not evidence of a completed resident adapter.
No current packet, held action or completed request is extended by this plan.

## Standalone implementation checkpoint

The original [GUI fence](../../../tools/reader_request_retirement.h) and
[owned fixture](../../../tools/reader_request_retirement_fixture.cpp) now model
this narrow boundary. The [focused test](../../../tools/reader_request_retirement_test.sh)
passes 743 checks on the host and ARM/qemu with unchanged sources. It covers
real nested event loops, deferred cleanup/capture destruction, a depth-blocked
queued check without repeated nested rescheduling, terminal/retired separation,
two generations, stale callbacks, sticky cleanup failure, explicit reconciliation
handoff, owner deletion, wrong-thread refusal before GUI token resolution and bounded request FD/watch cleanup. The JS bridge is
owned Qt/JS; the worker completion is a synthetic cross-thread queued Qt signal,
not a native worker command or effect.

Mandatory effect disposition distinguishes no mutation attempted from required
reconciliation; even an accepted ordinary creation needs reconciliation before
later mutation. The reconciliation acknowledgement is an explicit trusted caller
handoff, not evidence or authority manufactured by the fence. Its 256-nonce
history is a finite research-profile bound that fails closed at exhaustion, not
a selected production journal policy. Caller-owned absolute deadlines remain
outside this standalone component. Resident registration, native pair integration,
independent review and the two-request native gate remain unfinished.

Source basis: current proposal/design and runtime-mechanism comparison; current
one-shot Entry/startup and both owned witnesses; separately recorded worker and
joined-creation results; historical 0404 comparison and extent analysis. The
retirement state machine, GUI depth fence and two-request experiment above are
proposed engineering requirements, not measured native lifetime guarantees.
