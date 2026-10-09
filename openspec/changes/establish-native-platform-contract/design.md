## Context

The proposed [owner/window capture refinement](capture-observation-plan.md) adds
one purpose-isolated development acquisition before facts admission. Bootstrap
readiness, active owner observation and rendered-content provenance are distinct.
The retained owner/epoch brackets one active Qt grab and survives visual review;
the later facts request remains a separate one-shot admission. This does not
establish asynchronous native page-worker readiness or a qualified capture batch.

The prospective private diagnostic is specified in
[input-observation-proposal.md](input-observation-proposal.md): bounded
non-consuming eventFilter records, explicit purpose-isolated end admission and
one separate GUI-thread window image. This adds no facts/native/render capability;
implementation awaits exact SDK/consumer proposal coordination.

This is an active design, not a stable release or implemented interface. The current Buddy DeviceBackend provides a useful seam but mixes product history/status semantics with device operations. At fetched Buddy main `ff8ad75`, native_page explicitly distinguishes persisted last-opened candidates from proof that a page remains on screen; the SDK must preserve that distinction.

SDK owns native facts and mechanisms. Buddy owns conversation identities, binding CAS, source retention, cancellation policy and the local Prepared/NativeCommitted/BindingCommitted/ReconcileRequired journal. Native receipts provide evidence to that journal; they do not constitute a second binding registry.

## Goals / Non-Goals

Goals: one semantic, mockable API; independently useful device primitives; bounded operations; explicit evidence strength; no duplicate creation on ambiguous retries; build-time consumption; exact adapter qualification.

Non-goals: bulk extraction of all Buddy internals, layout/composer/business rules, LLM/provider behavior, sync services, general device administration, firmware recovery, an alternate tablet shell or declaring Paper Pro tested through compilation. Supervised session-scoped XOVI is an eligible integration candidate, not a selected implementation.

## Decisions

### Contract shape and versioning

The initial implementation should provide a Rust library boundary for the current Rust consumer. A C ABI, daemon protocol or additional language bindings are not assumed requirements. The native mechanism may use an auxiliary component only if lifecycle and package evidence justify it; Manager must not acquire a separately versioned SDK runtime for ordinary Buddy use.

Semantic contract revision and adapter implementation/qualification revisions are separate. A release manifest maps target architecture/model, supported firmware/runtime fingerprints, artifact integrity and implemented capability revisions. Unknown capability keys or enum variants are unsupported, never implicit success. Final serialization/FFI is deferred until a real transport requires it.

### Proposed narrow public operations

| Operation | Input / output | Required boundary |
| --- | --- | --- |
| `capabilities` | DeviceProfile and per-operation CapabilityStatus | Read-only; supported includes exact contract and qualification identity, unsupported includes reason |
| `observe_page` | PageObservation or explicit unavailable/ambiguous result | Persistent PageKey plus runtime session, visit, order/content revision and evidence strength |
| `capture` | expected observation, deadline/cancellation -> CapturedFrame | Matching before/after identity/session/visit plus geometry, digest and observation interval; no global mutable last-frame state |
| `navigate` | expected observation, semantic target, deadline/cancellation -> navigation outcome | Source ownership, one guarded attempt and observed target; no menu automation |
| `create_after` | immutable CreationRequest, deadline/cancellation -> CreationOutcome | Mutation-time source/order guard and client-selected target or qualified native-assigned target correlation; no implicit navigation or conversation binding |
| `reconcile_creation` | original request + prior ambiguous evidence -> reconciliation outcome | Observe only; never reissue a creation while outcome is unknown |

Names are proposed API vocabulary, not published stable symbols. Operation-specific unsupported responses remain possible even when other capabilities are qualified on the same device.

### Identity and evidence

`PageKey` contains stable native document and page UUIDs in validated canonical lowercase form. It contains no conversation ID, actor/device identity, current index or mutable title. Copies with duplicated IDs remain an ambiguity for callers; an index is never identity.

`PageObservation` adds an opaque runtime session token, visit token, document order revision/digest, relevant persisted content revision and evidence level. Tokens are comparable only within the adapter/contract and relevant session; they are not portable global clocks. A persisted metadata candidate cannot be promoted to fresh UI ownership without the adapter's qualified observation procedure. Restart, revisit, external input or mismatched owner invalidates the operational lease; callers cannot reset it to recover a lost request.

