# Contributing

This repository is currently a research/design bootstrap. Public GitHub discussion and the checked-in requirements are sufficient; Linear, Mem, Codex and private device exports are optional maintainer tools.

The experimental Rust model declares Rust 1.88 as its minimum; verification currently used Rust 1.98.1, so the minimum itself is not tested. Direct dependencies are pinned to image 0.25.10 (default features disabled, PNG enabled) and sha2 0.10.9, both MIT OR Apache-2.0; Cargo.lock records the resolved graph. Run `cargo fmt --all -- --check`, `cargo test --all-features`, and `cargo clippy --all-targets --all-features -- -D warnings`. It is unpublished (`publish = false`) pending the full contract/native/license gates.

Use a topic branch and read AGENTS.md. The active SDK change records planned behavior, not an implemented baseline. No native capability is advertised before its acceptance evidence exists. Preserve unfinished work and use isolated checkouts.

To check specification structure with the OpenSpec CLI:

```text
openspec validate --all --strict --no-interactive
```

The manual equivalent is to check every proposed requirement has an observable WHEN/THEN scenario, trace every task to the design, and report unresolved implementation/hardware gates. A structural validator does not qualify an adapter or prove interface safety.

Do not publish proprietary firmware, extracted QML/source bodies, credentials or personal documents. Prefer original implementations of independently observed interoperability contracts. Track source licenses and publication constraints before incorporating third-party code. The first implementation must settle the SDK license and dependency policy with the maintainer; this bootstrap does not relicense any prior-art source.

SDK changes that affect Buddy consumption need coordinated consumer integration changes in ReMarkableBuddiesDocs and ReMarkableBuddies. Manager installs Buddy artifacts; it must not infer compatibility from matching version numbers. Record exact revisions, evidence limits and merge order in PR comments. Keep a single unfinished delivery rather than marking an independent planning-only PR as completion of native page creation.
