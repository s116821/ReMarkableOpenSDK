## ADDED Requirements

### Requirement: Independent semantic capability contract
The SDK SHALL expose versioned semantic operations through device/firmware adapters, SHALL keep product conversation/storage policy outside its API, and SHALL report unsupported operations explicitly for unknown or unqualified runtime combinations.

#### Scenario: Unknown firmware
- **WHEN** a caller requests page creation on an unqualified firmware/runtime fingerprint
- **THEN** the SDK returns Unsupported without dispatching a native mutation even if another capability is qualified

#### Scenario: Mock consumer
- **WHEN** a consumer substitutes the deterministic mock adapter
- **THEN** it uses the same semantic contract and receives explicitly synthetic evidence that cannot qualify production support

### Requirement: Stable page identity and separate operational ownership
The SDK SHALL identify a native page by validated document/page UUIDs and SHALL represent device-connection/adapter-instance scope, session, visit, order/content revision and evidence strength separately. Operational handles SHALL NOT cross device or adapter-instance boundaries. Persisted last-opened metadata SHALL NOT alone prove current UI ownership.

#### Scenario: Page revisited or runtime restarted
- **WHEN** stable page UUIDs match but the runtime session or visit differs from a request precondition
- **THEN** the old operational guard is rejected and cannot be silently repinned

#### Scenario: Ambiguous native identity
- **WHEN** a page/document identity is duplicated or order evidence conflicts
- **THEN** observation returns explicit ambiguity and guarded mutations do not dispatch

#### Scenario: Two devices share page UUIDs
- **WHEN** a guard from device A is presented to device B with the same document/page UUIDs and coincident session-token text
- **THEN** the SDK rejects the foreign operational scope before dispatch

#### Scenario: Adapter recreated
- **WHEN** a new adapter instance receives a prior instance's guard or historical receipt
- **THEN** it rejects that evidence as mutation authority and requires fresh scoped observation for any permitted reconciliation

#### Scenario: Native source identity cannot be established
- **WHEN** identity observation is unsupported or cannot resolve exactly one current nonnil document/page identity
- **THEN** the SDK returns explicit Unsupported or UnknownIdentity and does not fabricate UUIDs or a qualified source observation

#### Scenario: Known source lacks a consumer binding
- **WHEN** native source identity is known but no Buddy conversation binding exists
- **THEN** the SDK retains the known identity and leaves the separate unbound-conversation decision to the consumer

### Requirement: Capture carries bounded provenance
The SDK SHALL return immutable capture content with digest, dimensions/transform and matching before/after page ownership observations, plus a session-bound monotonic observation interval. The SDK SHALL reject stale or mismatched capture ownership and SHALL leave durable source storage to its consumer.

#### Scenario: Page changes during capture
- **WHEN** document/page, session, visit or required revision differs across capture observations
- **THEN** the frame is not returned as qualified evidence for the original page

#### Scenario: Pixel match without owner proof
- **WHEN** pixels match an earlier frame but current native ownership cannot be established
- **THEN** the SDK reports unavailable evidence rather than promoting the matching image digest to page identity

#### Scenario: Immutable overview and detail batch
- **WHEN** a qualified capture batch is returned
- **THEN** its exact immutable native acquisition parent and derived overview/detail bytes, computed digests, encoding/dimensions, shared source/procedure/time provenance and checked crop/transform relationships cannot be substituted by later mutable framebuffer or cache contents

#### Scenario: Stable identity with stale rendered buffer
- **WHEN** current ownership observations identify page B but the selected buffer contains prior page A content
- **THEN** the SDK rejects the capture because its qualified logical render/acquisition binding does not match page/content/viewport state, regardless of matching before/after IDs or recent timestamps

#### Scenario: Plausible metadata with unrelated detail pixels
- **WHEN** valid image bytes are presented with plausible native-parent digest, crop and transform but no trusted or verified parent-pixel derivation
- **THEN** the SDK rejects their attribution as a derivative of that native acquisition

