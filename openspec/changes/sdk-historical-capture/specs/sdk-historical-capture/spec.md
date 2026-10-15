## ADDED Requirements

### Requirement: Bounded historical correspondence
The opt-in validator SHALL validate saved v11 completion and PNG bytes against separately supplied expected bindings and retain the original bytes as explicitly unqualified historical evidence.

#### Scenario: Matching saved evidence
- **WHEN** bounded receipt and PNG match the expected binding and encoded metadata
- **THEN** original owned bytes and historical classification are available

#### Scenario: Invalid saved evidence
- **WHEN** schema, bounds, binding, digest or decoded image validation fails
- **THEN** construction refuses

### Requirement: No live authority
Historical validation SHALL NOT construct live observations, admissions or native effects; the module SHALL be disabled by default.

#### Scenario: Ordinary dependency
- **WHEN** no optional feature is enabled
- **THEN** the historical module and external dependencies are unavailable
