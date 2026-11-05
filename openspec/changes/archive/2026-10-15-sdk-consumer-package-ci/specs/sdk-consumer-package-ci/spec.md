## ADDED Requirements

### Requirement: Actual current SDK consumer and package containment
Checks SHALL compile a separate feature-disabled consumer against this actual SDK checkout and SHALL verify the exact selected source package without maintaining an own source version or admitting private development files.

#### Scenario: Current public API
- **WHEN** the consumer runs with the minimum or selected host compiler
- **THEN** identity and pure gesture calls succeed and unavailable development modules fail with the expected unresolved-import diagnostic.

#### Scenario: Package boundary
- **WHEN** package contents are listed with untracked private decoys present
- **THEN** only the ten selected source/generated members are admitted and removing the include list reproduces the leak.

### Requirement: Documentation-only changes avoid SDK compilation
CI SHALL use maintained path classification before Rust jobs, retain lightweight validation/results for documentation-only changes, and treat application/package/CI, unknown and mixed changes conservatively.

#### Scenario: Documentation-only change
- **WHEN** only named documentation/OpenSpec guidance paths change
- **THEN** Rust installation, compilation and consumer execution are skipped while structural checks and the result remain available.

#### Scenario: Application or rename
- **WHEN** a changed path or either side of a rename affects application/package/CI or an unknown path
- **THEN** required current-crate Rust and consumer/package checks execute.
