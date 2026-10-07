# Fixed development normalization discriminator

Status: proposal for exact independent review, October 7, 2026. Parent selected
this investigation through focused implementation, review and a small controlled
trial once its gates pass. Main alone operates the tablet. This does not select a
navigation correction, production debugger dependency or generic tracing API.

## Evidence and ownership

The private Astra trace receipt `93c27c98129e99bc02c36d24545312a1944a8236436cd02231ed439b06ebac45`
and consumer Docs `8a3133d6c75e87fc664986ba403325afc013ba50` describe the exact
saved RM2 plugin `678574220af960704c8f2b622f3e9208f5a0548dad2f636f3f13ead330c312ec`
and QtGui `93fe582cc61673342ca49e12306d7f689860016582fa46ce135ffa280a972839`.
The helper's independently observed interior axes are 164,1396. Historical setup
reports X0..1403/Y0..1871, protocol B and filtered=no; the two later handler points
already have normalized(0,1) and rectangle center(0,1871). These are historical
engineering observations, not promotion of the strict unknown receipts.

Inspected normalization, QPointF mapping and field copying use doubles. Identity
and the inspected centered rotations/reflections cannot turn the expected interior
point into the corner. Actual handler contact, bounds, matrix, binding and later
mutation remain unresolved. SDK owns this exact runtime measurement contract and
original focused tooling. Buddy/Docs owns candidate preparation, one input action,
independent recovery, fixture preservation and the eventual integrated workflow.

## Mechanism choice and qualification gap

Prefer standard cross-GDB plus a task-owned ARM gdbserver. Existing SDK public-Qt
event observation occurs too late; sampled process memory cannot correlate one
normalization call. Interposing QTransform::map would miss the identity bypass and
does not expose the input contact. A fixed inline trampoline would add instruction
relocation, cache coherency, register/flags preservation and in-process recovery
risks. No custom hook engine or speculative coordinate/ABI patch is selected.

Sol's private inventory `636eaf4444b9d1ead0fcc68f95d94a38d9ddfa7e4635c058bc9489afcbc587f9`
reports vendor-image416c7a7 cross-GDB14.2, SHA
`07cccce897774dffb8530630fefbde363e75f4d42dcf6b573040e840e448feef`, working
Python3.12.13 and ARM configuration. Main reports no target gdb/gdbserver and no
readable ptrace policy file. Neither report proves target ptrace/breakpoint support.
Sol reports a signature-verified GNU14.2 server build, 429068 bytes, SHA
`06e575662e78e28142decba916b9a684f73ccf30c7ceaffb5873b4bdaf486670`, with
provenance receipt `b92617a31f71f8ecf7a1c15924cc48351759eacc45b10bfb2845618b8ddf8975`.
ELF inspection and QEMU version/help passed; neither establishes target operation.
The ARM hard-float executable uses `/lib/ld-linux-armhf.so.3` and needs libstdc++,
libgcc and libc, including GLIBC2.38 and GLIBCXX3.4.29. Main's target string inventory
is preliminary compatibility evidence, not ELF symbol-version binding proof.
Exact provenance, license, build invocation, ABI, interpreter, shared-library closure
and actual target operation must be reviewed and qualified. Do not install
packages or replace system libraries. A temporary exact-path upload is Main-only.
GPL distribution obligations remain explicit if these tools are later distributed;
this development proposal does not bundle them into the SDK product.

