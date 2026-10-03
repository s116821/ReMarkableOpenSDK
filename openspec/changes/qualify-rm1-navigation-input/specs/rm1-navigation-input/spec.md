## ADDED Requirements

### Requirement: RM1 input metadata remains read-only and unqualified
The RM1 input probe SHALL read only fixed device metadata, verify stable model,
device/capability/range identity and exclude current coordinates or personal
content. It SHALL perform no input event read/grab/injection or native mutation.
A caller SHALL bound the process and clean up its owned process on failure.

#### Scenario: Stable metadata
- **WHEN** the same verified RM1 touch descriptor yields matching static axis metadata
- **THEN** the bounded report contains axis ranges with native navigation unsupported.

#### Scenario: Malformed or changing metadata
- **WHEN** model/device/capabilities or static bounds differ or are malformed
- **THEN** no success record is emitted and no event or native operation occurs.

### Requirement: Physical mapping cannot grant logical navigation authority
RM1 mapping SHALL follow the exact shared logical-navigation contract. Kernel
bounds or matching architecture SHALL not qualify gesture orientation, native
source/target identity, rendering ownership or permission to write.

#### Scenario: Unknown physical or source profile
- **WHEN** orientation, native source or per-model mapping lacks RM1 evidence
- **THEN** navigation remains unsupported; no RM2 fallback or repeated gesture is used.
