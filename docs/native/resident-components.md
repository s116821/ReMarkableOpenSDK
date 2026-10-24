# Resident native adapter components

The SDK's Rust `native-adapter` feature and `native/resident` implement a bounded
request protocol and Linux/Qt endpoint. `native/rm2_3_28_0_172` contains the native
Reader mechanisms being integrated for the exact RM2 3.28.0.172 profile. These
components do not yet form a qualified, activatable product adapter.

## Source provenance

The native profile, selected-Reader join, insertion predicate, conditional hook
and worker/QML witnesses were imported from SDK research revision
[`05af65656e07c939349c9a10419ba94cd8b7457e`](https://github.com/s116821/ReMarkableOpenSDK/tree/05af65656e07c939349c9a10419ba94cd8b7457e/tools).
`upstream-source.json` records the SHA-256 of each original Git blob. These are
baseline source hashes, not hashes of later adaptations or a compiled adapter.
The imported post-order fixture adds an explicit `return 0` so its owned models
can be reused under a renamed test entry point. The predicate test runner now
resolves this directory and requires a caller-selected output directory. The native
profile now pins exact owner-control table prefixes and provides a private
non-last-only release gate. Core registration/cancel/snapshot now use opaque
request tokens; the Coordinator retains and passes its original token. The owned
ARM hook fixture is imported with baseline provenance and adapted to these APIs.
The resident executor and authenticated dispatcher now connect these adapted
mechanisms. No proprietary firmware binary or extracted asset is included.

## Current integration

`order_observation.h` adapts the measured public-metaobject order reader to
requests without an already known insertion order. It runs inside the original
selected-Reader callback, retains its guard, verifies document identity and both
page mappings, and reads the bounded order twice. It returns historical wire
facts only. Two matching passes do not establish an atomic snapshot, revision,
input epoch, durable insertion receipt or permission to write.

The current bound is 17 observed pages, matching the handoff's 16-original-plus-
one observation envelope. This is an integration bound, not a claim that ordinary
documents are limited to that size. Broader document support must be implemented
and qualified before product capability advertisement.

`ReadOnlyBackend` requires facilities produced by the exact native profile loader
and connects the actual selected-Reader join to Observe/Reconcile. Reconciliation
reports target presence only within the original document. No borrowed pointer,
native QVariant or guard escapes the synchronous callback. `ReadOnlyBackend`
refuses mutations. `MutationBackend` separately connects InsertAfter to the
resident executor with the original request identity and absolute deadline.

The bootstrap source binds independently checked profile/artifact hashes,
one-use supervisor admission and endpoint lifetime; its actual native activation
and recovery remain unqualified. See [build/admission contract](../../native/rm2_3_28_0_172/BUILD.md).
The executor connects exact Task completion, GUI callback disarm, private Core
retirement, attribution cleanup and non-last owner consumption. The bootstrap
still disables mutation admission; source integration does not qualify a resident
lifecycle. Do not install this source as an automatic boot payload or use a
diagnostic one-shot fallback as product behavior.

Matching worker completion and zero active adapter hook stacks do not establish
safe last-owner destruction. The exact native Worker disposal can synchronously
wait on its worker thread when called on GUI; Document destruction has additional
command-six waits. The handoff leaves the complete GUI dependencies unresolved.
The successful historical release was one measured sequence, not a general
teardown guarantee. Reusable registration must also bind cancellation, snapshots
and retirement to the original generation, and establish that the original
once-submitted Task cannot enter again. Logical request expiry cannot substitute
for any of these physical lifetime facts.

The new non-last primitive refuses a last reference and bounds compare/exchange
contention to eight attempts. Its private profile gate validates exact control
identity and does not call native disposal. The resident executor invokes it only
after its staged physical-retirement predicates succeed.
The typed slot consumes each local reference exactly once and preserves
refused/partial/unknown ownership. Its physical-retirement method is private to
the resident executor; native quiescence remains separately unqualified.
Retaining a last owner can still exhaust the single mutation slot; normal native
capacity is not established by this primitive.

`resident_worker_slot.h` replaces the ordinary-release fallback for its own
acquisition path. It captures native outputs directly into process-retained
storage, admits only fully returned pairs and binds every operation to opaque
state identity. The resident executor owns a single slot for admitted requests.
The Coordinator now requires this slot for acquisition and probe submission,
retains its opaque token even when a native callout deletes the facade, and
binds Core registration to the stable slot. It no longer creates a per-request
Bridge/Prepared fallback. The executor performs the explicit physical-cleanup
sequence, retaining uncertain state and refusing later admissions. The endpoint
routes authenticated mutation commands to this pipeline, but the bootstrap's
native mutation gate remains disabled.

Core now pins a shared registration record at outer entry and binds cancel,
snapshot and private retirement to its opaque original token. A short registry
mutex protects registration lookup/removal; no native dispatch or record-carrier
destruction occurs under it. Cancel/snapshot also take that mutex; the inner-hook
implementation itself is unchanged and does not acquire it. Retirement rejects
active outer frames, but independently proving no future Task invocation remains
the executor's responsibility. An empty stack count is not that proof.

Attribution creation also uses the original slot token. Its private cleanup
clears the exact profile's dispatch addresses and frees adapter-only readers
before any owner reference is consumed. Failed attribution or uncertain native
submission quarantines all later slot admissions. Owned pipeline tests establish
the modeled ordering; actual native reusable capacity remains unqualified.

The Linux Rust client authenticates the endpoint's kernel peer and process start
identity through a retained activation-directory descriptor. It binds the socket
inode and scope, uses one absolute deadline, requires durable mutation reservation
before writing and retains ambiguous original requests without replay. The caller
must supply Supervisor-verified artifacts and runtime identity; echoed hashes do
not provide that verification. Consumer integration is tracked separately.

## Owned verification

The order-reader fixture reuses the original Qt document models and exercises
1–17 pages, malformed selections, duplicate/reordered IDs, inverse-map faults,
permit loss at every getter, reentrancy, wrapper deletion and foreign engines.
Host sanitizer and ARM/QEMU results are distinct from actual Reader evidence.
The backend has owned endpoint-to-executor and Coordinator-to-retirement coverage,
including real Qt callback delivery with modeled native callouts. The Rust client
also interoperates with the owned Qt endpoint. Actual bootstrap activation,
native dispatch, repeated mutation and product acceptance remain open in the
[active integration change](../../openspec/changes/integrate-resident-native-adapter/tasks.md).
