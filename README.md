# ReMarkableOpenSDK

Dependency-free persistent identity primitives: canonical nonnil `Uuid`, document/page `PageKey` and `OperationId`. No native operations or supported firmware claim. GPL-3.0-only; see LICENSE.

This unpublished Rust crate requires Rust 1.88. Source manifests omit a maintained project version; Git tags are the release authority. Cargo development metadata is not release identity.

Run `cargo test --locked` and `cargo clippy --locked --all-targets -- -D warnings`. Existing RM1 baseline/input tools retain their separate qualifications.
