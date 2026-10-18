# Change: Compose the final SDK release stage

## Why
Current main qualifies SDK artifacts and the maintained uploader, including trusted cross-job transfer, but never binds an official Git tag or publishes a release. The retained selected pipeline already supplies the maintained mechanisms; compose them against current main without introducing a publisher/version engine.

## What Changes
Add the genuine final main-only Release stage after all four successful application stages. Consume exact SDK/tool artifact IDs and trusted receipt/provenance digests; verify source, candidate and observed merged PR before final maintained tagging. Observe/create a draft by exact tag/ID, verify retained bytes, upload only missing names with the qualified create-only uploader, and require the original ID/full bytes/tag again before maintained finalization. Serialize main workflows from candidate calculation through release using GitHub's native pending queue; PR runs keep independent groups.

## Activation boundary
This draft changes production behavior when merged to main. Do not merge it until the coordinator explicitly clears the existing first-official/protected-remote owner scope. Read-only investigation found immutable releases disabled and no repository rulesets; this change does not alter settings, copy admin credentials, create experimental production releases or authorize its own activation. Source/spec/PR validation can proceed without production effects.

## Impact
No SDK/Buddy runtime/API, source version, rebuilt/relabelled candidate bytes or new compilation/integration suite. Only the final Release job gets contents:write. Documentation and PR events cannot publish. Native/firmware compatibility remains explicitly unqualified. Whole REM50 and first-publication acceptance remain separate from a reviewed source-only draft.
