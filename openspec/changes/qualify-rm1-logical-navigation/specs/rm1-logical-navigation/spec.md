## ADDED Requirements

### Requirement: RM1 historical observation has independent native admission
An RM1 observation-only adapter SHALL use its own privately constructed admission
record after independently verifying its exact reviewed executable, adapter and
required loaded-provider identities, file/mapping correspondence, ABI, actual
pre-main application absence and eager-binding conditions. It SHALL NOT depend
on installation of insertion hooks or manufacture insertion Facilities. Existing
RM2 admission, provider/code/owner checks and installed bundle behavior SHALL
remain intact. Static metadata, a version string, test facts or a renamed RM2
hash/address table SHALL NOT satisfy missing RM1 evidence or advertise support.

#### Scenario: Reader declarations match but provider evidence is incomplete
- **WHEN** private static RM1 resources and metadata match observer declarations but the actual required provider, ABI, binding or startup evidence is missing or mismatched
- **THEN** RM1 admission refuses before exposing an endpoint, and capabilities remain unsupported without substituting static hashes, mocked facts or an RM2 profile.

#### Scenario: Observation startup without insertion machinery
- **WHEN** an explicitly selected RM1 observation artifact passes all its independently reviewed admission checks
- **THEN** it starts only the guarded historical observation facade, without installing insertion hooks, constructing mutation/selection executors, acquiring worker owners or linking Dobby and the creation bridge.
- **AND** the existing RM2 facade still requires its original Facilities gate and retains its original behavior.

### Requirement: Observation-only startup preserves resident admission and lifetime
The RM1 bootstrap SHALL preserve one-use Activation, exact actual artifact and
executable correspondence, boot/parent incarnation and nonce/expiry checks,
retained-directory/socket correspondence and authenticated peer admission. It
SHALL retain required ownership/stability/ELF/mapping/ABI/provider checks, actual
pre-main ordering, inherited injection-environment clearing, queued GUI startup,
absolute deadlines and process-resident lifetime. Missing or changed admission
SHALL fail closed without endpoint publication, consumption-marker removal,
automatic retry, routine process restart or module unload.

#### Scenario: Directory changes after consumption
- **WHEN** the consumed activation directory is replaced before startup or ceases to match during a request
- **THEN** no replacement endpoint or late successful result is published, and the original consumption evidence remains intact.

#### Scenario: Expired or retired observation request
- **WHEN** cancellation, connection loss, reentry, object loss or the original absolute deadline retires observation or capture
- **THEN** no late successful result is admitted and incomplete capture encoding is not published as valid image evidence; retirement does not unload the adapter, restart xochitl or create a retry admission.

### Requirement: RM1 observation reuses only historical dispatch semantics
The admitted RM1 path SHALL reuse the existing bounded Observe, presence-only
reconciliation, source capture and visual-only capture semantics. It SHALL keep
the original GUI/window/engine/lifetime/notification/source/order guards and final
owned-image publication checks. It SHALL refuse mutation, selection, close/reopen
and unknown commands without dispatch or clearing outstanding effect uncertainty.
Admission and historical facts/images SHALL NOT create operational page guards,
settled-render or writing authority, or a supported RM1 navigation capability.

#### Scenario: Non-observation command reaches the RM1 facade
- **WHEN** an admitted client requests insertion, selection, close/reopen or an unknown command
- **THEN** the path refuses before any native effect, without a fallback executor or quarantine/reservation clearing.

#### Scenario: Final source or capture context changes
- **WHEN** the original source/order, guarded object/context, epoch or deadline becomes invalid before final publication, including after PNG encoding
- **THEN** no successful historical capture or intermediate PNG is published, and no consumer binding/write authority is granted.

#### Scenario: Host fixture or source build succeeds
- **WHEN** owned model/process/socket controls or a source build pass without actual RM1 runtime qualification
- **THEN** results establish only their named mechanics and RM1 capabilities remain unsupported; the original native acceptance tasks remain open.

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

### Requirement: RM1 binding admission distinguishes definition and effective storage
RM1 observation admission SHALL retain exact artifact/executable/provider file,
size, ELF/ABI, ownership, stability, mapping, pre-main and Activation checks.
It SHALL match typed, versioned definitions and all reviewed binding sites rather
than infer every import is an ARM function or every effective address is ET_DYN.
Missing evidence SHALL refuse. reviewedProfile SHALL remain null until a separate
evidence-backed profile review; RM2 hook/Facilities checks SHALL remain unchanged.

#### Scenario: Exact Thumb function pointer
- **WHEN** a reviewed FUNC has a Thumb-tagged raw value
- **THEN** admission compares the complete callable pointer including its state bit, checks the untagged code extent and appropriate alignment within verified executable file-backed mappings, and refuses a flipped tag even when the masked address matches.

