# Finite observation-only facts slice

This is a source/fixture proposal for independent review before implementation.
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
Use wrapper pageCountChanged(int,int), pageMapChanged(), pageAdded(int), pagesAdded,
pageMoved(int,int), pagesMoved(), pagesRemoved(), redirectionPageMapChanged(), plus
conservative pageUpdated/documentMetadataChanged/orientationChanged. Use scene
pageIdChanged/documentWrapperChanged/workerChanged/viewportChanged and existing
owner/focus/lifetime notifications. Do not decode worker job/PageMap custom types.

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

Source basis: accepted SDKf6b7dc8/consumer Docs9c5f302 selected route, existing
owner/open observation source, Main's current scope, and Astra's bounded saved-
evidence assessment. Static wrapper/scene metadata hash03c8fcefccc1f7e13971bc2f60b3fcbf368b4dd853dc90e3ef9f4eb72feb955c
was independently rechecked locally; private evidence has no public transcript link.
Proposed reuse/output is design, not new hardware qualification.
