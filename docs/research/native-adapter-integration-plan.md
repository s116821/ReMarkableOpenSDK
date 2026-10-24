# From native diagnostics to the resident adapter

October 24, 2026 UTC. This is an integration plan, not an implemented transport
or supported native capability. Candidate-specific native findings now include
[conditional insertion and full post-order](../experiments/conditional-insertion-native-result.md),
[actual adjacent target selection](../experiments/conditional-target-acquisition-native-result.md),
[mutation retirement followed by a fresh read-only request](../experiments/resident-mutation-readback-native-result.md),
and [a complete intended-target write, stock close and reopened capture](../experiments/combined-native-write-close-reopen-result.md).
That later candidate completed inside its original window and preserved stored
target ink after restoration. The earlier expired sequence and separate
readback remain distinct historical results. Actual selected page and saved
remembered position diverged in the successful case, so metadata cannot replace
live-page observation or qualify default reopening. A later
[controlled writer death during contact](../experiments/controlled-writer-failure-native-result.md)
qualifies independent release for one native failure, with conservative Buddy
withholding and separately verified restoration. Broader failure, lifetime,
consumer and model/firmware qualification remain open.

## Stock-first product boundary

Buddy integrates with xochitl's existing pages, drawing, navigation and saving.
This work does not introduce a replacement tablet interface or migrate the
tablet to another UI framework. Retained conversation pages must remain usable
as ordinary stock pages after Buddy is stopped or uninstalled. Optional removal
of those pages is a separate product decision. Vendor module names such as
legacydevice do not imply an architectural goal to replace the stock UI.

Using stock behavior avoids maintaining a parallel tablet UI. It does not make
private object names, method signatures, provider ABI or runtime ordering stable
across firmware updates. Keep versioned profiles and real-device compatibility
checks; do not promise automatic compatibility from this design alone.

## Candidate boundaries

These are preserved consumer PR heads and the executed SDK checkpoint. The SDK
checkpoint identifies native test source; it is not the research PR's later
documentation/test-publication head.

