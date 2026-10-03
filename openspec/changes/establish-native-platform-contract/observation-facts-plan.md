# Finite observation-only facts slice

The source/fixture proposal was independently accepted at SDK `b6f5244` before
implementation. The development implementation checkpoint below remains subject
to independent source review and fixture reproduction.
It supports the selected logical navigation route; direct native opening remains
deferred to the post-1.0 REM-51 work. It neither constructs Rust PageObservation
nor qualifies native creation, input continuity, rendered pixels or write authority.
No current-process entry, startup payload, recovery recipe, build or device run is
selected by this proposal. Main selects any later native access/recovery separately.

## Small output and finite read

Add a development-only observation session/helper independent of PageOpenSession.
Reuse finite focused owner discovery in `qt_page_owner.h`: retained window/engine,
unique compatible receiver/SceneView, ancestry, document equality, active focus,
visibility and GUI affinity. Preserve its 4096-node/depth24 limits and weak owners.
Require a caller-provided expected document/order with canonical unique nonnil IDs
and an explicit finite page cap; validate 1–256 pages and matching order length.
The cap bounds this research slice, not supported product document size. No fixed
six-page assumption, openPage callable requirement, openOnce, claim or mutator.

Output a distinct `ObservedFacts` record containing observed document/current-page
ID/index, full observed order, and local observation interval/notification generation.
Label it development facts with no change observed during this read. It is not an
atomic snapshot, native/durable revision, native visit, PageObservation or lease.
Use no raw QObject addresses or proprietary source in tracked records. A local
instance/generation is ephemeral observer provenance and cannot survive restoration
as authority. Source IDs in future actual evidence remain private where required.

Install narrow owner/document/scene/lifetime observers before the first baseline
read. A no-argument dirty handler increments a local epoch; structural signals are
invalidation, never success. Require compatible runtime typed metadata and successful
connections. Missing or incompatible required signals/properties/methods refuses.
Use wrapper pageCountChanged(int,int), pageMapChanged(), pageAdded(int), pagesAdded(QList<int>),
pageMoved(int,int), pagesMoved(), pagesRemoved(), redirectionPageMapChanged(), plus
conservative pageUpdated/documentMetadataChanged/orientationChanged. Use scene
pageIdChanged/documentWrapperChanged/workerChanged/viewportChanged and existing
owner/focus/lifetime notifications. Do not decode worker job/PageMap custom types.
Topology discovery may read getters before connections; the accepted baseline is
taken only after all required connections and retained owner/context revalidation.

One bounded read snapshots local epoch, document ID via the tested String(document.id)
conversion, receiver current index/currentPageId, scene pageId and document pageCount.
Read every idForPage(i) and pageForId(id) once under the explicit cap; require unique
canonical IDs, exact expected order and forward/reverse index consistency. Require
all current aliases/index mappings to agree. Reread owner/current/count and local
epoch before returning. Any observed mutation, unexpected value, owner/lifetime/
context change, cancellation or deadline refusal ends the session; no repeated scan
to obtain a favorable result. Redirection changes invalidate; this first record does
not claim a verified redirection map or replace Reader's mapping contract.

Keep nested native-call depth and deferred teardown until the outer call unwinds;
retain weak owner checks and post-getter deadline/cancellation checks. Getter calls
or conversions can pump events. Check progress after each getter/mapping operation,
not only before it, including the final reread/return boundary. The caller supplies
one finite monotonic budget; tests cover exact boundary/late returns. Post-return
checks do not preempt a hung native getter. A later native access recipe still needs
independent process recovery; fixture deadlines are not that recovery proof.

## Facts do not establish authority

Saved wrapper/scene metadata establishes availability, not signal ordering or
completeness. Queued cross-thread delivery can miss a worker mutation/ABA during
the read. A zero local dirty count cannot certify zero worker changes. No inspected
durable native visit or monotonic order/content revision was established; persisted
lastOpened/lastModified and local counters are not substitutes. Qualifying atomic
serialization, retained input, native operation correlation and render ownership
remains separate active work. No stale guard may be reset using these facts.