#### Scenario: Invalid detail provenance
- **WHEN** a detail has a foreign parent, invalid crop/transform, encoding/dimension mismatch or different source ownership
- **THEN** the complete qualified batch is rejected rather than emitting partial or misattributed source evidence

#### Scenario: Historical source evidence used for native output
- **WHEN** a caller holds a persisted capture or source record but lacks a current scoped native guard
- **THEN** that historical evidence grants no permission to render or mutate a page

### Requirement: Guarded native navigation
The SDK SHALL navigate only from a valid expected observation through a qualified semantic operation and SHALL enforce its ownership preconditions when execution occurs, not only before dispatch. It SHALL report observed target completion separately from command dispatch. It SHALL NOT use autonomous menu automation or a per-request UI restart to satisfy navigation.

#### Scenario: Source ownership lost
- **WHEN** external input or runtime state invalidates the source guard before dispatch
- **THEN** navigation stops without issuing the operation

#### Scenario: Uncertain navigation completion
- **WHEN** an operation was dispatched but the target cannot be freshly verified
- **THEN** the result reports uncertainty and grants no permission to write on the presumed target

### Requirement: Creation has exact preconditions and durable receipts
The SDK SHALL accept an immutable operation/request identity, exact source/order preconditions and explicit target-allocation policy for creation-after-source. It SHALL enforce those preconditions at native mutation execution through a qualified race-closing mechanism and SHALL report Unsupported if the backend cannot do so. It SHALL return Committed only with qualified evidence binding the request to the observed target, exact permitted after-order and persisted revision. Client-selected IDs SHALL match the intended UUID; native-assigned IDs SHALL have a qualified durable atomic operation/request-to-target mapping. Generic callbacks, idle indicators or position-based inference SHALL NOT substitute for that evidence.

#### Scenario: Intended and observed target differ
- **WHEN** client-selected allocation reports a target UUID different from the immutable request
- **THEN** the result is Indeterminate and is not usable as a successful consumer binding receipt

#### Scenario: Ownership changes between dispatch and execution
- **WHEN** external input or source/order state changes after request dispatch but before native mutation executes
- **THEN** the qualified backend rejects the stale request without mutation; an adapter that cannot enforce this guarantee does not expose that mutation capability as supported

#### Scenario: Native assigns the page UUID
- **WHEN** the native operation durably maps the immutable operation/request identity to a generated UUID and exact insertion evidence matches it
- **THEN** the SDK may return a receipt under its qualified native-assigned allocation mode without pretending the caller selected that UUID

#### Scenario: Native allocation lacks durable correlation
- **WHEN** an interrupted native-assigned request can only be matched by new page position or timestamps
- **THEN** the SDK reports Indeterminate and does not infer attribution or dispatch a retry

#### Scenario: Unexpected concurrent order change
- **WHEN** observed order contains changes beyond the one requested insertion
- **THEN** the SDK refuses a Committed receipt and requires reconciliation

### Requirement: Ambiguity never causes blind replay
The SDK SHALL distinguish failure/cancellation before dispatch from uncertainty after dispatch. Reconciliation SHALL observe the original request outcome without repeating an uncertain mutation. Historical receipt reuse SHALL NOT recreate removed pages or grant fresh rendering ownership.

#### Scenario: Deadline after dispatch
- **WHEN** a native call was issued but the deadline expires before durable outcome evidence
- **THEN** the SDK returns Indeterminate rather than asserting cancellation or safely retrying creation

#### Scenario: Retry after receipt persistence
- **WHEN** the same request has a verified historical receipt but the page has since changed or disappeared
- **THEN** the SDK preserves historical evidence, reports current state separately and does not recreate the page

### Requirement: Safe adapter lifecycle and evidence scope
Production adapters SHALL preserve ordinary tablet startup/use with fail-closed compatibility and recoverable lifecycle behavior. Stable direct/native mechanisms SHOULD be preferred where robust. A supervised, session-scoped lazy XOVI adapter MAY be selected only after qualifying its exact runtime/payload capabilities and lifecycle, including runtime and supervisor failure. Permission to investigate SHALL NOT imply selection or support. Support claims SHALL distinguish native qualification from host/mock/cross-build evidence and SHALL NOT publish proprietary firmware or reconstructed source.

