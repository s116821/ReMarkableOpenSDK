## ADDED Requirements

### Requirement: Purpose-isolated development Qt input observation
The private development entry SHALL optionally observe bounded touch/mouse events
through its existing always-false application filter, without changing accepted
state or performing filter I/O, capture or document traversal. It SHALL publish
readiness only after collection is enabled for an exact retained live GUI window,
consume at most one distinct observation-end token, seal records before one
separate GUI-thread grabWindow callback and publish bounded completion evidence
only after actual callback completion with scope/deadline checks. It SHALL NOT
start PageFactsSession, grant native/render/UI authority, retry capture, extend
existing clocks or interpret a facts-request as observation end.

#### Scenario: Selected diagnostic completes
- **WHEN** the purpose-bound end request and retained window/root/generation remain valid through the separate GUI callback
- **THEN** bounded observed events, explicit truncation and separate image availability may be reported with all operational authority false.

#### Scenario: Missing events or unsupported capture
- **WHEN** this observer sees no events or grabWindow yields no usable image
- **THEN** the result is limited to this observer/capture path and cannot establish kernel rejection, handler acceptance, native ownership or physical-panel state.

#### Scenario: Wrong purpose, stale scope or delayed completion
- **WHEN** a cross-purpose/stale/duplicate token, destroyed or changed window, closure, generation drift or expired deadline occurs
- **THEN** no usable successful completion or further grab is authorized, and consumer restoration remains required.

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

The SDK SHALL expose logical Next and Previous independently of physical swipe
direction through qualified per-tablet and orientation-aware implementations.
A supported gesture implementation SHALL NOT require a direct native page-opening
method. Each request SHALL dispatch at most one gesture and SHALL determine its
intended adjacent PageKey from fresh ordered native identity. Dispatch success
SHALL NOT imply destination or rendering readiness. Unknown model/orientation
profiles SHALL remain explicitly unsupported rather than borrowing coordinates.

#### Scenario: Device-specific gesture mapping
- **WHEN** a qualified device/orientation profile receives logical Next or Previous
- **THEN** its adapter selects the qualified physical gesture while preserving the same logical adjacent-page contract, without exposing physical left/right as the semantic direction

#### Scenario: Unsupported device or orientation
- **WHEN** the adapter lacks qualification for the requested device/orientation profile
- **THEN** navigation returns Unsupported without dispatching input

#### Scenario: Intended inserted target is already active
- **WHEN** fresh qualified observation identifies the exact inserted target in the expected after-order
- **THEN** the consumer can skip navigation, but still requires fresh capture ownership and binding/write guards before output

#### Scenario: Inserted target requires one adjacent transition
- **WHEN** fresh observation still identifies the expected source and the exact intended inserted target is its next neighbor in the verified after-order
- **THEN** the SDK may dispatch one qualified logical Next and reports completion only after fresh native target identity, expected order and qualified visual readiness agree

#### Scenario: Acquisition after legitimate insertion
- **WHEN** insertion changes the expected order or active visit before navigation
- **THEN** a qualified acquisition/handoff preserves observer/input-epoch and runtime/session continuity, validates correlated creation and the exact permitted after-order, and never silently repins an invalid old guard or clears sticky input/ownership loss

#### Scenario: Wrong or unverified destination
- **WHEN** the gesture reaches another page, order or ownership changes, or pixels cannot be freshly attributed to the intended target
- **THEN** navigation reports refusal or uncertainty as appropriate, does not dispatch another gesture, and grants no permission to write

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
- **WHEN** a candidate requires automatic cold-boot injection, persistent injection-enabling xochitl/preload/startup changes, configuration that recreates injection or crash loops across cold boots, or recovery dependent on the modified UI
- **THEN** it is excluded from production regardless of successful research demonstrations

#### Scenario: Ordinary Supervisor autostart preserves stock boot
- **WHEN** consumer installation configures ordinary Supervisor autostart without enabling injection and activation still requires an authorized fresh trigger
- **THEN** that autostart is permitted only while xochitl boots stock and stale session state cannot activate injection after reboot.

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

#### Scenario: Recovery guard is lost while supervisor survives
- **WHEN** an armed recovery guard exits or loses protection during activation or Ready while the consumer supervisor remains alive
- **THEN** Ready is withdrawn, further activation is latched off and the surviving recovery path restores stock; failure remains explicit until independently observed restoration, without treating runner cleanup or a watchdog expiry as success.

