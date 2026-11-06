# RM1 profile, evidence and refusal matrix

Reference: shipped SDK v0.2.0 at
`a0c6480be6785480c2e0da76a3c06697a18efae4`. This is a prospective source
qualification matrix, not a native profile manifest or executable allowlist.
The native RM2 profile cannot be renamed into an RM1 profile. No new API,
runtime framework, device action or mock producer is introduced.

| Boundary | Reusable shipped part | RM1 evidence available | Missing RM1 evidence / refusal |
| --- | --- | --- | --- |
| Persistent identity | Uuid/PageKey/OperationId and logical adjacent requests | Baseline and metadata do not identify a current source page | Refuse missing/stale current source, target or complete ordered identity; never create a live guard from a persistent key |
| Exact runtime | Exact-build loader/activation and artifact binding pattern | Historical ARMv7, firmware3.28.0.172, Qt6.10.3; hashes below | RM1 provider/ABI/native joins and mappings remain unqualified; same versions or ARM target do not admit the RM2 profile |
| Kernel input metadata | Delivered read-only metadata helper and refusal approach | Historical event2/cyttsp5_mt, X0..767,Y0..1023, slots0..31 | No fresh device/orientation/contact-release mapping; bounds do not authorize input |
| Logical gesture | LogicalDirection and existing gesture-planner interface | Delivered planner explicitly returns RM1 UnimplementedPair | Qualify one RM1 model/orientation mapping and dispatch/release path; unknown pairs remain unsupported without RM2 fallback |
| Resident transport | Revision4 frames, authenticated retained-directory peer, boot/process/nonces, absolute deadlines and journal-before-dispatch | Source contracts transfer; no RM1 endpoint is qualified | Mismatched/stale peer/profile/admission refuses before work; connection loss after effects preserves uncertainty, never replay |
| Native current page/order | Reader join/full-order observation algorithm and fixture patterns from returned research | No qualified RM1 source/target/order producer | Validate own original Reader/document/controller, selected page and unchanged full order; unknown/stale join refuses before input |
| Historical capture | Identity-bracketed native capture and strict owned PNG correspondence | Old RM1 framebuffer mxc_epdc_fb/16bpp; allocation1408x3840 only | RM1 window/page geometry,DPR, retained objects and source-to-pixels evidence remain absent; never treat allocation as screen geometry |
| Completion/settling | Request/result correspondence and separate target reacquisition pattern | No qualified RM1 post-transition or settling evidence | Intended target/order plus required settled render must be established independently; historical pixels alone cannot authorize writes |
| Native selection | SelectAdjacent wire semantics and candidate implementation references | Installed RM2 insertion variant leaves native Select disabled; no RM1 route enabled | No dispatch merely because a frame can be encoded; do not substitute direct opening for logical gestures |
| Physical lifetime | Existing resident owner retirement/quarantine and independent supervisor recovery approach | RM1 native object/control/Task lifetime and service recovery unqualified | Timeout/logical retirement is not physical callback completion; unknown owners/effects remain quarantined without write/retry |
| Capacity/install | Existing non-root staging and ownership principles | Historical root12,284,928 free reserved bytes/zero ordinary available; home6,661,337,088 available | Refresh capacity before artifact planning; no root install, cleanup or native artifact is part of this source pass |
| Result advertisement | Unsupported defaults and explicit historical/synthetic facts | Baseline always native_operations=unsupported/source_authority=false | A successful collector, architecture build or RM2 result cannot advertise RM1 capture/navigation/creation support |

## Observation-only source preparation checkpoint

The November5 private comparison recorded in REM-49 found the four Reader anchors
in embedded compressed QML and matched the checked static page-map/readiness and
library lookup declarations. It also confirmed that the current read-only facade
does not call its insertion core, although startup still installs it. These are
static/source findings; no RM1 profile or native acceptance is established.

