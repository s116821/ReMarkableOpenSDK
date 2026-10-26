## 1. Protocol and observation contract
- [x] 1.1 Define a feature-gated bounded canonical frame schema and historical identity/order facts without invented revision/input epochs.
- [x] 1.2 Implement client handshake/request correlation, monotonic request sequences and sticky indeterminate mutation loss with no automatic replay.
- [x] 1.3 Verify frame/schema faults, fragmentation, reconnect/stale replies, sequence exhaustion and actual Rust/Qt endpoint interoperability; preserve dependency-free defaults and package containment.
  - [x] 1.3a Host protocol suite: 13 integration tests plus sequence-exhaustion unit regression; default/all-feature builds, both ordinary consumer toolchains and package checks pass.
  - [x] 1.3b Actual Rust/Qt local endpoint interoperability, peer authentication and Qt-side stale-request rejection; owned endpoint evidence, not native Reader qualification.
  - [x] 1.3c Add the production Linux client transport with kernel peer/process authentication, pinned directory/socket identity, absolute IO deadlines and journal-before-write; verify owned socket faults, Qt interoperability, MSRV and package boundaries. Consumer journal/lifecycle integration remains 3.1.
- [ ] 1.4 Independently review the final exact implementation and record source/build evidence separately from native qualification.
  - [x] 1.4a Independent source review accepts the protocol foundation after capacity and sticky-ambiguity fixes; [checkpoint](verification.md). Final endpoint/native/consumer review remains open.
  - [x] 1.4b Independent endpoint review completed after connection-incarnation, pinned-inode and post-encoding deadline fixes. Native executor/consumer review remains open.
  - [x] 1.4c Fix and independently source-review atomic listening-socket publication after consumer CI exposed bind/listen startup failure; final host and ARM/QEMU endpoint suites pass 1825 checks each under endpoint-publication.md. Rebuilt adapter and consumer exact-revision CI/native qualification remain required.

## 2. Resident native executor
- [x] 2.1 Implement the owner-checked asynchronous Unix endpoint and exact runtime/profile handshake (source/build scope; native activation remains 2.2b).
  - [x] 2.1a Implement bounded Linux/Qt framing, owner/peer process checks, fresh server nonce and socket-incarnation/deadline guards.
  - [x] 2.1b Bind the endpoint to independently verified profile/executable/adapter artifacts in the native bootstrap; add one-use supervisor admission and an ARM build with pinned Dobby inputs.
- [ ] 2.2 Integrate the measured Reader ownership observation and execution-side insertion predicate with unchanged native Task bookkeeping.
  - [x] 2.2a Import the exact handoff mechanisms with original Git-blob provenance; add guarded full-order observation and Observe/Reconcile backend source with host/ARM owned coverage.
  - [x] 2.2c Connect authenticated mutation requests to the executor with original identity/deadline, conservative reply mapping and facade-independent cleanup; qualify the mapping and endpoint boundary on owned host/ARM tests before native activation.
  - [ ] 2.2b Connect and qualify actual native observation through the admitted resident bootstrap; integrate conditional insertion with its original Task bookkeeping.
