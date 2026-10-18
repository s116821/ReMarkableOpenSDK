## ADDED Requirements

### Requirement: Compact ordered stages own each SDK validation once
SDK CI SHALL provide PR Title before Checks, then Build and artifact-consuming Integration, assigning each validation one explicit stage/platform owner without separate candidate-version, uploader or result-wrapper jobs.

#### Scenario: Application PR or main change
- **WHEN** an application change is admitted
- **THEN** required platform checks gate Build and Build gates Integration, preserving MSRV, feature-disabled consumer, package, semantic and RM1 fixture coverage without duplicate unit-suite execution.

### Requirement: Real Windows portability remains explicit
Compact SDK CI SHALL retain source/admission staging controls on real Windows, report the extra required platform job honestly, and propagate failures before Build.

#### Scenario: Portable staging failure
- **WHEN** Linux or Windows required staging/check controls fail, cancel or do not complete successfully
- **THEN** later application build/integration cannot proceed and platform coverage is not silently dropped.

### Requirement: Documentation and edited metadata avoid application builds
Maintained conservative path/title admission SHALL occur once per applicable event, reject documentation misclassification and retain structural checks while skipping SDK compilation for docs-only changes; metadata-only edits SHALL revalidate title without replacing failed application-check contexts or restarting application builds.

#### Scenario: Documentation or metadata-only event
- **WHEN** only named docs change or PR metadata is edited
- **THEN** applicable title/structure validation remains available and unnecessary Rust/application build and integration jobs do not run.

### Requirement: Open SDK candidates cannot restore obsolete CI layouts
Every active SDK PR SHALL be reconciled against the compact workflow contract through an accounted superseded closure or an owner-applied update, preserving unrelated source and recorded qualification limits.

#### Scenario: Older workflow candidate
- **WHEN** an older SDK PR contains obsolete workflow definitions
- **THEN** its exact source/disposition is recorded and it cannot normally merge those definitions over compact CI without reconciliation.