The documented standard transport is `gdbserver --once --attach stdio PID` over
one authenticated SSH stdio channel, with host GDB `target remote | ...`. No TCP
listener, reconnect, --multi, arbitrary inferior launch or public debug service.
Sources: [GDB server command documentation](https://www.sourceware.org/gdb/current/onlinedocs/gdb.html/gdbserver-man.html)
and [remote connection semantics](https://www.sourceware.org/gdb/current/onlinedocs/gdb.html/Connecting.html).
These describe upstream behavior; qualify the exact14.2 build before relying on it.

The authorized SSH client and credentials live on Windows, while pinned cross-GDB
lives in Docker. Do not copy credentials into the image. First assess the installed
Windows gdb-multiarch15.2 direct `target remote | ssh.exe -T ...` path. Sol reports
that explicit `set osabi GNU/Linux` before ARM selection resolves the Windows-default
OSABI warning; exact ARM ELF recognition, embedded Python and a harmless local pipe
open/EOF check passed. These local checks do not qualify remote ARM/VFP operations.
If the actual owned fixture qualifies all required Linux/ARM register, breakpoint,
thread, flags, memory and detach operations, this standard path avoids a new relay.
Otherwise the candidate is a narrow one-session duplex pipe relay between Windows
SSH and `docker -i`: container stdin/stdout exclusively carry RSP, and an internal
loopback-only bridge supplies cross-GDB in that same `--network none` container.
No host/tablet listening socket, host.docker.internal dependency or external port.
Keep fixed bounded streaming buffers; diagnostic output and evidence use separate
bounded private sinks. This is a fallback transport proposal, not permission to
build a general proxy. Freeze ONE successful path and its exact commands before
the target dummy gate. Disconnect/EOF/timeout must close both directions and invoke
independent recovery; do not claim pipe closure itself safely detaches the inferior.

## Two-stop same-call contract

Offsets below are private exact-build adapter details, never public API or portable
addresses. Derive load bias from the exact candidate's file-backed mappings and ELF
PT_LOAD offsets, cross-check executable permissions/device/inode and complete file
hashes. Refuse ambiguous mappings, wrong ARM state, unexpected instruction bytes,
wrong provider, changed process generation or absent readable ranges before arming.

Use PRE at plugin+0x1563c, after both divisions and stores, and POST at+0x15664,
the join after identity bypass or QPointF mapping. Both belong to the function
starting+0x15528. This refines the earlier entry/post sketch: two stops here supply
the actual pre-map doubles without a third entry breakpoint. Expected ARM words
are 0x0a000001 and 0xe59d006c respectively; qualification must independently verify
them from the frozen ELF. Preserve CPSR, including condition flags for PRE's branch.
Standard software breakpoints temporarily change two instruction words in the
candidate's process memory. They are not read-only observations or on-disk patches.
This effect must be explicit in Main's advance notice and recovery qualification.
Do not silently substitute hardware breakpoints without qualifying that exact path.

At PRE: r5 is handler-private H, r4 contact C, r6 H+0x90, and SP is the fixed
normalization frame. Capture integer registers PC/CPSR/SP/r4/r5/r6 and thread ID;
active slot at H+0x40; C id/X/Y/major/pressure at offsets0/4/8/12/16 and state byte
at20; four signed bounds at H+0x58..0x64; nine IEEE754 doubles at H+0x90..0xd0
and two cached type bytes at H+0xd8; actual pre-map doubles at SP+0x20/+0x28.
The 0xb0-byte frame follows the inspected push/vpush/local allocation; saved caller
LR is SP+0xac. Current-contact caller return+0x17e5c and old-contact release return
+0x17fd0 are separate observed call sites, not interchangeable attribution.

Select the first normalization hit of the armed one-input window. Require the
frozen expected contact id/state for the selected down action (id1/state1), ARM
state and current-contact caller; a different first hit refuses rather than looping
until convenient data appears. Never require expected X/Y to choose the sample.
Disable/delete PRE after its one hit, arm POST for that same debugger thread, and
resume normally. No scheduler locking that could strand a loader/other-thread lock.
At POST require the same generation, thread, SP, H, C and saved caller LR. Read
post-map stack doubles and repeat the small contact/bounds/matrix snapshot. A
mismatch is incomplete evidence, not a second attempt. Cached type bits may change
legitimately during type calculation and must be recorded rather than forced equal.

All-stop debugger observations perturb scheduling. Contact/bounds/matrix snapshots
are taken adjacent to the arithmetic, not proof of every earlier load under arbitrary
concurrent mutation. Preserve original values and raw bit patterns, including
nonfinite doubles, without substitution. Numeric summaries use explicit validity
and never create a capability. Observed thread/frame correlation cannot establish
physical input origin or an unperturbed navigation result.

## Bounds, persistence and restoration

Use an atomic private one-use claim before attaching. Fixed script only, with GDB
init/auto-load/debuginfod and automatic shared-library loading disabled; no inferior
function calls, expression evaluation with effects, arbitrary address walk, stack
backtrace, memory search or general RSP debug log. Read only reviewed fixed ranges:
at most1024 target bytes total for the two snapshots and instruction checks, at most
two successful capture stops and one attach. Cap host raw evidence at8192 bytes and
combined diagnostic stdout/stderr at65537/1024 respectively. Record individual
read lengths/status and bounded partial bytes before semantic classification; save
and read back private bytes/hash/count after each stop. No raw target memory or
logs are printed or published. GDB's own protocol bookkeeping is not claimed to
obey the snapshot byte cap; the host process and channel have a separate wall bound.

Proposed debugger subdeadline is5s from attach dispatch through detach, within the
unchanged candidate's original total/lifetime budget; no timer enlargement. This
subdeadline and acceptable stop latency must pass the exact owned target dummy
before candidate selection. Main's input begins only after exact armed/resumed
readiness, and the existing helper's bounded down/release/echo limits remain intact.
If those limits cannot coexist, report the incompatibility and revise the proposal;
do not silently relax either or retry input. No post-stop waiting for user interaction.

Recovery is independent of GDB, gdbserver, the candidate and the host SSH session.
Before attach, Main must arm and qualify a bounded external guard for the exact
debugger/candidate generations. Normal completion deletes breakpoints, detaches,
verifies gdbserver exit and continues the existing completion/stock restoration.
Host failure, debugger disconnect, missed POST, unexpected stop/signal, target exit,
partial persistence or deadline consumes the claim, records unknown, and invokes
the qualified recovery sequence. Do not assume disconnect detaches/resumes safely.
If tracer teardown may leave modified text or a stopped candidate, terminate that
exact disposable candidate and restore stock; never resume unknown patched code.
Qualification must cover a stopped tracee that cannot process SIGTERM: the external
guard must be able to terminate the exact tracer and candidate generations and
verify their exit before stock service restoration, without depending on the pipe.
Prove original stock configuration, fresh stock generation, fixture preservation
and exact task-only cleanup after every actual trial, including failure. No reboot,
firmware/account/pairing/sync change or persistent activation is authorized here.

## Required gates and interpretation

Before code: exact SDK proposal/design/tasks/delta plus consumer owning plan receive
independent review. Then focused implementation and exact source freeze review.
Before native activation: exact utility ABI/dependencies and complete scripts pass
host synthetic tests and Main-only harmless owned target dummy qualification.
Use an original owned ARM fixture, not copied proprietary function bodies. It must
exercise real debugger register/memory/flags preservation and paired stops for
identity and nonidentity double transforms, multiple threads, wrong thread/frame/
provider/instruction/generation, unexpected first hit, no hit/missed post, timeout,
disconnect, host/debugger/server death and partial/unwritable evidence. Verify
that every failure consumes the attempt and recovery leaves no attached/stopped
owned dummy. Qemu/GDB tests alone cannot prove target ptrace or recovery behavior.

Main selects a fresh candidate/baseline and one controlled action only after those
gates; workers never attach to the tablet. MESSAGE/envelope fixes are unnecessary
for this direct measurement and remain unselected. Existing spent packets/receipts
remain immutable. Do not sync/archive the investigation or declare navigation fixed.

Interpret the same-call evidence conservatively:

1. Wrong raw contact: inspect the slot/contact/event-consumption path.
2. Correct raw contact but changed bounds/matrix: inspect runtime configuration/state.
3. Consistent raw/bounds/matrix but unexpected actual pre/post arithmetic: inspect
   exact runtime implementation/binding or concurrent mutation, preserving both values.
4. Interior POST with a later corner: inspect later point storage/mapping; this pair
   alone does not measure a later log/event and requires a separately correlated result.

Only a source-supported cause permits the smallest justified correction, followed
by independent review, controlled validation and the requested integrated navigation
workflow test. This proposal does not preselect that correction.

Source basis: current parent/Main selection, Main target inventory, Sol host inventory,
exact private binary trace and historical bounded forensic evidence; linked upstream
GDB documentation. Mechanism, budgets and tests above are proposals awaiting exact
review, not claims of implementation, target support or successful recovery.
