## MODIFIED Requirements

### Requirement: Exact maintained uploader qualification
SDK publication SHALL consume the reviewed immutable upstream upload Action revision directly using its distributed entrypoint. CI SHALL NOT rebuild its dependency closure, transfer a locally repackaged uploader between jobs or treat historical custom-bundle audit results as evidence for the upstream distribution.

#### Scenario: Selected upstream distribution
- **WHEN** the final eligible Release stage requires an uploader
- **THEN** it uses the pinned maintained Action directly while ordinary SDK Checks and Integration require no custom uploader bundle.

### Requirement: Create-only uploader limits remain explicit
SDK publication SHALL pass the original release ID and explicit no-overwrite, duplicate-check and draft options to the maintained Action. Application-owned checks SHALL preserve retained bytes and refuse finalization if the original identity changes, without maintaining loopback tests of upstream internals.

#### Scenario: Retry and refusal
- **WHEN** an exact-ID draft upload is interrupted or a duplicate races
- **THEN** only verified missing bytes may be uploaded and conflicting retained bytes refuse without replacement or publication.

#### Scenario: Missing-ID fallback
- **WHEN** the upstream Action returns a replacement draft ID
- **THEN** the original-ID check refuses finalization rather than accepting the new ID as authority.
