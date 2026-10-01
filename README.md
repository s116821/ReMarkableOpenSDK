# ReMarkableOpenSDK

An independent, unofficial reMarkable hardware and native-runtime SDK, intended for ReMarkableBuddies and unrelated applications.

**Status: active research, contract design and an unpublished experimental Rust model. No native adapter, supported firmware matrix or shipping page-creation capability is implemented here yet.**

The first change is [establish-native-platform-contract](openspec/changes/establish-native-platform-contract/proposal.md). It targets a capability-oriented boundary for page identity, evidence-bound capture, navigation and safe native page creation. RM2/ARMv7 is the first qualification target. Paper Pro/AArch64 is an adapter target whose native qualification remains later work; cross-builds do not imply hardware support.

SDK specifications and workflow live in this repository. [ReMarkableBuddiesDocs](https://github.com/s116821/ReMarkableBuddiesDocs) owns Buddy and Manager product/integration requirements and references exact SDK contract revisions. SDK definitions are not duplicated there.

Buddy consumes the SDK at build time. Manager selects a compatible Buddy release artifact, without separately installing an SDK runtime. The SDK must remain independently consumable and mockable.

Prefer robust direct/native integration. Supervised, session-scoped lazy XOVI is an allowed candidate where necessary under the October 1 lifecycle refinement, not a selected or qualified production dependency. See the [mechanism comparison and acceptance gates](openspec/changes/establish-native-platform-contract/runtime-mechanism-comparison.md). Cold boot must remain stock, recovery must survive runtime/supervisor failure, and restart invalidates old source handles. Proprietary firmware binaries and reconstructed proprietary source must not be committed. Interface discovery is not proof of safe native operation.

Read [contributor guidance](CONTRIBUTING.md), the [workflow](openspec/README.md), and the active design before implementation.

The experimental Rust prototype models scoped page observations, creation/reconciliation outcomes and bounded synthetic PNG capture evidence. It pins image 0.25.10 (PNG only) and sha2 0.10.9; Cargo.lock records transitive dependencies. Its optional `mock` feature is synthetic; native defaults are unsupported. Run `cargo test --all-features` and `cargo clippy --all-targets --all-features -- -D warnings`. See [implementation limits](openspec/changes/establish-native-platform-contract/implementation.md) before using the prototype.
