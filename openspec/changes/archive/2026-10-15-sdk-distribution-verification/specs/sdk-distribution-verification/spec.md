## ADDED Requirements

### Requirement: Generated package identity preserves source authority
Verification SHALL require build-only tag-derived own metadata, unchanged source/dependency identities and exact selected package, license, notice and VCS inputs.

#### Scenario: Actual owned package
- **WHEN** Cargo creates a package from an exact staged owned source
- **THEN** its generated version/lock and selected bytes match verified inputs while the original source stays version-free and unchanged.

#### Scenario: Changed package
- **WHEN** source, lock, selected member, license, notice, dependency metadata or VCS identity is changed or missing
- **THEN** verification refuses rather than reporting a valid distribution.

### Requirement: Retained artifact names do not establish byte identity
Verification SHALL compare bounded regular retained files and their manifest to trusted expected identities, reject malformed or undeclared inputs, and distinguish an incomplete verified subset from a complete distribution.

#### Scenario: Partial retained set
- **WHEN** a permitted subset has exact expected bytes but required members are missing
- **THEN** partial verification reports incomplete and complete-distribution verification refuses.

#### Scenario: Invalid retained bytes
- **WHEN** a matching name has wrong bytes, stale manifest, invalid path/type/size, symlink, FIFO or undeclared entry
- **THEN** verification refuses without accepting or modifying retained data.