#### Scenario: Cross-build succeeds for Paper Pro
- **WHEN** an AArch64 artifact builds but no Paper Pro native acceptance evidence exists
- **THEN** its native capabilities remain explicitly unqualified and it is not advertised as hardware-tested

#### Scenario: Native mechanism requires fragile boot changes
- **WHEN** a candidate requires automatic cold-boot injection, persistent startup modification or recovery dependent on the modified UI
- **THEN** it is excluded from production regardless of successful research demonstrations

### Requirement: Explicit runtime activation and bounded recovery
Capability queries SHALL NOT activate runtime injection. A conditional adapter SHALL expose its exact compatibility, activation and recovery requirements separately from operation support. Activation SHALL require a qualified model/firmware/runtime/payload match, bounded readiness and health checks, and independently reachable recovery from runtime or supervisor failure. A failed activation or rapid restart loop SHALL latch the injected path unavailable for the session and restore stock operation through the qualified recovery mechanism. A cold reboot SHALL return to a non-injected baseline. SDK contracts SHALL define these device/runtime outcomes while consumer trigger policy, supervision orchestration and installation remain consumer-owned.

#### Scenario: First-use activation on a qualified runtime
- **WHEN** the consumer explicitly requests activation after complete compatibility preflight
- **THEN** the adapter may perform one qualified session activation/restart, reports ready only after bounded execution-context checks, and does not require repeated restarts for subsequent ordinary operations.

#### Scenario: Runtime or supervisor fails during activation
- **WHEN** the injected process fails, readiness times out, a restart loop starts, or its consumer supervisor dies at any activation/rollback boundary
- **THEN** independently reachable recovery restores stock operation within the qualified bound without relying on the modified UI or retrying activation automatically, and unproved operation outcomes remain indeterminate.

#### Scenario: Compatibility or recovery evidence is missing
- **WHEN** the runtime/payload fingerprint is unknown or recovery cannot be demonstrated on the target profile
- **THEN** the affected capability remains unavailable without changing preload/startup state or attempting speculative injection.

### Requirement: Restart invalidates source authority before continuation
An activation/recovery restart SHALL invalidate prior runtime and adapter-instance handles, observations and guards. Resuming a retained consumer intent SHALL require a new qualified source observation and execution-time validation under the fresh scope. Stable IDs or historical captures alone SHALL NOT authorize continuation. An operation that may have started before failure SHALL be reconciled without redispatch.

#### Scenario: Triggering intent survives first-use restart
- **WHEN** the consumer retains an intent through activation and obtains a freshly qualified matching source and eligible action
- **THEN** a new request may use only the fresh scope and preconditions; old pointers, visits and guards are rejected.

#### Scenario: Source is changed or unknown after restart
- **WHEN** the requested source/action cannot be validated after rebind
- **THEN** continuation refuses before native effects rather than replaying a stale request or treating the restart as proof that no earlier mutation occurred.

### Requirement: Build-time independent consumption
SDK releases SHALL identify exact semantic contract and adapter qualification revisions per target artifact. Buddy SHALL consume the appropriate target at build time, and ordinary Manager installation SHALL require only its compatible Buddy artifact rather than a separately installed SDK runtime.

#### Scenario: Independent release versions
- **WHEN** SDK and Buddy release versions differ
- **THEN** compatibility is determined from the pinned contract/target qualification metadata rather than version-number equality

#### Scenario: Unsupported artifact selection
- **WHEN** connected model/architecture/firmware is outside an artifact's qualified capabilities
- **THEN** installation/integration fails closed instead of selecting the closest device build

#### Scenario: Correct architecture but incompatible dynamic ABI
- **WHEN** an artifact has the expected CPU architecture but its interpreter, float ABI, required library or versioned symbol cannot be satisfied by the qualified target runtime
- **THEN** compatibility fails and the artifact is not advertised as supported, even if it built or ran under a different emulator/sysroot

