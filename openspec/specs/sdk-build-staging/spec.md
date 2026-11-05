# SDK build staging

## Purpose
Preserve exact source and tag-derived build identity through explicit official or unofficial staging without maintaining source versions or publishing.

## Requirements

### Requirement: Version identity stays in build staging
Staging SHALL preserve version-free source and reject dirty, mismatched or version-maintaining input; official admission SHALL require the exact existing tag and source SHA, while candidate admission SHALL remain explicitly unofficial.

#### Scenario: Verified existing tag
- **WHEN** a clean exact source checkout has the requested existing stable tag
- **THEN** only a new stage receives the tag-derived version and source remains unchanged.

#### Scenario: Unofficial candidate
- **WHEN** a candidate is staged before official tag verification
- **THEN** its official tag is null and default official verification refuses it.

### Requirement: Portable staging preserves source identity
Staging SHALL preserve explicit regular package-input identity and canonical POSIX keys across supported hosts, refuse source drift, and distinguish unverified source-only checks from verified lock identity.

#### Scenario: Portable source check
- **WHEN** LF or CRLF owned input is staged on Linux or Windows
- **THEN** generated version lines preserve endings, target output does not affect source identity, and missing lock or source drift refuses full verification.
