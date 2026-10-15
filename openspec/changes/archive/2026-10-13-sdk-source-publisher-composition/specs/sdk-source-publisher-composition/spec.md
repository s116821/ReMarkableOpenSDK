## ADDED Requirements

### Requirement: Maintained tag-to-build publisher composition
Qualification SHALL bind an existing remote intended tag/source to actual compiled/package identity and maintained source-built publisher recovery without retagging/rebuilding retained output.

#### Scenario: Partial draft recovery
- **WHEN** actual retained partial assets match the verified build and the same remote tag/source remains
- **THEN** recovery retains prior bytes and completes the draft, with complete asset identity checked before explicit finalization.

#### Scenario: Local-only tag or tampered retained bytes
- **WHEN** intended remote tag/source is missing or retained bytes differ from declared identity
- **THEN** build/publication or finalization refuses without inventing remote authority or overwriting existing bytes.
