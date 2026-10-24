# Native writer failure during contact

October 24, 2026 UTC. A separate fixed fault writer was killed during actual
tablet contact. Its independent parent release was written and observed, the
child was reaped, and all descriptors closed. The sequence correctly refused
normal completion. Stock restored automatically; Buddy needed a separately
verified, input-free reconciliation. All 26 stored document files stayed identical.
This qualifies one failure mechanism on the recorded RM2 build.

## Source and owned controls

The normal writer remains unchanged. The separate
[fault source](../../tools/fixed_pen_contact_fault_writer.cpp) is the exact
retained native build input, SHA-256
77b300d489c4267340bfef49104af1b41b8cc266cbba5fb8486575e16b70a1d3.
Its changes are reversible to the normal writer: a signal include, successful-frame
counter, self-SIGKILL after eight complete writes, and an explicit fault-profile
label. If signal delivery returns, it exits91 without further input. All existing
ABI, one-thread, descriptor/device, parent-incarnation, original-budget and sole-GO
guards remain. The profile returns before device access.

[Owned regression source](../../tools/fixed_pen_contact_fault_fixture.cpp) uses
actual spawn/exec, FIFOs, pipes, wait status and independent release. Two parent
background threads remain alive; context/device snapshots are explicit models.
Host and sanitizer runs each pass one case/27 checks: SIGKILL9, no complete child
message, eight written frames plus one release, six contact frames, one contact
interval, partial stroke, released final state and closed descriptors. Original
owned traces are592 bytes/37 records/9 frames. The first strict build failed on an
unused inherited test helper; that failed run remains retained before the
maybe-unused annotation. A later assertion incorrectly expected one QEMU thread;
the actual QEMU profile reports two. That failed assertion is preserved separately
from the passing owned executions and native one-thread measurement.

The same ARM owned-FIFO regression ran on the tablet and passed1/27 without
input-device or UI/service changes. The separate native fault-image profile
measured one thread, ARM event layout16/8/10/12, and no profile input IO.
Native-owned archive SHA-256:
76d20d7725e443916afec067e24f1fe7087501801e8615ae1a8872f503ce7d03.

The [public runner](../../tools/fixed_pen_contact_fault_test.sh) accepts source
and output directories and host/asan/arm via READER_FIXED_PEN_CONTACT_FAULT_MODE.
For example, run `sh tools/fixed_pen_contact_fault_test.sh tools /tmp/fault-host`
in the existing Linux Qt build environment. ARM mode uses the same pinned
RM2 SDK environment as the neighboring spawn test. It compiles/profiles only;
it does not contact or activate a tablet. QEMU does not qualify native child exec.

The published runner was executed against all11 exact frozen public inputs:
host and sanitizer each1/27, ARM compile/profile, and original input rechecks pass.
Its ARM fault and normal images reproduce the two native digests below exactly.
Host compiler is GCC13.3.0 with Qt6.4.2; the native build uses the RM2
5.8.203 SDK environment and the retained pinned build image
sha256:416c7a7be0038156797b0892f031f352b841d1921fae83f712d0a272e4724618.
These publication checks establish source/artifact correspondence; they are not
another device-input test.

## Exact real-input candidate

| Item | Recorded value |
| --- | --- |
| SDK base | 82ffa46656be9657faf9ccfd3609aa2360142d81 |
| Spent nonce | 57ea6fd6bc3146b8b8d790f206c07437 |
| Profile | RM2, firmware3.28.0.172, Qt6.10.3, ARM32LE hard float |
| Fault writer | 1f66fb22be5da0f0b1ba29d2179983a723dd67b94559f8a108d2838cff9e982b |
| Unchanged normal writer | a4b768cc39b73dd5912304c06694f290a2fffc3173d6d69b745c0feaf07805eb |
| Payload | b9dec6a575b7a6383c03187dc17f3d129a43a83125d32655c70a4171a8a58de4 |
| Native unit control | 9a00f9723ff17ead84911b0effa6ee61579ec08e6fb35e3c9699f520d6678700 |
| Fixed control source | fcd20912c39cea318a543679f6b9d060b4c141f375ae5b2046925146d73da54c |
| Existing blank target | index8/count12, ee6cd386-54c7-4fdd-8b9f-b43b2e4fe3a9 |
| Candidate | PID6611/start137675046 |
| Holder | PID6754/start137675288 |
| Writer | PID9398/start137680760 |
| Outer actor | PID5202/start137671340 |