#### Scenario: Provider OBJECT binding
- **WHEN** a reviewed OBJECT import has a GLOB_DAT binding and no main COPY
- **THEN** admission verifies exact definition/version/size/provider, readable data extent and backing mappings, and the consumer's exact resolved slot without function alignment or code-range substitutions.

#### Scenario: Main COPY preempts provider storage
- **WHEN** an OBJECT binding resolves to a main executable copy
- **THEN** admission requires the exact main COPY relocation and destination allocation, compatible pinned source definition with proved loaded source scope, and effective-address correspondence at every reviewed consumer site; default lookup or dladdr on the copy alone cannot prove the source.
- **AND** mutable or relocated data contents are not compared to unrelocated file bytes, and missing source-scope or mapping proof refuses.

#### Scenario: Unsupported or ambiguous binding form
- **WHEN** a definition/version/provider, relocation/site, addend, copy size/source or mapping is missing, malformed, unreviewed or ambiguous
- **THEN** admission refuses without a generic ET_EXEC-provider exception, masked-address fallback or test/profile bypass.

### Requirement: Link-only diagnostics cannot qualify RM1 runtime admission
Any separately selected observation link diagnostic SHALL preserve the null
production profile and production build refusal, retain the actual optimized
observation path, record exact source/toolchain/CRT/link/ELF/import/version inputs
and outputs, and remain non-installable and never loaded. Its evidence SHALL be
limited to its own composition; final adapter closure and native gates stay open.

#### Scenario: Optimized observation diagnostic links
- **WHEN** a separately selected diagnostic links with the matching RM1 SDK and retained observation wrapper
- **THEN** only its actual emitted imports/versions are established; differing bootstrap/closed-admission paths require later production reconciliation, and no approved manifest, profile, tablet activation or native qualification follows.

### Requirement: Finite observed ABS32 and artifact-self exact-site correspondence
The private RM1 source verifier SHALL distinguish versioned provider definitions
from finite artifact-self definitions. It SHALL preserve exact symbol/version/
index/type/value/size, PT_DYNAMIC/table, consumer mapping and main-COPY refusal.
Provider ABS32 SHALL permit only FUNC A0, OBJECT A0 and exact Cxx role/CXXABI_1.3
size44 class-type-info vtable OBJECT A8. Original REL file addends, unique aligned
non-executable four-byte places and checked whole-word state/extent correspondence
SHALL be required; no masked pointer or inferred live addend SHALL substitute.

Artifact-self SHALL use a separate finite record domain, defined WEAK/default
symbols with raw versym1 and Adapter-only exact sites. Only observed ABS32 A0,
OBJECT GLOB_DAT A0 and FUNC JUMP_SLOT A0 SHALL be admitted. Exact own-image code/
data extent and live address SHALL be proved; same-name external resolution,
duplicate sites or missing correspondence SHALL refuse. Optional CRT/TLS/closure
SHALL remain outside this slice, with null production profile and native gates open.

#### Scenario: Exact provider ABS32 word
- **WHEN** original addend and complete typed provider/site/version/extent evidence match an observed allowed form
- **THEN** the full live word must equal checked definition-plus-addend with exact ARM/Thumb state, and a main COPY remains refused

#### Scenario: Artifact-self binding is preempted
- **WHEN** a finite self site's live word differs from its exact reviewed own definition address
- **THEN** admission refuses even if another image exports the same name

#### Scenario: New or ambiguous form
- **WHEN** a function interior, unknown object addend/vtable, malformed/overlapping place, wrong state/version/extent, weak undefined CRT or TLS/dependency gap is presented
- **THEN** the verifier refuses without profile enablement, source acquisition or fallback

### Requirement: Finite optional CRT null-only bindings
The RM1 source SHALL represent only the three observed undefined optional CRT
names in a distinct no-provider domain and SHALL refuse every nonzero live slot.

#### Scenario: Exact eager zero bindings
- **WHEN** the exact weak/default/NOTYPE/SHN_UNDEF/value0/size0 raw-versym1 imports
  and four reviewed Adapter GLOB_DAT/JUMP_SLOT sites match the pinned file and
  existing eager/mapping admission, and every actual slot is zero
- **THEN** only that null-only binding condition passes, without a call or lookup

#### Scenario: Unknown or nonzero optional binding
- **WHEN** a name, definition, version, site, count or mapping differs, a domain
  collision occurs, or any live word is nonzero (including a lazy PLT stub)
- **THEN** admission refuses without a call or fallback

#### Scenario: File initializer and earlier constructor limits
- **WHEN** a PLT file initializer exists or current slots are zero
- **THEN** the file initializer is not live zero evidence, and current zero is
  not retroactive constructor-safety proof or native qualification
