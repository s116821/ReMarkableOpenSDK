# Explicit-fixture caller-selected insertion proposal

Task 2.3 follow-up. The [research selection and outcome table](../../../docs/research/client-selected-insertion-discriminator.md)
are the canonical rationale. This proposal adds a narrowly configured one-call
allocation experiment to the original creation Probe, not the held FactsEntry or
shutdown diagnostic. Native capabilities remain Unsupported.

## Proposed source change

Add an explicit optional target UUID to CreationConfig. The existing omitted-target
five-argument mode retains its behavior. The new explicit mode requires a canonical
nonnil target different from the document and every baseline page, and the exact
fresh six-page baseline for the selected existing fixture. Serialize values through
the existing JSON construction. Check all forward/reverse baseline IDs and target
absence before mutation claim, then pass target as argument six exactly once.
Keep the original index, template, size, callback, engine/thread/lifetime guards,
deadline and no-replay semantics. Do not introduce a generic executor or public
native receipt constructor. Private fixture values stay outside Git.

## Focused checks and remaining work

- [ ] Review exact source amendment with Main against the original successful path.
- [ ] Verify the explicit mode passes all six arguments unchanged in one call;
  omitted-target behavior remains five arguments; invalid/duplicate target and
  stale baseline refuse before claim. Reuse the existing owned QML fixture.
- [ ] Freeze exact source/artifact/configuration with Main and confirm applicable
  independent restoration without replaying any rejected operation.
- [ ] Main selects at most one new actual call; preserve order, PDF/ink/unknown
  metadata and independently read back allocation/persistence after restoration.

No fresh target build, private packet or device action is selected yet. The owning
change remains unfinished; canonical sync/archive and full native qualification
are not completed by this proposed test or a component success.
