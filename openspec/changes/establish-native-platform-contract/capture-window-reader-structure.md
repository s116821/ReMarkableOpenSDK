# Same-grab reader-structure diagnostics: proposed contract

October15 precode, not implementation or a native selection. Dependency:
accepted helper publicationfd71b8b9138a5ef2f12eaaab1a0a7ef01a4d0fac,
[review5529237632](https://github.com/s116821/ReMarkableOpenSDK/pull/1#pullrequestreview-5529237632),
and [source assessment](../../../docs/research/current-document-after-loading-capture.md).
Main's exact private draft SHA256ad71541d7f244246991f647341522beb7d00baab4e9065ef6fe8c0567afad0d3
is design input, not an accepted packet or public execution transcript.
Main's subsequent exact same-grab-main-phase-contract.r1.json supplies the fixed
key, phase and bound closure below; its identity is retained in the review manifest.

## Fixed scope and mode

Propose `VISUAL_CAPTURE_READER_STRUCTURE=1`, default absent/off. Any other defined
value, or missing PASSIVE_FRAME_READY=1, BINDING_WITNESS=1 or explicit
FRAME_SWAPPED=1, is compile refusal. Runtime construction likewise requires all
three dependencies and immutable FrameSwapped before ready publication. Main's
fresh source/artifact/manifest must explicitly freeze reader_structure=true and
expected_signal=frameSwapped; no fallback, incoming-profile inference or silent
upgrade of earlier artifacts. Existing modes/API/output contracts remain unchanged.

Only two bounded metadata-only samples are added around one existing grab.
No application property VALUE reads, private offsets/getters/backend hooks,
renderer/input forcing, timers/sleeps, menu/navigation/document opening, new
native capability or mutation/admission authority is introduced.

## Actual core window, synchronous callback and lifetime

Use a distinct private core entry or source-bound typed observer path, preserving
the existing default capture call. Conceptual callback inputs are SamplePhase
(BeforeGrab/AfterGrab), guarded QPointer<QQuickWindow>, exact QPointer<QQuickItem>
content root and the original core context guard. Its Boolean return means one
typed snapshot completed, not favorable reader structure. Entry owns the two
reader_metadata::Result values and phase-completion bits. No duplicate phase,
alternate selection, callback retention, queued callback or mutable external
observer is permitted.

The core passes its actual already selected/guarded window/root, not a separately
queried focused window. New mode also checks correspondence with Entry's retained
frame/binding window/root. Invoke BeforeGrab once after valid capture context and
immediately before the sole GUI grab. After the same grab returns with valid
context and bounded/non-null image, invoke AfterGrab once before encoding.
Recheck context around both samples and encoding/publication. A failure before
grab permits zero grabs; a failure after grab never retries it.

Each callback invokes accepted censusWindow with those exact guarded inputs and
a permit subordinate to the original Entry deadline. Callback/root/window/engine/
thread/focus/ownership loss, cancellation or expiry prevents joined success.
The new core path must re-prove its retained context without a further callback
after any permit/observer can reenter Qt; the existing core's cached pre-permit
Boolean is not sufficient for this new path. No default-mode guard is relaxed.

Use guarded Entry lifetime in the original permit and observer. Do not dereference
an Entry after a callback destroys it, capture a stack sample pair in returned
Result.context, or let the callback/phase-local references escape the synchronous
core call. Returned context may retain only safe window/root/engine/application
and original permit/lifetime guards, not the temporary observer storage.

This lifetime invariant covers the ENTIRE synchronous new-mode stack:
capture returns to image, image returns to check/finish, finish invokes context
and publish/stop, and publish/commit resumes after context/postLink callbacks.
At every such callback/callee return, recheck the Entry weak guard BEFORE any
subsequent Entry member read, write or call. A safe core refusal does not permit
check to invoke finish on a destroyed Entry, nor finish to invoke stop/publish
after a context destroyed it. Copy potentially self-destroyed callback objects
(including the fixture-only postLink hook) to local storage before invocation.
Compute each helper Result in a LOCAL temporary, then recheck Entry before
writing its phase pair; retain no member reference across a census permit.
If Entry dies, clean up only independently owned LOCAL FD/sink resources exactly
once, without dereferencing Entry or using its closed/reused root FD for further
publication/unlink/stop. No default-mode rewrite or additional native scope is
implied by this new-mode constraint.

A bounded helper refusal is a completed diagnostic snapshot, with unknown or
unreached reader facts. It does not by itself prohibit diagnostic pixels while
the capture context remains valid. Missing/duplicate samples or unexpected
callback failure instead refuse joined success (fixed reader-observation-refused
failure stage); no fabricated snapshot, rearm or second grab. Native faults and
non-preemptible Qt/codec/allocation work remain process-recovery limitations.

## Exact new output and interpretation

One final `visual-reader-structure`, pending `.pending-reader-structure`, cap1024.
It contains exactly ten fields:

| Field | Exact domain |
| --- | --- |
| schema | integer1 |
| nonce | same32 lowercase hex as frame/scalar/packet |
| pid/start | same canonical positive strings, at most20 digits each |
| stage | capture-window-census |
| scope | retained-capture-window-root |
| png_sha256 | same64 lowercase hex as the normal image scalar and PNG |
| before_grab/after_grab | exactly the nine-field typed helper snapshots below |
| native_authority | Boolean false |

Snapshot fields: stage, application_thread, current_window, owner_links,
metadata, page_string_domain, visited, helper_count, container_count.
Booleans are exact; counts are integers0..128, anchor counts no greater than
visited. Stage is one of context-refused, canceled-or-expired, window-cap,
engine-ambiguous, census-refused, census-cap, foreign-window, anchor-uniqueness,
owner-links, metadata-refused, endpoint-refused, metadata-observed. Favorable
metadata-observed requires application_thread/current_window/owner_links/metadata
true and unique anchors. page_string_domain remains an independently measured
Boolean: the helper can finish readable metadata checks without QString page
types, so false must not be silently upgraded or made an impossible stage;
other stages retain partial/unreached meanings. Never convert false defaults
into proven absence, unequal samples into continuity, or equal samples into a
reader-object identity lease. No addresses, arbitrary property names/paths,
document/page values, titles or payload text are serialized.

Conservative compact ASCII JSON with newline is721 bytes using20-digit PID/start,
128 counts, five false booleans per snapshot and the longest stage literal.
This is a static serialization envelope, not a Qt/native execution result; an
owned exact serializer control must verify the implementation. Existing unique
producer bytes1051211 plus1024 gives1052235. The pending/final hard link is one
unique payload, not a second allowance. Over-cap/refused serialization cannot
publish normal success or silently truncate.

## Publication and normal-result completion fence

New mode requires both new names absent before ready. Publish PNG first, then
the new reader scalar, and existing normal visual-result LAST. Use the same
exclusive nofollow0700-root/0600-owned-regular-file, exact retained inode, bounded
write/close, hard-link/post-link proof and pending removal protocol. Reader
publication is bound to the same frozen digest, two completed snapshots and
original capture context. Cache exact committed reader bytes only after commit.

Before publishing normal success, require that new final file equals the cached
reader bytes with single-link identity proof, pending-reader absent and unchanged
context/root/owner/process. Use a distinct bounded1024 reader readback; do not
raise or bypass the existing512-byte frame/witness read limits. Failed reader
publication/readback prevents normal success. No overwrite or replacement.
The image/reader/normal scalar are not an atomic triple: orphans, partial files,
two-link states, mismatch or loss during publication remain unknown/refused and
are preserved for bounded forensic cleanup without renewed admission.

## Main collector, polling and final consumer closure

The fixed new collector always emits top-level reader_structure_hex and both
before/after reader_structure_meta plus pending_reader_structure. Missing keys or
wrong types refuse; legacy collector schemas stay unchanged. Proofs retain exact
device/inode/uid/mode/links/size types plus the collector's internal full retained
stat/byte checks. Its separate consumer requires the exact new file/identity/hash/
snapshot domains and normal scalar/PNG join across the existing one binary
transfer, followed by unchanged bounded full decode and operator visual review.

The exact collector top-level keys are phase, request_created, ready_hex,
result_hex, frame_ready_hex, binding_status_hex, window_bound_hex,
reader_structure_hex, before, after. Each frame has root, owner, identity,
owner_meta, identity_meta, ready_meta, result_meta, frame_ready_meta,
binding_status_meta, window_bound_meta, image_meta, reader_structure_meta,
process_current, pending_ready, pending_result, pending_image, pending_frame_ready,
pending_binding_status, pending_window_bound, pending_reader_structure, cancel,
restore. File proofs have exactly device/inode/uid/mode/links/size. All flags are
exact Booleans; numeric proofs/counts reject Boolean coercion. New hex is canonical
lowercase even-length, at most2048 characters, with explicit null absent proof.

The agreed static full-metadata envelope is11403 bytes, leaving4981 of16384,
using all six hex caps75/1024/512/512/512/1024, two frames with nine full proofs,
20-digit device/inode/links/size and PID/start,10-digit UID,4-digit mode and false
Boolean spellings. These widths intentionally overapproximate admissible file
states; the envelope is not a consumer-admissibility fixture or compiled collector
result. Exact fixture SHA256
5809f5bfec9dd22aab0a9b0c74728c43bc4f0c3c9ccdfa5a228570f338cbada9
is shared by SDK/Main planning. Actual source/serializer/transport controls must bind
these field shapes and verify the unchanged16KiB cap before artifact selection.

Pre-request new-reader bytes must be empty, proofs explicitly null in both
brackets and new pending false. The request-creation reply must allow fast
publication as the existing result path does: do not require new-reader
before==after across request creation or interpret it as admission. Observe/image
collection instead requires stable reader presence, bytes and full file identity
within each collector bracket; a race is unknown, not a renewed request/retry.

Extend the accepted nonadmitting poll phase explicitly: only after the sole
consumed request, while normal result bytes are exactly empty and result proof
explicitly null in both brackets, may typed pending_image/pending_result/
pending_reader_structure permit WAIT. Immutable pre-request ready/frame/binding
bytes/proofs, root/owner/current-process must remain exact; every other pending/
cancel/restore flag is false. A new-reader file present during WAIT needs bounded
canonical hex and matching regular UID0/mode600 size/file proof in both brackets;
single-link final or two links only while its own pending flag is explicitly true.
Do not parse it as reader eligibility. A new file appearing while normal result remains
absent grants no acceptance. Missing keys, coercible types or partial normal
result are not WAIT. No callback re-admission or lifetime renewal occurs.

An all-pending-false quiet gap is also possible while the normal result remains
absent, including after reader commit and before normal pending creation. Run
the unchanged strict old diagnostics and exact new-reader presence/file proof,
but do not transfer pixels, parse reader eligibility or grant acceptance. Continue
the same original bounded result loop; do not invent pending state, renew clocks
or treat this gap as a completed triple.

Settled success requires all pending flags false, complete new reader proof,
unchanged old strict frame/witness/scalar/pair guards and the new reader consumer.
Carried validators do not know the new pending key: an explicit outer guard is
required, not an assumption that unknown fields are checked. Binary image mode
requires the complete single-link final reader, pending false and identical bytes/
inode proof before and after streaming; any collector exit/guard failure
invalidates all binary bytes under the existing stdout/stderr caps. Main must add
the new final/pending file to fixed inventories and forensic retention capped1024
only after verified stock recovery. Forensic bytes never grant acceptance; unknown
proof prevents assumed cleanup. Its source/manifest may never accept missing new
evidence or fall back to an old profile.

## Limits, verification and selection boundary

Keep original60s Entry/50s host,36s request reserve,32 producer checks,45 collector
calls,one request/grab/image transfer,1MiB binary/4KiB stderr/4MiB aggregate and
128MiB decoder/original decode reserve. Two existing cooperative census bounds
are subordinate to those clocks; they do not extend them or qualify hard memory/
CPU/time recovery. No render freshness, document/page/owner identity, continuous
reader lifetime, physical panel state or NoDocumentOpen inference follows.

Independent contract review precedes affected producer/Main implementation.
Only new meaningful owned controls: actual-window/root callback correspondence;
one-before/one-after/one-grab and missing/reentrant/context-loss refusal; exact
serialization/cap/hash/file/type/duplicate/mutation proofs; new pending WAIT
versus unchanged strict settled triple/pair/decode admission. No old matrix,
native payload build/nonce/device action, held/spent replay or opening follows.
Future artifact/provider/whole-packet/native selection remains Main-owned and
separate from source/owned acceptance. No such selection exists in this proposal.
Focused Entry-deletion controls at observer/permit before and after grab,
finish-context and postLink boundaries must cover the full stack and safe local
cleanup, not merely a core lambda returning false after weak-guard loss.

Source basis: Main's exact draft, current core/Entry/publication and accepted
helper source, existing guarded packet contracts, and static JSON envelope
analysis. The callback/mode/publication/phase changes are proposed design, not
measured runtime behavior. Full SDK/current-document/Reader gates remain open.

The existing independent reviewer accepted exact R3 precode manifest2378f08f,
including the clarified whole-Entry-stack lifetime/local-FD contract, with the
prior precode P2 closed and no remaining findings. The other two workflow files,
five planning inputs and four source bindings were unchanged from R2;721-byte
scalar and11403-byte metadata envelopes are verified static bounds, not executed
collector/native evidence. Earlier R1/R2 review context is preserved. This is
source-contract acceptance only; private review has no public transcript link.
