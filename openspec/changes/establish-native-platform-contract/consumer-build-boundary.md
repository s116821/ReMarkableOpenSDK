# Ordinary consumer build boundary

October 12, 2026. Main assessed this SDK-only increment before implementation.
The existing Buddy Git dependency is already build-time consumption; this change
does not upgrade it, add a native adapter, or finish task 3.4.

## Design

Use a separate standalone Cargo package in `tests/fixtures/consumer-boundary`,
outside the SDK workspace and unit-test/dev-dependency graph. Its sole dependency
is this SDK by relative path with `default-features = false`; both packages retain
exact lockfiles. An ordinary executable checks existing UUID and unsupported
capability/observation APIs without device, filesystem, network or native actions.
Cargo metadata must confirm the SDK dependency resolves without enabled features.

Two separate compile checks intentionally import `mock` and `development_capture`.
Require compiler error E0432 for the expected unavailable SDK module; any unrelated
compiler failure, successful import or dependency resolution failure fails the
gate. Run a positive ordinary build/execution first. The fixture is not a new
synthetic policy model and cannot construct operational observations.

REM-50 / SCRAPPY owns existing [SDK PR 3](https://github.com/s116821/ReMarkableOpenSDK/pull/3)
at `7fb824e8a218b7beabb138f3aa38df2c5bcdc930`, including host/MSRV/ARMv7/AArch64
checks and release qualification. Its `release/test_cargo_consumer.py` uses an
owned minimal SDK to verify Git/tag/patch identity; it does not test this actual
SDK's feature-disabled public surface. This increment supplies only the missing
ordinary consumer fixture and callable checker. No CI/release workflow, version
engine, consumer pin, repository setting or native tool changes are included.
The REM-50 owner can invoke the checker from the existing workflow after merging
the actual source. Test locally on Rust 1.88.0 and the current host toolchain,
using locked offline resolution; report hosted integration separately.

## Acceptance

- [x] Standalone ordinary consumer builds/runs and resolves no SDK features.
- [x] Both forbidden imports fail with the expected unavailable-module diagnostic.
- [x] Windows MSVC Rust 1.88.0 and 1.98.1 checks pass; REM-50 hosted integration remains separate. See [actual scope and commands](../../../docs/reference/consumer-build-boundary.md).
- [x] Existing Sol reviewer independently read the source/docs and reran the checker on installed Rust 1.88.0: PASS, no blocking finding. No consumer pin/API change.

The broad active change remains unfinished: native qualification, integration,
license/publication, canonical sync and archive gates remain open. The held 8112
operation and all native development sources are outside this increment.
