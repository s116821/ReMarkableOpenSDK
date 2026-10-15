# SDK candidate artifact qualification

## Purpose
Build exact SDK candidate outputs and verify retained bytes without granting official release or native compatibility authority.

## Requirements

### Requirement: Actual frozen SDK source produces unofficial candidate artifacts
The pipeline SHALL derive a prospective version through maintained Git-based tooling and build the exact frozen SDK source into a source crate, host/ARMv7/AArch64 compiler libraries and host test executables without maintaining a source version or creating an official SDK tag.

#### Scenario: Actual candidate build
- **WHEN** application source is admitted and its read-only candidate is resolved
- **THEN** the build records exact source, compiler, candidate and artifact identities, leaves source unchanged, and labels native/model compatibility unqualified.

### Requirement: Integration consumes the retained build bytes
A separate integration job SHALL validate trusted build receipt/source and all transferred artifact identities, execute the transferred host tests without rebuilding, and refuse tampering before executing changed data.

#### Scenario: Retained build
- **WHEN** exact verified build output is downloaded
- **THEN** integration executes those test binaries and verifies the same source crate and compiler libraries.

#### Scenario: Changed output
- **WHEN** receipt/source, crate bytes, executable bytes or library file type is altered
- **THEN** verification refuses before executing altered content and does not grant release or native authority.
