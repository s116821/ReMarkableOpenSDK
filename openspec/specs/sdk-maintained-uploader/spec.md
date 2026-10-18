# SDK maintained uploader qualification

## Purpose
Qualify the exact maintained uploader recipe and create-only loopback behavior before separate official-release composition.

## Requirements

### Requirement: Exact maintained uploader qualification
SDK Checks SHALL build the selected unmodified maintained upload Action with its exact minimal build/runtime recipe, require a fresh clean full selected dependency audit, and verify source, license, bundle and provenance identity before executable qualification.

#### Scenario: Selected source recipe
- **WHEN** the exact maintained source is built in the existing application-admitted Linux Checks stage
- **THEN** selected dependency audit has no advisories, upstream library/license bytes remain unchanged, and emitted bundle/provenance match the selected identity without publication permission.

### Requirement: Create-only uploader limits remain explicit
SDK qualification SHALL execute the actual emitted Action against an owned loopback API, preserve retained bytes without replacement or publication, and retain observed missing-ID fallback as a future finalization limitation.

#### Scenario: Retry and refusal
- **WHEN** an exact-ID draft upload is interrupted, a duplicate races, or a glob matches no file
- **THEN** missing-only retry preserves retained bytes and conflicting or absent inputs refuse without deletion, replacement or publication.

#### Scenario: Missing-ID fallback
- **WHEN** the original draft ID is absent and upstream creates a replacement private draft
- **THEN** qualification records that behavior and does not treat the new ID as authority to finalize the original release.
