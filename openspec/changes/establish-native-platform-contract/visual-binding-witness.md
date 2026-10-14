# Bounded passive binding-stage witness

Status: independently accepted source-only design. Existing29a2e8d producer
is unchanged. No implementation/build/nonce/native attempt is selected. This is
diagnostic separation of binding from frame delivery, not a remedy for missing
signals or blank images. Main owns any future whole packet and device work.

## Actual gap

The [native passive a6 observation](../../../docs/experiments/visual-only-capture-owned.md#first-native-passive-mode-result-no-witnessed-frame-original-closeout-passed)
retained14 same-generation samples with no accepted frame marker and no request,
image or result. Original live-host primary restoration/cleanup passed; content/
vendor signal/lifetime and complete Reader gates stay open. Missing output cannot
distinguish never-eligible binding from later signal/context/publication/margin
failure. The spent packet is not replayed to guess a cause.

Retained launch/ready/publication records establish only infrastructure readiness:
source publishes ready before binding/observer installation. The14 samples span
about4.250–14.109 seconds after activation, not the full entry deadline. Existing
capture and frame paths share public eligibility guards; c69's different process/
time cannot establish a6 startup eligibility. This proposal addresses that exact
missing stage evidence rather than assuming native signal absence, a stricter
guard or a blank-frame cause.

## Smallest declared source delta

Propose an explicitly declared default-off VISUAL_CAPTURE_BINDING_WITNESS=1 mode
that requires VISUAL_CAPTURE_PASSIVE_FRAME_READY=1. Invalid/incomplete mode values
compile-refuse. Existing default/passive behavior remains unchanged when absent;
old artifacts/ready files cannot implicitly advertise this witness.

Retain current bootstrap/focus-event window snapshot16, one associated engine,
GUI thread/current visible-active focused window/content-root guards. Use only
already-evaluated states; no additional application property values, names,
private pointers/addresses, native getters, backend selectors or traversal.
No forced update/rendering, input/navigation, timer renewal, polling, connection
model workaround or alternate-window chase.

Publish at most two separate exclusive diagnostic witnesses:

| File / pending name | Emission point | Fixed stage and booleans |
| --- | --- | --- |
| visual-binding-status / .pending-binding-status | Initial binding attempt defers or rejects while safe entry/root publication guards remain valid | waiting-first-eligible-window or binding-rejected; window_bound=false; frame_observer_installed=false |
| visual-window-bound / .pending-window-bound | First eligible window/engine/root is latched and the queued afterFrameEnd connection is valid, before processing its delivery | window-bound-observer-installed; window_bound=true; frame_observer_installed=true |

Each scalar is maximum512 bytes with exactly eight fields: schema1, nonce/PID/start
strings, fixed stage, window_bound and frame_observer_installed booleans,
native_authority=false. No pointer/frame/page ID or arbitrary reason string.
Initial status is emitted once only if that initial deferred/rejected state actually
occurred; immediate binding can emit only the bound witness. A later bound witness
does not overwrite waiting evidence. Cap/ambiguity/thread/guard failure does not
invent a waiting state; initial rejection may report binding-rejected only if
safe root/entry publication is still possible. Root loss/expiry/publication failure
can leave no status. Later rejection cannot overwrite an earlier waiting status;
witness absence remains unknown, and no arbitrary reason string is added.

The bound witness describes sampled public context and connection installation,
not actual signal emission or delivered frame content. It is emitted after the
connection is established; queued callbacks cannot become direct rendering-thread
work. Already queued callback volume/internal allocations remain unqualified.
Connection/write failures or loss of latched context terminate without chasing
another window or making a readiness claim.

Use the existing retained root/owner/PID/start and regular root-owned0600 input
guards, bounded exclusive pending write/checked-close/original-inode/link/
post-link guard/unlink protocol. Waiting publication requires unchanged valid
entry/root state; bound publication additionally requires the latched window/
engine/root context. Pending/two-link/partial/path-substituted output is invalid.
No atomic witness/frame/request transaction or cancellation fence is claimed.
Late IO/context/cancel ambiguity remains unknown.

## Interpretation and unchanged admission

A retained valid bound witness with no valid frame marker supports only
"binding/observer installation was witnessed; qualifying frame publication was
not witnessed". It does not prove the signal never emitted: missed timing,
queued delivery, later context loss or output failure remain possible. A waiting
witness without bound output says only the initial attempt deferred and no bound
publication was witnessed. Neither absence nor stale waiting evidence proves
the current binding state. Main must not turn these booleans into page/UI authority.

The witness never admits an image request. Main still requires the separate valid
frame-ready marker and existing pair/decode/visual guards. Preserve repaired
premature-request consumption before latch and request-absence publication checks.
One request/one grab, original construction60s, Main original cutoff/margins and
32 directory checks remain. The additional file events share those existing
checks; no extra clock, poll/retry budget or reopening is introduced.

## Explicit quota and integration delta

Two additional512-byte witness files add1024 unique output bytes to1050187,
yielding1051211. Each pending/final pair can temporarily be hardlinks to the same
bytes. Main must extend its exact producer inventory, pending-name rejection,
retained-handle scalar/meta collection, error/transport/preservation/cleanup quotas
and typed interpretation. Capped files do not bound codec, Qt event queue, syscall
latency or process memory; independent recovery and decoded/scratch policy remain.

Focused source controls only after design acceptance: initial eligible versus
deferred binding witnesses; later focus-event binding without overwriting waiting;
connection/context/cancel/expiry/publication loss; premature queued request still
refused; witness presence never substitutes for missing frame-ready or dispatches
another grab. Preserve actual no-signal outcome and reject first-match/frame/input/
deadline workarounds. No old matrix or identical native retry is requested.

Source basis: accepted29a2e8d binding/delivery source, retained a6 samples/receipts
and separately attributed Main execution reports. The witness design is inference
to separate two source stages; neither stage's native success is demonstrated.
Independent design/source-consumer review precedes implementation; exact fresh
source/artifact/whole-packet selection precedes deployment. Prior insertion/ink/reopen milestones
and all original failures remain credited; no canonical archive follows.

## Design review

Root's existing Sol reviewer verified frozen r2 manifest
def8d207099e8b0f7f74909c2e172e4a24da06d32846c4a890651208426f5571
and all four file hashes/sizes, and accepted the bounded design with no actionable
finding. It also accepted Main's historical-only validator source: exact capped
schema/identity/type/meta domains, initial false/false waiting or rejected states,
positive true/true installed state, and no capture authority. Three valid/seven
rejection cases were inspected in source and owner-saved PASS JSON; execution is
owner-attributed, without an independently retained raw execution/source-hash
transcript or reviewer rerun. Internal review messages have no public transcript
link. Concrete implementation/collector/cleanup amendments are the next increment
to review; this acceptance selects no native artifact, nonce or experiment.