The [observation-only design](design.md#observation-only-admission-before-navigation)
separates that incidental insertion dependency while retaining exact independent
RM1 admission. The original matrix's live-object, ABI/provider, capture, input,
orientation, freshness and recovery gaps remain. The retained xochitl/Qt Core/libc
files do not establish every provider used by the linked observation artifact.
Missing evidence refuses; neither static declarations nor modeled facts fill it.
This preparation does not change the RM2 loader/bundle, claim product capability
or authorize another baseline read, acquisition, build or device trial.

## Exact historical fingerprint mismatch

| Artifact | Retained RM1 observation | Delivered RM2 installed profile |
| --- | --- | --- |
| xochitl SHA256 | `1f4fbb6e14650704b5b036e482da9948e73553178f6116ad464ab072f7b90117` | `071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df` |
| Qt Core SHA256 | `e774905f474cc6516440b30ff8c1807b7a8707a7ae14c41d414062767070b9b5` | `43b0e210d64e59b534490d78c4c82cc1d2958999b0969aa0f77e11a082704e5d` |

RM1 observations date from the October2 local/October3 UTC baseline and prior
metadata receipt. They are not today's device state. RM1 libc was not measured
by this collector; the RM2 libc hash is not an RM1 value. No new research need is
asserted for mechanisms already returned by the RM2 research and integrated into
v0.2.0. Unresolved rows identify model-specific evidence to assess first, without
selecting reverse engineering, an upstream query or another native experiment.

## Smallest later observation and decision

After the existing hold/operator checks, run the transport-corrected collector once
with a fresh private output path (commands shown for review, not executed here):

```sh
# Current SDK layout (the preserved research branch used tools/):
python3 scripts/collect_rm1_baseline.py --ssh-host remarkable1-usb --output /fresh/private/rm1-baseline.json
```

The operator must establish that this alias is still direct USB/key-only for the
intended RM1, with existing trusted host identity and no WLAN/proxy fallback.
The collector accepts an alias; it does not prove or configure that route.
Its SSH invocation must explicitly set UpdateHostKeys=no and ConnectionAttempts=1
alongside strict host-key checking, key-only authentication and existing bounds.
Do not edit config/known_hosts or add a wrapper to obtain these guarantees.
Use a fresh path: successful output is an atomic replacement, while refusal
preserves the previous selected output. Do not delete the old baseline.

A stable matching result only refreshes its stated platform facts. A different
fingerprint requires reviewing the new candidate before any native preparation;
changing facts/SSH failure/invalid output stop without an automatic second call.
No outcome proves input ranges, libc, orientation, page/capture correctness,
physical retirement, stock recovery or a supported adapter. The delivered input
metadata helper is separate and is not bundled into this first refresh.

The first later functional slice remains own-fixture observation/capture and one
qualified logical transition, with exact RM1 profile, source/order/target checks,
no gesture if already there, separate target/render evidence, refusal on
uncertainty and independently functioning stock recovery. This is an acceptance
boundary, not a newly selected trial. Creation, native direct opening, provider
requests, Manager packaging and final package matrices remain separate scope.

## Evidence references

- [Exact shipped planner](https://github.com/s116821/ReMarkableOpenSDK/blob/a0c6480be6785480c2e0da76a3c06697a18efae4/src/navigation/gesture.rs)
- [Exact installed RM2 profile](https://github.com/s116821/ReMarkableOpenSDK/blob/a0c6480be6785480c2e0da76a3c06697a18efae4/native/rm2_3_28_0_172/installed-reader-insert-v1.json)
- [Owning resident contract](https://github.com/s116821/ReMarkableOpenSDK/blob/a0c6480be6785480c2e0da76a3c06697a18efae4/openspec/specs/resident-native-adapter/spec.md)
- [Returned mechanism provenance](https://github.com/s116821/ReMarkableOpenSDK/blob/a0c6480be6785480c2e0da76a3c06697a18efae4/docs/native/resident-components.md)
- [Delivered baseline collector](https://github.com/s116821/ReMarkableOpenSDK/blob/a0c6480be6785480c2e0da76a3c06697a18efae4/scripts/collect_rm1_baseline.py)
- [Delivered metadata scope and observations](https://github.com/s116821/ReMarkableOpenSDK/blob/a0c6480be6785480c2e0da76a3c06697a18efae4/docs/rm1-input-geometry.md)

Source basis: exact shipped source, preserved sanitized RM1 baseline and reviewed
metadata records. Reuse and sequencing are design judgments. No builds, native
artifacts, tablet observations, recovery tests or compatibility passes are added.

## Typed-binding precode checkpoint

The matching RM1 SDK compiles the selected ABI/admission/facade/bootstrap/Epoch
units. SDK Core/libc match saved anchors; six other provider files remain SDK
candidates. O0 import metadata includes Thumb FUNC and OBJECT references; the
saved main has 17 corresponding COPY relocation declarations. These observations
expose a real limitation in the current FUNC/JUMP_SLOT-only verifier, not an
approved profile. See the typed-binding proposal in [design.md](design.md).

The proposal separates definition provenance, callable pointer/code extent,
consumer slots and effective main-copy storage. COPY source scope must be proved
or refused. Mutable/relocated data contents are not file-byte pins. The planned
link-only observation diagnostic measures its own optimized versions/imports;
bootstrap and closed-admission optimization prevent treating it as final adapter
closure. No link or native action is selected. reviewedProfile stays null and
all existing provider/mapping/startup/Reader/metatype/capture/recovery gaps and
all nine native acceptance tasks remain open. Source basis: current SDK source
and REM-49's saved compiler/passive-file receipts; proposed rules are design.

## Typed-binding implementation checkpoint

Root accepted the precode and selected the local diagnostic and bounded source
implementation. The optimized observation-only diagnostic links, retains the
actual dispatcher call, selects __aeabi_atexit@CXXABI_ARM_1.3.3 and has 15 undefined
OBJECT imports. Its 56 external ABS32 sites and local TLS/weak CRT forms remain
outside this increment; they are concrete next review inputs, not implicit support.

Typed versioned FUNC/provider-OBJECT metadata, exact reviewed JUMP_SLOT/GLOB_DAT
sites, tagged-callable/untagged-code ranges and data mapping checks are implemented
locally. Main COPY explicitly refuses because loaded source scope is unavailable.
The original profile remains null. Owned emitted ARM-file and host mapping controls
prove only metadata/guard mechanics; actual RM1 compiler checks prove source
compatibility. They do not execute the ARM fixture or diagnostic, establish actual
loader bindings or qualify the tablet. No RM2 checks/pins/helper ownership changed.
The exact final adapter and all nine native acceptance tasks remain unfinished.

## Finite source slice selected: observed ABS32 and artifact-self

| Case | Source check in this slice | Still required / refused |
| -- | -- | -- |
| External FUNC A0, OBJECT A0 and exact Cxx vtable A8 ABS32 | Original file addend, exact definition/version/site, full mapped extent and whole live word/state | Actual provider/runtime correspondence and main COPY source scope; no native pass |
| Exact weak/default artifact-self ABS32/GLOB_DAT/JUMP_SLOT A0 | Separate finite table, raw versym1/defined symbol and exact own-image address | No blanket weak exports or global RTTI/interposition proof |
| Optional CRT/TLS/dependency closure | No implementation selected | Remain unreviewed/refused; original nine native tasks remain open |

Finite ABS32/self source verification uses owned compiler-emitted ARM/Thumb files
parsed by host code, never loaded. Eight positive groups include provider ARM,
Thumb, OBJECT A0 and both exact A8 vtables, plus weak self ARM/Thumb/OBJECT.
Ninety-four refusals cover wrong addend/name/version/site/domain/definition,
whole-word/state/image arithmetic and executable/ambiguous/partial/zero-fill
relocation places; existing typed98refusals and PT_DYNAMIC13refusals remain.
Owned host mappings separately exercise full storage extent and wrong live words.
A common passive probe refuses the same emitted FUNC ABS32 A0 on delivered0109269
and accepts it after this change; it is a runtime parser behavior difference,
not a new-API compilation failure. Matching SDK ARM admission compilation and
fixture syntax checks pass. Production reviewedProfile remains null. All nine
native tasks, optional CRT/TLS/closure and actual runtime bindings remain open.

Optional CRT source selection covers only _ITM_registerTMCloneTable GLOB_DAT,
_ITM_deregisterTMCloneTable GLOB_DAT and __gmon_start__ GLOB_DAT/JUMP_SLOT.
Separate no-provider records require exact weak/default undefined NOTYPE raw1
value0/size0 and actual live zeros. Retained emitted-file initializers do not
prove live zero, startup safety, loader behavior or any native qualification.

Finite CRT local evidence: the matching compiler emits the exact three undefined
weak NOTYPE raw1 names and four sites. Host checks accept exact passive metadata
and actual mapped zero words, refuse nonzero/provider-like words and the actual
nonzero PLT file initializer copied to owned host storage. Eighty-eight controls
include actual symbol/type/visibility/version/value/size and relocation-table/
original-word mutations, alongside missing/duplicate/unknown/domain checks.
Sixteen profile controls cover count, names, forms and cross-domain collisions.
Matching SDK admission compilation and fixture syntax pass; no ARM file is loaded.
