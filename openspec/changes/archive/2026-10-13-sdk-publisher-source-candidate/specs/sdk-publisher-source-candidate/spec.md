## ADDED Requirements

### Requirement: Honest maintained source-built publisher qualification
A source-built publisher candidate SHALL bind exact maintained upstream source, supported resolved dependencies and actual audit/build/behavior evidence, without claiming official bundled-artifact identity or bypassing original candidate failures.

#### Scenario: Supported clean source build
- **WHEN** exact unchanged upstream source builds and behavior checks pass with a fully audited supported lock
- **THEN** evidence records source/lock/build provenance as a separate candidate without selecting production publication.

#### Scenario: Unpatched or incompatible dependencies
- **WHEN** supported refresh leaves advisories or breaks source/build/behavior checks
- **THEN** qualification remains incomplete without forcing unsupported versions or suppressing failures.
