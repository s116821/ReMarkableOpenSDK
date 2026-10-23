# Original capture readback: owned files, process clocks and frame

October 23, 2026 UTC. Separate source/owned checkpoint after the
[capture Entry](identity-capture-entry-owned-result.md). The file reader,
process/clock wrapper and strict original-byte frame parser now have independently
reviewed owned controls. **No tablet reader execution, new native capture or
whole operator/provider/recovery qualification is claimed.**

## Three separate boundaries

The file helper duplicates its borrowed directory before permission callouts and
retains six original files: owner, attempt identity, ready, request, scalar and
PNG. It checks exact declared binding bytes, owner-only regular single-link files,
named/FD identity and metadata, absence of pending/cancel/restore/stop markers,
bounded chunks and EOF. All descriptors remain pinned through both reads.
Refusal returns no partial bytes and neither retries nor modifies the files.

After the last permission call, the helper destroys its copied callable while
descriptors and the reentry gate are still live. A final no-callout metadata/gate
check and checked closes precede byte exposure. Only owned originals escape.
Its static liveProcessAuthority remains false: an injected permission is not a
live process or provider observation.

The separate Admission wrapper checks the original root pathname, pinned
deadline/window identities and bytes, actual declared PID/start and boot ID.
Its 10s cooperative collection budget begins at construction, and admission is
repeated after helper/callable unwind and before/after one frame write.
Production main is ARM32 little-endian hard-float, requires UID0/Qt6.10.3/no
inherited injection variables, and derives one fixed nonce root. That main was
compiled/linked only; owned controls exercise its exact helpers.

Entry's scalar times are relative QElapsedTimer values. The wrapper invents no
absolute Entry clock. Its absolute operation bound is the original boot-clock
observation BEFORE request publication plus 30s: consumption cannot precede
publication, so this bound expires conservatively. The candidate's original 90s
window begins before Entry construction and similarly expires conservatively
for setup. Neither clock is renewed. Full target executable/provider/payload,
actor/service/route and final whole-reader clock brackets remain obligations of
the separately reviewed operator.

The C++ frame has one versioned ASCII header, declared bindings and bounded exact
lengths, original scalar/image bytes and one terminal marker. The Python parser
requires exact binding, header, total length and terminal bytes, then passes
unchanged slices into the strict JSON/PNG consumer. Embedded markers cannot split
opaque payloads. Frame acceptance preserves external_admission=unverified and
all native/render/durable/atomic flags false.

## Actual owned verification

| Component | Result | Evidence scope |
| --- | --- | --- |
| Pair file reader | Host/ARM-QEMU/ASAN+UBSAN each 181 scenarios / 3,936 checks; production header compile | Real owned files and lifetime/permission boundaries, modeled external admission |
| Process wrapper | Each platform 104 / 2,676, empty stderr | Actual own-process PID/start/boot, clocks, files and single-write behavior; target/provider facts unqualified |
| Frame consumer | Four synthetic methods plus 3 original C++ frames | Exact frame/byte-consumer compatibility only |
| Production reader main | ARM compile/link only | No main execution or device qualification |

Controls cover file/type/mode/link/symlink/size/EOF changes, descriptor retention
after caller closure, permission loss/throw/reentry and partial output refusal.
Wrapper controls distinguish a disappeared process from a killed but unreaped
zombie, exercise pinned control/root replacement, malformed/overflowed clocks,
request/candidate expiry, and an actual >10s constructor-budget expiry. Exception
unwind closes retained FDs. Child RLIMIT controls produce real short writes
without retry.

Each final C++ frame is 2,125 bytes with SHA
bbe8d74e83ad4d7c520ff2e2f6d64b983313cb1964ecef884ddd5c870278fe2b.
It wraps the exact shipped owned scalar / 119-byte PNG, with its original declared
PID/start and actual owned-test boot ID. This is explicitly format-only: the
static declared PID is not relabeled as the fixture's live process, firmware
readiness or a tablet observation. All original assets and frame outputs are
retained unchanged.

