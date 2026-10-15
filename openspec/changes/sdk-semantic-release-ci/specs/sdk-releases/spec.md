## ADDED Requirements

### Requirement: Semantic Git tags are version authority
SDK release automation SHALL use maintained upstream tooling to derive SemVer from enforced PR-title-derived main squash messages. A read-only upstream candidate version MAY be resolved before building the exact intended main source. Official tags and releases SHALL be created only by one final release job after all required checks, builds and integration of those artifacts succeed; the final tag/version/source and published bytes SHALL match those qualified inputs. This October 15 REM-56 order supersedes the prior tag-before-build plan. docs: merges SHALL not bump/tag/release or compile application code on main. Application code mislabeled docs: SHALL be refused by upstream classification/title checks.

#### Scenario: Feature and fix history
- **WHEN** eligible feature/fix merges follow prior immutable tags, including equal commit timestamps
- **THEN** upstream tooling chooses the corresponding minor/patch version without changing older tag/source identities.

#### Scenario: Documentation only
- **WHEN** a docs: merge contains only documentation
- **THEN** no SDK tag/release/application build occurs.

#### Scenario: Failure or concurrent advancement
- **WHEN** build/publication fails after tagging or main advances
- **THEN** automation refuses wrong-source/version publication; failed, skipped or canceled required predecessors create no official tag/release, while recovery after an interrupted final publication preserves the intended immutable tag/assets without inventing duplicate versions.

#### Scenario: Failed push retains local tag
- **WHEN** upstream tagging leaves a local tag after a refused or uncertain remote push
- **THEN** official publication remains refused until the exact remote tag is observed and its fetched commit equals the intended source; local tag existence or a no-release retry is insufficient.

### Requirement: Package and runtime versions are generated during builds
No project version field or placeholder SHALL be maintained in source. Packaging and runtime versions SHALL be generated only in build output from the upstream read-only candidate, whose exact value and source SHALL be bound to the official Git tag only after qualification succeeds. They SHALL NOT be committed as source versions.

#### Scenario: Official package staging
- **WHEN** a build stages the exact intended source using the read-only upstream candidate before official tagging
- **THEN** package/runtime identity is generated from that candidate in build output and final official tag binding is separately required without editing the source checkout or creating a version-bump commit.

### Requirement: Existing-tag draft recovery verifies complete byte identity
Publication SHALL use maintained tooling for existing-tag drafts and SHALL verify retained assets against independently verified build identity before uploading missing names. Finalization SHALL require the complete declared manifest/distributions and exact remote source identity. Recovery SHALL NOT retag, clobber retained bytes or treat a functional fixture as a security-gate waiver.

#### Scenario: Interrupted draft upload
- **WHEN** an upload fails after some verified assets reached an existing-tag draft
- **THEN** recovery retains those bytes, verifies the subset, uploads missing declared names and checks completeness before finalization without minting another version.

#### Scenario: Unresolved publisher advisory
- **WHEN** the declared publisher binary security gate fails despite passing functional or restricted source-reachability checks
- **THEN** production selection remains refused until supported remediation or an explicitly reviewed policy change resolves the gate; its failure is not silently suppressed.

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

### Requirement: Ordinary consumer feature isolation is checked separately
Research CI SHALL verify an exactly identified SDK checkout as an ordinary
dependency without default SDK features or SDK dev dependencies. It SHALL run
the positive consumer and separately require forbidden mock and development
imports to fail with their exact unresolved-import diagnostic and compiler exit.
The checker SHALL use committed registry pins locked/offline after explicit
prefetch. These host checks SHALL NOT imply shipping or native qualification.

#### Scenario: Frozen research consumer boundary
- **WHEN** an application PR invokes the consumer job before research source merges to main
- **THEN** CI checks out and verifies the existing immutable accepted snapshot, runs the ordinary consumer on the declared minimum and maintained toolchains, and retains older compiler and release gates without copying broad research work into the release branch.

### Requirement: Declared crate contents retain exact source identity
Packaging SHALL distinguish all tagged staging inputs from the explicitly
reviewed crate inventory. It SHALL verify every staged source input unchanged
and require selected crate inputs and Cargo metadata/dependency lock to match
that exact source. Missing, changed and undeclared archive inputs SHALL refuse.
The first integration SHALL refuse unsupported package patterns rather than
reimplement Cargo selection. Native and licensing gates SHALL remain separate.

#### Scenario: Research sources are not crate inputs
- **WHEN** the exact tagged tree contains tracked research files excluded by its reviewed literal package inventory
- **THEN** crate verification accepts the selected exact source bytes without including those research files, while any staged-source drift or unexpected archived research/tool input refuses.

### Requirement: Incoming SDK changes use the current candidate
Consumer CI SHALL test the actual PR merge candidate when SDK sources exist
there and SHALL fail if its required checkers or fixtures are missing. An SDK-free
release branch MAY explicitly qualify a named immutable historical snapshot,
but SHALL NOT describe that success as current-source or shipping integration.

#### Scenario: SDK source reaches the candidate branch
- **WHEN** the candidate contains the integrated SDK crate
- **THEN** CI verifies that candidate's exact event SHA and runs the ordinary consumer and package-list check there without retaining a frozen-source substitute.

### Requirement: Reviewed license inputs retain package identity
The SDK package SHALL retain its reviewed grant and selected original notices.
Artifact verification SHALL match normalized license/license-file/publication
metadata to the exact staged source, and SHALL require the selected license and
notice file bytes. Historical research verification SHALL NOT change the chosen
grant or imply official publication.

#### Scenario: License or notice drift in a crate
- **WHEN** a crate omits or changes selected license/notices, or relabels normalized license/publication metadata while its original manifest remains correct
- **THEN** verification refuses that artifact and retains the reviewed source contract.

### Requirement: Generated Cargo defaults do not mint release identity
Source package/workspace release-version declarations SHALL be refused. A
unique generated unsourced own-package Cargo lock node at implicit0.0.0 MAY
retain dependency reproducibility without becoming release authority. Staged
package and own lock identity SHALL derive from the verified Git tag, with every
external locked dependency unchanged and no source update.

#### Scenario: Locked unversioned source stages from a tag
- **WHEN** a clean unversioned tagged source has only Cargo's generated own-root default lock metadata
- **THEN** upstream Cargo regenerates the own node in the new tagged build copy, external pins stay unchanged, and altered or ambiguous own metadata refuses.
