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
| Current native source and actual page order under one guard | v11 checks a source tuple against expected fixture order; source-only helper reads actual mappings separately | A reviewed single-acquisition path must bind the actual mapping and pixels to the same retained owner and continuous invalidation interval |
| Runtime session, visit, input and content/order revision scope | Local entry epoch, process/root binding, selected observers and relative times | Define and qualify what changes invalidate each native scope; an equal local epoch is not a content revision or proof of complete input coverage |
| Logical rendered content matches the source | One window grab, copied pixels, matching identity checks, visual inspection | Demonstrate the vendor render-to-acquisition relationship; no existing native render generation is available to copy into synthetic generation fields |
| Image coordinates describe the page plane | Window dimensions and device-pixel ratio | Observe the page viewport/transform and valid page region, including chrome, zoom, scrolling and orientation; dividing full-window coordinates by width/height is insufficient |
| Clock and qualification provenance | Entry-relative elapsed measurements and exact development profile bindings | Define the actual clock origin/scope and native adapter/procedure qualification, without inventing cross-attempt comparability |
| Safe retained operation and recovery | Weak ownership, stack-aware cancellation and bounded development recovery evidence | Native lifecycle and future live-admission validity remain unqualified, including the unresolved shutdown fault |

The smallest source-order integration candidate would reuse the existing native
mapping read inside the retained capture lifetime and original deadline, while
leaving v11 unchanged. It is **not a mechanical call insertion**: the current
source-only mode has a different request purpose, terminal completion and exclusion
rules. A combined flow requires a reviewed lifecycle/purpose amendment and an
actual runtime result. The 771 ms remaining in one old capture does not establish
that added getters fit. No combined mode, new diagnostic module or speculative
native constructor is implemented by this assessment.

Even that combined read would address only source/order correspondence. It would
not establish the render, geometry, scope or recovery rows above, and could not
produce a qualified `CapturedBatch`. The
[accepted export contract](../../openspec/changes/establish-native-platform-contract/evidence-export-contract.md)
already requires separate review of native origin/profile/clock/ownership
semantics. Extend those actual semantics when evidenced; do not change synthetic
labels or use caller assertions to select a qualified origin. Main owns the
sealed `ReaderSourceAdmission` consumer binding and validity checks for each
selected effect; SDK owns the qualified producer.

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
