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
Other imported mechanisms remain unchanged at this checkpoint. No proprietary
firmware binary or extracted asset is included.

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
native QVariant or guard escapes the synchronous callback. Mutation returns
Unsupported until physical completion and owner reclamation are integrated.

The bootstrap source binds independently checked profile/artifact hashes,
one-use supervisor admission and endpoint lifetime; its actual native activation
and recovery remain unqualified. See [build/admission contract](../../native/rm2_3_28_0_172/BUILD.md).
The native profile's attribution allocation and retained Coordinator owners must
still be made safely reusable. Core record retirement is private and has no
production caller. Neither copying these mechanisms nor compiling them
qualifies a resident lifecycle. Do not install this source as an automatic boot
payload or use a diagnostic one-shot fallback as product behavior.

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
identity and does not call native disposal. No production owner invokes it yet.
The typed slot consumes each local reference exactly once and preserves
refused/partial/unknown ownership. Its physical-retirement method is private and
has no production executor caller yet; native quiescence remains separate.
Retaining a last owner can still exhaust the single mutation slot; normal native
capacity is not established by this primitive.

`resident_worker_slot.h` replaces the ordinary-release fallback for its own
acquisition path. It captures native outputs directly into process-retained
storage, admits only fully returned pairs and binds every operation to opaque
state identity. It is compiled with the bootstrap but is not instantiated by it.
The imported research Coordinator still uses its original one-shot Bridge path;
that path is not a production mutation endpoint. Connecting the slot to reusable
Core/Coordinator registration and exact Task retirement remains open.

Core now pins a shared registration record at outer entry and binds cancel,
snapshot and private retirement to its opaque original token. A short registry
mutex protects registration lookup/removal; no native dispatch or record-carrier
destruction occurs under it. Cancel/snapshot also take that mutex; the inner-hook
implementation itself is unchanged and does not acquire it. Retirement rejects
active outer frames, but independently proving no future Task invocation remains
the executor's responsibility. An empty stack count is not that proof.

## Owned verification

The order-reader fixture reuses the original Qt document models and exercises
1–17 pages, malformed selections, duplicate/reordered IDs, inverse-map faults,
permit loss at every getter, reentrancy, wrapper deletion and foreign engines.
Host sanitizer and ARM/QEMU results are distinct from actual Reader evidence.
The backend currently has production-path compile coverage; actual bootstrap,
native dispatch, repeated mutation and product acceptance remain open in the
[active integration change](../../openspec/changes/integrate-resident-native-adapter/tasks.md).
