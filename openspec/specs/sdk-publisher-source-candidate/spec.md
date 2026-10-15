# sdk-publisher-source-candidate Specification

## Purpose
TBD - created by archiving change sdk-publisher-source-candidate. Update Purpose after archive.
## Selection status
This records the original controlled candidate qualification, retained as historical evidence. REM-56's [ordered replacement candidate](../sdk-gated-release-pipeline/spec.md) supersedes its release orchestration/selection; old selector, tag-before-build and publisher mechanisms are not selected active production behavior. Original requirements/results and tool failures remain preserved. This status neither archives unfinished foundation/source-main/REM-50 work nor qualifies a first official release, remote protection or native behavior.

## Requirements
### Requirement: Honest maintained source-built publisher qualification
A source-built publisher candidate SHALL bind exact maintained upstream source, supported resolved dependencies and actual audit/build/behavior evidence, without claiming official bundled-artifact identity or bypassing original candidate failures.

#### Scenario: Supported clean source build
- **WHEN** exact unchanged upstream source builds and behavior checks pass with a fully audited supported lock
- **THEN** evidence records source/lock/build provenance as a separate candidate without selecting production publication.

#### Scenario: Unpatched or incompatible dependencies
- **WHEN** supported refresh leaves advisories or breaks source/build/behavior checks
- **THEN** qualification remains incomplete without forcing unsupported versions or suppressing failures.

