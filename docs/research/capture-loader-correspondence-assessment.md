# Capture Loader correspondence: next diagnostic assessment

Status: source-only review proposal, 2026-10-15. No implementation, build,
profile, nonce, activation or device operation is selected or authorized here.

## What the retained evidence answers

The [completed static investigation](window-navigator-provider.md) connects the
successful registration path to MainView and its DocumentView Loader. It does
not connect that component instance or successful creation to the window/root,
frame or grab in actual 7a4.

Actual 7a4 reached `anchor-uniqueness` in both existing samples, with 35 visited
objects and zero helper/container matches. Its nine-field snapshots did not
retain a count for the name `DocumentView`. Loader presence, status and creation
order are therefore unknown, not false. The later owner/metadata/page checks
were unreached. The spent loading-only observation remains unchanged.

## Smallest useful candidate for Main review

Count the fixed objectName `DocumentView` alongside the existing fixed anchor
names in the same guarded traversal, before the anchor-uniqueness return.
Reuse the retained capture window/root and the two existing sample positions.
This adds one comparison to an already-read name; it does not require a second
walk, application property value read, signal hook, wait or window selection.

Preserve all existing guards, QObject/visual edge deduplication and bounds:
128 objects, depth 12, 64 edges per node and the cooperative one-second local
clock. Preserve capture admission, its sticky refusal, context checks, one grab
and publication ordering. No wait-until-present, rearm or fallback selection.

Report traversal completion explicitly. A proposed completion flag starts false
and becomes true only after the traversal exhausts its queue and its final
context checks pass, before anchor-uniqueness and later owner/metadata checks.
A later check refusing must not relabel the completed traversal as partial.
Completion describes this guarded walk, not an atomic or continuously valid
snapshot of the object tree. Existing context-loss publication refusal remains.

| Observation | Permitted conclusion |
| --- | --- |
| Completed walk, zero matches | No matching name was reached from this root in this walk. |
| Completed walk, one match | One named candidate was reached; exact role and Loader type remain unknown. |
| Completed walk, multiple matches | Name-based correspondence is ambiguous. |
| Refused/partial walk | Count is partial; zero cannot establish absence from the root. |
| Different counts before/after | The walks differed; no particular instance's creation time follows. |
| Equal counts before/after | No instance continuity or readiness follows. |

The exact source uses `DocumentView` as the Loader's objectName. QML ids such as
`root`, `rootItem` and `documentView` do not provide the missing runtime instance
join. A name match alone cannot establish MainView's particular Loader, its
loaded item, successful construction, document state, pixels or loading cause.

## Remaining ordering boundary

The source Loader is configured for synchronous DocumentView instantiation when
its source assignment executes; successful construction remains a condition,
not a runtime observation. A frame or MainView completion label alone does not
establish that successful assignment and construction preceded the frame/grab.
Two name counts cannot establish this ordering either.

Reading `status`, `item`, `sourceComponent`, `active` or application state would
be a separate observation mechanism outside this proposal. Event attachment
would also need separate qualification: late attachment can miss an earlier
event, and no observed event is not evidence of failure. Qt documents that a
successful initial local load can lack a status-change notification while still
emitting `loaded`; Loader readiness also does not establish document readiness.
See [Qt Loader semantics](https://doc.qt.io/qt-6.10/qml-qtquick-loader.html).

## Contract and recovery prerequisites

The [accepted same-grab contract](../../openspec/changes/establish-native-platform-contract/capture-window-reader-structure.md)
fixes schema 1, exactly nine snapshot fields and a 1024-byte reader envelope.
The counter and completion field cannot silently extend it. Any implementation
requires an explicitly reviewed schema/producer/consumer agreement, exact
serialization and collector envelope checks, and unchanged inode/publication
proofs. Review whether the existing envelope fits before changing any quota.
No new mode or parallel experiment matrix is proposed.

If Main selects this question for further work, the review must first cover the
contract delta and focused owned controls for complete/partial walks, duplicate
names, context loss, existing refusals and strict old/new schema handling.
Those controls are future prerequisites, not tests run by this assessment.
Fresh artifact/provider/current-state and whole-packet review remain separate
gates; there is no permission to create a packet or replay actual 7a4, held 8112,
spent 0404 or an equivalent excluded route.

Main remains the sole tablet operator. The prior observed recovery was the
original live-host primary path. It did not qualify timer fallback or recovery
after host loss. A future proposal must preserve and review independent recovery
under the existing workflow; this diagnostic provides no recovery evidence and
does not relax those gates.

## Review decision

Retained evidence alone supports recording these runtime questions as unknown.
The narrow extension is useful only if distinguishing a reachable named parent
candidate from no reached candidate would change the next decision. It cannot
resolve the central readiness/order question alone. If exact instance ordering
is required immediately, this extension is insufficient and that mechanism must
be separately proposed and qualified. Repeating the old packet, adding a delay
or treating an unobserved Loader status as false does not close the gap.

Source basis: current SDK census and capture observer source, the accepted
same-grab contract, retained actual 7a4 records, the reviewed private MainView
source and the linked static investigation; official Qt documentation supplies
general Loader semantics. Private records have no public transcript link.
The proposed counter, completion semantics and value-of-experiment assessment
are author/Astra analysis, not observed runtime behavior.

Coordinator review compared the assessment with the current traversal, context
guards and caps, accepted schema contract, retained-result report, static
creation mapping and official Qt Loader documentation. It found no actionable
documentation inconsistency. This was bounded source/document review, not
implementation review or runtime verification.

Main subsequently declined the counter-only extension: zero, one or multiple
name matches would not change the operator/admission decision. Its next gate
requires exact captured window/root correspondence to the known Loader instance
and successful construction before the admitted frame/grab. The counter is
therefore not selected and creates no required implementation/build/test cycle.
Further source-only feasibility work should address that exact gate without
reopening the completed static registration investigation. No getter, callback,
implementation, profile, build or native action is selected by this decision.
