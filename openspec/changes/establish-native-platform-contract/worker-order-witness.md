# Fixed worker-local page tuple witness

The [execution-time boundary](../../../docs/research/execution-time-page-order-boundary.md)
leaves source/order exclusion unresolved. This separate research increment tests
whether six passive calls on the already-qualified native document worker can
correlate three frozen page IDs and indexes with an earlier GUI source join.
It does not implement page creation, an atomic map transaction or a shipping API.

## Proposed behavior and limits

Freeze the selected source index, page count and three distinct canonical page
IDs on the GUI thread. Require both immediate neighbors to exist. Submit one
native callback to the strong-owned worker obtained from that same sampled
document/controller/native-ID join. In that callback, visit only the frozen
indexes source-minus-one, source and source-plus-one. For each, call the pinned
native ID-at-index accessor, then its inverse index-for-ID accessor. Stop on an
empty/invalid ID, mismatch, inverse mismatch or exception; never scan, follow a
new index, count pages, retry or invoke a mutator. Six is a call-count ceiling,
not a bound on mutex waiting or native execution time.

Load both accessor addresses only after exact executable/ABI/type admission and
disk-versus-mapped executable-byte verification. Use the vendor compiler's typed
QString return and reference ABI, not reconstructed string storage. The worker
callback uses immutable values and plain shared state; it calls no QObject/QML
property getter, GUI API, file operation or native-owner release. Each native
accessor locks its own read. A positive tuple does not prove that all six reads
share one map version or that a later insertion has the same source/order.

## Ownership and completion

A capacity-one resident quarantine accepts the strong Document/Worker lease
before connecting or submitting. Request observer destruction, retirement,
cancellation and deadline expiry must never release a possibly queued/running
raw-worker receiver. The resident storage has no automatic/native shutdown
cleanup. Unknown or canceled work remains pinned until process exit and blocks
another worker lease request. This deliberately limited diagnostic policy is
not a production leak-management or cancellation policy.

The callback claims execution once before any raw-worker use; later copies
cannot dereference that receiver. Its captured values and destructors retain no
native ownership or native-pointer action. Successful reclamation requires a
nonzero returned job ID, one off-GUI entry/returning pair, and the exact worker's
matching completion whose static contract follows callback return. Completion
may precede command-value destruction; this is why capture destruction must
remain passive. Buffer bounded early completions while submit is on the stack.
Release only on a later zero-depth GUI turn after submission and consumer cleanup
unwind, then publish/reclaim on a further zero-depth turn after native release
and capture destruction unwind. Refuse nested admission while notifying.

WHEN cancellation, source destruction, timeout, thrown/zero-ID submission,
duplicate delivery, completion overflow or finalization failure occurs, THEN
the lease stays quarantined and there is no automatic close/retry. WHEN the
matching delivery and quiescent GUI boundaries pass, THEN close Worker before
Document on the GUI thread and report the release outcome separately from the
tuple. The strong-owner model does not protect against arbitrary native
explicit deletion, memory corruption or interposition.

## Registration and evidence

Keep the existing one-request file identity, immutable nonce/process/start-time,
root/owner/request-inode, cancellation/restoration and publication checks. Use
one original setup/recovery window and a fixed plan deadline; no callback or
cleanup renews it. One normal fixture open and existing loading opportunity may
precede a fresh GUI join. Record the pre-submit source, actual worker delivery,
tuple results, matching completion and explicit release/quarantine as separate
facts. All mutation/render/durable authority flags remain false. Keep old no-op,
creation and resident-pair artifacts unchanged.

## Tasks

- [x] Implement/review the pinned accessor shim and owned fixed-tuple controls.
- [x] Implement/review resident ownership quarantine and cancellation/reentry,
  foreign/early/duplicate completion, late callback and finalization controls.
- [x] Integrate one separate registration/startup and verify its source/clock,
  binding, publication and original recovery controls on host and ARM.
- [x] Freeze/review a fresh native profile; execute once if its concrete gates
  pass, preserve complete document bytes and record restoration/cleanup.
- [x] Interpret the result without claiming execution-time mutation exclusion,
  full lifecycle qualification, supported capability, canonical sync or archive.

Native checkpoint: [attempt 9276](../../../docs/experiments/worker-local-order-result.md)
observed all six expected worker ID/index reads, one off-GUI callback and exact
job 25 completion, explicit GUI owner release and settled depth-zero publication.
The original stop/restoration succeeded, all twenty fixture files were unchanged,
and the exact archived inactive RAM root was removed. This closes the fixed
success-path diagnostic, not native cancellation, atomic insertion exclusion or
the broader platform/consumer capability gates.

Owned checkpoint: the accessor passes 99 checks each on host, ARM and host
ASAN/UBSAN; the quarantine passes 170 checks on host and ARM. The separate Entry
passes 17 owned real-QML/worker modes on host Qt 6.4.2 and ARM Qt 6.10.3 under
QEMU, including its compile-only startup. The final Entry runs took 45.3 and
69.2 seconds including compilation. All source hashes were unchanged throughout.
The worker functions are owned fakes; no proprietary accessor was invoked.

Independent source review repaired guard-deletion checks and the publication
boundary under nested release notification. Combined testing identified that
creating a receiver/timer during the Sample delivered child events and invalidated
the global observation epoch before submission. The registration now reserves
those objects before sampling; a new receiver is reserved only after the previous
Sample and cleanup unwind. Historical disconnected receivers and plain records
remain bounded by the research 256-generation cap. No deferred QObject deletion
may invalidate the next Sample.

The Entry separately requires the later `settled` notification and depth zero
before success publication. Its nested-loop regression verifies no result inside
the earlier release notification. The GUI join observes the current center ID,
index and count; both neighbor IDs remain expected values from the frozen profile.
The worker tuple must not be described as three freshly sampled GUI neighbors.

Private receipts, retained outside Git: accessor
`3190bfcacdc06db19e28d89ab118f4e85e6db33c4a0286122e878c69dd75e066`;
quarantine `cddb6392ea3f1c5f8b10162389ed2a9119214fdd8d7615c920e1f1e92ad927cb`;
independent source review
`22ecdb2337bcdbf9955e06b68e0dbc0574241ea2ecfaaa87bc29f646eef1d865`.
Earlier compile refusals, the pre-submission epoch refusal and superseded passing
controls remain preserved. This closes source/owned controls only; a fresh exact
native profile and complete preservation/restoration evidence remain separate.

Source basis: retained REA/Ghidra worker-dispatch and page-map findings; exact
ARM accessor/helper/EHABI inspection and vendor compile-only QString ABI checks;
the prior native no-op worker and resident-pair results. Proposed quarantine
and tuple behavior are engineering design, not observed native execution.
