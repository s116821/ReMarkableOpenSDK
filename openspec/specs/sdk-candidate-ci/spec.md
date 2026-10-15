# sdk-candidate-ci Specification

## Purpose
Require actual candidate SDK compiler, consumer and staging coverage while preserving source ownership and separate production/native gates.
## Selection status
This records the original controlled candidate qualification, retained as historical evidence. REM-56's [ordered replacement candidate](../sdk-gated-release-pipeline/spec.md) supersedes its release orchestration/selection; old selector, tag-before-build and publisher mechanisms are not selected active production behavior. Original requirements/results and tool failures remain preserved. This status neither archives unfinished foundation/source-main/REM-50 work nor qualifies a first official release, remote protection or native behavior.

## Requirements
### Requirement: Actual integrated SDK candidate coverage
CI SHALL compile and qualify the SDK source present in its current candidate checkout using the declared compiler/MSRV/ARM/consumer and source-staging checks. SDK-free historical fallback SHALL remain explicitly labeled and SHALL NOT establish current candidate coverage.

#### Scenario: Complete current SDK candidate
- **WHEN** the candidate contains the complete recognized owner SDK source
- **THEN** the existing selector admits the exact current Git identity and current compiler/consumer/staging checks operate on those bytes.

#### Scenario: Absent or partial SDK inputs
- **WHEN** the SDK inputs are absent or incomplete
- **THEN** only fully absent inputs may use explicit historical qualification; partial inputs refuse rather than borrowing historical success.

### Requirement: Source ownership and broader gate preservation
Integration SHALL preserve the owner's immutable runtime/API/package/license blobs and SHALL retain unsuppressed security, production publication, native and source-main obligations independently. It SHALL NOT mint production versions/tags or imply shipping eligibility from compilation.

#### Scenario: Mechanical source integration
- **WHEN** reviewed source and CI are combined
- **THEN** source-owner behavior blobs remain unchanged, compiler checks are current-candidate evidence, and unfinished release/native scopes stay open.

