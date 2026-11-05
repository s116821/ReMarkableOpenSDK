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

### Requirement: Documentation skips builds and corrected metadata resumes the core
Maintained conservative path/title admission SHALL occur once per applicable event, reject documentation misclassification and retain structural checks while skipping SDK compilation for docs-only changes. Edited PR metadata SHALL use the same title status and ordered core, permitting correction of an initially invalid title without another source push and never substituting skipped application checks for a previous failure.

#### Scenario: Documentation-only event
- **WHEN** only named documentation paths change
- **THEN** applicable title/structure validation remains available and Rust/application build and integration do not run.

#### Scenario: Corrected title on unchanged source
- **WHEN** a title initially fails and is then corrected on the same source head
- **THEN** the edited event executes the same title gate and full required core after success; body-only edits likewise run real application gates rather than replacing them with skipped status.

### Requirement: Open SDK candidates cannot restore obsolete CI layouts
Every active SDK PR SHALL be reconciled against the compact workflow contract through an accounted superseded closure or an owner-applied update, preserving unrelated source and recorded qualification limits.

#### Scenario: Older workflow candidate
- **WHEN** an older SDK PR contains obsolete workflow definitions
- **THEN** its exact source/disposition is recorded and it cannot normally merge those definitions over compact CI without reconciliation.
