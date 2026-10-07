## ADDED Requirements

### Requirement: RM1 gestures preserve shared logical navigation semantics
The RM1 adapter SHALL use the coordinated SDK logical Next/Previous contract,
qualify its own model/runtime/orientation profile and issue at most one supported
gesture for a freshly verified adjacent target. Unknown profiles SHALL remain
unsupported without an RM2 fallback or blind retry.

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
