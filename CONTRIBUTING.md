# Contributing

This repository is currently a research/design bootstrap. Public GitHub discussion and the checked-in requirements are sufficient; Linear, Mem, Codex and private device exports are optional maintainer tools.

The experimental Rust model declares Rust 1.88 as its minimum; the exact source/lockfile now passes all-feature and no-default-feature suites on Windows Rust 1.88.0, alongside prior 1.98.1 verification. See [minimum-compiler evidence and limits](docs/reference/rust-minimum-verification.md). Direct dependencies are pinned to image 0.25.10 (default features disabled, PNG enabled) and sha2 0.10.9, both MIT OR Apache-2.0; Cargo.lock records the resolved graph. Run `cargo fmt --all -- --check`, `cargo test --all-features`, and `cargo clippy --all-targets --all-features -- -D warnings`. It is unpublished (`publish = false`) pending the full contract/native/technical release gates.

Use a topic branch and read AGENTS.md. The active SDK change records planned behavior, not an implemented baseline. No native capability is advertised before its acceptance evidence exists. Preserve unfinished work and use isolated checkouts.

The October 1 lifecycle refinement permits supervised, session-scoped lazy XOVI as a candidate; it supersedes earlier categorical bans without selecting a loader. Read the active runtime-mechanism comparison and qualify exact payload/runtime dependencies, stock cold boot, runtime and supervisor failure recovery, and fresh source scope after restart. A process boundary alone is not safety evidence. Product gestures and Supervisor orchestration belong in Buddy/Docs; device/runtime capability semantics belong here.

To check specification structure with the OpenSpec CLI:

```text
openspec validate --all --strict --no-interactive
```

The manual equivalent is to check every proposed requirement has an observable WHEN/THEN scenario, trace every task to the design, and report unresolved implementation/hardware gates. A structural validator does not qualify an adapter or prove interface safety.

Do not publish proprietary firmware, extracted QML/source bodies, credentials or personal documents. Prefer original implementations of independently observed interoperability contracts. Track source licenses and publication constraints before incorporating third-party code.

The maintainer selected GPL-3.0-only for the SDK on October 12, 2026. Retain
applicable original copyright/license notices and document adapted material in
THIRD_PARTY_NOTICES.md. Dependencies retain their own terms. Contributions to SDK
material use the declared GPL-3.0-only grant unless separately agreed and clearly
recorded; no upstream merge is required by this license. Do not relicense prior
art or another repository through an SDK contribution.

SDK changes that affect Buddy consumption need coordinated consumer integration changes in ReMarkableBuddiesDocs and ReMarkableBuddies. Manager installs Buddy artifacts; it must not infer compatibility from matching version numbers. Record exact revisions, evidence limits and merge order in PR comments. Keep a single unfinished delivery rather than marking an independent planning-only PR as completion of native page creation.

## Documentation

Use [the SDK knowledge index](docs/README.md) for reusable research, references and
experiment findings. New documentation belongs under `docs/`, except root project
guidance, conventional tool-discovery files and genuine OpenSpec workflow artifacts.
Change-specific plans/checkpoints remain in `openspec/`; link to the canonical
knowledge body and preserve evidence/provenance instead of copying it.

For native findings, update the [simulator evidence map](docs/reference/simulator-evidence-map.md)
with a pointer to the primary report and the affected model/test layer. Add a
focused regression when practical; otherwise record the explicit coverage gap
and owning follow-up rather than silently treating a Qt fixture as integrated
Buddy simulation. Keep native-observed, synthetic/owned and inferred behavior
separate. Refresh `python scripts/research_evidence_catalog.py --write` and verify
`--check` when Markdown knowledge/specification files change. Product simulator
requirements and implementation remain in the owning Buddy/Docs repositories.
