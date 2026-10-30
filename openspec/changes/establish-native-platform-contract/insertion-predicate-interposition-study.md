# Owned study of an insertion predicate with preserved native execution

The [order boundary](../../../docs/research/execution-time-page-order-boundary.md)
remains unresolved. New exact static evidence locates the final native map lock
and insertion call. The ordinary handler supplies neither an expected-source
predicate nor a normal refusal branch. Task completion without pageAdded is
distinct from insertion. Native handler exceptions currently unwind beyond the
worker job; throwing into stock is not a rejection protocol.

## Hypothesis and strictly owned first stage

A possible design uses two function-entry wrappers: an outer wrapper around the
existing handler and an inner wrapper at its actual map-insertion helper. An
immutable per-operation record identifies the expected worker/map/source and
requested target. An eligible inner wrapper could validate direct map facts while
the handler's final map mutex is already held. A failed predicate would throw a
private exception **only to the outer adapter wrapper**, after the original
handler's C++ cleanup has unlocked and destroyed its temporary values. The outer
wrapper would record explicit refusal and return without fabricating pageAdded.
Accepted operations would forward through the entire ordinary handler and Task.

This is a proposed engineering approach, not an existing native API or selected
tablet mechanism. It depends on exact private signatures, matched ARM unwind
semantics at the insertion callsite, safe instruction relocation and no earlier
creation mutation. The native function's ordinary throw-to-worker behavior is
not assumed caught. The private guard-rejection exception must not escape the
proposed outer wrapper. Unrelated exceptions must preserve their original
propagation; the owned test supplies an outside catch to verify this. Do not
catch all failures and return normally as though a real native failure had been
an intentional refusal. A future native non-private exception/outcome policy
remains a separate qualification requirement.

First test this principle entirely on owned ARM/Qt code. Use a pinned, locally
built interception library and explicit function-entry hooks; do not use a
mid-instruction register callback, skipped C++ cleanup, foreign exception escape,
an external wrapper around an already-locking native getter, queue surgery or
copied proprietary handler code. Test all of:

- Exact integer/reference and hard-float argument forwarding through the
  relocated original, including transparent calls outside the eligible record.
- Owned worker-thread execution, final mutex held at the inner predicate,
  ordinary accepted insertion and its separate notification/completion facts.
- Predicate mismatch/cancellation: no insertion, no false page-added event,
  explicit refusal, all temporaries destroyed and mutex available afterward.
- Private exception caught by the outer wrapper after original cleanup; unrelated
  exceptions preserve their original behavior under an outside owned test catch.
- One eligible immutable record and unambiguous scope; no second submission,
  success resurrection, deadline renewal or premature owner/code teardown.
- UUID collision causing a different effective target and index normalization to
  append: retain outer operation eligibility and reject the changed inner
  arguments. Do not silently bypass the guard because the inner UUID changed.
- Cancellation versus insertion claim with one atomic linearization point. A
  winning cancellation prevents insertion; a winning insertion claim cannot
  later be reported as canceled before effects. Keep GUI-visit coherence separate.
- An early loading/status side effect followed by refusal remains observable;
  post-mutation exceptions remain distinct from private pre-insertion refusal.
- Nested foreign maps/threads, duplicate inner calls and observer loss must not
  borrow the eligible operation's permission or destroy its retained record.

Configure hooks before worker threads start, then keep them and their code resident
until process exit. A passing QEMU fixture would establish only the owned
interception/unwind principle. It cannot establish native signatures, live patch
installation, original C++ landing-pad coverage, absence of earlier effects,
native no-insertion outcome, safe last-owner disposal or supported capability.

## Evidence and next admission

Initial primary-source survey is pinned: [XOVI ARM32 pivotSymbol](https://github.com/asivery/xovi/blob/2b99649f5e4fd6288be7792a8570bd16418adb70/src/trampolines/arm32/arm32.c)
resolves a symbol name through dlsym and temporarily rewrites the original entry
when forwarding. Its exposed interface is not a ready-made private critical-site
guard. [Dobby's header](https://github.com/jmpews/Dobby/blob/5dfc8546954ce3b3198132ab13fddb89ee92cdd7/include/dobby.h)
and [function-entry routing](https://github.com/jmpews/Dobby/blob/5dfc8546954ce3b3198132ab13fddb89ee92cdd7/source/InterceptRouting/InlineHookRouting.h)
provide an address-based function hook and relocated original pointer. Its ARM32
register-callback interface does not describe floating-point register capture;
this study uses typed function entries. Owned forwarding/unwind results are
recorded below; native forwarding/unwind behavior remains unqualified.
Source availability is not SDK/build/runtime qualification.

Two unchanged upstream build attempts failed under GCC 13.4: the initial pin
also lacks an included Cpu.h, while the coherent earlier
[e9fe7f revision](https://github.com/jmpews/Dobby/tree/e9fe7fbecae47a2287e761080f8b1133cc22e8fa)
restores that header but still uses an unsupported Clang feature-query macro.
Both failed archives and build logs are preserved. The owned followup uses
the earlier pin with a separate preinclude header guarded for ARMv7 hard-float
GCC, defining that query as false for the unsupported pointer-authentication
feature, plus a C++17 target override. A derived build copy additionally makes
the header-defined Logger::Shared inline to resolve duplicate strong
definitions; its exact one-token diff and before/after hashes are retained.
The pristine upstream tree remains unchanged; the compiled overlay is explicit.
This is a compatibility experiment, not an unmodified default build or native
runtime qualification.

The [owned ARM result](../../../docs/experiments/insertion-guard-owned-result.md)
passed 17 cases and 361 checks, with independent source review and a second
execution reproducing the same binary/result. Original public fixture/build
inputs are linked there. This closes the owned principle only; the native
proposal requirements below remain open.

XOVI and alternative tool interfaces must be checked at exact upstream revisions.
Keep any downloaded tool sources/builds isolated, pinned and removable, without
changing global client/MCP/skill setup or the tablet. Do not add an interception
dependency to the shipped SDK from an owned experiment.

Only after independent source and owned-result review may a separate native
proposal identify the exact two callable signatures, expected original bytes,
ARM/Thumb/ABI and unwind constraints, installation point, operation matching,
direct-map read contract, original Task/outcome channel, original clocks,
independent restoration and full preservation checks. That later proposal must
separately address a UUID collision substituting another target, pre-insertion
loading effects and failures after the first mutation. Rejected-before-insertion
must not become an unproved all-effects transaction rollback claim.

- [x] Review/pin library support and exact build inputs without native changes.
- [x] Build and independently review the owned ARM interception/unwind fixture.
- [x] Record the actual owned result and limitations before any native proposal.
- [ ] Decide the next native contract question from that evidence; no native
  interposition candidate is selected or authorized by a fixture pass alone.

Source basis: current exact-build critical-section and Task/unwind findings,
current bridge/ownership design and primary upstream interception sources.
The two-wrapper mechanism is an inference to test, not native qualification.
