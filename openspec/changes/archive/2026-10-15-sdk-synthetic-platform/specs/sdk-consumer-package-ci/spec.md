## MODIFIED Requirements

### Requirement: Actual current SDK consumer and package containment
Checks SHALL compile a separate feature-disabled consumer against this actual SDK checkout and SHALL verify the exact selected source package without maintaining an own source version or admitting private development files.

#### Scenario: Current public API
- **WHEN** the consumer runs with the minimum or selected host compiler
- **THEN** identity and pure gesture calls succeed and unavailable development modules fail with the expected unresolved-import diagnostic.

#### Scenario: Package boundary
- **WHEN** package contents are listed with untracked private decoys present
- **THEN** only the thirteen selected source/generated members are admitted and removing the include list reproduces the leak.

