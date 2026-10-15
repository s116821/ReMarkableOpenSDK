# Fixed public signal profile: source-only contract

This proposal follows the [pinned assessment](../../../docs/research/vendor-frame-signal-applicability.md).
It selects no native artifact or attempt. Existing afterFrameEnd behavior remains
the default; the distinct proposed profile observes frameSwapped only.

## Exact producer selection

Introduce `VISUAL_CAPTURE_FRAME_SWAPPED=1`, valid only with
`VISUAL_CAPTURE_PASSIVE_FRAME_READY=1`; any other defined value or missing
dependency is a compile refusal. Absence selects afterFrameEnd. Startup passes
an explicit typed profile to the entry; runtime construction also rejects a
frameSwapped profile without passive readiness before publishing ready.
The profile is fixed for the entry's lifetime. No autodetection, simultaneous
connections, fallback or reconnect to another signal is allowed.

Install exactly one selected public QQuickWindow signal connection on the same
eligible witnessed window, using the existing queued GUI receiver. The callback
retains the existing first-delivery latch, current context and early-request
refusal. In witness mode, bound publication still precedes accepted delivery.
No renderer/backend getter, private hook, offset, forced update/render/input,
sleep or added queue is introduced. A queued event is not a qualified memory cap.

## Exact consumer and packet binding

The existing eight-field `frame-ready` JSON, 512-byte cap, pending-file protocol
and `stage="frame-stage-observed"` remain. Only `signal` varies with the fixed
profile: `afterFrameEnd` or `frameSwapped`; `gui_delivery_observed=true` and
`native_authority=false` retain their meanings. No additional output file or
quota is introduced; total producer output remains 1051211 bytes.

Main freezes `expectedSignal` as exactly one of those literals in the reviewed
source/artifact/manifest closure. Validation requires equality with that one
literal; accepting either value, inferring it from incoming output or changing
it after activation is prohibited. The frame marker remains necessary for image
admission; historical status/bound witnesses never substitute for it. Retained
handle/inode/ownership/current-process, pending absence, complete marker/scalar
and image checks, stream limits and whole-packet invalidation remain unchanged.

The producer's original 60-second construction deadline, 32 watch checks,
one request and one GUI grab remain. Main's existing 50/36-second margins and
original packet clock remain; no observation period is restarted. Missing or
invalid selected-signal evidence refuses image admission with unknown outcome.

## Meaning and focused verification

frameSwapped reports a public notification associated with presentation queuing,
not physical panel completion, useful/current pixels, document/page identity or
owner authority. Static vendor references do not qualify the active backend,
emission or queued delivery. Native62416 sampled a limited interval; a signal
could have occurred before attachment, but that is an unproved possibility.

After independent contract review, verify only affected controls: default literal
preservation; explicit profile/dependency refusal; selected connection and exact
literal; opposite-profile marker refusal in Main; one latch/request/grab; existing
early-request/current-context/witness guards under the new selected callback.
Synthetic signal delivery establishes plumbing only. No old matrix or native
build/nonce follows automatically. Main alone selects any separately reviewed
genuinely new permitted device attempt; all content/lifetime/recovery gates stay open.

Source basis: current producer/consumer contracts, actual62416 evidence and the
linked pinned assessment; proposed selection and controls are design decisions.

## Implementation and focused owned checkpoint

The new implementation keeps an immutable `FrameSignal` enum, default
AfterFrameEnd, and exactly one selected public-signal connection. Explicit
FRAME_SWAPPED requires passive1 at compile time; runtime invalid enum or swapped
without passive readiness refuses before ready. The marker's signal literal is
derived from that immutable selection; its guards and admission remain unchanged.

Only the new `fixed-signal-only` runner branch executed: strict ARM fixture compile,
default/explicit startup syntax, missing-passive and macro0/2 compile refusals,
12 two-profile synthetic mode cases and two runtime refusal cases. Queued delivery,
opposite-signal rejection, exact marker domain, premature request, context/cancel,
missing bound witness, original expiry and one-grab/no-rearm controls passed.
The startup syntax checks did not produce a shared artifact; no native binding
or tablet operation followed. Negative compiler logs were temporary and checked
by the frozen runner, not separately retained; raw overall stdout/stderr remain.
Qt-header GCC ABI notes are retained without warning suppression.

The first owned run failed the opposite-signal assertion. The author attributes
the failed synthetic-only assumption to possible natural software emission during
event pumping; that causal explanation was not independently measured. This is
not a native delivery observation. The repaired owned fixture alone blocks natural
window signals while pumping, unblocks for explicit injection, then reblocks
before queued delivery. The producer never blocks signals. Initial raw exit1 and
receipt `324266ccce1d1cb84bddb760572b9b386414944c0376cb5cbd025320d72c9ed5`
are preserved. A second-run host mount-index assertion occurred before subprocess
execution; that setup failure is also preserved, then corrected without an extra
compile/control execution.