### Requirement: Historical facts remain separate from operational authority
The SDK SHALL expose bounded, versioned, lossless historical evidence with original identity, scope, origin, order, receipt and capture lineage facts. It SHALL explicitly identify unavailable qualification/clock facts and SHALL NOT reconstruct operational guards from exported or imported historical records. Consumers SHALL NOT invent missing facts or use Debug output as an evidence protocol.

#### Scenario: Persisting and restoring a synthetic capture
- **WHEN** a consumer exports and restores the synthetic batch facts and exact image bytes
- **THEN** original numeric and geometry values, source/order facts and parent/derivative lineage survive without loss, synthetic qualification and fixture-local clock meaning remain explicit, and the record grants no native operational authority.

#### Scenario: Evidence exceeds export limits
- **WHEN** source order or variable metadata exceeds the validated bounded export schema
- **THEN** export returns an explicit error before copying oversized variable data and does not truncate, omit or fabricate facts.

#### Scenario: Required native evidence is absent
- **WHEN** a consumer requires a qualified profile or native clock/ownership evidence that the synthetic model cannot supply
- **THEN** that consumer path refuses before effects rather than inferring qualification from stored IDs, hashes or elapsed time.

### Requirement: Historical derivative composition uses shared SDK geometry
The SDK SHALL provide the pure derive_geometry helper defined in evidence-export-contract.md, available without mock features and shared with synthetic capture construction. It SHALL validate checked dimensions/crop and parent/derived source-plane geometry while preserving current arithmetic. Consumers SHALL compare recomputed affine and valid-region fields to restored descriptors by exact f64 bits for the supported pinned procedure, preserve stored values and refuse unknown procedure semantics. This validation SHALL NOT create operational evidence or imply pixel authenticity or native qualification.

#### Scenario: Individually valid but inconsistent derivative
- **WHEN** a restored child affine or valid source region is individually valid but differs from SDK composition of the parent, crop and output
- **THEN** historical structural validation refuses the descriptor, including a signed-zero bit mismatch, without rewriting the original stored fields.

#### Scenario: Invalid derivation geometry
- **WHEN** dimensions exceed the mathematical bound, crop arithmetic overflows or escapes the parent, valid-region intersection is empty, or source-plane geometry is invalid
- **THEN** derive_geometry returns InvalidGeometry without producing capture evidence or allocating decoded media.

#### Scenario: Supported composition remains historical
- **WHEN** all recomputed fields match exact stored bits under the pinned supported procedure
- **THEN** the consumer may accept structural composition only; the explicit source-plane roundoff allowance does not relax descriptor equality or restore a live guard.

### Requirement: Explicit legacy history remains outside SDK capture evidence
The SDK SHALL NOT convert identity-free legacy pixels into Capture facts, SourceObservation or operational authority. A consumer MAY retain explicitly selected legacy acquisition in a separate unbound historical container with native identity and qualification absent, as defined in capture-contract.md. An attempted SDK capture failure SHALL remain a refusal and SHALL NOT implicitly select the legacy path. Consumer container policy SHALL NOT weaken SDK qualification, native guards or receipt semantics.

#### Scenario: Explicit unbound historical association
- **WHEN** a consumer selects legacy acquisition explicitly before capture and persists its actual ordered provider images plus any supplied acquisition parent
- **THEN** the consumer record may belong to an unbound historical conversation while native identity/qualification and an unavailable parent remain explicitly absent, with no invented geometry, SDK facts or native binding.

#### Scenario: SDK capture cannot downgrade into legacy history
- **WHEN** an attempted SDK capture is unsupported, loses identity, fails or lacks required facts
- **THEN** that attempt refuses its qualified provider-source path and cannot be converted into a legacy record to continue dispatch.

#### Scenario: Legacy history cannot authorize native output
- **WHEN** a consumer operation requires qualified native capture, page ownership, binding or output completion
- **THEN** legacy historical association supplies none of those capabilities and cannot satisfy the required SDK guard or receipt.