Every operational observation/guard is also bound to a concrete device connection and adapter-instance identity. Reopening an adapter creates a new instance scope even if the device, document UUIDs and runtime token text happen to match. Foreign-device or previous-instance handles are rejected before dispatch. Receipts retain their original device/adapter provenance as historical evidence; they cannot authorize mutation on a new instance. Reconciliation can examine historical evidence only after explicitly establishing a fresh device/runtime scope and verifying the original operation attribution. Stable PageKey remains free of device/actor identity.

`CapturedFrame` carries immutable bytes or an owned immutable buffer, content digest, encoding/dimensions, normalized viewport/rotation transform, before/after observation and a bounded monotonic capture interval tied to the observation session. Wall-clock timestamps alone do not prove freshness. An image digest identifies pixels, not native page ownership. Callers own durable source retention; the SDK does not write a conversation ledger.

### Creation, retries and receipts

`CreationRequest` contains a caller-generated operation ID and immutable request fingerprint over source PageKey, source observation preconditions, insertion relation, target-allocation policy, template/paper requirements and contract revision. Target allocation has two candidate modes: a client-selected intended UUID, or a native-assigned UUID with a durable atomic mapping from the caller's operation/request identity to the created target. Capability discovery distinguishes these modes and their reconciliation guarantees. A native-assigned mode cannot guess by position, page timestamps or newly seen IDs; its authoritative operation-to-result mapping must survive the relevant interruption/retry cases. Missing client-selected UUID support alone does not rule out all automatic approaches. Consumer journal support for native-assigned allocation requires a coordinated REM-37/Docs amendment; this proposal does not silently change that consumer contract.

Pre-dispatch checks are necessary but insufficient. The adapter must enforce source/session/visit/order/input preconditions at the point native mutation executes, through qualified native compare-and-act/serialization or an equivalent race-closing mechanism. If input or order changes after dispatch but before execution, native execution must reject without mutation when provably not started. A backend unable to enforce the stated preconditions exposes the mutating capability as Unsupported; detecting an invalid after-order cannot repair an already unsafe mutation. Avoid sleeps as proof and unbounded global locks. After a valid mutation begins, timeout/cancellation can still leave its result indeterminate; the post-dispatch result model remains necessary.

Outcomes distinguish `Unsupported`, `RejectedWithoutMutation(reason, stage)`, `CanceledBeforeDispatch`, `Committed(receipt)` and `Indeterminate(evidence)`. Rejection stage distinguishes pre-dispatch from a qualified native executor's proven rejection before mutation after dispatch. Errors after dispatch—including cancellation, timeout, transport loss or helper death—are indeterminate unless native evidence proves a stronger outcome. A generic boolean return, callback or idle task tracker is not a commit receipt. Deadline expiration bounds the caller's wait; it does not prove native cancellation.

The receipt binds operation/request fingerprint, contract and adapter fingerprint, original device/adapter-instance provenance, source precondition, target-allocation policy and observed target UUID, exact observed after-order, persisted revision, observation session/time and evidence procedure. For client-selected allocation, `Committed` requires intended=observed target; for native-assigned allocation, it requires the qualified durable operation-to-target mapping and matching observed target. Both require exactly the permitted insertion. Extra changes, attribution mismatch or insufficient persistence evidence return indeterminate and force reconciliation. Receipt construction must not be publicly forgeable as native evidence by ordinary production callers; mocks use a distinct synthetic evidence origin.

The consumer persists Prepared before dispatch and the receipt before binding CAS. The SDK has no parallel conversation journal. Repeated requests with a known receipt may return that historical receipt plus separately labeled current observation, never recreate or resurrect a removed/reassigned page. If the backend cannot prove no earlier insertion, retries do not dispatch. Idempotency and durable recovery are acceptance obligations, not promises inferred from a caller-supplied UUID.

Native page commit does not grant permission to render. Buddy requires a fresh current guard and successful binding before writing. Automatic deletion to compensate for uncertain creation is prohibited; a user may already have written to that page.