#### Scenario: Recovery overlaps a late activation write
- **WHEN** restoration races an in-flight activation or another restoration actor
- **THEN** transaction ownership and idempotent restoration prevent stale activation effects from reenabling injection and preserve unrelated configuration; ambiguous or blocked restoration is reported as failed or unknown rather than completed.

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

### Requirement: Development normalization measurements are exact-build and same-call
The SDK SHALL restrict the development normalization discriminator to the reviewed
exact provider/profile and fixed two-stop contract in input-normalization-discriminator.md.
It SHALL preserve bounded raw scalar bytes before interpretation, correlate the same
generation/thread/frame/handler/contact, retain nonfinite bit patterns explicitly,
and expose no operational capability, coordinate correction or general tracing API.
The consumer SHALL qualify the complete debugger transport and independent recovery
on an owned target fixture before one separately prepared native candidate, within
existing total/lifetime budgets. A failed, interrupted or ambiguous attempt SHALL
remain spent and unknown without automatic attach/input retry.

#### Scenario: Provider or instruction mismatch before arming
- **WHEN** generation, module fingerprint/load mapping, ARM state or either fixed instruction differs from the reviewed profile
- **THEN** the discriminator refuses before breakpoint/input effects and creates no measurement authority.

#### Scenario: Paired normalization stops match
- **WHEN** the two bounded stops match the same generation, thread, frame, handler, contact and caller
- **THEN** the private receipt preserves the actual pre/post double bytes and adjacent state with scheduling/concurrency limitations, without claiming physical origin or unperturbed navigation success.

#### Scenario: Post stop or persistence fails
- **WHEN** the post stop is absent/mismatched, evidence persistence is partial, the debugger channel dies or its deadline expires
- **THEN** bounded acquired bytes remain historical, the attempt stays spent/unknown and independently qualified recovery removes the debugger and restores stock without retrying input or resuming unknown patched code.

#### Scenario: Ordinary normalization implementation is statically consistent
- **WHEN** static inspection finds no justified correction but runtime operands remain unknown
- **THEN** the SDK retains the investigation as unfinished and does not patch ranges, rotation, event ABI or rounding based only on endpoint resemblance.

### Requirement: Disposable debugger failures cannot release unknown patched code
The development discriminator SHALL use only the independently reviewed task-only
server contract in disposable-debug-server.md. It SHALL require checked mandatory
EXITKILL protection on every live LWP and option reset before instrumentation or
collection readiness, retain an unmodified partial-attach window, and kill the
selected disposable process on failure rather than automatically detaching it.
Only an explicit successful release authorization followed by protocol D after
breakpoint removal and original-word verification SHALL permit detach. Source,
new artifact identity and actual owned-target failure evidence SHALL precede native
qualification; the original unmodified GNU14.2 server SHALL remain held.
Every debugger text modification, including internal step and loader breakpoints,
SHALL be limited to the reviewed bounded site ledger and verified restored through
raw reads before release. Two capture stops SHALL NOT imply only two text writes.

#### Scenario: Protection or complete thread attachment fails
- **WHEN** any live LWP cannot be accounted for or mandatory EXITKILL application is unsupported, masked, failed or later lost
- **THEN** no instrumentation readiness is granted, the attempt remains spent and the selected disposable process enters bounded kill recovery without ordinary detach or unsafe resume.

#### Scenario: Client channel or tracer dies with breakpoints installed
- **WHEN** EOF, client death, server SIGKILL or internal failure occurs before successful release
- **THEN** the qualified live-server kill policy or kernel EXITKILL prevents release of unknown patched code, and independent recovery verifies target/tracer exit before restoration; a delayed external kill alone is insufficient evidence.

#### Scenario: Explicit successful detach
- **WHEN** bounded evidence is complete, breakpoints are removed, original words are verified and the exact one-use release authorization is followed by D
- **THEN** only that checked process may detach; incidental unarmed D, intervening errors or partial detach invoke failure recovery, and lost acknowledgement cannot authorize replay.

#### Scenario: Additional debugger breakpoint site
- **WHEN** ARM stepping or implicit client behavior requests another instruction modification
- **THEN** the site must match the fixed reviewed profile and bounded ledger before writing, and all recorded sites must pass raw restoration checks before release; unprofiled sites, overflow or restoration mismatch fail without widening the trial.
