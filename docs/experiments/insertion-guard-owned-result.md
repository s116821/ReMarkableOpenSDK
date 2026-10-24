# Owned ARM insertion guard and unwind result

October 23, 2026 UTC. The owned ARM/QEMU fixture passed **17 cases and 361
checks**. Independent source review and a second execution reproduced the
same binary and result. Two actual function-entry hooks forwarded through
relocated originals. A private rejection from the owned insertion entry
unwound the original handler's QString temporaries and mutex before its outer
wrapper caught it. This qualifies the tested owned principle only.

## Source, tool inputs and explicit compatibility

Original [fixture source](../../tools/insertion_guard_owned/owned_fixture.cpp)
SHA-256 is
`ad67d1c111372a62fd6f24aa36b561ebfaecc8fb31a68b5a03d23a38ef64f961`.
It contains no firmware bodies, native declarations, private offsets or tablet
actions. The intercepted functions, map, events and operation record are owned
models. Its Task-like completion signal is a model, not the stock Task.

The optional test library is [Dobby e9fe7f](https://github.com/jmpews/Dobby/tree/e9fe7fbecae47a2287e761080f8b1133cc22e8fa),
an earlier coherent revision, not a claimed stable release. Its source archive
SHA-256 is
`b5dddb530ae3b1abe8b61dd3a690eaf0ffe74900b0313c3d1bee0597691cf2d3`.
The pristine 280-file tree is preserved and rechecked. Dobby's Apache-2.0 license
and notices remain applicable; no dependency is added to the Rust SDK.

The build required explicit compatibility work:

- The [preinclude header](../../tools/insertion_guard_owned/compat-gcc-armv7.h)
  accepts only GCC ARMv7 hard-float without pointer authentication. It supplies
  the missing Clang feature query as false. All eight upstream occurrences
  query only pointer-authentication calls; target predefines were retained.
- The [owned CMake wrapper](../../tools/insertion_guard_owned/CMakeLists.txt)
  selects C++17 and preincludes that header for the library targets.
- A derived build tree applies the one-token
  [logging overlay](../../tools/insertion_guard_owned/logging-inline.diff):
  make the header-defined Logger::Shared inline. The pristine tree stays intact.
  An actual earlier link failure from its duplicate strong definitions is
  preserved. No arbitrary duplicate-symbol linker allowance was used.

The newest upstream pin's missing-header/build failure, the earlier pin's
feature-query failure and the first fixture link failure remain separate
failed attempts. They are not reclassified by this pass.

## Reproduction and observed controls

The checked-in [library script](../../tools/insertion_guard_owned/build-library.sh)
and [fixture script](../../tools/insertion_guard_owned/build-fixture.sh) record
the actual compiler commands. They expect an operator-provided RM2 SDK at the
declared path, the tool files mounted at /inputs, library-build source at
/source, output at /out and the built archive at /library for the fixture.
Create a separate copy of the exact upstream source, apply only the logged
overlay there, and retain its license/notices. Do not overwrite an upstream
checkout. The fixture's /source needs the matching upstream public header.

Actual toolchain: GCC 13.4.0, Qt 6.10.3, CMake 3.31.10 and QEMU 8.2.7 in the
pinned SDK image
`sha256:416c7a7be0038156797b0892f031f352b841d1921fae83f712d0a272e4724618`.
Containers had read-only roots, no network and dropped capabilities. Source
inputs stayed unchanged. The SDK image is local build infrastructure, not a
published redistributable toolchain.

| Case group | Checked behavior |
| --- | --- |
| Accepted and transparent | Exact integer, fractional hard-float QSizeF and QString reference forwarding; one insertion and separate queued GUI notifications |
| Wrong source/order, collision, normalized index | Explicit refusal with no insertion/page-added; generic completion remains a separate fact |
| Cancellation and observer loss | One cancellation/insertion CAS winner; loss before claim refuses, loss after claim does not erase an effect |
| Deadline and early effects | Original deadline remains fixed; an earlier modeled loading effect survives later refusal |
| Private and unrelated exceptions | Original RAII cleanup precedes the private catch; unrelated pre/post-insertion exceptions propagate to an outside test catch |
| Nested and duplicate work | Foreign map/thread work stays transparent; duplicate eligible dispatch cannot insert twice; postclaim failure is not a no-effect refusal |

All cases check worker-side scope restoration, original temporary destruction
and mutex availability. Hooks are installed before worker creation and remain
resident until process exit. No runtime unhook is tested.

Both executions produced fixture SHA-256
`a71d37c3ea5a07ed49ee0c3d8ae291e9720fc425a9a615eca9808f58751d0367`,
linking archive SHA-256
`127e314de06686551e57784d70df092c54c29f2fb275ff0e71aa4b0d29367797`.
The source-author package receipt SHA-256 is
`ea7b3459f8d221bab7f1119c3558837c73765a37c11da2564b5029a49d4f7551`.
Independent replay remains separately recorded rather than attributed to the
source author's run.

## Limits and next question

Both first-stage prologues relocate a simple ARM push and frame-pointer setup.
This is not coverage for Thumb, PC-relative instructions, branches, arbitrary
generated-trampoline PCs or the shipped executable's original unwind tables.
An upstream ARM relocator still warns about a shift count equal to its type
width; other preserved warnings include a missing default return in a
memory-permission helper. Passing this path does not clear those issues.

The [native order boundary](../research/execution-time-page-order-boundary.md)
and [separate study](../../openspec/changes/establish-native-platform-contract/insertion-predicate-interposition-study.md)
still require exact native ABI/prologue/install qualification, a bounded
nonlocking map predicate, operation matching, an explicit Task outcome channel,
GUI-visit coherence and ownership/recovery evidence. No tablet mechanism,
native rejection, rollback, safe unload or supported creation capability is
selected by this owned result.

Source basis: original owned source, exact upstream archive and logged overlay,
current compiler/ELF/QEMU outputs, independent source review and actual replay.
The proposed transfer of this principle to xochitl remains an inference.

## Separate machine-location and VFP followup

A distinct owned increment passed **four cases and 56 checks**, with independent
source/assembly review and a second actual execution. Its
[carrier declarations](../../tools/insertion_guard_owned/abi/carriers.h),
[compile-only callers/callees](../../tools/insertion_guard_owned/abi/compile_carriers.cpp),
[owned entry veneers](../../tools/insertion_guard_owned/abi/owned_entries.S),
[runtime controls](../../tools/insertion_guard_owned/abi/runtime.cpp) and
[build script](../../tools/insertion_guard_owned/abi/build.sh) are original code.
Mount that ABI directory at /inputs for this script, with the same upstream
headers at /source, previously built archive at /library and fresh /out.

GCC 13.4 caller/callee assembly puts the chosen owned outer carrier at
R0/R1/R2/R3-low-byte, D0/D1 and entry stack offsets zero/four/eight. The inner
carrier uses four core registers and two full signed 32-bit stack words.
Public Qt size/alignment assertions support these owned declarations. They
match observed native machine locations, not recovered native C++ types.
The optional QString-shaped carrier is an owned object, never a fabricated
native instance.

The two actual hooks relocate generic ARM push-nine-registers/core-move
prefixes. Subsequent VFP saves run normally. Accepted, private-rejected,
unrelated-exception and transparent-wide-inner cases preserve exact arguments,
original RAII/mutex cleanup and D8/D9 register sentinels. The transparent case
retains both full stack words, including a sixth argument larger than a byte.
The outside catch and hooks have no D8/D9 save/restore masking these probes.
The extra owned veneer/body frame and its EHABI are explicit differences from
the actual native body and its original unwind tables.

Independent replay produced the same runtime result and identical loaded ELF
segments, code and unwind sections. Full binary hashes differ:
`153f7b51333664bff5988f9b3eb1742b8452acb1d18a4014f083fb2953c753ea`
and `093c2ffe630ff5871dcd221c949f121be295029a37e9d084b19842d5c8838a1e`.
Exact comparison attributes the only difference to a temporary object filename
in a nonallocated STT_FILE string-table entry. The binaries are not described
as byte-identical. The followup package receipt SHA-256 is
`62952bad675d628dbb7e4593eaee56d46712e51b611534b8b43f7645f238002c`.

This narrows carrier and simple-prefix uncertainty without qualifying live
native installation, every relocation instruction, trampoline-PC unwinding,
original native C++ declarations, map predicates or ownership. The
[negative native witness proposal](../../openspec/changes/establish-native-platform-contract/native-insertion-guard-refusal-witness.md)
therefore remains separately gated. This increment does not repeat or expand
the first-stage 17-case matrix by counting its replay as new coverage.

## Later ARM mode repair and remaining installation limits

A subsequent source-only review found that the pinned library's constructor
sets its mode flag true for Thumb addresses but does not initialize it false
for even ARM addresses. The earlier owned paths passed, but those observations
do not supply a general allocator-zeroing guarantee.

The original [placement test](../../tools/insertion_guard_owned/repair/constructor.cpp)
prefills storage with nonzero bytes, constructs the real class using owned data
tokens and reads only the unsigned object representation. It never evaluates
the old uninitialized bool and never patches or executes those token addresses.
The old constructor reproduces the defect in two even-address cases; the new
constructor initializes false in both while retaining true for odd addresses.
Each variant passes 13 checks across four placement cases.

A separate derived tree applies the prior logging change plus exactly one
[mode initialization](../../tools/insertion_guard_owned/repair/thumb-mode-init.diff)
before the existing Thumb branch. The pristine tree and earlier library/results
remain intact. The [constructor script](../../tools/insertion_guard_owned/repair/test-constructor.sh)
uses the same SDK and public header tree, with old/new archives at /oldlib and
/newlib. Its /inputs contains the constructor source and the unchanged guarded
compatibility header; use fresh /out.

Repaired archive SHA-256:
`622b17dc65d0dcad717d83264f783ad12044565ed8ea86f9e5088d9abf5328ef`.
The unchanged 17-case/361-check and four-case/56-check sources both pass against
this archive. Source and saved-result review are separate from the earlier
independent executions of the prior archive. Repair-package receipt SHA-256:
`e1876de59b4fcfa64c006039a0468b5a1bdf28afe13a457123c57f66712eb08e`.

That repair does not change routing, memory protection, error returns or unload.
The source review also finds that DobbyHook can report zero without propagating
patch failure, while the Linux patch helper ignores protection-call results.
Native admission must verify actual entry/trampoline bytes and mappings;
uncertain installation fails the candidate and uses independent stock recovery.
Registry removal is not a thread drain or safe unload. Exact source-review
receipt SHA-256:
`6b858a124249434f959d6cbc3edf379066cffa4cc8af06a0617f7360bfffb831`.
The relocator warning and all native qualification boundaries remain open.
