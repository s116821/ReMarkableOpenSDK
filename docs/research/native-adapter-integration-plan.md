# From native diagnostics to the resident adapter

October 23, 2026 UTC. This is an integration plan, not an implemented transport
or supported native capability. The research has separately observed a selected
stock page/controller join, ordinary idle-fixture creation and adjacent selection,
normal worker completion/release, two resident logical observations, and
[cancellation with retained owners](../experiments/worker-running-cancellation-result.md).
The [execution-time source/order predicate](execution-time-page-order-boundary.md)
and broader failure/consumer qualification remain open.

## Candidate boundaries

GitHub currently reports these open draft consumer heads:

| Candidate | Reviewed revision | Integration ownership |
| --- | --- | --- |
| [SDK research PR 30](https://github.com/s116821/ReMarkableOpenSDK/pull/30) | native cancellation source `031959d6c3ebfd3839dfb62a84b6f945702cbc6f` | Native mechanisms, observed facts, capability and lifetime contract |
| [Buddy PR 60](https://github.com/s116821/ReMarkableBuddies/pull/60) | `fe7463993b93dedaa57fc80a7fd3f45adc5e084f` | Bindings, acquisition decisions, operation journal, supervisor and consumer tests |
| [Docs PR 56](https://github.com/s116821/ReMarkableBuddiesDocs/pull/56) | `7c4c7e264617372d281a9b47ef508c927e3d9759` | Product acquisition, recovery and acceptance requirements |

These revisions locate preserved work; they do not validate unchanged consumers
against a future SDK implementation. The coordinating integration task must
review new exact heads, pin the SDK revision in Buddy, and record the resulting
consumer/test revisions before delivery. Keep the existing PRs and dirty work.

The Rust [Platform contract](../../src/lib.rs) currently has only Synthetic
evidence construction and `UnqualifiedPlatform` returns NoNativeAdapter. Its
`PageObservation` includes full order, revision and input epoch. The fixed native
three-ID witness supplies neither a full order nor a qualified revision/input
epoch. Do not construct that type by filling the missing facts with constants,
borrow its mock constructor, or mark the synthetic origin as native.

## Ordered implementation

1. **Finish the native precondition.** Establish an execution-side predicate
   between dequeue and mutation, with the ordinary Task's bookkeeping and
   persistence path intact. Determine the actual renderer initialization/error
   and replacement ordering. GUI-ready snapshots, a worker callback followed by
   another enqueue, six separately locked getters and an externally held
   nonrecursive mutex do not provide that predicate. If exhaustive investigation
   leaves no robust method, return the evidence to the product owner for the
   explicitly exercised manual-successor fallback decision.
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
   slot is not a production capacity/reclamation solution.
5. **Integrate Buddy decisions.** Resolve a correctly bound current Buddy first,
   then the immediate right Buddy when needed. An unknown/unbound neighbor is
   not positively ordinary. If the right neighbor is ordinary, preserve the
   original source anchor and insert before that neighbor. Avoid a whole-document
   Buddy inventory. Verify actual selected target; do not navigate if already
   there, otherwise use one qualified adjacent transition. Keep direct page
   opening outside the MVP. Identity-bracket capture/write and verify the result.
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

Source basis: current SDK source and active native-contract specification,
retained native reports, product acceptance requirements and freshly queried
GitHub consumer heads. Transport framing, handshake and implementation sequence
are proposed design; they have not run on the tablet.