### Native mechanism investigation

Prefer existing IPC/runtime services or independently implementable native contracts with normal boot/use semantics. Inventory first, then inspect narrow interfaces without invoking mutators. The saved RM2 binary contains D-Bus sync/device-policy leads and page-related Qt metadata, but neither proves an out-of-process page API. Passive runtime evidence must come through the sole coordinated tablet owner.

Compare candidates against annotated-PDF preservation, ordering, durability, user input, cancellation, repeated operations, cache/serialization coordination, failure isolation and lifecycle recovery. Direct file mutation while xochitl is unaware is not accepted. The October 1 23:40 UTC decision supersedes the prior blanket XOVI ban: supervised, session-scoped lazy activation is eligible where robust direct seams are insufficient. An out-of-process wrapper is not inherently safer if its underlying mechanism is brittle. Compare the actual mechanisms using [the pinned-source matrix and staged experiment](../../../docs/research/runtime-mechanism-comparison.md); retain Unsupported until qualification.

The SDK owns exact model/firmware/runtime/payload capability fingerprints, explicit activation requirements, bounded readiness/health semantics, runtime generation changes and invalidation, native dispatch guards and recovery outcomes. Capability queries remain observational and never activate an injected runtime. Buddy owns supported gesture recognition, intent retention, the independent Supervisor and its policy/orchestration; Manager owns the unified artifact lifecycle. These consumer requirements remain canonical in ReMarkableBuddiesDocs, with exact revision links added at integration.

If selected, the injected component is a narrow internal semantic adapter loaded only after explicit consumer activation and complete preflight. No automatic cold-boot injection, persistent injection-enabling xochitl/preload/startup modification, arbitrary extension-directory loading, broad QML replacement or UI-dependent recovery is acceptable. Ordinary consumer Supervisor autostart is permitted when boot remains stock and injection still requires an authorized fresh trigger; stale session state must never re-enable it after reboot. A single first-use restart/rebind may be part of qualified activation; subsequent requests must not routinely restart xochitl. A reboot restores the stock non-injected baseline. Readiness must prove the required execution context is responsive, rather than merely that a heartbeat thread exists. Runtime failure, supervisor failure and races during activation/rollback must have independently reachable bounded recovery, with a session latch preventing repeated activation after failure. An external supervisor alone does not contain a loader fault in xochitl.

Every activation restart creates a fresh runtime/adapter scope and invalidates old pointers, observations, guards and pending native dispatch. A retained product intent is not an operational handle. Continue only after the consumer reacquires a qualified source and the SDK validates its new session, identity, visit, order/content/input preconditions and operation eligibility; unknown or changed source refuses continuation. Historical pixels, matching UUIDs alone or successful rebind do not restore authority. If an earlier mutation might have started, retain Indeterminate and reconcile its original operation identity without redispatch. No component may call restart a cancellation proof.

### Tests and releases

Use deterministic mocks of the same contract with injected clocks, input, stale observations and uncertain native outcomes. Synthetic receipts are labeled and must never appear in production qualification manifests. Cross-builds test architecture/linking only. Actual RM2 fixtures establish native support; Paper Pro native coverage remains REM-29 and is reported unqualified until completed.

Package the SDK target into the Buddy build. Independent third-party SDK releases can contain multiple target artifacts. Pin exact dependency/contract revisions and publish checksums/provenance and supported capability matrices. License/publication review precedes distributing any implementation derived from third-party materials.

Target toolchain qualification includes ELF class/machine, ARM floating-point ABI where applicable, interpreter, NEEDED libraries and every versioned symbol requirement against the exact qualified target providers. Record compiler/linker/sysroot provenance. A host distribution's cross-linker can introduce newer glibc imports even when the architecture target is correct. A vendor SDK baseline is useful build evidence but does not alone verify the libraries actually mapped on a tablet. A version command under a different emulator/sysroot cannot override an unresolved provider mismatch.

## Risks / Trade-offs

