## ADDED Requirements

### Requirement: Immutable synthetic capture evidence
The SDK SHALL retain owned image bytes, checked geometry and lineage with bounded synthetic validation; native capture SHALL remain unsupported.

#### Scenario: Synthetic mismatch
- **WHEN** supplied image pixels, lineage, render assertion or owner guard disagree
- **THEN** synthetic batch validation refuses

### Requirement: Bounded one-way historical facts
Export SHALL preserve exact scope, identity, order, integer/time values and image descriptors under explicit bounds without reconstructing live authority.

#### Scenario: Historical export
- **WHEN** valid model evidence is exported
- **THEN** immutable facts preserve original values and cannot become operational guards

#### Scenario: Bound overflow
- **WHEN** export exceeds configured admitted bounds
- **THEN** export refuses before unbounded copying
