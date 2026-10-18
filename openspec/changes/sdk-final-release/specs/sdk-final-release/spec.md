## ADDED Requirements

### Requirement: Official release follows successful retained artifact qualification
Only the final main-only Release stage SHALL receive publication permission and bind an official Git tag/release after all four application stages succeed. It SHALL consume exact retained SDK/uploader artifact IDs and independently trusted digests, verify actual source/merged-PR/candidate identity, and preserve source versions and tested bytes without rebuilding or replaying probes.

#### Scenario: Eligible main application
- **WHEN** the exact successful main source has an observed merged PR and unchanged candidate with fully verified SDK/tool artifacts
- **THEN** maintained tooling may bind its official tag and release without source version commits or rebuilt artifacts.

#### Scenario: Ineligible predecessor or event
- **WHEN** a predecessor fails/skips/cancels, the workflow is cancelled, or the event is a PR or documentation-only change
- **THEN** the final Release stage cannot mutate official tag/release state.

### Requirement: Retained bytes and original release identity govern finalization
Release composition SHALL use maintained exact-tag lookup/create/finalize mechanisms and the qualified create-only uploader, verify retained subsets against independent expected bytes, upload only missing names, and require the complete expected set/original ID/remote tag immediately before finalization.

#### Scenario: Partial draft or conflicting state
- **WHEN** a prior exact-ID draft contains a retained subset, conflicting bytes, or the uploader creates a replacement ID
- **THEN** only verified missing names may be uploaded, while conflicts and changed original identity refuse finalization without overwriting retained bytes.

### Requirement: Native queuing and explicit activation scope remain bounded
Main delivery SHALL use the maintained native queue across candidate/build/integration/release, preserve independent PR groups and cancellation/failure guards, and retain the existing owner protection/first-official activation boundary without admin credentials or settings mutations in CI.

#### Scenario: Concurrent main delivery
- **WHEN** multiple main runs wait for delivery
- **THEN** the native queue preserves pending runs up to its documented limit, candidate/tag conflicts refuse, and no custom queue or retagging repair is introduced.

#### Scenario: Source-only draft acceptance
- **WHEN** source review and PR checks accept this composition before owner activation clearance
- **THEN** the PR remains held from production merge and no real tag/release or native qualification is inferred.
