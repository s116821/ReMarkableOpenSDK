# Change: Preserve trusted uploader identity across jobs

## Why
Checks now qualifies the maintained uploader but discards it. Later release composition must consume exactly those qualified files. A copied provenance inventory cannot independently authenticate itself: an owned diagnostic changed harmless action metadata and its recorded hash while the selected JS bundle stayed unchanged, and the local bundle-only verifier accepted it.

## What Changes
Emit the qualified provenance SHA from Checks, retain the exact qualified directory with the maintained artifact Actions, and expose immutable artifact ID plus trusted provenance SHA. Existing Integration downloads that ID and binds provenance bytes to the trusted Checks value before full file/bundle verification. Add three controls against the actual transferred tool: valid input, jointly changed metadata/provenance, and changed payload.

## Impact
No new job, compiler/platform matrix, tool rebuild, repeat loopback suite, credentials, final Release job, production tag/publication, source version or native action. Existing same-job source qualification remains available; transferred verification explicitly requires the trusted provenance input.