Independent review reproduced a predecessor callable-destruction bug:
observed1/destructor_fired1/nested_refused1/named_scalar_changed1. The copied
function's destructor could reenter or replace a file after a positive result
had been selected. The corrected helper destroys it before the final pinned
no-callout checks; destructor reentry/replacement/caller-FD closure controls pass.

The first reader fixture also observed a legitimate sampled-metadata limit:
creating an unrelated directory entry can leave equal coarse metadata. The
explicit metadata-change control now changes mtime deterministically; no
directory scanner or atomic-stability claim was added. FailedR1, pre-fixR2 and the
actual predecessor counterexample remain preserved. WrapperR1 passed its initial
controls; R2 adds zombie/unwind cases and a stable expiry setup.

## Source and evidence identity

| Item | SHA-256 |
| --- | --- |
| File reader | 344c94b1a54da888f28c80f725618b1132d2473d67a06147dec39a02689c5c79 |
| File fixture | 97224be106a49b9d40abd2fcb81bd76ae8ad23557eb842651d61e0391e1cbf6e |
| Process wrapper | 2b69d699fb33a1051de1b740d2cbdefb4b1fec314d1445e6efa078c44c5db809 |
| Process fixture | 20610b956f45aeea93c6c213258769617dde14de631730055c5a0ae18a2402c8 |
| Frame parser | ffad976c0d72b005743fb711fd5bc27d777ea032b6186c3da8ab72e399801c4a |
| Verified file matrix | 99e3cfbb7dff358143140fb6ed766dbb46f085831f102ebdc7e3cd95c3db89fa |
| File lifetime counterexample | 6fa9eddc38d8bff897c4fcbec55021565c60d173f6a3d92adf034ea463676f92 |
| Independent file-reader audit | 6f94514de26cb763ca4ed4ff855aa253b22f64bd6773d3af37c39d8e3256083d |
| Verified wrapper matrix | 5fce16e2d3548773d93efb0bbb1fe2b3f9beb46c217c5ded5a26efe179bfb093 |
| Root frame/actual-output audit | 21d2400297f0c00d26dbb8bd8891e1c31cc7da4aad7e38d8afba22a0366dd9ef |
| Independent wrapper/frame audit | 267dbad1acc3e895acf2b2ffa320c1ed0646eefefe4576e77a320784ca81e8f3 |

Tests use the existing pinned RM2 SDK container, host Qt6.4.2 and ARM Qt6.10.3/QEMU.
ASAN+UBSAN leak detection is disabled. Python byte/frame checks use the existing
Python 3.12.14 / Pillow 12.3.0 environment; no installation or global MCP change was
made. Private raw logs, complete source manifests, binaries and original outputs
remain outside Git. The existing shipped synthetic pair is reused.

With the existing native toolchain or SDK environment:

    sh tools/reader_capture_pair_read_test.sh
    sh tools/reader_identity_capture_readback_test.sh
    python -B tools/reader_capture_pair_frame_test.py

Optional --cpp-export accepts each wrapper runner's frame-exports directory.

## Remaining native obligations

The [file design](../../openspec/changes/establish-native-platform-contract/identity-capture-readback.md)
and [process composition](../../openspec/changes/establish-native-platform-contract/identity-capture-process-readback.md)
are research controls. They do not replace exact full operator/actor/provider
review, fresh admission against the eleven-page/24-file fixture, independent
recovery, actual image/refusal collection, original document preservation,
restoration or inactive-root cleanup. A new native capture remains a separate
attempt, with no second creation or grab retry.

Source basis: current source, actual saved owned-process/file executions,
unaltered original C++ frames and independent source/output reviews. No SDK native
capability or physical-page/content relationship is inferred.

Later native result, October 23, 2026: [useful identity-bracketed capture and live collection](identity-capture-native-result.md) separately records one fixed idle-RM2 result and its recovery/preservation/cleanup audits. This owned checkpoint is not relabeled as native evidence; broader native/product capability remains open.
