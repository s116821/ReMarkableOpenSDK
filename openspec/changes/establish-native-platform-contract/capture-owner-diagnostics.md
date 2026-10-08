# Bounded first-failure evidence for development capture ownership

Status: proposal only, against SDK `f0e6ff4ccb37b887f7f278b0b820a7d047de1559`.
Astra owns this specification; Sol owns ordinary implementation and consumer
integration; Main owns artifact selection, device operation and canonical sync.
This selects no implementation, artifact, retry, debugger or device action.

## Evidence and scope

The retained October 7 attempt produced `capture-observation-owner-refused` with
both callback thread flags true and no capture completion. Source inspection shows
that this stage combines initial progress, discovery, observer installation and
final owner validation. The flags are sampled at later callback publication.
Neither the failed subcondition nor native render behavior is established.
Source basis: exact SDK source and private callback/operator receipts read back
by Astra; private receipts have no public link. No proprietary source is required
to understand or implement this proposal.

Add diagnostic evidence only inside the existing default-off
`developmentCaptureObservation` setup120000/facts5000 path. No additional mode,
request, watcher, topology scope or clock origin. Facts/input modes remain unchanged.
Keep the original `capture-observation-owner-refused` callback stage and its exact
four-field callback schema. Capture completion version1 and facts schemas remain
unchanged. Failure remains terminal and the attempt remains spent.

## First original evaluation, not reconstructed state

Split the existing short-circuit expression in `captureOwnerWindow` into named
steps with exactly the original order and number of predicate/getter evaluations.
A fixed-size in-memory diagnostic accumulator is local to this one invocation.
Optional diagnostic sinks on shared owner helpers default to null; other callers
receive the same return values and behavior. Do not invoke an observer, getter,
traversal, activeOwner or progress predicate again to explain its result.

Latch exactly one terminal `branch`:

| branch | Original operation which failed |
| --- | --- |
| `initial-progress` | First progress call before discovery |
| `owner-discovery` | Single findPageOwner call returned other than open-owner-observed |
| `observer-install` | First unsuccessful required observer installation |
| `owner-revalidation` | Existing captureAllowed call after observer installation |

The terminal branch cannot be overwritten by later signal delivery, closure,
timeout, diagnostic publication or callback sampling. Helper diagnostics collected
inside discovery are bounded explanatory observations, not additional terminal
failures: some candidate pairs may be rejected before a valid pair is found.
Clear unused helper fields when latching a branch that did not use them.

Retain the actual `findPageOwner` return string as a finite enum, unchanged:
`open-engine-thread`, `open-current-window-unavailable`, `open-context-lost`,
`open-item-lost`, `open-topology-bound`, `open-candidate-bound`,
`open-owner-ambiguous`, `open-owner-unavailable`, `open-owner-observed`.
Use null if discovery was not evaluated. Do not replace these with guessed causes.

For progress, retain its first failed existing subcondition:
`context`, `live`, `invalidated`, `token`, `deadline`. For captureAllowed retain
the first failing existing step: `reentrant-check`, `invalidated-before`,
`context-before`, `lifetime-before`, `active-owner`, `invalidated-after`,
`context-after`, `lifetime-after`, `owner-pointers`, `owner-threads`, `deadline`.
Do not expand context/live/token into extra probes. Preserve captureAllowed's
reentrancy invalidation and all Scope/queued teardown behavior. A discovery-time
progress failure retains its own detail; do not call progress again afterward.

## Finite owner and observer detail

In the existing traversal, count visited items, receiver candidates, scene
candidates and matched pairs from existing locals. Keep the current limits:
4096 visited items, depth24, eight candidates per role and refusal on a second
matching pair. Counters may include the one overflow observation which triggers
the existing refusal (4097, nine, or two); they are not new traversal allowances.
Null means not reached, zero means evaluated with none found. Do not enumerate
more objects, collect names/classes/addresses or export the tree.

For `open-owner-unavailable`, retain the first rejected pair in the existing
receiver-major/scene-minor iteration, using zero-based candidate ordinals, and its
first failed predicate. If the receiver document read preceding activeOwner
fails, use `receiver-document`; do not invent an activeOwner call. If no pair was
evaluated, pair and rejection are null. A later rejection cannot replace the
first, even if it appears more informative. This is an explanation of one rejected
pair, not a cause attributed to all candidates.

Inside the original activeOwner evaluation preserve short-circuit order and use
the finite groups `pointers-or-threads`, `window-focus-active-visible`,
`engine-association`, `window-association`, `item-visible-enabled`,
`scene-active-focus`, `receiver-ancestor`, `drawing-area-focused`,
`receiver-document`, `scene-document`, `document-identity`. Capture the failed
group from its original evaluation. Preserve the original combined getter checks,
QPointer validation and traversal bounds; do not evaluate skipped terms. These
groups intentionally avoid claiming a finer failure than the evaluated expression.
Use the same finite reason for an original final activeOwner failure during
discovery or captureAllowed; distinguish it from the first rejected-pair reason.

