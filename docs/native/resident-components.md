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
resolves this directory and requires a caller-selected output directory. Other
imported mechanisms remain unchanged at this checkpoint. No proprietary firmware binary or extracted asset
is included.

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
The native profile's current one-shot
hook registration, attribution allocation and retained Coordinator owners must
be made safely reusable. Neither copying these mechanisms nor compiling them
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

## Owned verification

The order-reader fixture reuses the original Qt document models and exercises
1–17 pages, malformed selections, duplicate/reordered IDs, inverse-map faults,
permit loss at every getter, reentrancy, wrapper deletion and foreign engines.
Host sanitizer and ARM/QEMU results are distinct from actual Reader evidence.
The backend currently has production-path compile coverage; actual bootstrap,
native dispatch, repeated mutation and product acceptance remain open in the
[active integration change](../../openspec/changes/integrate-resident-native-adapter/tasks.md).
