## ADDED Requirements

### Requirement: Read-only bounded RM1 baseline
The development collector SHALL use a fixed read-only SSH command and strict existing-host verification without prompting, forwarding, target installation, runtime mutation or personal-data/configuration access. It SHALL bound transport time and output and refuse missing, duplicate, unknown or malformed fields without publishing raw diagnostics.

#### Scenario: Authorized RM1 key transport
- **WHEN** a Linux operator supplies an existing authorized SSH alias and complete valid RM1 observations
- **THEN** the collector emits only validated identity, library/runtime hash, storage and framebuffer observations.

#### Scenario: Refused observation
- **WHEN** transport fails or exceeds bounds, an identity changes, or fields are invalid
- **THEN** collection fails with a generic diagnostic and leaves existing output intact.

### Requirement: Observation grants no native authority
A report SHALL explicitly mark native operations unsupported and source authority false. It SHALL distinguish free from unreserved available storage and virtual framebuffer allocation from logical geometry. It SHALL refuse unsupported model/architecture/framebuffer combinations and never infer native capability from matching RM2 fingerprints.

#### Scenario: Shared architecture is insufficient
- **WHEN** an RM1 report records ARMv7, Qt and firmware fingerprints
- **THEN** native operations remain unsupported and no document/page operation becomes callable.

#### Scenario: Reserved storage
- **WHEN** free blocks are positive but available blocks are zero
- **THEN** the report preserves both values rather than declaring the filesystem physically full.
