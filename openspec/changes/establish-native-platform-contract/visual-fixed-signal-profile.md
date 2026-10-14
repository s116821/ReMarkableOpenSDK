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
