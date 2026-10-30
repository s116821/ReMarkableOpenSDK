# Native source capture integration

Status: source implementation with owned producer/transport verification; native
qualification and consumer integration remain open. The frozen native
bootstrap candidate continues to use SDK `83ed037636707ce8036774f04d19f28f42c0019a`;
this design does not change that candidate or its evidence.

## Producer and evidence

Import the measured `tools/reader_identity_capture.h` and
`tools/visual_capture_core.h` closure from research commit
`05af65656e07c939349c9a10419ba94cd8b7457e`, retaining original blob provenance.
Adapt the producer beside the resident read-only backend. It must use the actual
Facilities admission, original request lease and deadline, GUI thread, and one
capture claim. Keep the same guarded window/root/engine and Reader/document/
controller objects across the capture and fresh second join.

The request supplies expected historical `ReaderFacts`. Sample the full order
with `observeOrder` inside **both** validated source callbacks, under their
borrowed guards. Both samples must match the expected document, selected page
and full order. An earlier Observe reply or a later unrelated order sample must
not be attached to the image as though it was measured inside these callbacks.

The result is an owned PNG plus historical before/after source observations,
dimensions, DPR, encoded size, digest and a named procedure. It does not provide
a content revision, input epoch, atomic snapshot, physical panel freshness,
persistence, continuing ownership or writing permission. Matching page identities
does not detect an intervening content change and return. Do not construct a
synthetic `PageObservation` or `CapturedBatch` from this evidence. Product input
admission and fresh native effect-boundary validation remain separate obligations.

## Publication and transport

Use bounded private memory for encoding. The research sink may contain an entire
PNG before the final join refuses; those bytes are not a successful result. Only
publish after all producer checks, final request admission and deadline checks
pass. Discard intermediate bytes on refusal, cancellation, owner loss, reentry or
expiry. Do not write directly to the socket from the capture callback.

Use one inline capture reply under a new protocol revision, retaining existing
runtime/connection/sequence/operation/request-digest correspondence. Keep the PNG
bound at 1 MiB and permit at most 1 MiB plus 64 KiB for its frame payload.
All frame limits here exclude the four-byte length prefix; wire storage is
bounded by the applicable payload limit plus four bytes.
Ordinary control frames and historical revision 1/2 frames retain their 64 KiB
bound. Validate the message-specific bound as well as the outer allocation bound;
an oversized ordinary message must not become valid under the capture allowance.
Update Rust/Qt codecs, client allocation, incremental decoder and endpoint queued
output together. Retain one pending request and bounded output; do not introduce
artifact paths, FD passing or a separate uncorrelated image fetch.

Capture is read-only. Delivery loss discards this capture but cannot clear an
earlier unresolved insertion/selection or the process effect reservation. Retain
caller progress checks and the original absolute deadline throughout transfer
and final admission. GUI grab/encoding are synchronous: checks can refuse late
publication but cannot preempt those native calls.

Historical decoding explicitly retains revision 1 **and** revision 2 with
their original tag grammars and byte/digest identity when the live revision
advances. The old `1 | PROTOCOL_REVISION` predicate was replaced with explicit
1/2/current revision admission. Live transport accepts revision 3 only.

## Profile bounds and completion

The research capture accepts at most 16 pages; resident order observation permits
17 (16 original pages plus an insertion). Qualify an explicit capture extension
to 17 with producer boundary cases and native evidence before advertising that
profile. Do not silently widen the research evidence or represent a refusal on
the post-insertion page as successful capture support.

This increment is complete only when an authenticated SDK caller receives and
validates the actual PNG and its historical source evidence through the resident
path. Producer-only tests, saved diagnostic files and codec-only tests do not
complete it. Required coverage includes changed/wrong full order; final-join
failure after encoding; destroyed/replaced owners; nested entry; cancellation and
expiry before publication and during transfer; malformed, truncated, corrupt and
oversized images; wrong reply correspondence; preserved sticky effect loss; and
original v1/v2 historical vectors. Inspect the actual native image and retained
source observations separately from owned host/ARM fixture results.

Buddy consumes this evidence through its native source-admission path and shared
image preparation. It must not enable synthetic dispatch or infer a write lease
from receipt of the capture. Its installed Session, durable operation journal,
binding and Reader/Writer/blank-start integration remain coordinated consumer
work in the existing delivery.