| Candidate | Reviewed revision | Integration ownership |
| --- | --- | --- |
| [SDK research PR 30](https://github.com/s116821/ReMarkableOpenSDK/pull/30) | complete combined native sequence source `37c4a8516adb245d06d259e879fc3f595439691b` | Native mechanisms, observed facts, capability and lifetime contract |
| [Buddy PR 60](https://github.com/s116821/ReMarkableBuddies/pull/60) | `fe7463993b93dedaa57fc80a7fd3f45adc5e084f` | Bindings, acquisition decisions, operation journal, supervisor and consumer tests |
| [Docs PR 56](https://github.com/s116821/ReMarkableBuddiesDocs/pull/56) | `7c4c7e264617372d281a9b47ef508c927e3d9759` | Product acquisition, recovery and acceptance requirements |

These revisions locate preserved work; they do not validate unchanged consumers
against a future SDK implementation. The coordinating integration task must
review new exact heads, pin the SDK revision in Buddy, and record the resulting
consumer/test revisions before delivery. Keep the existing PRs and dirty work.

The Rust [Platform contract](../../src/lib.rs) currently has only Synthetic
evidence construction and `UnqualifiedPlatform` returns NoNativeAdapter. Its
`PageObservation` includes full order, revision and input epoch. Later native
diagnostics supply a full observed order, superseding the earlier three-ID-only
limit, but not a qualified public revision/input epoch. Do not construct that
type by filling the missing facts with constants,
borrow its mock constructor, or mark the synthetic origin as native.

## Ordered implementation

1. **Integrate the measured native precondition.** The conditional insertion
   candidate checks source/count/immediate-neighbor/target absence in the
   qualified original insertion scope and forwards the ordinary helper once
   with unchanged arguments. Preserve that exact execution boundary and native
   Task bookkeeping; the old collection of separately locked getters does not
   become equivalent. Port the measured predicate into the versioned adapter
   and qualify other-writer interference, stale-source rejection, replacement,
   notebook/PDF middle/end and renderer-error cases. Retain the
   [earlier boundary analysis](execution-time-page-order-boundary.md) as historical
   rationale, with the later native reports as the current positive finding.
2. **Separate facts from authority.** Move reviewed native ownership/observation
   mechanisms into a versioned adapter component, retaining exact executable,
   Qt/ABI, artifact and operation fingerprint gates. Define native construction
   of the public observation/receipt only after every required fact has a real
   producer. Otherwise return a smaller explicitly partial diagnostic fact or
   Unsupported. Qualification for one RM2 build does not qualify another model.
3. **Implement resident transport and handshake.** A proposed route is an
   owner-only local Unix socket, asynchronous on the GUI side, with bounded
   length-prefixed typed control frames. The final transport/serialization is
   still to be selected and tested. Handshake binds protocol/build identity,
   executable fingerprint and boot/PID/start incarnation plus a fresh connection
   generation. Requests carry operation ID, request generation, expected
   document/page and applicable native preconditions; replies echo all identity
   fields. No QObject pointers or native addresses cross into Buddy. Reject old
   connections and stale/duplicate frames before dispatch. The current atomic
   request/result files are forensic tooling, not this production protocol.
4. **Keep the adapter resident across conversations.** Buddy's independent
   supervisor owns activation, health and exceptional recovery; the native
   adapter lives with xochitl. Logical request retirement disconnects/retires
   that request's observation/publication authority. It does not unload the
   adapter or restart xochitl. Prove physical callback completion and subsequent
   owner reclamation separately. The research's intentionally retained canceled
   slot is not a production capacity/reclamation solution. The native mutation
   followed by a fresh read-only request now demonstrates this separation in
   one unchanged process; it does not prove indefinite capacity or safe dlclose.
   The historical shutdown rendering fault remains unexplained. Process restart
   in forensic recovery is not normal conversation lifecycle.
5. **Integrate Buddy decisions.** Resolve a correctly bound current Buddy first,
   then the immediate right Buddy when needed. An unknown/unbound neighbor is
   not positively ordinary. If the right neighbor is ordinary, preserve the
   original source anchor and insert before that neighbor. Avoid a whole-document
   Buddy inventory. Verify actual selected target; do not navigate if already
   there, otherwise use one qualified adjacent transition. Actual insertion can
   update remembered metadata while the active stock Reader stays on the source;
   the later combined sequence observed and wrote the target while remembered
   metadata stayed at the prior page. A completion callback or remembered index
   cannot skip the fresh join. Keep
   direct page opening outside the MVP. Identity-bracket capture/write and verify
   the result. Do not turn the diagnostic's fixed stroke into a general renderer.
6. **Reconcile ambiguous outcomes.** Persist operation ID, source and requested
   target identity before dispatch. A timeout, cancellation, lost connection or
   helper death after dispatch does not prove absence of mutation. Reconnect
   read-only and reconcile the original operation/target before another mutation;
   do not generate another UUID and retry. Source change rejects without mutation
   only when the qualified execution-side predicate establishes that result.

## Required qualification and delivery

| Boundary | Required evidence |
| --- | --- |
| Transport | Framing limits, malformed/partial/duplicate/stale frames, reconnect, peer/process generation and handshake mismatch |
| Resident lifetime | Many ordinary requests without xochitl restart; parent cancellation, timeout, late/nested delivery, document/window replacement, bounded retained capacity and separately proven reclamation |
| Automatic create-after | Disposable PDF and notebook, middle/end; exact source/target IDs and immediate-neighbor order, concurrent/stale source rejection at execution |
| Reuse | Current Buddy and immediate-right Buddy; unknown/unbound handling; duplicate operation produces no duplicate page |
| Preservation | Original full page objects, PDF mapping, ink/highlights, unknown metadata and complete inventory; differences explicitly reconciled |
| Target and write | Actual target selection, intended blank page, identity-bracketed writing, navigation away/back, reopen and persistence |
| Failure | Interruption/cancel/ambiguous completion, consumer/supervisor failure, independent recovery and actual stock policy/process verification |
| Consumer flows | Blank-start/Writer and relevant simulator coverage; UI capability follows exact native qualification |

Keep capability queries observational: they must not activate the native runtime.
Publish the supported model/firmware/runtime/artifact matrix only for measured
profiles. Capture/render/persistence and creation have distinct evidence gates.
Integrate SDK before its pinned consumer revision, then complete coordinated
acceptance, specification sync and archive. Preserve unfinished gates rather
than relabeling a passing diagnostic as release readiness.

The complete normal sequence provides positive input/release, all three original
identity-bracketed captures, stock close/reopen and stored target ink after
restoration. Earlier expired trials remain failures; four rendered stroke pixels
still differ on reload. Controlled native writer death also demonstrates actual
independent release, while the actor's missing positive witness withholds Buddy.
The separately verified reconciliation restores it without input. Implement and
qualify production recovery policy around these distinct facts rather than
accepting the negative writer result as normal success. Do not infer power-loss
durability, default remembered-page reopening or pixel equality.
The [simulator evidence map](../reference/simulator-evidence-map.md) and generated
catalog index measured transitions for future consumer simulation; focused Qt
and owned-file controls are not an implemented full-tablet simulator.

Source basis: current SDK source and active native-contract specification,
retained native reports, explicit stock-first product direction, product
acceptance requirements and freshly queried GitHub heads. The stock-first
compatibility benefit is an engineering expectation with the limits above.
Transport framing, handshake and production implementation sequence are proposed
design; they have not run as a production adapter on the tablet.
