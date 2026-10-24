# Fresh adjacent target acquisition

This next integration step exposes target selection separately from insertion.
The measured helper in `reader_insertion_target_acquisition.h` already admits
zero movement when the target is selected, or one `moveForward()` call followed
by a fresh joined observation. Its existing insertion-coupled path remains a
research mechanism; a successful insertion reply alone never selects a target.

## Request and result

Add a typed selection request carrying complete expected historical Reader facts
(document, current page and bounded order) and an existing target UUID. The target
must be the expected current page or its immediate successor in that order.
Reject other-document, missing, duplicate and nonadjacent targets before dispatch.
This does not implement direct page opening, menu navigation or page insertion.

The executor obtains a fresh actual selected-Reader join and complete order.
Document, current page and order must match the request before any effect. Keep
the exact Reader, document, window and engine identity through this request.
When already on the target, report a historical target observation without a
navigation invocation. Otherwise consume one request-owned navigation claim and
invoke the qualified adjacent method once. Do not inspect the old Sample guard
after that call starts. Rejoin under a new Sample and require the same actual
objects, intended target and unchanged order before reporting success. Only
bounded readiness observations may repeat, under the original absolute deadline.

A dispatch return, elapsed time, completion callback, remembered index or target
presence alone is not success. A timeout/cancellation, changed objects/order,
partial transport or unknown invocation after admission retains original
operation/target uncertainty. It must not retry navigation or create a page.
A result is historical selection evidence, not a capture/write lease, saved
page position or durable persistence receipt.

## Transport, lifecycle and consumer boundary

Selection is an effectful request: canonical original-frame journaling precedes
pending admission and bytes, and transport loss is sticky just as for insertion.
The protocol must distinguish selection replies from insertion/observation replies
and verify exact target/order correspondence. Both Rust and Qt codecs/session
tests must cover the new command and wrong-kind replies.

The resident executor serializes selection with mutation. Cancellation ends its
publication/effect permission; it does not unload native code, restart xochitl or
consume the original insertion's ownership slot. A request-owned Qt object may
retire only after its synchronous sampled callback has unwound. Late timers and
facade destruction must not dispatch or publish on a replacement request.
If selection was attempted and remains uncertain, a process-resident shared effect
gate stays quarantined across connections, operations and conversations. Callback
unwind alone cannot release it: the invoked native transition may still be pending.
Read-only requests remain available. This step provides no API to clear quarantine
from target presence or elapsed time; separately admitted Supervisor recovery or
a future independently qualified settlement mechanism is required. Known no-effect
refusals can release capacity after callback unwind. Positive fresh-join completion
still needs native profile qualification before reusable capacity is advertised.

Buddy must journal selection independently of its insertion reservation, including
reuse of an already existing bound Buddy. That consumer journal and binding
decision remain a later required part of this step; protocol support alone is not
permission to wire the installed Reader. Uncertain selection permits only
read-only observation/reconciliation of the original target. Add a distinct typed
selection-reconciliation payload retaining the exact original expected order,
source and target. Existing Reconcile(InsertAfter) requires an absent target and
must not be reused by deleting that target or inventing insertion facts. Read-only
presence/current-selection reports cannot settle the original effect or clear its
quarantine. Current/adjacent
Buddy recognition and source-anchor ownership remain consumer prerequisites.

The production profile remains unqualified and effect admission disabled until
the native acceptance gates pass. Capability queries must not start the adapter.

## Required checks

Exercise owned Qt readers for already-selected zero calls, adjacent exactly one
call, nonadjacent/mismatched/refused zero calls, changed order/object/engine,
nested callbacks, deletion during permission or invocation, lost/late completion,
cancellation and deadline. Verify the fresh post-transition join, not a constructed
result from pre-transition facts. Then exercise authenticated Rust/Qt transport
and original-attempt consumer retention. Actual native target/write/reopen
qualification remains separate.

Source basis: current SDK helper and Coordinator post-navigation implementation,
the REM-57 integration handoff and current REM-25 target/reuse requirements.
The separate wire request and guarded existing-target inner call are implemented
and covered by owned tests. The native executor, shared effect gate, fresh
post-transition join and consumer composition remain unfinished and unqualified.
