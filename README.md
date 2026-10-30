# ReMarkableOpenSDK

Dependency-free persistent identity primitives: canonical nonnil `Uuid`, document/page `PageKey` and `OperationId`. No native operations or supported firmware claim. GPL-3.0-only; see LICENSE.

This unpublished Rust crate requires Rust 1.88. Source manifests omit a maintained project version; Git tags are the release authority. Cargo development metadata is not release identity.

## Resident adapter integration

The opt-in `native-adapter` feature supplies bounded binary framing, runtime and
request correspondence, and historical Reader identity/order facts. It depends
on pinned SHA-256 and, on Linux, optional Nix socket/poll wrappers; default builds
remain dependency-free. Linux `native_adapter::transport::Client` connects through
the supervisor's retained activation-directory descriptor and authenticates the
kernel peer PID/UID, boot and process start time. Artifact verification and fresh
nonces remain supervisor responsibilities. Its bounded blocking IO belongs on a
consumer IO thread. Requests require an absolute deadline and a journal callback;
mutations must be durably recorded before that callback returns success.
This is an integration boundary under implementation; native qualification is open.
Enabling it does not activate xochitl or change `UnqualifiedPlatform` capabilities.

Native facts have no fabricated content revision or input epoch and cannot become
operational `PageObservation` guards. Insertion observations do not prove durable
commit, current target selection or writing authority. Persist the original
operation/source/target before dispatch. An uncertain mutation closes its session
and retains that uncertainty across cleanup; reconnect only to reconcile it.
Consumer/server integration must enforce durable deduplication across connections;
client sequence correlation alone does not implement that pending obligation.

Run `cargo test --locked --features native-adapter --test resident_protocol`.
The [active integration design](openspec/changes/integrate-resident-native-adapter/design.md)
tracks the endpoint, native lifetime, consumer and device acceptance still needed.

Run `cargo test --locked` and `cargo clippy --locked --all-targets -- -D warnings`. Existing RM1 baseline/input tools retain their separate qualifications.

Development helpers live in `scripts/`; release inputs live in `cicd/release/`.
See [release maintenance](cicd/release/README.md) for the small set of
SDK-specific checks retained alongside maintained GitHub Actions.

```rust
use remarkable_open_sdk::{OperationId, PageKey, Uuid};
let document = Uuid::parse("01234567-89ab-cdef-0123-456789abcdef").unwrap();
let page = Uuid::parse("11234567-89ab-cdef-0123-456789abcdef").unwrap();
let key = PageKey { document, page };
let operation = OperationId(page);
assert_eq!(key.document.to_string(), "01234567-89ab-cdef-0123-456789abcdef");
```

These values identify persistent records; they cannot prove the current tablet page, authorize creation, or reconcile native operations. Later observation/creation/capture contracts will build on these types in separate coherent deliveries.

## Ordinary gesture planning

`navigation::gesture::plan_gesture` returns the existing RM2 portrait path in Reader's 768 by 1024 virtual input space. Other pairs return typed unsupported results. This calculation emits no input and establishes no current-page identity or native completion.

```rust
use remarkable_open_sdk::navigation::{LogicalDirection, gesture::*};
let plan = plan_gesture(TabletModel::Remarkable2, Orientation::Portrait, LogicalDirection::Next).unwrap();
assert_eq!(plan.start(), (700, 512));
assert_eq!(plan.movements().last(), Some(&(100, 512)));
```

Reader retains model/orientation admission, input conversion, dispatch, release and observed completion. See THIRD_PARTY_NOTICES.md for adapted Buddy source provenance.

## Saved development capture evidence

Enable `development-capture` to validate already-collected v11 completion/PNG bytes with `development_capture::ReadOnlyDevelopmentCapture::from_collected_v11(&expected, completion, png)`. Supply an `ExpectedCaptureBinding` from the saved collection record, never a guessed current document. The result owns the original bytes and reports `DevelopmentUnqualifiedHistoricalCorrespondence`. Bounds, schema, expected bindings, digest and decoded image dimensions are checked before acceptance.

This validates past receipt correspondence. It neither acquires pixels nor establishes current page, pixel freshness, render ordering or native operation authority. Collection/restoration and consumer persistence remain caller responsibilities. Default builds have no external dependencies; optional pinned dependencies and complete original notices are in THIRD_PARTY_NOTICES.md.

Run `cargo test --locked --all-features --test development_capture`. The ignored private-fixture check is optional historical evidence and requires local files; no private files are shipped.

## Synthetic platform contract

`UnqualifiedPlatform` implements `Platform` with unsupported native operations. `capabilities()` reports availability only and never admits work. Enable `mock` for `mock::MockPlatform`, an in-memory scoped observation, creation/reconciliation and logical navigation/handoff fixture. Receipts are synthetic; they establish no native behavior or durable reconciliation.

```rust
use remarkable_open_sdk::{Platform, UnqualifiedPlatform, ObservationFailure, UnsupportedReason};
assert_eq!(UnqualifiedPlatform.observe_page(), Err(ObservationFailure::Unsupported(UnsupportedReason::NoNativeAdapter)));
```

Capture acquisition and synthetic image/facts APIs remain a separate delivery. Historical development capture validation retains its own optional boundary.

## Capture model and historical facts

`capture` supplies immutable image/geometry descriptors; `evidence` exports bounded one-way historical facts from model observations, receipts and batches. Original encoded media must be stored separately alongside exported descriptors. Exact integers, durations and geometry bits remain available without converting historical facts into live guards.

Enable `mock` for synthetic batch validation against parent-derived crop/resize pixels and explicit modeled owner/render assertions. These assertions establish model consistency only. `Platform::capture` defaults to unsupported; no native acquisition/freshness or current-page producer is supplied. Ordinary builds retain dependency-free defaults; image/hash libraries are selected for mock, development capture or tests. Reader's historical SDK_SOURCE remains its original provenance until coordinated consumer migration.
