# Development insertion after ordinary document opening

October 9, 2026. Source-only proposal for the next REM-25 question. No new
implementation, artifact, nonce or device action is selected by this document.

## Evidence gap

The [completed caller-UUID trial](../../../docs/experiments/caller-selected-insertion-result.md)
does not establish insertion while the intended PDF was already open in the
running interface. Its before capture belongs to the old stock process. The
configured creation Probe ran at replacement-process startup and dispatched after
Library readiness: readiness accepted at 12608 ms, mutation attempted at 12610 ms.
The actual receipt has `access_anchor=library-ready-observation`, zero inspected
document-view candidates and `source_authority=false`. Post-restoration reopening
does not supply missing call-time evidence. Main and the SDK author independently
reached this conclusion from the retained receipts and exact entry source.

## Smallest distinct operation

Retain the original creation entry, existing-engine acquisition, creation bridge,
single GUI-thread native call, callback and independent recovery. Add an explicit
default-off development creation admission gate before `queueCreation`. Reuse the
existing private one-shot permission mechanics in `qt_page_open_arm.h` through a
small fixed creation protocol; do not invoke page-open configuration or an opening
method, and do not revive deferred native opening or diagnostic entry modules.

The selected creation protocol must use distinct `creation-waiting` and
`creation-arm` files bound to the fresh nonce, candidate PID/start and owned private
directory. Preexisting/malformed/stale tokens, directory replacement, restoration
claim, cancellation and expiry refuse without a mutation. A valid token queues
one GUI-thread continuation, which rechecks original engine/thread/object context,
Library readiness, complete page count and forward/reverse order, target absence
and the one-shot mutation claim. No same-token retry or delayed replay is allowed.

Main ordinarily opens the exact fixture in the candidate before publishing the
token. Fresh source-page pixels, observed current metadata, stable candidate
identity and idle input are development operator admission evidence; they are not
an atomic SDK active-page guard. Any intervening navigation, ambiguous page,
input or process change invalidates selection. The final implementation must state
what context it checks at dispatch and must not advertise metadata or a screenshot
as qualified native active-page authority.

The new explicit gated configuration accepts the fresh seven-page baseline, with
the existing stroked page preserved as nonblank. Legacy omitted-target five-page
and prior explicit-target six-page modes retain their behavior. Use one new target
UUID, index 1 and the existing inherited template/size. All prior operation
identities remain spent. The full current inventory now includes the previously
inserted page file and thumbnail: sixteen document files, of which fourteen are
immutable and only exact content/metadata are mutable, plus ten runtime providers.
Main must verify that 26-path inventory afresh; the old exact-24 preparer cannot
silently omit the two new immutable files or be reused unchanged.

## What distinguishes a useful result

Before candidate stop, collect the native return/callback, exact new order and
target identity, actual displayed page and corresponding capture while the same
candidate PID/start remains alive. Do not require the new page to auto-select:
record actual source/target/other/unknown behavior without inventing authority or
navigating on an unverified guess. This tests insertion/cache/UI behavior in an
already-open document. Later stock restoration and preservation remain mandatory,
but a result known only after restart would not answer this selected question.

One initial experimental preload startup is distinct from a restart to perform
the triggered insertion. Even a positive result does not establish reusable
production activation, repeated requests, atomic compare-and-act, shutdown safety
or a restart-free product implementation. Existing UUID and ink persistence
findings need not be repeated as a separate test matrix.

## Timing and source gates

Main's review of consumer b744937 confirms the old actor cannot run this test
unchanged: its pre-start reserve is 195 seconds and its candidate cutoff is the
minimum of start plus 30 seconds and absolute deadline minus 185. At the reserve
boundary this leaves only ten seconds; the prior actual Library readiness took
12.608 seconds before ordinary opening or inspection.

For a distinct explicit after-open mode, require at least 255 seconds remaining
before candidate start. Reserve 70 seconds for candidate work and 185 for recovery.
The fixed candidate cutoff is `min(candidate_start + 70, absolute_deadline - 185)`.
The original absolute deadline remains 360 seconds from actor arming. Failure to
meet the start reserve refuses startup and restores stock; it never shortens
verification to fit or extends the deadline.

Proposed stage caps within those 70 seconds are readiness 20, ordinary opening plus
pixels/metadata/order/idle admission 20, trigger 5, post-call result/capture 15,
stop publication 5 and margin 5. These are feasibility budgets, not measured timing
guarantees. Publish creation-arm only with at least 215 seconds left on the original
clock (30 post-trigger plus 185 recovery), before the independent candidate cutoff
and with no stop/restoration marker. Waiting/ready/arm events never renew a cutoff.
Expose candidate PID/start, waiting state and absolute stage expiry to the host;
no chat roundtrip may be required during the active interval. Host loss still
reaches independent actor recovery. Pre-stop observations must bind the same PID
and start time and retain actual source/target/other/unknown UI state.

The consumer partition needs a separate exact-26 mode rather than loosening its
legacy exact-24 contract. Require seven page IDs, all seven thumbnails, the four
known existing ink IDs (original source, the stroked prior target, and the other
two existing ink pages), PDF/local/pagedata/content/metadata, and ten exact runtime
providers. Reject a missing stroked page/thumbnail or the old 24-file selection.
Fresh configuration binds the actual ink-ID set; page order alone is not an
assumption that all pages already have ink files.

Main has reviewed this source-only feasibility plan. Implementation remains pending
in the SDK gate and consumer actor/coordinator/preparer. Focused timing fixtures
must exercise valid admission, start/arm boundary expiry and recovery after
admission, preserving old mode behavior. Artifact selection and a new usage/reserve
assessment follow source verification; no real run is selected by this review.

Only focused checks of valid single admission, stale/duplicate/closed admission,
seven-page configuration and unchanged legacy behavior are proposed. Exact source,
artifact/configuration, expanded preservation partition and consumer timing review
precede any new device selection. At the current usage checkpoint, implementation
and a real run remain pending; preserve enough allowance for cleanup and handoff.

Source basis: current REM-25 requirements, exact SDK creation/permission source,
saved 026e74 receipts and before/after process chronology, subsequent writability
archives and Main's independent operability assessment. The new trigger and its
outcomes are proposed, not observed native behavior.
