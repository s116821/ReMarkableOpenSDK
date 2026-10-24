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

## Selected fixed transport continuation (16.7a)

Extend the existing diagnostic Entry behind an explicit default-off resident
follow-up option, requiring conditional forwarding and target acquisition plus
distinct activation/mutation/readback nonces. Construct/start/delete the selected
Coordinator under registration frames. Its own request directory duplicate and
entire QObject subtree are inventoried and retired; the whole-trial root/final
publisher stays a separate registration resource. Checked close uncertainty is
sticky and is never retried. Native Resident/Prepared/Core/code remain retained.

Freeze A's owned result before retiring it. After its retired notification and
all deletion frames unwind, B performs a fresh join and full selected-target
order observation with callback-local document/actualId/guard; no old pointer or
Sample permission crosses the boundary. Retire B and test fresh C refusal only
in a later turn after B Retired/depth0, while A's unresolved latch remains set.
No acknowledgement of reconciliation is manufactured by these observations.

Keep the original setup/operation deadline through A cleanup, B, C refusal and
publication. Failure/cancellation/unexpected destruction suppress B and positive
publication. A's true attempted/committed facts survive a failed follow-up; an
incomplete combined result cannot mean no mutation or authorize replay.

Schema4 adds a closed resident result to the unchanged schema3 mutation facts.
Only this selected mode has an explicit8192-byte scalar cap; existing modes keep
4096. Measure the worst initial16/post17-page serializer and test caps. The strict
consumer separately validates the mutation facts and combined resident verdict.
Final publication rechecks sticky application-close suppression and weak owner
lifetime around link hooks; a late quit/deletion cannot grant a positive result.

Owned tests must exercise the actual Entry, valid Coordinator/witness/Qt path,
fresh readback helper, real request-FD and QObject cleanup, original deadlines,
late callbacks, cleanup failures, and pre/post-link revocation. Only native
acquisition/Core and bootstrap/readback identity joins may be explicitly modeled.
Native build/provider/packet/restoration qualification remains16.7b.
