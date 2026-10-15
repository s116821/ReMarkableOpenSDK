# SDK identity

## Purpose
Provide independently usable persistent identity primitives without native authority.

## Requirements

### Requirement: Canonical persistent identities
The SDK SHALL expose nonnil canonical lowercase UUID identities, page document/page keys and operation IDs without granting native operation authority.

#### Scenario: Valid identity
- **WHEN** a canonical nonnil UUID is parsed
- **THEN** bytes and display preserve that identity exactly

#### Scenario: Invalid identity
- **WHEN** input is nil, malformed, uppercase or noncanonical
- **THEN** parsing returns no identity

### Requirement: Contained unpublished crate
The crate SHALL have no external dependencies, remain unpublished and explicitly select package inputs.

#### Scenario: Package selection
- **WHEN** Cargo lists package contents
- **THEN** unrelated tools and experimental files are excluded
