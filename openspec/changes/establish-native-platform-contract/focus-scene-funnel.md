# Bounded development scene-classification funnel

Implemented bounded checkpoint29f8484, proposal60c0699 against runtime430be339
and guidancec6e6d018. The prior a6d3 capture
remains immutable. This checkpoint does not authorize device actions or select
a new nonce. Main owns the next bounded native observation and rollback.

## Requirement and observable scenarios

WHEN developmentFocusAncestry performs its one cached-chain classification,
THEN each visited item contributes to exactly one numeric scene category:
engine rejection, class rejection, pageId metadata rejection, pageIdChanged
signal rejection, documentWrapperChanged signal rejection, or pass. Preserve
the original predicate order and short circuit. No document getter, additional
parent/tree walk, forced focus, fallback, or owner acceptance change is allowed.

WHEN the selected discovery returns, THEN emit one fixed tagged numeric journal
line containing chain completeness, chain items, classified items and those six
counts. Partial classification remains identifiable by its denominators. No names,
pointers, document data, arbitrary reason text or companion output file occurs.
WHEN this development mode is unselected, THEN no funnel journal line occurs.
The v3 refusal envelope remains exactly37 fields; v1/v2 stay unchanged.

## Design and API

Add a zero-initialized FocusSceneFunnel numeric accumulator and an optional
trailing accumulator pointer to findFocusPageOwner. Existing source callers
continue to compile. A metadata-only scene helper implements the exact original
ordered predicate sequence and increments its first failing category, or pass.
FactsEntry owns a stack-local accumulator for its existing one-shot discovery
and logs the fixed line immediately after that call returns. Owner discovery,
observers, deadlines and refusal priority remain unchanged.

## Tasks and evidence boundaries

- [ ] Implement the helper, optional output and selected one-shot journal line.
- [ ] Check positive and each of the five first-failure stages against actual
  Qt metadata, including that later stages are not counted.
- [ ] Compile the exact ARM payload source and run the existing bounded focus
  checks; record source/output hashes and obtain independent review.
- [ ] Main selects and executes the next concrete bounded native observation
  using frozen inputs and independent rollback. No native result is claimed here.

Broad fixture/source qualification, native-profile qualification, canonical
specification sync and archive remain open in the owning unfinished change.

## Bounded verification checkpoint

Implementation29f84848d9748dfdd2d6d46df24be63e3b7372dd passed eight focused
Qt ARM/QEMU checks: each first-failure stage, positive pass, wrong pageId type,
and zero native property reads. Existing22 focus cases and four compiler privacy
negatives also passed. The production shared payload compiled from that exact
commit archive with the pinned RM2 SDK, warnings-as-errors and no fixture macro.
This is host compile and owned-fixture evidence, not native qualification.
Main and Astra reviewed the bounded source delta without findings; Astra did
not independently rerun the fixture suite for this checkpoint.

The first three tasks above are satisfied for this bounded checkpoint. The
native observation task and broader qualification/lifecycle work remain open.
Private compile receipt/logs are retained by Main with the handoff; no firmware
or attempt-specific configuration is included here.

An initial-progress refusal does not enter discovery and emits no funnel line.
A construction refusal can emit visited=-1 and all six counters0, meaning
classification did not start. A complete chain can still have partial
classification when an existing candidate bound refuses. Logging configuration
can suppress qInfo; a missing line means evidence unavailable, never zero counts.
The installed message handler executes synchronously. Preserve the original
external clock/rollback and scope journal evidence to the attempt and PID.
