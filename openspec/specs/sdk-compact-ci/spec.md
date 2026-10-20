# Compact SDK CI

## Purpose
Assign SDK checks once to compact ordered stages, preserve real platform qualification and integrate exact retained candidate bytes without activating unqualified publication.

## Requirements

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
Maintained conservative path/title admission SHALL occur once per applicable event, reject documentation misclassification and retain structural checks while skipping SDK compilation for docs-only changes. Edited PR metadata SHALL use the same maintained title status. Corrected titles SHALL resume the required core without another source push. Only positively identified exact body-only changes SHALL omit core work, preserving the original failed application check authority; base, mixed, malformed and unknown changes SHALL retain normal validation.

#### Scenario: Documentation-only event
- **WHEN** only named documentation paths change
- **THEN** applicable title/structure validation remains available and Rust/application build and integration do not run.

#### Scenario: Corrected title on unchanged source
- **WHEN** a title initially fails and is then corrected on the same source head
- **THEN** the edited event executes the same title gate and full required core after success, with normal executed context names.

#### Scenario: Body edit after actual core failure
- **WHEN** only exact known changes.body.from string metadata changes on a head with a witnessed failed application check
- **THEN** maintained title validation runs, core work is omitted under distinct actual contexts and the original normal failed check ID/conclusion is not replaced or masked.

#### Scenario: Base or unrecognized edit
- **WHEN** an edit retargets the base, combines body/title/base changes or has missing, malformed, empty or unknown change details
- **THEN** normal validation runs because unchanged head alone does not establish an unchanged merge context.

### Requirement: Open SDK candidates cannot restore obsolete CI layouts
Every active SDK PR SHALL be reconciled against the compact workflow contract through an accounted superseded closure or an owner-applied update, preserving unrelated source and recorded qualification limits.

#### Scenario: Older workflow candidate
- **WHEN** an older SDK PR contains obsolete workflow definitions
- **THEN** its exact source/disposition is recorded and it cannot normally merge those definitions over compact CI without reconciliation.

### Requirement: Flat SDK metadata isolation preserves stage custody
The existing five flat SDK core job contexts SHALL be distinct when omitted for body-only metadata without adding stages or changing normal executed names, required platform/artifact predecessors, cancellation guards, main skipped-title behavior or publication permissions. Actual skipped expression labels SHALL be recorded honestly rather than claimed to render friendly names.

#### Scenario: Exact omitted SDK contexts
- **WHEN** a positively identified body-only event omits Linux Checks, Windows staging, Build, Integration and Release
- **THEN** none creates or replaces its normal application context, the nested title context remains maintained, and actual skipped context names/IDs are preserved as evidence.

#### Scenario: Ordinary source or main event
- **WHEN** normal source validation or an eligible main merge is evaluated
- **THEN** original executed names/stage counts, real Windows coverage, explicit required-success/cancellation guards, retained artifact custody and main-only final release remain unchanged despite the intentionally skipped main title.
