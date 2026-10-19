# Change: Compose the final SDK release stage

## Why
Current main qualifies SDK artifacts and the maintained uploader, including trusted cross-job transfer, but never binds an official Git tag or publishes a release. The retained selected pipeline already supplies the maintained mechanisms; compose them against current main without introducing a publisher/version engine.

## What Changes
Add the genuine final main-only Release stage after all four successful application stages. Consume exact SDK/tool artifact IDs and trusted receipt/provenance digests; verify source, candidate and observed merged PR before final maintained tagging. Observe/create a draft by exact tag/ID, verify retained bytes, upload only missing names with the qualified create-only uploader, and require the original ID/full bytes/tag again before maintained finalization. Serialize main workflows from candidate calculation through release using GitHub's native pending queue; PR runs keep independent groups.

## Delivery authority and evidence boundary
The October 19 coordinator reconciliation clears the earlier generic first-official hold under the existing human authorization for automatic official SDK releases. Normal scoped review, required CI and coordinated delivery still apply. A separate test repository and deliberate live provider failure probes are not product prerequisites. The prepared synthetic probe packet remains historical and unexecuted.

Read-only investigation found immutable releases disabled and no repository rulesets. This change does not alter settings, copy admin credentials or create experimental production releases. Canonical task scope preserves existing tags/settings/releases; unrelated repository guidance is not an SDK settings prohibition. Point-in-time source/byte verification does not establish permanent server protection or an attestation. Record actual protection and verification results honestly on the first genuine release; disabled or unknown protection cannot be presented as verified immutability.

## Impact
No SDK/Buddy runtime/API, source version, rebuilt/relabelled candidate bytes or new compilation/integration suite. Only the final Release job gets contents:write. Documentation and PR events cannot publish. Native/firmware compatibility remains explicitly unqualified. Source/fixture acceptance and actual-main first-publication evidence remain distinct.
