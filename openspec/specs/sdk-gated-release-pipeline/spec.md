# sdk-gated-release-pipeline Specification

## Purpose
Define the qualified replacement SDK CI/release candidate: checks, frozen builds, integration consuming those artifacts, then one eligible main-merge publication job. Canonical here records accepted candidate behavior, not deployment to SDK main or native qualification.

## Delivery boundary
The replacement candidate supersedes the old release candidates for active selection. SDK PR8 remains stacked on unmerged foundation inputs; foundation/source-main, REM-50, first official publication, protected remote policy and runtime/native qualification remain separate open gates. No production tag, release or repository setting is authorized by candidate acceptance. Historical tool defects and archived fixture results remain evidence for their original scopes. See the [completed change](../../changes/archive/2026-10-15-sdk-gated-release-pipeline/results.md).

## Requirements
### Requirement: Ordered SDK artifact qualification before official release
SDK CI SHALL order required checks before build and integration/package tests using those build artifacts where supported. Read-only candidate version resolution MAY precede build, but official tags/releases SHALL be created only by one final eligible main-merge release job after every required predecessor succeeds. It SHALL publish the same source, candidate version and artifact bytes that passed qualification. The actual successful main source commit SHALL independently be an eligible scoped application message under the configured maintained semantic parser; an earlier failed/unreleased message SHALL NOT lend eligibility to a docs or unrecognized actual squash message.

#### Scenario: Eligible main merge succeeds
- **WHEN** an application-relevant main merge has passed all required checks, builds and artifact integration with unchanged candidate/source identity
- **THEN** the final dedicated release job binds the official immutable tag to that source and publishes only the verified artifacts without a source-version commit.

#### Scenario: Predecessor or identity refuses
- **WHEN** a required predecessor fails, skips or is canceled, the event is a PR/docs-only change, the actual main message is ineligible despite an earlier unreleased candidate, or candidate/source/tag/artifact identity conflicts
- **THEN** no official tag/release is created and no failed or historical qualification is promoted to release authority.

### Requirement: Maintained release mechanisms and preserved ownership
The pipeline SHALL use maintained qualified upstream calculation, tagging, Actions and publisher mechanisms, preserve semantic-title and docs-only rules, independent component versions, immutable tags, honest retry/concurrency checks and unrelated owner source. Recorded candidate failures SHALL NOT be waived or hidden. SDK-only qualification SHALL NOT establish ecosystem cleanup or production/native compatibility.

#### Scenario: Retry or concurrent state changes
- **WHEN** a failed or competing publication is retried
- **THEN** existing tag/source and retained artifact bytes are revalidated, conflicts refuse without replacement, and only the original tested identity may complete publication.
