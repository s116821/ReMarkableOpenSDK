# Cooperative Reader witness: purpose, boundary and decision

2026-10-15. Source-only assessment; no implementation or native operation.
Based on the [established provider creation/result boundary](view-provider-creation-ownership.md).

## Concrete next-step decision

Assess a creator-scoped lexical witness as the alternative to external name or
private-offset discovery. Do not select it for implementation or a device
packet now: the saved application exposes no qualified way to install this
cooperation, and source modification/interception is a different effect scope.
It cannot be delivered by replaying a held activation through another executor.

The useful advance is a discriminating SDK evidence contract: either a
source-qualified creation record and lexical witness agree on the same outer
root, nested MainView and Loader generation, or correspondence remains unknown.
No partial observation upgrades the current unsupported correspondence claim.

## One proposed cooperative mechanism

The creator supplies a session-scoped observer reference through initial
properties to the exact outer view being created. The explicitly modified
LegacyMainWindow forwards that reference to its MainView instance. MainView's
explicit witness uses its lexical `root`, `documentView` and
`documentViewComponent` references, not objectName or context-name lookups.
The retained source already has the outer MainView instantiation/property
forwarding and those inner ids; the observer property and witness are new.

The observer's session must be bound to the actual outer creation result and
verified source selection at the creator boundary. A component URL, name or
self-reported source hash is insufficient. The observer is a proposed scoped
reference, not a security attestation supplied by Qt. No supported existing
injection point for that reference has been qualified.

After successful outer creation, a single cooperative GUI-thread call obtains
the nested lexical references and present Loader state. Install complete
invalidation coverage before sampling and verify no generation/context change
during the sample. Require the expected source-component identity, Ready state
and a non-null loaded item. Registering during a Component.onCompleted callback
or attaching a late loaded listener does not substitute for this snapshot;
nested loading may already have happened. Absence, non-ready state, reentrancy,
duplicate registration or an unobserved transition refuses the attempt.

This snapshot can establish present successful construction for a future
synchronization witness if its source and identity premises are qualified. It
does not reconstruct prior load/frame ordering. There is no wait-until-ready,
rearm, source assignment or new application view creation to obtain a nicer
result. A provider cache hit without the bound creation/source record refuses;
the cache must not be cleared to manufacture a new event.

Require the outer view, MainView and Loader/item to belong to the exact retained
capture window/root. Retain that same generation through a separately qualified
synchronization/frame pairing and one grab, as specified by the
[generation witness assessment](loader-generation-witness-feasibility.md).
Any replacement or loss of observation coverage invalidates the joined claim.
Lexical identity does not by itself establish frame ordering or image contents.
The witness alone must not grant capture admission. Guarded references must not
extend application-object lifetimes; deferred consumption requires continuous
generation validity, not merely a previously recorded pointer tuple.

## Independently reviewable evidence contract

| Evidence | Required acceptance condition | Missing or refused evidence |
| --- | --- | --- |
| Executed source selection | Verified original and modified bytes, exact resource/module selection and cache provenance, tied to the actual creator operation | Source identity unknown; do not trust a witness's own claimed hash |
| Creation association | Same session, actual outer result, expected source/component and successful completion | No outer-source correspondence |
| Lexical association | Exact MainView and its Loader/component references, linked to that outer result | No nested-source correspondence |
| Present construction | Ready, non-null item, expected component and stable generation under complete invalidation coverage | Unknown/refused construction, not inferred absence of a document |
| Capture association | Same retained window/root and qualifying synchronization/frame token through grab | No ordered capture correspondence |

An eventual output would carry bounded verdicts and session/generation tokens;
raw pointers, private addresses, QML source and document content need not be
published. This table is an admission design, not a new serialized schema.
The accepted nine-field capture schema remains unchanged. No control tests or
producer/consumer changes are justified until the delivery mechanism is selected.

The proposed terminal outcomes are `outer_creation_failed`,
`source_provenance_unqualified`, `identity_mismatch`, `nested_not_ready`,
`invalidated`, and `observed_success`. A creator-owned session is single-use:
one sample consumes it, and invalidation before consumption permanently refuses
it. Success describes only the qualified present construction observation;
it grants no document, page, visibility, pixel or historical-capture claim.

## Purpose, effects and recovery

The purpose is specifically source-to-instance correspondence and present nested
construction, not navigation or current-page authority. The candidate introduces
application properties and forwarding, witness code, state reads, invalidation
observers and creator cooperation. Those can allocate, invoke bindings/getters,
change creation timing and introduce reentrancy. It is not a metadata-only or
behavior-neutral observer merely because it does not intentionally change UI.
Initial-property delivery must reach the nested witness before use. The proposed
cooperation must preserve the existing onLoaded flow and must not introduce a
required-property failure into ordinary application creation.

The resource-rebuilder prior art shows resource modification is possible in a
different design, not that its activation is permitted here or that patched
bytes were executed. Any future delivery must independently establish the
actually selected source/cache path, bounded observer lifetime and teardown,
stock restoration and recovery that does not depend on the modified UI or a
live host. Existing host-primary recovery is not host-loss/timer qualification.
No patch, resource replacement, cache clearing, restart, build or activation was
performed by this assessment.

## Scope boundary and independently permitted SDK work

The present decision rejects turning this candidate into a packet using the
held or equivalent route. A lexical witness cannot retroactively identify
actual 7a4, and a new observer name does not make a replay independently allowed.
Main remains the sole operator and must select any genuinely permitted future
experiment under the existing workflow.

The independently permitted work completed here is the source-backed admission
contract and effects/recovery assessment. It identifies the exact missing
delivery prerequisite: supported creator cooperation that binds verified source
and the real outer result to this observer. Further implementation should start
only from a concrete permitted delivery proposal that satisfies that prerequisite,
not from speculative private-cache getters or another simulation matrix.
This decision does not block Main's Reader stdin implementation, SCRAPPY's
Manager consumer or unrelated SDK work, and does not assign work to those lanes.

Source basis: retained LegacyMainWindow/MainView source, reviewed provider
creation/ownership evidence and author/Astra design analysis. Public
[QML id semantics](https://doc.qt.io/qt-6.10/qtqml-syntax-objectattributes.html#the-id-attribute)
support direct lexical references; they do not attest the loaded source bytes.
Private records have no public transcript link. All proposed capabilities remain
unimplemented and unqualified; held/spent exclusions remain unchanged.