Actual insertion auto-selection remains unknown. The saved native UI call site
opens a page in its caller callback; that does not establish controller insertion
auto-selection. The six-argument caller-supplied page UUID and worker job correlation
leads remain unqualified and are unnecessary for this read-only slice.

## Owned fixture acceptance

Use original synthetic QObject/QML fixtures only. Cover one valid owner with varying
1/6/256-page orders, invalid limits/duplicate or malformed IDs, zero/multiple owners,
wrong window/engine/document/focus, missing/incompatible signals, count/order and
forward/reverse mismatch, current alias disagreement, dirty signals during reads,
observed away/back, document/scene destruction, nested getter reentry, cancellation,
post-getter expiry and final-boundary invalidation. Verify zero native mutation calls
and no openPage dependency. A deliberately silent/unobserved ABA demonstrates the
facts limitation; it must never be promoted to atomic or native authority.

Independent semantic review and owned fixture checks precede any exact native
artifact/access/recovery proposal. No new daemon, direct-open/setup guard, page
insertion or actual tablet operation is part of this implementation scope.

## Development implementation checkpoint

`tools/qt_page_facts.h/.cpp` implements the separate, single-use reader. Its only
output is local `ObservedFacts`; atomic/native/render authority flags remain false.
The helper never requires or calls openPage and has no mutation entry. Required
runtime signals use the exact signatures above, including no-argument pagesMoved()
and pagesRemoved(). Connections precede the accepted baseline. Every getter and
conversion is followed by owner/context, cancellation and deadline checks. One
bounded full mapping read plus final identity/count/index/alias rereads either
produces limited facts or refuses; failure is never retried for a favorable scan.

Completion is queued only after the outermost native/getter/read stack unwinds.
The queued delivery checks local epoch, weak lifetime, thread affinity, progress
and budget again without obtaining fresh native values. Reentry or late invalidation
discards facts. The caller retains this parentless session until completion; the
callback/result are moved to local storage so the callback may release the session.

`tools/qt_page_facts_fixture.cpp` and `tools/qt_page_facts_test.sh` contain original
owned Qt fixtures and a fixture-only build/run recipe. No startup/native probe SO,
device payload or access/recovery recipe is produced. Fixtures cover finite varied
orders, malformed configuration, missing/incompatible metadata, owner ambiguity and
context mismatch, exact dirty signals, mapping/value disagreements, getter expiry/
cancellation, nested reentry/teardown and delivery-boundary loss. The silent A-B-A
fixture intentionally passes only as limited, non-atomic facts, demonstrating the
unobserved mutation limitation rather than qualifying a native snapshot.

Author validation: all 63 owned fixtures pass using pinned image
`sha256:416c7a7be0038156797b0892f031f352b841d1921fae83f712d0a272e4724618`,
network disabled, source mounted read-only, vendor Qt SDK 5.8.203, ARM compiler
with `-Wall -Wextra -Werror` and qemu/offscreen execution. Final cases assert both
getter-side effects and the specific queued-delivery refusal boundary so an earlier
failure cannot masquerade as that test. Strict validation of this OpenSpec change
also passes. These are owned fixture/source checks, not actual native/model proof.
Independent review and reproduction remain pending against the frozen revision.
The wider native platform change remains unfinished; no
canonical spec sync, archive, integration or product authority follows from this slice.

Source basis: accepted SDKf6b7dc8/consumer Docs9c5f302 selected route, existing
owner/open observation source, Main's current scope, and Astra's bounded saved-
evidence assessment. Static wrapper/scene metadata hash03c8fcefccc1f7e13971bc2f60b3fcbf368b4dd853dc90e3ef9f4eb72feb955c
was independently rechecked locally; private evidence has no public transcript link.
Proposed reuse/output is design, not new hardware qualification.
