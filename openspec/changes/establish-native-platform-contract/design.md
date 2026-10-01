## Context

This is an active design, not a stable release or implemented interface. The current Buddy DeviceBackend provides a useful seam but mixes product history/status semantics with device operations. At fetched Buddy main `ff8ad75`, native_page explicitly distinguishes persisted last-opened candidates from proof that a page remains on screen; the SDK must preserve that distinction.

SDK owns native facts and mechanisms. Buddy owns conversation identities, binding CAS, source retention, cancellation policy and the local Prepared/NativeCommitted/BindingCommitted/ReconcileRequired journal. Native receipts provide evidence to that journal; they do not constitute a second binding registry.

## Goals / Non-Goals

Goals: one semantic, mockable API; independently useful device primitives; bounded operations; explicit evidence strength; no duplicate creation on ambiguous retries; build-time consumption; exact adapter qualification.

Non-goals: bulk extraction of all Buddy internals, layout/composer/business rules, LLM/provider behavior, sync services, general device administration, firmware recovery, XOVI production integration or declaring Paper Pro tested through compilation.

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

Outcomes distinguish `Unsupported`, `RejectedBeforeDispatch`, `CanceledBeforeDispatch`, `Committed(receipt)` and `Indeterminate(evidence)`. Errors after dispatch—including cancellation, timeout, transport loss or helper death—are indeterminate unless native evidence proves a stronger outcome. A generic boolean return, callback or idle task tracker is not a commit receipt. Deadline expiration bounds the caller's wait; it does not prove native cancellation.

The receipt binds operation/request fingerprint, contract and adapter fingerprint, original device/adapter-instance provenance, source precondition, target-allocation policy and observed target UUID, exact observed after-order, persisted revision, observation session/time and evidence procedure. For client-selected allocation, `Committed` requires intended=observed target; for native-assigned allocation, it requires the qualified durable operation-to-target mapping and matching observed target. Both require exactly the permitted insertion. Extra changes, attribution mismatch or insufficient persistence evidence return indeterminate and force reconciliation. Receipt construction must not be publicly forgeable as native evidence by ordinary production callers; mocks use a distinct synthetic evidence origin.

The consumer persists Prepared before dispatch and the receipt before binding CAS. The SDK has no parallel conversation journal. Repeated requests with a known receipt may return that historical receipt plus separately labeled current observation, never recreate or resurrect a removed/reassigned page. If the backend cannot prove no earlier insertion, retries do not dispatch. Idempotency and durable recovery are acceptance obligations, not promises inferred from a caller-supplied UUID.

Native page commit does not grant permission to render. Buddy requires a fresh current guard and successful binding before writing. Automatic deletion to compensate for uncertain creation is prohibited; a user may already have written to that page.

### Native mechanism investigation

Prefer existing IPC/runtime services or independently implementable native contracts with normal boot/use semantics. Inventory first, then inspect narrow interfaces without invoking mutators. The saved RM2 binary contains D-Bus sync/device-policy leads and page-related Qt metadata, but neither proves an out-of-process page API. Passive runtime evidence must come through the sole coordinated tablet owner.

Compare candidates against annotated-PDF preservation, ordering, durability, user input, cancellation, repeated operations, cache/serialization coordination and lifecycle recovery. Direct file mutation while xochitl is unaware is not accepted. XOVI is excluded as production and is retained only as research. Replacing its name with a similarly fragile custom injection scheme does not satisfy the operability requirement.

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

Bootstrap this active SDK plan, agree receipt/identity semantics with REM-37, narrow native research, implement the smallest tested contract/mocks, qualify RM2 mechanism, then move only current Reader/native creation seams behind it. Pair exact SDK and Buddy revisions with the Docs product delta. Keep old synthetic XOVI experiments as research evidence only. Sync/archive only after implementation and required native/consumer gates pass.

## Open Questions

Which native service/mechanism can satisfy creation and durable reconciliation without fragile startup modification? Which exact current firmware/runtime fingerprints are qualified? Which existing Reader capture/navigation primitives should move in the first slice? What SDK license and dependencies are approved for independent public consumption? These remain explicit implementation gates, not assumed facts.
