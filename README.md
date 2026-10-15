# ReMarkableOpenSDK

Dependency-free persistent identity primitives: canonical nonnil `Uuid`, document/page `PageKey` and `OperationId`. No native operations or supported firmware claim. GPL-3.0-only; see LICENSE.

This unpublished Rust crate requires Rust 1.88. Source manifests omit a maintained project version; Git tags are the release authority. Cargo development metadata is not release identity.

Run `cargo test --locked` and `cargo clippy --locked --all-targets -- -D warnings`. Existing RM1 baseline/input tools retain their separate qualifications.

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
