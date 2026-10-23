# Optional mutation request scope composition

Status: source/owned implementation increment. No native run or product protocol
is selected. This extends [resident retirement](resident-request-retirement.md)
and preserves the [runtime mechanism](../../../docs/research/runtime-mechanism-comparison.md).

## Selected scope

Hold one GUI retirement lease across the actual coordinator, witness and JS
producer stacks, including synchronous emission and local capture destruction.
Use an optional default-unbound binding so existing one-shot modes retain their
behavior and link inputs. Keep the existing coordinator scheduling depth separate.
No lifetime binding enters a copied native worker closure.

The plain scope header has no QObject/Fence dependency. A binding owns immutable
acquire/active hooks; the selected adapter supplies only a valid existing Fence
Frame. Snapshot hook ownership before acquisition. Scope owns the lease until
after its last hook capture is destroyed, and cannot be copied or moved. Missing,
throwing or null bound acquisition refuses; an entered terminal scope is not
active permission. Production hooks must be nonreentrant, GUI-only and noexcept
in destruction; they are adapter internals, not arbitrary client callbacks.

An optional constructor binding is immutable for the participant. Acquire the
scope before reentrant locals, callouts and queue-prefix changes. Active-token
permission precedes borrowed getters/new work. Cancellation/disarm may still
unwind under an entered terminal scope. A registration must additionally frame
the complete construction/start and delete expressions: local destructor frames
cannot cover by-value parameters, members or QObject-base/child destruction.

One controlled normal-retirement disarm may suppress the coordinator destructor's
late logical Core cancellation after normal finish and full observer disarm.
Default destruction and abnormal refusal retain their original cancellation.
Native Resident/Prepared/Core/code remain resident; no native Close, cancel,
drain, hook uninstall or unload is introduced.

## Observable acceptance

- WHEN acquisition deletes the source Binding, THEN owned hook storage survives
  acquisition; null/missing/throwing hooks never become default-unbound admission.
- WHEN terminal notification or hook-capture destruction pumps a nested event
  loop, THEN the lease remains held through that producer's final epilogue.
- WHEN a bound token is terminal, THEN stale callbacks cannot perform a borrowed
  getter/new phase even while their cleanup/unwind scope remains entered.
- WHEN normal request deletion occurs, THEN an outer registration frame covers
  all facade/child/capture destruction, and retained native owners are not closed.
- WHEN one modeled mutation A retires, THEN only a later fresh read-only B may
  observe the new order in the unchanged window/engine; unresolved mutation
  remains latched and another mutation C refuses.

## Verification and limits

Exercise the plain scope/Fence adapter and actual owned bridge, coordinator and
witness producers on host Qt, ARM emulation and sanitizers. Preserve existing
default fixture behavior. Model native acquisition/worker/mutation boundaries
explicitly; no modeled success establishes native lifetime or write authority.
Resource inventory and sticky registration failure must precede a native A/B
profile. There is no production publisher/FD protocol in this increment, and no
claim that logical retirement establishes native-owner reclamation or the fix
for the historical shutdown fault. Overall native-platform delivery remains open.
