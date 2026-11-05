## ADDED Requirements

### Requirement: Immutable Cargo Git source identity
SDK consumer qualification SHALL verify the exact repository URL, stable Git tag and locked source SHA using actual Cargo metadata, refuse ambiguous, changed or locally overridden official inputs, and preserve version-free source.

#### Scenario: Owned source and generated default lock
- **WHEN** an owned Git-tag consumer runs before and after source branch advancement, including a committed generated default lock
- **THEN** the locked tag and SHA remain exact, Cargo's default package version is not treated as release authority, and only verified generated build environment values derive from the tag.

#### Scenario: Development override and official refusal
- **WHEN** a temporary path patch replaces the Git dependency or cached source is dirty or maintains an own version
- **THEN** local development can build the patch, official source admission refuses it, and removing the patch restores the exact Git pin.

### Requirement: Current SDK Git consumer qualification
The existing Linux Checks stage SHALL qualify the actual current SDK through an owned Git-tag transport using the ordinary feature-disabled consumer, without changing SDK runtime/API or publishing a tag.

#### Scenario: Actual public consumer
- **WHEN** the current tracked SDK is copied into an owned Git repository and selected by fixture tag and locked SHA
- **THEN** ordinary identity and pure geometry calls succeed with no SDK features, source build identity matches the fixture tag and SHA, and a temporary local override is refused for official admission.
