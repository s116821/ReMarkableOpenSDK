# Fixed stroke and independently retained release

## Question and boundaries

After a fresh ready identity for the intended existing target, can one fixed
short pen stroke be observed, released, saved by the ordinary stock path, then
read back after a separate close/reopen without changing original content?
The [native mutation/readback result](../../../docs/experiments/resident-mutation-readback-native-result.md)
now establishes one native request-retirement sequence. It does not establish
this writing path. No further creation is needed for this question.

The retained Rust helper's geometry/event construction and the vendor ABI are
indexed in the [pen boundary note](../../../docs/research/pen-event-release-boundary.md).
Use the fixed virtual path360,480 to372,484 only: native11138,7371 to11056,7616,
52 interpolation steps,56 SYN_REPORT frames and178 records. Construct vendor
input_event records directly; require the observed16-byte layout. No screenshot,
menu, eraser, navigation or arbitrary-stroke branch belongs in this helper.

## Small owned policy first

Add a plain C++ policy with fixed frame construction, original absolute stroke
and recovery deadlines reserved before child start, bounded one-call-per-frame
dispatch, and an independent one-use release guard. Test host/vendor-ARM/ASAN
through explicit modeled IO and real owned child/descriptor controls. This
policy is not an input-device implementation or native qualification.

The [owned policy result](../../../docs/experiments/fixed-stroke-release-owned-result.md)
records the actual controls and preserved defects. Consume a noncopyable
whole-stroke claim before every IO callout; per-frame no-retry alone permits
duplicate whole strokes. Arming recovery is also one attempt, with its latch
before callouts and a spent recheck afterward. Check time after release write
and after the final identity callout against the original recovery deadline.

Any short/zero/error/interrupted write stops further stroke frames and retains
possible-dispatch/contact facts. Never retry bytes or redraw. A complete write
sequence means accepted writer calls, not rendered ink. Context/clock changes,
exceptions and exhausted original budgets also stop; a later phase cannot grant
another budget. The release guard is spent before its callouts and can emit only
pressure0, distance100, touch0, pen0, SYN_REPORT. It cannot move X/Y or erase.

## Separate native composition

A later fixed helper must check the current RM2 device identity/capabilities,
all contact/eraser keys and pressure released, and an empty observer queue before
contact. It retains independently opened observer/recovery/writer descriptors
for the same device. Do not grab the input device away from stock Qt. The
separately scheduled parent survives writer-child death, owns the already-open
recovery descriptor and issues at most one release group, with no redraw. Native
descriptors, child lifecycle and observation require their own actual checks;
closing a descriptor or killing a helper is not release evidence.

Successful release needs a bounded complete observer stream, a released kernel
snapshot and checked descriptor identity; partial/dropped/conflicting events
refuse confirmation. The kernel may filter unchanged events, so178 constructed
records is not an expected observer-event count. Snapshot alone cannot prove
that no delayed partial frame remains. Holder/device/kernel failure remains an
explicit best-effort recovery limitation. Do not close/reopen with unresolved
contact. Recovery release authority is independent of the document-context
permission which may already have been lost.

The native write, fresh post-write identity/capture, ordinary close/save and
separate fresh reopen/join/capture remain a distinct packet. Keep complete
before/after backups, full page-object/order/PDF/unknown-field preservation, and
allow target .rm/thumbnail plus known bookkeeping only. No replay or rollback
after possible dispatch. One idle-fixture observation cannot atomically bind
kernel routing to a document or grant general input/revision/native authority.
