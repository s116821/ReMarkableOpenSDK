# Minimal logical navigation API proposal

Coordinate with consumer Docs2693680ae067a065534d0e4e2dbb562ab37795bd.
This proposal defines the next source-only types/mock slice, not a native adapter
or write authority. Independent review precedes implementation. Native capabilities
remain Unsupported; no direct-open method, menu automation, new runtime or timer
is introduced.

## Types and default methods

Add a small `navigation` module with `LogicalDirection::{Next, Previous}` and an
immutable `NavigationRequest` containing operation ID, opaque fresh source
PageObservation, direction and exact intended adjacent PageKey. Its constructor
checks same document, unique valid order, source occurrence and correct neighbor;
physical coordinates/orientation do not enter this request. An adapter must still
enforce source scope/session/visit/revision/input and adjacency at execution.

`Platform::navigate(request, canceled)` defaults to Unsupported. Outcomes distinguish
unsupported, rejected without dispatch, canceled before dispatch, synthetic verified
destination, synthetic unchanged source, and indeterminate post-dispatch outcome.
Post-dispatch cancellation, deadline, wrong neighbor, stale pixels or lost guard
are indeterminate, never no-effect claims. Synthetic completion observations model
identity and readiness; they do not contain native pixels or authorize rendering.

At most one gesture is dispatched for an immutable operation/request. Repeated
dispatched operation returns an already-dispatched indeterminate result; a changed
request using that operation returns conflict. Neither result repeats a gesture or
refreshes authority. No implicit swipe chain or direct-open fallback exists. Future
native adapters must qualify five-second monotonic observation and complete pixels/
chrome/input predicates; a mock fault is not a measured timer or physical mapping.

Add `Platform::acquire_after_creation(receipt, canceled)` with a default Unsupported
result. This method establishes the explicit creation-to-observation transition;
generic observe_page is insufficient for this handoff. Outcomes are unsupported,
rejected, canceled after creation, or synthetic acquired observation. Cancellation
after creation preserves the committed page and forbids navigation/write; it does
not claim creation had no effect.

Acquisition validates the receipt's exact locally correlated request/operation,
device/instance, runtime/session/input continuity, expected committed revision and
exact after-order. It accepts only the original source visit still active or the
intended target with its explicitly expected new visit; unexpected visits/pages
refuse. A real adapter must carry its retained observer through this transition,
not reinitialize it after external input. No sticky loss is cleared. The current
mock models these opaque tokens and cannot create native evidence.

## Mock and consumer scope

Extend the existing MockPlatform with explicit synthetic navigation events, count
of dispatched gestures and immutable operation correlation. Model valid Next/
Previous, unchanged source, wrong neighbor, external input/order/session/visit
change, post-dispatch cancellation/deadline and unready pixels. Creation can model
either unchanged active source or explicit target selection, making both acquisition
branches testable without inferring real insertion selection behavior.

Buddy consumes an exact reviewed SDK commit through an explicit acquisition seam.
It invokes creation once, requires the acquisition handoff, skips navigation if
already target, or makes one logical Next only for source plus intended adjacent
target. Unsupported remains Unsupported; uncertain creation/hand-off/navigation
requires reconciliation without another creation or gesture. A successful model
outcome is named synthetic preparation, not a native binding/write permission.
Installed Reader behavior is not rewired to synthetic evidence. Native owner,
pixels, insertion, durable binding and write-guard integration remain active gates.

## Focused acceptance cases

Validate constructor adjacency/direction and foreign document rejection; valid
Next and Previous; stale/foreign execution guard; operation replay/conflict with
no extra dispatch; source/target acquisition with correct new visit; unknown active
page and unexpected visit; input/session/order/revision drift after creation;
post-dispatch cancellation/deadline/wrong target/unready pixels as uncertainty;
no movement without a repeat; unqualified defaults with no operation. Buddy tests
both zero/one-gesture branches, canceled and uncertain creation, refusal/no writes,
and exact default Unsupported. No hardware/ARM build or synthetic-to-native promotion.

Source basis: accepted SDKfb68f56 plan, clarified consumer Docs2693680 contract,
existing opaque SDK identity/creation types and explicit MockPlatform, and Main's
current authoring scope. Proposed signatures/outcomes are not yet implemented.
