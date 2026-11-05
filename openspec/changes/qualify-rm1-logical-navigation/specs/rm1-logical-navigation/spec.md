## ADDED Requirements

### Requirement: RM1 gestures preserve shared logical navigation semantics
The RM1 adapter SHALL use the coordinated SDK logical Next/Previous contract,
qualify its own model/runtime/orientation profile and issue at most one supported
gesture for a freshly verified adjacent target. Unknown profiles SHALL remain
unsupported without an RM2 fallback or blind retry.

#### Scenario: Shipped SDK has no RM1 gesture implementation
- **WHEN** the shared SDK is available but its planner reports the RM1 model/orientation pair unimplemented
- **THEN** the adapter remains unsupported without substituting RM2 geometry or waiting for another generic SDK API.

#### Scenario: Matching versions but different native profile
- **WHEN** RM1 firmware or Qt version strings match the RM2 profile but exact executable, provider or required ABI evidence differs or is missing
- **THEN** the RM2 profile cannot admit RM1 activation or input and a model-name/hash substitution does not qualify the RM1 adapter.

#### Scenario: Unknown physical mapping
- **WHEN** current model, runtime or orientation lacks qualified RM1 evidence
- **THEN** navigation is unsupported and no input is issued.

#### Scenario: Stale source or order
- **WHEN** source ownership or expected ordered identity changes before dispatch
- **THEN** the request refuses without a gesture or consumer write.

### Requirement: Native completion requires attributed target and render evidence
Navigation completion SHALL verify the intended native page, expected order and
fresh settled pixels before granting consumer binding/write authority. Metadata,
synthetic/emulated results or raw development facts SHALL NOT grant that authority.

#### Scenario: Target already active
- **WHEN** a fresh native observation identifies the intended active target
- **THEN** no gesture is issued and render/ownership checks still precede a write.

#### Scenario: Uncertain completion
- **WHEN** one gesture lacks qualified target/order or settled-render confirmation
- **THEN** completion is uncertain or unsupported, with no retry or consumer write.

#### Scenario: Historical capture without a settling producer
- **WHEN** historical source/image correspondence exists but no RM1 evidence establishes the required fresh settled target rendering
- **THEN** it remains historical evidence and cannot become a live guard or authorize consumer binding/write.

### Requirement: Read-only baseline refresh preserves observation scope
RM1 baseline refresh SHALL reuse the delivered bounded collector without native
activation, input or document reads. Its observations SHALL NOT authorize native
operations or fill evidence fields absent from its schema.

#### Scenario: Inherited SSH configuration permits host-key maintenance or retries
- **WHEN** the collector prepares its SSH invocation
- **THEN** it explicitly sets UpdateHostKeys=no and ConnectionAttempts=1 while retaining StrictHostKeyChecking=yes and existing key/forwarding/deadline controls, without editing config or known_hosts.
- **AND** owned transport validation checks the actual spawned arguments for a single non-conflicting instance of each required option before the host alias and fixed remote command.

#### Scenario: Matching baseline does not complete qualification
- **WHEN** a fresh successful collector report matches the retained model/runtime fingerprints
- **THEN** native operations remain unsupported and orientation, provider ABI, page identity, capture and recovery qualification remain separate.

#### Scenario: Baseline changed or collection refused
- **WHEN** model/runtime facts differ from the retained candidate, change during collection, or transport/output validation refuses
- **THEN** the old candidate is not admitted, no native action or automatic retry follows, and the prior evidence is preserved.