Observer failure records `observer_role` (`document`, `scene`, `receiver`),
`observer_member` and `observer_failure`. Members are a closed enum of the existing
eleven document signatures, four scene signatures, or four receiver property names
listed in captureObservers at the pinned source:

- Document: `pageCountChanged(int,int)`, `pageMapChanged()`, `pageAdded(int)`,
  `pagesAdded(QList<int>)`, `pageMoved(int,int)`, `pagesMoved()`, `pagesRemoved()`,
  `redirectionPageMapChanged()`, `pageUpdated(int)`, `documentMetadataChanged()`,
  `orientationChanged()`.
- Scene: `pageIdChanged()`, `documentWrapperChanged()`, `workerChanged()`,
  `viewportChanged()`.
- Receiver: `document`, `currentPage`, `currentPageId`, `drawingAreaFocused`.

Serialize those fixed labels,
never a runtime-discovered method signature. Failures are `object-missing`,
`property-missing`, `notify-missing`, `signal-missing`, `slot-missing`,
`return-type`, `connect-failed`, from the original check. The existing typed
window/item/lifetime connections gain no new acceptance check in this change.
Do not read properties to diagnose notify metadata; do not install extra signals.

## Time and fixed private receipt

At the first terminal failure, sample `failure_ms` once from the original entry
clock, before finishing; this is failure-latch time, not proof of the precise
instant an earlier getter failed. Retain the actual existing deadline-comparison
operand as `deadline_check_ms` if that failing evaluation reached it, otherwise
null. Do not force short-circuited deadline evaluation. Preserve capture acceptance
and `effective_deadline_ms = min(120000, capture_accepted_ms + 5000)` from existing
state. No deadline is extended, resampled to pass, or replaced with wall time.

The additive private file is `capture-owner-refusal.json`, at most8192 bytes,
exclusive0600 in the existing held root. Exactly these version1 fields:

`kind`, `version`, `nonce`, `attempt_pid`, `attempt_start`, `root_device`,
`root_inode`, `setup_profile`, `capture_accepted_ms`, `failure_ms`,
`deadline_check_ms`, `effective_deadline_ms`, `branch`, `predicate`,
`discovery_result`, `visited_items`, `receiver_candidates`, `scene_candidates`,
`matched_pairs`, `first_pair_receiver`, `first_pair_scene`,
`first_pair_rejection`, `active_owner_rejection`, `observer_role`,
`observer_member`, `observer_failure`, `native_authority`, `render_authority`,
`ui_acknowledged`.

`kind=development-capture-owner-refusal`, `version=1`, profile remains
`main-dev-facts-120s`; all three authority booleans are false. Identity encoding
matches existing positive decimal strings (maximum20 digits); times are
nonnegative JSON integers from the original clock; unevaluated optional fields
are null. `predicate` uses the progress/allowed enum only, otherwise null. For
discovery it is nonnull only when the original progress call failed. Unknown
enum, extra/missing field, invalid bound or inconsistent branch/field combination
is undecodable diagnostic evidence, never an alternative successful capture.

Publish at most once at the existing deferred completion boundary using only the
latched scalars and existing retained-root/generation/closure checks; do not call
owner predicates or getters for serialization. Because this is failure evidence,
an already expired acquisition deadline does not erase the failure, but grants no
additional execution budget: existing host/recovery timers remain controlling.
No await, timer restart, retry or publication-dependent recovery. Partial/write
failure leaves unknown evidence and cannot replace the original callback stage.
If closure or root/generation loss precludes safe writing, publish no diagnostic.
New-file stale checks apply only to the selected capture mode. Never remove or
overwrite foreign/preexisting evidence to obtain a valid record.

The consumer must explicitly admit this filename, cap/schema, immutable copy/hash
and preservation/cleanup policy before an artifact is selected. It must retain
the original refusal even if diagnostics are absent, malformed, partial or late.
Main's visual-before-facts gate and all old cleanup ownership rules stay intact.
Diagnostic evidence neither satisfies that visual gate nor authorizes another
request, input, capture, render operation or native capability.

## Implementation and acceptance gates

Before implementation, Main and Sol review this exact source contract and the
consumer amendment. Source review must verify original short-circuit evaluation
order, no extra getter/traversal calls, preserved default-off behavior and fixed
caps. Owned fixtures should exercise all four branches, finite discovery failures,
zero candidates versus rejected pair, ambiguity/caps, observer metadata failures,
and original-time expiry. Instrument getter/progress counts to prove no diagnostic
reevaluation; make later state disagree and verify the first record stays fixed.
Cover partial/foreign diagnostic output, closure/generation loss, callback-stage
compatibility, and consumer malformed/absent diagnostic preservation.

Run existing focused capture/entry tests and targeted regression cases after code
exists. Exact source, consumer, artifact and packet review remain separate; no
new native attempt follows from host passes. Capture task5.7 and native owner/
render qualification tasks2.9/4.11 remain unfinished. No canonical sync/archive of
unimplemented requirements.