One fresh request/GO followed actual target identity and blank capture. No page
was inserted and neither earlier written target was reused. The original actor
was1376713..1377073 seconds; setup1376750..1376870. Setup120, operation maximum90,
actor360 and recovery reserve185 remained unchanged. Full493-input selection
c9f3193c2aaf8cec305053efedac36f7e27531b574b09cda66cb2c4ec8956884
binds source/build/provider/packet/owned controls and prewritten collectors.
The host's negative-result publication originally preceded its expiry check;
actual modeled fault tests exposed that ordering and the repaired operator
passes97 checks. Earlier failures remain retained.

## Original negative result and actual release

The child was reaped with signal9, exit=-1 and no complete child result. Dispatch
remains unknown/contact-possible. The original pen result is:
624320dff72a28fe339dc06a61eb29ba1480b89d0f9aba4c01b1ab549d2f6143.
The native trace has448 bytes,28 records,8 frames,6 contact frames and one contact
interval. Start endpoint is present, end endpoint absent. Filtering explains the
difference from the written/owned record count; this is an incomplete stroke.
Original trace:
4808fae07c96f2856680abd2e8ebdb7b198d224f5b8257fcf364bf3ed12fcdb6.

Independent parent release was armed, attempted, written and observed with
unknown=false. Same-device, context, initial snapshots/empty queue, final hardware
snapshot, quiet20ms and device/IPC/trace closure facts pass. Separate
[pure C++ replay](../../tools/fixed_pen_contact_fault_trace_replay.cpp) consumes
the untouched native/host/sanitizer traces and matches all report fields. Its
owned controls also reject truncated records, SYN_DROPPED and a held final
snapshot. Byte replay does not independently establish device provenance; the
original native bindings/snapshots/outer evidence supply that separate boundary.

The sequence returned observed=false, pen-refused and reconciliation_required=true;
no close/reopen dispatched. Its normal positive pen-result/trace acknowledgement
hashes are empty. They remain empty: the negative evidence was audited separately,
without manufacturing a positive18-file frame. Original sequence:
d6eb63ccf78d75e970a47c0d68b93868b36403fe427d9efff7d1c893f8ff33ca.
Original full runtime archive:
df687c89c0fcfae8a224e10fb55712f1d243aaaac3e9d48427b4ea24d60e06ef.

## Restoration and preservation

The independent actor restored stock10366/start137682767. It retained
recovery.failed/input uncertainty and withheld Buddy: pen unit exited1, actor90.
Both MainPID0/empty jobs and cgroups, and actual candidate/holder/child/actor
incarnations gone, were separately verified. This is a real limit of automatic
whole-service restoration, not normal recovery success.

The full stable before/archive/after comparison finds all26 files and12 page IDs
byte-identical to the pretest/b339 fixture. Archive750080 bytes:
03442292e88ffe219a29bd9d45ce26a4b2ad823387a1be33cb0557a9178abf27.
No stored target ink, thumbnail, order, PDF mapping, prior ink, local or unknown
metadata changed. This does not negate actual dispatched contact or prove that
an ambiguous operation is safe to retry. The target remains spent.

A prewritten, one-use input-free reconciliation verified original negative
release, C++ replay, complete preservation, exact stock/provider/service state,
gone owned processes and empty units before restoring the initially active
Buddy. It restored11238/start137697200 with exit0/stderr empty. Fresh read-only
checks confirm stock/Buddy/sync active, no diagnostic mappings or runtime shadows,
and empty test units; terminal properties/journals remain retained. This separate
reconciliation does not erase the actor's conservative failure outcome.

Together with the [complete normal sequence](combined-native-write-close-reopen-result.md),
this supplies a measured success path and one child-failure/release path. It does
not qualify arbitrary OS/D-state faults, a general renderer, power-loss durability,
hot unload, indefinite owner reclamation or other models/firmware. Production
transport, policy and consumer qualification remain separate.

Source basis: exact source/artifacts, actual host/sanitizer/native-owned execution,
original admitted native records and raw trace, actual C++ replay, complete
storage backups and fresh restored-service/process/cgroup outputs. Proprietary
binaries, raw documents and private receipts remain local.
