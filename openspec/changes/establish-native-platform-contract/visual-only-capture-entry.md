# Independent visual-only Qt entry

Status: independently accepted contract, source and owned verification; no native
artifact or device selection. Main owns integration, current state, transport, independent
recovery and any later device operation. This is one diagnostic image for Main's
visual check. It cannot construct PageObservation, SDK Capture or native authority.

## Purpose and source closure

The legacy helper's actual two-allocation refusal remains credited. Do not choose
the first allocation, reuse an address or reinterpret an unknown capture failure.
Use one public GUI-thread QQuickWindow::grabWindow call instead of heap discovery.
The backend may return framebuffer pixels, block, allocate or affect rendering;
neither a successful grab nor the requested window proves fresh/window-specific
pixels, native page ownership, worker completion or physical panel state.

New original closure: visual_capture_startup.cpp, visual_capture_entry.h and
visual_capture_core.h, plus a focused owned fixture. Only public Qt and POSIX APIs;
no metadata-census entry alteration, FactsEntry/Probe, source5 config/witness,
application document/page/controller/current-view values, native getters,
navigation/input/creation, forced updates, frame wait loops or worker-thread grab.
No dormant callable endpoint is assumed. Held8112/equivalent and spent0404 or
later spent packets remain excluded. The old capture120s machinery is not reused.

## One entry, one request and original time

Startup schedules one app-context GUI invocation; require QGuiApplication,
application thread and effective UID0. Compile-time nonce must be 32 lowercase
hex characters. Module remains resident through process exit; no hot unload.
App-parented entry remains alive/inert after terminal state. Test hooks must be
compile-forbidden in the startup artifact.

Entry construction starts one 60,000ms monotonic setup clock. Ready/open/request
never resets it. Watch the fixed /run/rmb-visual-capture-NONCE root, coalesce
directory events, at most32 checks including the initial queued check; no polling,
retry, reconnect or automatic second grab. Missing request waits. First present
request is consumed before calling Qt, even if it subsequently refuses. A claimed
request/in-flight flag prevents nested callbacks from dispatching another grab.
aboutToQuit, cancellation, expiry, identity loss and terminal output are sticky.
After any nested event delivery inside grab/encoding, recheck termination before
continuing. Application/window/engine references use QPointer; lifetime leases
and atomic change exclusion are not claimed.

Main's proposed request cutoff is50s from a conservative before-launch anchor,
with explicit positive budget for dispatch/grab/encoding/collection. That is a
packet admission rule, not a fresh entry clock or hard native runtime bound.
Qt grab, allocations, codec work and filesystem calls are non-preemptible.
Independent numeric recovery must stop the process if it stalls; this precode
does not supply or qualify that actor.

## Request-time window admission

Take one allWindows snapshot, maximum16; retain QPointers before further calls.
Require all sampled windows live on the GUI thread and exactly one associated
existing QQmlEngine (multiple windows sharing it are permitted). The focus window
must be a live visible/active QQuickWindow on that thread, associated with that
engine. Retain its contentItem pointer. Recheck focus identity, engine, root and
visibility/active state before grab, after grab, before/after encoding and each
publication guard. Null, ambiguity, foreign thread/window, changed context or
deadline/cancel loss refuses. No reader tree sweep or application-value reads.
Pointer equality and endpoint checks cannot exclude ABA or changes between them.

## File binding and quotas

Root is retained O_DIRECTORY|O_NOFOLLOW, root UID0/mode0700/device/inode bound.
Fresh path opens must match it. Fixed input files use O_NOFOLLOW|O_NONBLOCK,
regular UID0/mode0600/single link, bounded exact read plus stat metadata/time
comparison and checked close. Owner is exact32 nonce bytes; attempt.identity is
canonical positive PID/start plus LF, maximum42 bytes, matching /proc/self/stat
from a bounded4096-byte read. visual-request and visual-ready are exact
NONCE PID START LF, maximum75 bytes. Existing output/pending names refuse startup.
visual-cancel or restore.claim presence/error invalidates admission/publication.

Returned image must be nonnull, both dimensions positive and at most1872 each,
pixel count at most2,628,288, and actual QImage::sizeInBytes at most10,513,152.
Validate using overflow-safe arithmetic before codec work, with device pixel
ratio recorded as diagnostic metadata only. These checks bound returned image
storage, not allocations already performed by grab or internal codec scratch.
Do not convert/scale/crop to disguise an oversized image. Encoder-only temporary
storage remains unqualified and must be included in Main's process resource policy.