- Native IPC may not expose creation. Unknown remains unsupported, not evidence that every alternative has failed.
- Persisted metadata can lag the UI; identity/durability require qualified combined observation, not a timestamp heuristic.
- Cooperative deadlines cannot preempt a stalled native call. Isolate execution where needed and retain ambiguity after dispatch.
- Target UUID control may be unavailable. Evaluate native-assigned IDs with durable operation correlation without weakening binding/duplicate guarantees; coordinate the consumer journal change first.
- An SDK abstraction can hide unsafe implementation details without fixing them; adapter evidence and recoverability remain required.

## Migration Plan

### October 3 navigation priority correction

User direction selects SDK logical Next/Previous with qualified per-tablet
gesture implementations; logical direction is distinct from physical swipe and
orientation. Direct native PageKey opening is deferred and is not an MVP gate.
The smallest route is native insertion, fresh active-page/order observation,
zero gestures when already on the intended target or one logical Next from the
verified source when that exact target is adjacent, then fresh target/pixel
verification before consumer binding and guarded output. Native insertion's
automatic selection behavior has not been observed and must not be assumed.

Reuse finite owner topology and native document/index/key checks independently
of invoking openPage. Existing PageOpenSession cannot be reused unchanged: its
helper requires openPage even during observation, begin queues openOnce, and
pre-claim page notifications invalidate its state. An observation/navigation
contract must permit exactly the expected source-to-target transition while
retaining sticky invalidation for unrelated owner/input/order changes. No helper
result, metadata alias or screenshot alone creates a production PageObservation.
See [active route and preserved research](logical-navigation-plan.md).

The SDK owns semantic direction, device/orientation mapping and qualified
navigation observations. Buddy retains product intent, conversation binding and
write policy. Main coordinates integration/device work; ordinary implementation
remains Sol and RM1 qualification remains with its independent owner. No deferred
direct-open setup harness, timer enlargement or injected runtime is a prerequisite
for implementing the gesture abstraction. Native identity and insertion runtime
qualification obligations remain open rather than being waived.

Bootstrap this active SDK plan, agree receipt/identity semantics with REM-37, narrow native research, implement the smallest tested contract/mocks, qualify RM2 mechanism, then move only current Reader/native creation seams behind it. Pair exact SDK and Buddy revisions with the Docs product delta. Old synthetic XOVI experiments remain research evidence only; the new candidate permission does not promote them to native qualification. Sync/archive only after implementation and required native/consumer gates pass.

## Open Questions

### Fixed development contact normalization investigation

Current lifecycle selection is [maintained launch-and-kill](launch-kill-discriminator.md),
which supersedes the attach/custom-release mechanics in the following historical
design. It uses standard launch/kill paths with explicitly limited actual-kernel
qualification, bounded startup plus5s armed measurement inside150s, and no successful
detach. Preserve the measurement contract and paused custom work without claiming
unmodified GNU provides checked-every-LWP enforcement.

The [two-stop discriminator design](input-normalization-discriminator.md) owns the
exact ARM register/layout, same-call correlation, bounded lossless persistence and
debugger-effect contract. It measures actual pre/post fractions inside the function
at0x15528 using PRE0x1563c and POST0x15664, preserving double bits. These private
fingerprint-bound offsets do not enter the public SDK. SDK provides focused tooling;
consumer orchestration owns the fresh candidate, one input and independent stock
recovery. A standard debugger is preferred to a new in-process trampoline; exact
host/SSH/server transport and target recovery remain qualification gates. Existing
native lifetime budgets and unknown historical receipts are not relaxed.

The [disposable-server lifecycle amendment](disposable-debug-server.md) supersedes
the unmodified server choice: attached-process EXITKILL must be checked on every
live thread and reset, partial attach must precede all text writes, and failures
must kill rather than detach. Only an explicitly authorized successful D after
word restoration may release. Exact modified-source/artifact and real-kernel death
qualification are required; an external delayed kill cannot prove no unsafe resume.

Which native service/mechanism can satisfy creation and durable reconciliation without fragile startup modification? Which exact current firmware/runtime fingerprints are qualified? Which existing Reader capture/navigation primitives should move in the first slice? What SDK license and dependencies are approved for independent public consumption? These remain explicit implementation gates, not assumed facts.

### Historical evidence export