- [ ] 2.3 Implement separately verified physical callback completion and bounded owner reclamation, including canceled/late/nested delivery and repeated operations in one process.
  - [ ] 2.3a Resolve safe native last-owner release separately from adapter-stack quiescence; bind all reusable registration APIs to their original generation and exact submitted Task lifecycle.
  - [x] 2.3b Implement and independently review a bounded non-last-only decrement primitive and private exact-control identity gate; verify owned host/ARM controls. This does not release native owners or qualify reusable capacity.
  - [ ] 2.3c Connect a typed retained-owner slot with per-owner consumption, no ordinary-release fallback, generation-bound physical retirement and native repeated-operation qualification.
    - [x] 2.3c1 Implement the single retained acquisition slot and opaque stale-safe tokens; verify partial/unknown acquisition, per-owner consumption and refusal retention on owned host/ARM fixtures. Executor wiring is tracked below.
    - [ ] 2.3c2 Connect exact Task/callback retirement and reusable Core/Coordinator registration to the slot, then qualify native capacity and repeated operations.
      - [x] 2.3c2a Bind Core registration, cancellation, snapshots and private retirement to opaque request tokens; adapt Coordinator token retention and verify owned normal/unwind/active/stale/foreign cases. Executor wiring is tracked below.
      - [x] 2.3c2b Import the handoff's GUI retirement fence and request-scope binding with source provenance; verify nested cleanup, stale scopes and real Qt/JS callback disarm on host/ARM. Executor wiring is tracked below.
      - [x] 2.3c2c Route Coordinator acquisition and probe submission through the retained slot, preserve its token across facade deletion and remove per-request ordinary-release fallback. Verify the real connection with owned ARM ABI callouts. Physical cleanup remains open.
      - [x] 2.3c2d Bind one-shot attribution creation to the owner-slot token and add private adapter-context cleanup before owner consumption; quarantine uncertain state across all later admissions. Executor ordering is tracked below; native validation remains open.
      - [x] 2.3c2e Implement the resident executor's staged GUI/Core/attribution/owner retirement and bounded quarantine; independently review its lifecycle and test injected settled states on host/ARM. Full Coordinator-to-executor coverage is tracked below; native activation remains open.
      - [x] 2.3c2f Exercise real Coordinator start, Qt worker/witness delivery, framed deletion, Core/attribution/slot retirement and reuse together with explicit native callout models; preserve uncertain quarantine while recovering capacity after known pre-acquisition refusals. Endpoint/native qualification remains open.
- [ ] 2.4 Integrate read-only reconciliation and fresh conditional adjacent target acquisition; retain unknown outcomes without mutation replay.
  - [x] 2.4a1 Add revision 2 selection/reconciliation framing, sticky effect loss, Linux/Qt transport coverage and the guarded existing-target inner call with owned host/ARM checks. This does not enable the native executor.
  - [x] 2.4a2 Integrate a process-shared effect reservation with mutation retirement, retaining unknown work across executors and releasing only after proven no-effect/reclamation plus completion capture destruction; verify owned host/ARM cases. Selection executor integration remains open.
  - [x] 2.4a3 Implement the resident selection request state machine and disabled production route: original deadline, zero/one adjacent call, same-object fresh postjoin, sticky reentrant/final-join refusal and shared effect retention. Owned source/model validation only; native settlement and consumer composition remain open.
  - [ ] 2.4a Implement the separate fresh target-selection request, native executor and owned/transport checks under [target-acquisition.md](target-acquisition.md); integrate its consumer reservation before enabling effects.

## 3. Consumer and delivery
- [x] 3.0 Preserve canonical v1/v2 historical journal bytes/digests with a separate read-only decoder and strict live v2 transport; verify old reservations survive consumer upgrade under [historical-journal.md](historical-journal.md).
  - [x] 3.0a Add and independently review the bounded SDK historical decoder, verify retained original v1 vectors and strict live v2 behavior, MSRV/default/feature/package boundaries. Consumer upgrade remains open.
- [ ] 3.1 Pin the SDK in the existing Buddy/Docs candidates and integrate bindings, journal, writer/capture and Supervisor recovery.
  - [x] 3.1a Add and review caller-thread progress during bounded handshake/request IO under transport-progress.md; verify cancellation before/after durable reservation, stalled/fragmented peers, final response admission and unchanged deadline with real owned sockets. Source review, stable/MSRV tests, Qt interoperability and strict lint pass; consumer wiring remains below.
  - [ ] 3.1b Pin the progress-aware SDK and connect Buddy handshake/request callbacks, including actual journal and startup cancellation behavior, before first-gesture integration.
- [ ] 3.2 Complete disposable PDF/notebook middle/end, reuse/no-duplicate, preservation, interruption, write/reopen and blank-start/Writer acceptance with appropriate simulation and exact native evidence.
- [ ] 3.3 Publish only qualified profile/capability results, pass exact-source CI/review, sync implemented specs and archive the completed coordinated change.