PNG encoded bytes cap1,048,576. Use a custom sequential QIODevice writing directly
to one exclusive pending file: reject any write exceeding remaining quota before
writing; no unbounded QBuffer/readAll. Check full writes, encoding return, output
count and checked close. Increment SHA256 only over actually accepted bytes.
No successful output on short write/error, null image, empty output or overflow.
Scalar JSON cap1024; ready cap75. Unique producer payload bytes thus at most
1,049,675; pending/final hardlink names may temporarily reference the same bytes.
Maximum image names are .pending-image and visual-image.png; scalar names are
.pending-result and visual-result. Root inputs, helpers, logs, decode scratch,
transport copies and Main receipts are additional whole-packet quotas.

## Publication and consumer acceptance

Use O_CREAT|O_EXCL|O_NOFOLLOW mode0600, verify pending regular root-owned single
link file. Finish PNG encoding and checked close; recheck context/file ownership
and cancellation; exclusive hardlink final image; recheck while pending remains
(two links), then unlink pending. Never overwrite. A failed post-link guard or
unlink leaves complete-looking but invalid/unknown output. Then publish scalar
by the same write/checked-close/link/post-link-guard/unlink sequence. It contains
schema1, nonce/PID/start strings, fixed stage, GUI/window admission booleans,
dimensions/storage/encoded byte count and lowercase PNG SHA256;
native_authority=false. No pointer, document/page ID or arbitrary names.
Success stage is visual-image-observed, with neither freshness nor UI claims.
Fixed refusal stages may publish a bounded scalar only when guards still permit;
any leftover image/pending state is not a successful result.

No cross-file atomic transaction or final cancellation fence is claimed. Main's
retained-handle collector must bracket original root/owner/process/attempt,
cancel/restore absence, pending absence and both final files' regular UID0/0600/
single-link/exact sizes; match scalar binding and PNG SHA256. Missing/changed/
partial files or any post-dispatch ambiguity remains unknown, never an implicit
retry. A scalar alone cannot certify an image. Main preserves and cleans exact
owned pending/final files under its independently reviewed packet.

Before decoding, consumer enforces encoded cap and PNG header dimensions/pixel
cap. Decode under an explicit decoded-storage/scratch/process bound; validate
actual dimensions, format/storage and digest. Decoder allocations are not
qualified by header checks alone. Main's visual confirmation is necessary for
Library/document context; nonblank pixels are only a handoff filter. No persisted
image or scalar confers native page/render/owner/action authority.

## Focused observable verification and open delivery

- WHEN a valid bound request reaches a stable owned GUI window, THEN call the
  injected owned grab seam once, produce bounded matching PNG/scalar and leave
  native_authority=false. One original owned QtWindow exercises public grab too.
- WHEN window/engine/thread/root changes, cancellation/expiry occurs or a nested
  callback attempts redispatch, THEN refuse or retain unknown without a second grab.
- WHEN dimensions/storage/encoded writes exceed caps, encoding/close fails or
  publication loses its guard after linking, THEN no consumer-acceptable success;
  complete-looking pending/two-link output remains explicitly invalid.
- WHEN a final PNG/hash/binding/pending/cancel bracket differs, THEN the consumer
  refuses and cleans only independently verified owned files; no request replay.

Only controls necessary for this new entry/sink/publication are planned; no broad
Qt/native matrix, simulated freshness proof or old FactsEntry suite. Exact
source/artifact/provider review and Main's whole image/decode/clock/recovery/
transport/fixture/cleanup packet precede any separate device selection. Full
capture/current-owner/lifetime/lifecycle tasks remain open. Prior insertion,
native ink and reopen milestones stay preserved.

Source basis: existing capture-observation-plan and capture-contract, current
metadata entry semantics, measured census/capture refusals and Main's conditional
visual-only precode. Quotas and this separate entry are accepted only for the
bounded source increment; no deployment follows from that acceptance.

## Implemented source checkpoint

The precode above was accepted before implementation. The independent source
closure and focused owned controls are now accepted; see the
[reusable source/verification record](../../../docs/experiments/visual-only-capture-owned.md)
for quotas, fixed scalar semantics, initial failures, exact owned-only artifact
and reviewer/execution attribution. The requirement admits grab before the
original deadline and refuses successful publication after expiry; it does not
promise Qt returns within that deadline. Main's collector/decoder/process policy
and complete fresh native packet remain separate unfinished gates.