The proposed [focus-ancestry discovery](focus-ancestry-discovery.md) uses one
complete bounded leaf-to-content-root chain and every eligible pair, grounded in
the conditional pinned-Qt proof in [SDK knowledge](../../../docs/research/qt-focus-owner-discovery.md).
It is an explicit new discovery domain, not a fallback after BFS refusal. Sticky
chain/anchor guards must survive visual review and the retained-owner facts path;
no second discovery, bound increase or implicit native qualification is allowed.

The [topology diagnostic amendment](capture-topology-diagnostics.md) distinguishes
the original visited, depth and cumulative queue-cap returns with four nullable
scalar/enum fields in private diagnostic version2. Existing version1 bytes stay
historical and unmodified; callbacks, capture/facts schemas and all bounds remain
unchanged. No reevaluation or additional device operation is selected.

The proposed [capture owner diagnostics](capture-owner-diagnostics.md) retain
the first failed original gate evaluation in the default-off capture path. Fixed
branch/discovery/observer enums, bounded candidate detail and original-clock times
explain refusal without repeating getters, traversal or input. The existing
callback stage remains authoritative for refusal; the additive private diagnostic
is optional evidence and grants no capability. Implementation, consumer admission
and exact artifact/packet review remain separate gates.

The proposed additive contract in [evidence-export-contract.md](evidence-export-contract.md) defines bounded, versioned, lossless facts for observations, creation receipts and capture batches. Export is one-way and separate from operational guards. Native qualification/profile and clock facts absent from the synthetic model remain explicitly unavailable; persistence cannot supply them. Exact review and consumer agreement precede implementation.

### Explicit fixture insertion feasibility trial, October 2

For the supervised disposable-fixture research trial only, active-view metadata
or M3 discovery is not a prerequisite. The configured existing-engine helper may
resolve a backed-up exact document through Library.entryForId and fail closed on
native identity/type/non-exporting status, page count, all five configured page
IDs and index round trips, and native page-zero template. Retain native document
ID and invoke one five-argument insertion at index 1 with inherited template,
RM2 trial size and an observation-only callback, omitting pageUuid. One-shot claim
precedes native entry; no return/exception/cancel/deadline permits resend.
Synchronous/reentrant/late callbacks must preserve owned-lifetime and deadline
guards. Receipt observations are not durable success: Main verifies files,
original relative order/content and one new ID after stock restart/reopen.
This research exception does not qualify source continuity, worker serialization,
consumer binding or the production creation capability. See
[trial implementation](../../../docs/experiments/qt_qml_access_probe.md#explicit-disposable-fixture-one-call-mode-october-2-unqualified-trial).

## October 8 shutdown discriminator

The later [caller-selected insertion proposal](client-selected-insertion.md)
investigates native allocation independently of the rejected shutdown diagnostic.
It preserves native Unsupported and does not infer a lifecycle fix from insertion.

Ordinary task 3.6 mapping is defined separately in [gesture-mapping.md](gesture-mapping.md).
Explicit tablet/orientation admission produces geometry only, never native authority.

The source-only [engine-ready variant](engine-ready-shutdown-diagnostic.md) adds
one synchronous diagnostic marker after the original waiting write and successful
engine-destroyed connection. Consumer admission requires a later higher-frame
render. Facts-waiting is optional corroboration; no actor phase or budget changes.

The selected source-only [pre-token follow-up](pretoken-shutdown-diagnostic.md)
adds the original FactsEntry lifecycle under a compile-time fence before all
request dispatch, with a second capture-admission guard. Its single-root
installation proof is distinct from engine readiness or shutdown completion.
Source fixtures and exact consumer review precede any fresh native artifact.

The [lifecycle-only diagnostic design](shutdown-lifecycle-diagnostic.md) defines the separate startup payload, fixed trace wire, counter/append failure behavior, hypotheses and consumer recovery boundary. The recorder remains process-resident so no callback races its own destruction; this experimental choice is not a production ownership solution. Per-frame Qt markers are diagnostic chronology, never native display-drain evidence. The consumer must keep its temporary failure guard through attempted-process termination, including the first stock stop, and record stop failure separately from later recovered stock. Main reviews the combined fresh packet; source-only fixtures do not select a tablet run.
