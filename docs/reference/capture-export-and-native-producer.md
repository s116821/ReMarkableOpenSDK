# Capture export and the missing native producer

October 12, 2026. Exact Rust API assessment at
[`d01b4bdfbc4dc11673f84ff82bbc62c8dc77bbb1`](https://github.com/s116821/ReMarkableOpenSDK/tree/d01b4bdfbc4dc11673f84ff82bbc62c8dc77bbb1).
Native development implementation assessed at `59556462a3be479ee3190029c63884257e065137`.
No native capability is qualified by this source reconciliation.

## The immutable data export exists

[`CapturedBatch::export_facts(ExportLimits)`](https://github.com/s116821/ReMarkableOpenSDK/blob/d01b4bdfbc4dc11673f84ff82bbc62c8dc77bbb1/src/evidence.rs#L348)
returns immutable `CaptureFacts`. This is the existing SDK-owned data boundary;
another registry, capture DTO or consumer facade is unnecessary.

| Existing type/accessor | Original values retained |
| --- | --- |
| `CaptureFacts::source()` / `ObservationFacts` | Device UUID, adapter instance, session, visit, content revision, input epoch, document/page key, complete ordered page IDs and origin |
| `CaptureFacts` | Operation, interval, render/buffer generations, viewport/conversion/procedure revisions and optional target bits |
| `native_parent()` / ordered `images()` / `ImageFacts` | Role, PNG MIME, byte length, digest, dimensions, exact affine and valid-region bits |
| `ImageFacts::derivation()` / `DerivationFacts` | Parent digest/dimensions, crop, output dimensions, resize filter and procedure |
| `DurationFacts` | Exact u64 seconds and u32 nanoseconds |

Facts contain descriptors, not image media. Preserve the original
[`batch.native_parent().bytes()` and each `batch.images()[i].bytes()`](https://github.com/s116821/ReMarkableOpenSDK/blob/d01b4bdfbc4dc11673f84ff82bbc62c8dc77bbb1/src/capture.rs#L388)
unchanged alongside their descriptors and ordering. Export checks variable-data
bounds before cloning; these are not consumer transaction or total-memory limits.
Facts cannot reconstruct an operational observation, request, batch or guard.

At this revision the only `EvidenceOrigin` is `Synthetic`. Capture qualification
is `UnqualifiedSyntheticModel`, clock interpretation is
`SyntheticInjectedDuration`, and render binding is `SyntheticEqualityAssertion`.
The existing batch producer is test/mock-gated `capture::synthetic::validate_batch`.
`Platform::capture` defaults to unsupported, and `UnqualifiedPlatform` supplies no
native observation or acquisition. `NativeParent` names an image role; it does
not establish native qualification. Therefore the answer to "does qualified live
capture export exist?" is **no**, despite the complete historical data shape.

## Smallest consumer mapping

Buddy already implements
[`HistoricalCapture::from_batch`](https://github.com/s116821/ReMarkableBuddies/blob/62a5376f3e14d06a5666c61b96dc6ad1bd35aa87/src/conversation/capture_facts.rs)
using the existing historical observation/image/derivation types and `ExactU64`.
Its [`Ledger::prepare_sdk_fixture`](https://github.com/s116821/ReMarkableBuddies/blob/62a5376f3e14d06a5666c61b96dc6ad1bd35aa87/src/conversation/mod.rs)
demonstrates staging original bytes and descriptors with the turn. That entry is
explicitly fixture-only; it is not a production selected-publication authority.
Main owns reuse of that mapping inside the selected request/draft transaction.

Keep complete parent/derivative media, image order, UUIDs, duration components and
exact integer/geometry bits. Restore remains historical. The committed mapper's
`SDK_SOURCE` is `2d473f0954120889f8a04c6294151241576ff8c3`, not d01. Review the
existing schema/source provenance policy during reuse; do not silently relabel
old records or equate current dependency identity with historical evidence origin.
No pin or protected consumer file was changed for this assessment.

## What the actual native producer currently supplies

The existing
[`FactsEntry` capture path](https://github.com/s116821/ReMarkableOpenSDK/blob/59556462a3be479ee3190029c63884257e065137/tools/qt_page_facts_entry.h)
discovers a receiver/scene/document, installs sticky observers, checks the small
document/index/page/alias tuple, performs one `grabWindow`, deep-copies the pixels,
and rechecks identity/epoch before bounded PNG/completion publication. Its v11
output explicitly sets native/render/atomic/UI/order authority false.

The separately selected receiver-source-facts path reuses
[`PageFactsSession`](https://github.com/s116821/ReMarkableOpenSDK/blob/59556462a3be479ee3190029c63884257e065137/tools/qt_page_facts.h)
to read page count and forward/reverse native mappings. It branches before capture
and finishes the entry. Its purpose guard explicitly excludes capture requests
and pixel/completion files. Joining those separate records later would not produce
a map bound to one capture interval, owner or runtime visit.

The [source5 contract](https://github.com/s116821/ReMarkableBuddiesDocs/blob/024a4db4992e461f71c7f82a098145702aa465e3/openspec/changes/qualify-open-pdf-capture/source5-stable-source-witness-contract.md)
defines a separate operator-conjunction policy, not a native capture
adapter. The [earlier 301a capture result](https://github.com/s116821/ReMarkableBuddiesDocs/blob/01ca4d3932e37ad7f1462f10fb756e692e2e3314/openspec/changes/durable-conversation-context/reader-persistence-seam.md)
records a visually inspected 1404x1872 PNG in
4229 ms under its five-second bound and persisted the original plus derivatives
through the Reader. That result remains collected historical input with all
authority flags false. Its subsequent source-facts trial 0404 did not qualify
the combined source/capture path or shutdown lifecycle. See the existing
[shutdown evidence](../research/shutdown-entrypoint-comparison.md).

## Required facts and precise source integration boundary

| Qualified contract requirement | Existing evidence | Missing producer work |
| --- | --- | --- |
| Current native source and actual page order under one guard | v11 checks a source tuple against expected fixture order; source-only helper reads actual mappings separately | The opt-in source amendment binds actual mapping and pixels within one guarded attempt in owned fixtures; source review and native validation remain required |
| Runtime session, visit, input and content/order revision scope | Local entry epoch, process/root binding, selected observers and relative times | Define and qualify what changes invalidate each native scope; an equal local epoch is not a content revision or proof of complete input coverage |
| Logical rendered content matches the source | One window grab, copied pixels, matching identity checks, visual inspection | Demonstrate the vendor render-to-acquisition relationship; no existing native render generation is available to copy into synthetic generation fields |
| Image coordinates describe the page plane | Window dimensions and device-pixel ratio | Observe the page viewport/transform and valid page region, including chrome, zoom, scrolling and orientation; dividing full-window coordinates by width/height is insufficient |
| Clock and qualification provenance | Entry-relative elapsed measurements and exact development profile bindings | Define the actual clock origin/scope and native adapter/procedure qualification, without inventing cross-attempt comparability |
| Safe retained operation and recovery | Weak ownership, stack-aware cancellation and bounded development recovery evidence | Native lifecycle and future live-admission validity remain unqualified, including the unresolved shutdown fault |

The source amendment in SDK `29bcfaaf25f21e5c8f30d295f8e943c305f30f19`
now implements that narrow correspondence candidate. Its default-off six-page
`developmentCaptureOrder` mode reads actual forward/reverse mapping at the
existing capture checkpoints under the original owner/epoch/deadline, uses a
separate token purpose, and emits version 3 with false native/render/atomic/UI
authority. It preserves v11 and source-only contracts. See the
[capture-order amendment and verification](../../openspec/changes/establish-native-platform-contract/capture-order-correspondence.md).
Twenty focused owned ARM-emulated cases passed, independent source review accepted
the candidate, and both available exact-source GitHub checks passed. Native
lifecycle qualification remains open. The 771 ms remaining in one old
capture is not a future timing guarantee, and checks around native calls do not
prove a hard call execution bound. No native startup configuration or activation
is included.
The [provenance amendment](../../openspec/changes/establish-native-platform-contract/capture-order-provenance.md)
subsequently advances completion to version 4 with explicit actual acquisition,
pixel ownership, attempt-local clock, unverified render and full-window coordinate
semantics. It adds no qualified native facts or consumer constructor.

Even that combined read would address only source/order correspondence. It would
not establish the render, geometry, scope or recovery rows above, and could not
produce a qualified `CapturedBatch`. The
[accepted export contract](../../openspec/changes/establish-native-platform-contract/evidence-export-contract.md)
already requires separate review of native origin/profile/clock/ownership
semantics. Extend those actual semantics when evidenced; do not change synthetic
labels or use caller assertions to select a qualified origin. Main owns the
sealed `ReaderSourceAdmission` consumer binding and validity checks for each
selected effect; SDK owns the qualified producer.

The subsequent [scene-tile investigation](../research/scene-tile-render-geometry.md)
identifies concrete manager/request/viewport relationships and a proposed
correspondence/geometry discriminator. Initial readiness and pending=false are
insufficient: the former disconnects its initial observer and cancellation can
clear the latter. Native transforms include rounded translation and still need
viewport-to-window placement, clipping and a justified page domain. These static
findings add no qualified render facts to the producer.

The current selected route needs an executing native candidate to measure these
facts. Held8112 or equivalent activation is excluded, and spent0404 cannot be
replayed. Any necessary access review stays with Main and the original platform
path. This identifies a dependency of the selected implementation, not a claim
that all native methods require that operation. Existing protected Buddy WIP
inspected here adds gesture/orientation handling, not the missing capture facts;
it was neither edited nor treated as a qualified adapter.

Source basis: exact SDK Git objects and accepted contracts; current native entry
and mapping-reader source; committed Buddy mapping and read-only protected WIP
diff; earlier published operator/Reader results and current Main coordination.
Runtime findings are attributed historical evidence, not rerun tests. The minimal
integration candidate and remaining dependencies are engineering assessment.
