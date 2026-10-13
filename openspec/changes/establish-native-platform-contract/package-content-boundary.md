# Rust source-package content boundary

October 12, 2026. Root selected this containment fix after the
[distribution inventory](../../../docs/reference/distribution-license-gaps.md)
found untracked historical GNU tools in Cargo's prospective file list.
No archive/build/publication or held native operation is selected.

Trace: Cargo's default library entry is `src/lib.rs`; its modules are capabilities,
capture, development_capture (optional), evidence, mock (optional/test), navigation
and navigation/gesture. There is no build.rs, native-link build dependency,
out-of-tree include macro, or runtime source/asset file needed by these modules.
Development capture accepts caller-supplied bytes; its private test files are not
runtime assets. Registry dependencies remain separately locked.

Add an explicit Cargo `include` list for Cargo.toml/Cargo.lock/README.md and those
eight Rust files. Cargo-generated Cargo.toml.orig/.cargo_vcs_info.json are permitted
metadata. Future source modules/assets or approved LICENSE/NOTICE files require a
reviewed list update. `publish = false` and all APIs/features stay unchanged.

Use focused Python unittest cases invoking only `cargo package --list --offline
--allow-dirty`: actual checkout membership; isolated source-tree decoys omitted;
and the same decoys selected without the allowlist to demonstrate the regression.
No archive is generated and no native or Rust payload is compiled by these tests.
Hash the four pre-existing untracked historical files before/after; leave them
untouched. Do not copy them into the isolated fixture: use original harmless text.

REM-50 owns eventual packaging and workflow integration. The proposed contents
are sent through Root to its existing owner before freeze. This is containment,
not selection of a distributable licensed SDK, and does not close task 1.4 or 3.5.

## Acceptance

- [x] Explicit allowlist and both focused package-list regressions pass on Windows Cargo 1.98.1: actual 13 members/zero tools, and isolated decoys excluded with the list but present without it. Initial path-comparison failure was corrected to normalize Windows separators; no member rule was weakened.
- [x] All four historical untracked SHA-256 values remain unchanged; no archive or build was performed.
- [x] Existing Sol independently reviewed the source trace/allowlist and reran both list-only regressions: PASS. [REM-50 owner confirms no scope conflict](https://github.com/s116821/ReMarkableOpenSDK/pull/1#issuecomment-6127144709); Root accepted the refinement from src/** to exact eight files.

The [REM-50 integration note](https://github.com/s116821/ReMarkableOpenSDK/pull/1#issuecomment-6127131882)
identifies its current whole-tracked-tree verifier mismatch. REM-50 must separate
build-source identity from selected crate contents; do not widen this SDK list to
fit that unfinished verifier. Its frozen-snapshot consumer CI is historical
qualification and must transition to the actual candidate before claiming
current-source/shipping coverage. License/notice file inclusion and publication
approval remain explicit release prerequisites.

The subsequent [authorized licensing increment](licensing-decision.md) adds the
selected LICENSE and THIRD_PARTY_NOTICES.md explicitly: current membership is
13 selected files plus two generated metadata files. The earlier 13-member result
above describes the pre-license containment checkpoint, not the current list.
License selection is resolved; REM-50 technical release integration remains open.