SDK author's successful source/control receipt SHA256
`d166a2eafe07f178b459602815c34081914a0386a6bedaeee270ac0b1639bfe8`
binds five exact source inputs, invocation, LF-normalized shell runner and raw
overall outputs. Working header/startup CRLF bytes require explicit Git-LF
mapping at publication; no all-byte-exact claim is made here. Private evidence
has no public transcript link. The existing independent reviewer verified all
five source and five output identities and accepted the affected implementation,
focused controls and reporting supplement with no actionable findings; no rerun
or natural/native delivery was claimed. Main's distinct inert host delta and
mandatory exact-profile consumer were also accepted separately. Fresh
source/artifact/Main whole-packet review remains a separate requirement;
synthetic plumbing does not qualify active backend/emission/content or recovery.

## Proposed Main-only nonadmitting in-flight poll phase

The [actual d578 result](../../../docs/experiments/visual-only-capture-owned.md#native-fixed-frameswapped-result-delivery-observed-in-flight-witness-refusal)
delivered the selected frame marker and attempted one request. Retained044 shows
an in-flight pending image and no scalar; optional historical-witness validation
refused before metadata reached the result wait loop. Producer completion/cause
remains unknown. This proposal changes Main's poll phase only, not SDK producer,
collector, historical-witness/frame/scalar/pair validators or artifact identities.

Before the sole request, freeze a complete strictly accepted metadata snapshot
with pending-free root/owner/current-PID, ready, binding and exact selected frame
proofs. The existing strict validators produce that baseline; no post-request
observation may create or renew admission. Consume the request exactly once.

A distinct helper may return only `WAIT` when the request is already consumed,
result_hex is empty and result_meta is null in both brackets, and at least one
literal Boolean pending_image or pending_result is true. Those two flags may vary
as publication progresses; all other pending flags, cancel and restore must be
literal false. Both root/owner/identity/current-process brackets and immutable
ready/frame/binding bytes and their file metadata must equal the frozen accepted
baseline. Missing/changed bytes, file proof, selected literal or identity refuse.
Collector root/file/stream guards remain intact; any observed image is ignored
for acceptance in WAIT. No catch-all exception suppression, truthy coercion,
blind pending-byte acceptance or inferred current binding is allowed.

WAIT is observation only: no marker re-admission, renewed lifetime, capture
authorization, scalar/image transfer, decode, success or native authority. Retain
raw metadata and continue only within the original result-wait clock/call cap,
with the existing poll cadence and no second request/grab/image attempt.
Timeout, missing result or ambiguity remains unknown/refusal.

When the snapshot is settled/pending-free, use the unchanged strict diagnostic,
selected-frame, scalar, image/pair and bounded decode validators. Pending state
can never reach image acceptance. Never alter the historical validators globally
or replace final proof with the cached baseline. The baseline grants no content/
owner/current-page/panel authority; all existing guard and recovery gates remain.

Independent review precedes Main implementation. Only meaningful new controls
are needed: exact retained044 becomes nonadmitting WAIT; changed baseline bytes/
metas, identity, current-process, other pending/cancel/restore and non-Boolean
flags refuse; a local callback sequence exercises WAIT then settled strict
validation without accepted pending pixels; opposite profile or incomplete final
pair remains refused. Saved assertions are synthetic/local evidence, not native
qualification. No old matrix, new build/nonce, clock extension or spent replay.

Source basis: frozen actual/raw metadata, original Main host/validator source,
SDK source inspection and scoped offline reproduction; the distinct phase and
control requirements are proposed design. Main owns implementation and Root's
existing reviewer owns independent contract/source acceptance.

The existing independent reviewer accepted the exact three-file contract/reporting
delta3654176f and covered later forensic-scalar correction with no findings.
Main's distinct narrow typed repair manifest4d8ac5e0/helper9cdd4676 was separately
accepted after malformed-metadata WAIT cases were repaired. Its24 local assertions
are owner-reported saved evidence, not independently executed raw/native results.
No SDK producer source or artifact changed; fresh Main packet/source/artifact
review and full native capture/content/owner/lifetime/recovery gates stay open.

Main's [fresh d1 native result](../../../docs/experiments/visual-only-capture-owned.md#native-in-flight-host-repair-result-normal-capture-of-loading-pixels)
completed the normal strict scalar/image pair and bounded full RGB decode with
original live-host restoration/cleanup. The inspected pixels still show loading.
This advances the host-repair capture integration milestone; it does not grant
useful current-document/page/owner/fresh-pixel/panel/lifetime authority. Independent
actual-evidence review accepted the original19 sources/164 streams/82 stopped
jobs and separate final two streams/one job with no findings; final full decode
is receipt-reported without independent raw decoder transcript. No reviewer
rerun/device operation occurred. No new producer/profile/build/nonce or
spent replay follows; missing content qualification cannot be replaced by a
guessed wait, another signal or a nonblank-pixel heuristic.
