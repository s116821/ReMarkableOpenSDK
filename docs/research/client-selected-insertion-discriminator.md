# Next native insertion discriminator: caller-selected page identity

October 8, 2026. Selected research direction, not an implemented or selected
tablet packet: exercise the existing native method's sixth `pageUuid` argument
once on a backed-up explicit disposable fixture. The successful original insertion
used five arguments and native-assigned identity. This tests a different allocation
contract; it does not claim a shutdown diagnosis or repair.

## Why this is the next useful question

The [original insertion path](../experiments/qt_qml_access_probe.md) demonstrated
one native page insertion and later persistence. The existing
[static insertion trace](native-platform-ledger.md#static-insertiontask-trace-beyond-metadata)
identified supplied-UUID validation and duplicate-identity checks. The saved
controller metadata declares
`addPageWithTemplateAndPageSize(entry::Id,int,QString,QSizeF,QJSValue,QString)`.
Actual caller-selected allocation remains untested. A successful exact-ID result
would narrow the operation-to-target attribution problem without requiring a new
native-assigned correlation mechanism. It would not establish retry idempotence,
atomic source/order comparison, crash durability or a production receipt.

The October 8 source refresh found no smaller existing external insertion route:

| Current primary source | Relevant finding |
| --- | --- |
| [rm-librarian 7d0fe086](https://github.com/rmitchellscott/rm-librarian/tree/7d0fe08678203c129b9d848419da2a7e96a2abe5) | HEAD still matches the earlier inspected revision. Its library commands include notebook creation/import, not page insertion. Loading it would not by itself answer this allocation question. |
| [QMD createPagesRM2Size 67d39e94](https://github.com/rmitchellscott/xovi-qmd-extensions/blob/67d39e943d8fda30936ad7f51a30f59f0292a9f8/3.28/createPagesRM2Size.qmd) | HEAD is unchanged; patches native page-creation size expressions in UI contexts. It supplies call-site precedent, not an independently callable insertion service. |
| [remarkable-mcp 9dcbbad9 api.ts](https://github.com/lukaisailovic/remarkable-mcp/blob/9dcbbad94a564f5867c1b385282d596320fb53af/src/api.ts) | addPage generates an ID and writes metadata/content/page files; its apply path uses an SSH xochitl restart or cloud commit. This does not supply the required live native cache/worker coordination. |

No community code is copied, installed or built for this proposal. The rejected
diagnostic module, rejected stock-shutdown preparation and rejected central Docs
save remain separate and must not be replayed or repackaged. Their exact policy
trigger is unknown; removing diagnostic code is not asserted to explain or avoid
the safeguard.

## One-call experiment

Retain the original creation Probe's existing-engine, Library-ready, explicit
fixture, one-shot bridge and bounded completion path. Do not add FactsEntry,
engine-ready/shutdown instrumentation, native opening or another loader.
The meaningful variable is supplying one fresh canonical nonnil target UUID as
the sixth argument; index 1, inherited template, RM2 paper size and callback
behavior stay as in the successful insertion path.

The old five-page configuration is spent and invalid for today's six-page fixture.
Main must freshly back up and bind the complete current document/order, retain
all six existing IDs and hashes, and verify the proposed target is absent. A new
private configuration records that six-page baseline and target. This is baseline
refresh, not permission to delete the previous successful insertion or replay its
old packet. Source validation must refuse duplicate/nil targets and any current
count/order mismatch before the single mutation claim. No automatic retry follows
false return, exception, timeout, missing callback or restoration trouble.

| Result after independent restoration/readback | What it teaches |
| --- | --- |
| Exactly one new page at index 1 with the supplied ID; originals preserved; same ID/order persists | Caller-selected identity works for this exact firmware/fixture/path. It is useful input to later durable reconciliation work, not an accepted native receipt. |
| One page is added under a different ID | The requested identity was not honored on this path; keep outcome uncertain for the intended operation and do not repeat. |
| No page is added and pre-dispatch refusal is established | This candidate was refused; the stage identifies the failed prerequisite, not broad native impossibility. |
| No page with accepted dispatch, missing/contradictory evidence, extra mutation or crash | Indeterminate/failed experiment; preserve partial evidence, restore stock independently and do not resend. |

Main alone selects and operates the packet after focused source/artifact review,
advance notice, fresh idle/fixture/provider checks and an independent restoration
path. Bound the effect to one added page in the explicit disposable document.
Keep the existing reviewed deadlines and no-replay behavior; do not use this as a
reason to reproduce rejected stock-shutdown source preparation. If the applicable
restoration path cannot be established without a rejected operation, the device
step remains unselected. A new rejection also stops that operation; another
transport, name or packaging is not a substitute.

The source amendment and focused argument/refusal checks are described in the
[owning proposal](../../openspec/changes/establish-native-platform-contract/client-selected-insertion.md).
No code, artifact, nonce, target UUID or device action is selected by this document.
Native capabilities, active-page authority and lifetime/shutdown qualification
remain open. The old 0404 fault and all completed attempts remain preserved.

Source basis: current REM-25 description and latest timestamped comments, Main
handoff version 50, exact SDK creation source and published findings, and freshly
read pinned upstream sources above. The experiment and expected interpretations
are proposals/inferences; no caller-selected native result is claimed.
