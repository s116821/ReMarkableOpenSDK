## ADDED Requirements

### Requirement: Semantic Git tags are version authority
SDK release automation SHALL use maintained upstream tooling to derive SemVer from enforced PR-title-derived main squash messages. Official artifacts SHALL be built after immutable tag creation from its exact SHA with matching version/provenance. No project version field or placeholder SHALL be maintained in source; packaging/runtime versions SHALL be generated only at build time from the checked-out tag and SHALL NOT be committed. docs: merges SHALL not bump/tag/release or compile application code on main. Application code mislabeled docs: SHALL be refused by upstream classification/title checks.

#### Scenario: Feature and fix history
- **WHEN** eligible feature/fix merges follow prior immutable tags, including equal commit timestamps
- **THEN** upstream tooling chooses the corresponding minor/patch version without changing older tag/source identities.

#### Scenario: Documentation only
- **WHEN** a docs: merge contains only documentation
- **THEN** no SDK tag/release/application build occurs.

#### Scenario: Failure or concurrent advancement
- **WHEN** build/publication fails after tagging or main advances
- **THEN** automation refuses wrong-source/version publication and recovers the intended immutable tag/assets without inventing duplicate versions.

#### Scenario: Failed push retains local tag
- **WHEN** upstream tagging leaves a local tag after a refused or uncertain remote push
- **THEN** builds and publication remain refused until the exact remote tag is observed and its fetched commit equals the intended source; local tag existence or a no-release retry is insufficient.

### Requirement: Explicit distribution and consumer identity
Each SDK release SHALL include all declared implemented distributions with source/contract/target/compatibility/hash provenance. Architectural compilation SHALL not imply native model support. Buddy SHALL consume a pinned SDK version at build time and bundle required target helpers; development MAY use local Cargo overrides, but official builds SHALL refuse local-path resolution. Unknown model/firmware/runtime combinations SHALL remain unsupported.

#### Scenario: Local consumer iteration
- **WHEN** a developer applies a local SDK path/[patch] override
- **THEN** local end-to-end builds use that source while official CI refuses developer-path dependencies.

#### Scenario: Unqualified target
- **WHEN** a built target lacks model/runtime evidence
- **THEN** its manifest reports that native support is unqualified and installation cannot infer compatibility merely from CPU architecture.

### Requirement: Actual source compiler checks retain qualification boundaries
CI SHALL check exact identified experimental SDK source on host/ARMv7/AArch64
without treating compilation as native model support or distribution readiness.
Read-only research snapshots SHALL retain source identity and owner source bytes;
shipping builds SHALL separately require integrated source and tag-derived staging.

#### Scenario: Experimental source check
- **WHEN** CI compiles an exact experimental source snapshot
- **THEN** it records source/toolchain/target identity, runs applicable host checks, and makes no native capability, official version or release claim.
