# sdk-source-publisher-composition Specification

## Purpose
Define controlled qualification of maintained tagger, tag-derived build and source-built publisher recovery, preserving separate production publication and native gates.
## Selection status
This records the original controlled candidate qualification, retained as historical evidence. REM-56's [ordered replacement candidate](../sdk-gated-release-pipeline/spec.md) supersedes its release orchestration/selection; old selector, tag-before-build and publisher mechanisms are not selected active production behavior. Original requirements/results and tool failures remain preserved. This status neither archives unfinished foundation/source-main/REM-50 work nor qualifies a first official release, remote protection or native behavior.

## Requirements
### Requirement: Maintained tag-to-build publisher composition
Qualification SHALL bind an existing remote intended tag/source to actual compiled/package identity and maintained source-built publisher recovery without retagging/rebuilding retained output.

#### Scenario: Partial draft recovery
- **WHEN** actual retained partial assets match the verified build and the same remote tag/source remains
- **THEN** recovery retains prior bytes and completes the draft, with complete asset identity checked before explicit finalization.

#### Scenario: Local-only tag or tampered retained bytes
- **WHEN** intended remote tag/source is missing or retained bytes differ from declared identity
- **THEN** build/publication or finalization refuses without inventing remote authority or overwriting existing bytes.

