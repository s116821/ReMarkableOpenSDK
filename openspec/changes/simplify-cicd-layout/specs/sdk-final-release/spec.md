## MODIFIED Requirements

### Requirement: Official release follows successful retained artifact qualification
Only the final main-only Release stage SHALL receive publication permission and bind an official Git tag/release after all four application stages succeed. It SHALL consume exact retained SDK artifact IDs and independently trusted digests, verify actual source/merged-PR/candidate identity, and preserve source versions and tested bytes without rebuilding or replaying probes.

#### Scenario: Eligible main application
- **WHEN** the exact successful main source has an observed merged PR and unchanged candidate with fully verified SDK artifacts
- **THEN** maintained tooling may bind its official tag and release without source version commits or rebuilt artifacts.

#### Scenario: Ineligible predecessor or event
- **WHEN** a predecessor fails/skips/cancels, the workflow is cancelled, or the event is a PR or documentation-only change
- **THEN** the final Release stage cannot mutate official tag/release state.

### Requirement: Retained bytes and original release identity govern finalization
Release composition SHALL use maintained exact-tag lookup/create/finalize mechanisms and the immutable-revision maintained uploader with explicit no-overwrite and original-release-ID inputs, verify retained subsets against independent expected bytes, upload only missing names, and require the complete expected set/original ID/remote tag and stable-release metadata immediately before finalization.

#### Scenario: Partial draft or conflicting state
- **WHEN** a prior exact-ID draft contains a retained subset, conflicting bytes, a mismatched/unknown prerelease flag, or the uploader creates a replacement ID
- **THEN** only verified missing names may be uploaded, while conflicts and changed original identity refuse finalization without overwriting retained bytes.
