# Task-only disposable debugger server amendment

Superseded for current selection by [maintained launch-and-kill](launch-kill-discriminator.md).
Custom-source expansion is paused; preserve the unfinished drafts and evidence.
The requirements below describe the prior attach-and-release proposal, not the
current candidate and not a qualified implementation.

Status: proposed October 7, 2026; source review required before modifying GNU
GDBserver. This amends [the normalization discriminator](input-normalization-discriminator.md).
It does not authorize a tablet attachment or qualify a new runtime artifact.

## Source finding and scope

The signature-verified GNU GDB14.2 source used for original server SHA256
`06e575662e78e28142decba916b9a684f73ccf30c7ceaffb5873b4bdaf486670` has these paths:

- `gdbserver/linux-low.cc:2200-2206` requests `PTRACE_O_EXITKILL` only when
  `!attached`. The selected `--attach` path does not request it.
- `gdb/nat/linux-ptrace.c:361-369` probes support; `378-393` masks unsupported
  options and ignores the final `PTRACE_SETOPTIONS` return value. Merely adding
  the bit to the caller is insufficient enforcement.
- `linux-low.cc:1012-1015`, `2356-2362` and `5938-5943` set/reset options using
  `proc->attached`; existing threads, new threads and later resets must be covered.
- The attach callback warns and continues for some non-disappearance failures.
  Partial thread attachment cannot establish coverage of a shared address space.
- `server.cc:3903` installs exit cleanup after `attach_inferior` at3893. Early
  partial-attach errors therefore need cleanup established before attachment.
- `server.cc:3483-3520` normally detaches attached inferiors on exit; the macro
  in `target.h:543` reaches `linux-low.cc:1539` and `detach_one_lwp:1460-1498`,
  which uses `PTRACE_DETACH`. This inspected chain does not establish restoration
  of all client software breakpoints. Normal EOF and tracer SIGKILL are distinct.
- The event-loop exception handler at `server.cc:4002` and top-level handler
  at4029 can unwind or exit; successful detach at1272 uses direct `exit(0)`.
  A scope destructor alone is not sufficient coverage of lifecycle exits.
- `linux-arm-low.cc:1014-1043` enables software stepping and reports no hardware
  single-step; `linux-low.cc:3921-3932` installs breakpoints at the predicted next
  PCs. Therefore step-over can modify more than the two primary capture sites.
- GNU14.2 `gdb/solib-svr4.c:2336-2412` uses `auto_solib_add` for symbol loading
  separately from creating the loader event breakpoint. Disabling automatic
  shared-library symbols alone does not establish absence of loader breakpoints.
  This is explanatory source evidence; exact Windows GDB15.2 behavior still needs
  qualification and must not be inferred from the14.2 source alone.

Original server06e575 remains immutable provenance evidence and is held from the
software-breakpoint attachment path. An external five-second guard cannot prove
that a process never resumed unknown patched instructions before that guard acted.
The kernel-enforced tracer-death behavior, plus controlled live-server failure
cleanup, must close that gap. Source inspection is not actual-kernel qualification.

## Fixed mode, identities and states

Propose one explicit task-only option, `--rem25-disposable`, requiring the existing
`--once --attach stdio PID` form, one selected disposable process and all-stop mode.
Absent this flag, GNU behavior remains unchanged, but that mode is never accepted
by the collector. Reject TCP, alternate COMM, launches, multiple inferiors, extended
remote, reconnect, non-stop, persistent/disconnected tracing and arbitrary monitor
operations in this mode. No firmware process names or hook addresses become a
public interface. The exact fixed native/owned-fixture profile and process generation
are supplied by Main's reviewed private preparation, not a general address CLI.

Host runtime remains SOURCEONLY/default-unarmed. Client hash remains pinned; server
hash must change to the reviewed patched build, with no fallback to06e575. Import
never attaches. The one-use claim and independent external cleanup guard are armed
before the first attachment, including an owned dummy. The guard binds boot ID,
PID/start time and tracer identity; it must not signal a reused process.

States are PRE_ATTACH, ATTACHING_UNQUALIFIED, PROTECTED_STOPPED, COLLECTING,
RELEASE_READY and RELEASED, with terminal FAILURE. These are reviewable lifecycle
semantics, not a new public SDK state API. Setup must establish failure cleanup
before the first ptrace attach, not after the existing GNU attach call returns.

## Mandatory protection before instrumentation

In ATTACHING_UNQUALIFIED, no candidate text, software breakpoint, general memory or
register writes are allowed. No collection/input readiness or ordinary resume may
be reported. GNU's internal transition from a preexisting job-control stop to the
initial ptrace stop requires explicit source review: it is not collection readiness
and must occur before any text modification. Refuse a profile if it cannot preserve
this unmodified partial-attach window.

Enumerate and stop every live LWP of the selected thread group using the existing
Linux attach/clone machinery. Unexpected attach failures are fatal; only positively
identified vanished threads may be removed from coverage. Unreadable or ambiguous
enumeration is failure, not an empty thread set. Require successful mandatory
`PTRACE_SETOPTIONS` with EXITKILL for each live attached LWP. Unsupported EXITKILL
must not be silently masked. Check the real syscall return at each application,
including after the feature probe, new-thread handling and later option resets.
Track enforced state only after success, and invalidate it whenever its premise
changes. No later option update may clear mandatory protection.

Do not permit breakpoint writes, explicit resume or armed readiness until every
live thread is stopped, accounted for and protected. Newly cloned threads must
remain protected through inherited options and checked initialization before resume;
qualification must cover clones created while the original thread is running and
while a breakpoint is active. Unexpected fork/vfork/exec or an extra inferior fails
this fixed session; cleanup must not accidentally detach a child sharing patched
memory. The exact patch must show how such event paths remain covered, without
adding an arbitrary process supervisor.

If attachment/protection fails before any text write, kill the selected disposable
process using the verified live identity and invoke independent cleanup. Failure
must never fall through an ordinary detach or a catch-and-continue handler. Errors
after protection use the same terminal failure policy. No standard GNU exception
or direct-exit path may convert failure into resume/detach of this process.

## Failure and the sole release path

Every text site modified by the debugger belongs to a bounded private ledger,
including primary capture breakpoints, internal software-step sites, implicit
loader/event breakpoints and any reinsertion. Propose at most16 distinct sites,
each at most4 bytes, further restricted to a reviewed fixed profile of permitted
module fingerprints/offsets/ARM-or-Thumb encodings. No arbitrary address input,
memory scan or unbounded accumulation is permitted. Read and preserve original
raw bytes before the first insertion, validate the expected provider/encoding,
and track insertion/removal state across reuse without overwriting the original.
An unexpected site, unknown write path or exhausted ledger refuses before that
write and enters failure recovery; do not widen the profile during a trial.

Explicit ledger capture and final verification reads count toward the existing
1024-byte requested-read limit, including failed/overlapping reads. GDB internal
instruction-decoding/protocol reads remain separately identified bookkeeping,
not a claim of zero memory access. Release must verify raw restored bytes for ALL
recorded sites, including already-removed step breakpoints, without memory shadows.
Source review and the actual owned fixture must identify all write paths, establish
that no ledger bypass exists and characterize the exact Windows client's internal
breakpoints. Prefer disabling unnecessary implicit breakpoint behavior where the
exact client supports it; `auto-solib-add off` alone is insufficient. No guessed
loader offset enters a native profile. Advance notice must describe additional
qualified internal breakpoint writes, not claim only two instruction words.

EOF, client death, protocol error, unexpected stop, evidence failure, deadline,
internal exception and incomplete coverage all select FAILURE. The live server
kills the selected disposable process instead of normal exit detachment. A bounded
failure marker is best-effort only. Independent cleanup must survive missing markers,
host/SSH failure and server death; SIGKILL of the server must rely on already-enforced
kernel EXITKILL rather than a signal handler. Verify target and tracer exit before
stock restoration. Dummy mode never restarts stock services.

Only explicit successful protocol D may release a protected process. The collector
must first persist both bounded snapshots, finish all semantic checks, remove its
breakpoints, verify original words and produce its bounded completion evidence.
An incidental D generated by client shutdown is not a successful release request.
Use a fixed, zero-address, one-shot private release authorization (proposed monitor
command `rem25-allow-detach`) issued only by the reviewed success path; it must not
exist as a general arbitrary-command interface. It arms one immediately following
D for this exact process, and any intervening error, resume, write, new thread or
identity change invalidates it. An unarmed D is terminal failure, never detach.

Before accepting that authorization and D, server checks must establish all threads
stopped/protected, no active inserted client/internal/step-over breakpoints or jump
pads, and ALL ledger words restored through a raw target-memory check that bypasses
GDB breakpoint-shadow substitution. Host checks after deletion remain mandatory;
their requested memory bytes, including any explicit server verification reads,
count toward the existing1024-byte collector budget. No read retry is introduced.
The exact fixture/native word bindings must be reviewed before implementation.

All checks precede the first actual detach. A partial detach error is terminal
failure and requires exact-process kill; restored text is mandatory before any
thread can detach, since kernel EXITKILL no longer covers a detached thread. Server
death during successful release must be tested separately from death with breakpoints
installed. RELEASED is recorded only after successful detach; a lost acknowledgement
still makes host outcome unknown, without replaying input or detach.

## Patch, provenance and qualification

Implement only after exact independent SDK/consumer amendment acceptance. Keep the
signed original archive/build intact. Retain the minimal GNU-source patch, changed
source hashes, patch hash, build invocation/compiler/sysroot, license/source bundle,
ELF dependency evidence and new artifact hash. The patch is explicitly a local
modification, never described as unmodified standard GDBserver. No system library
replacement, persistent service, credential copy or generic debugger framework.

Independent source review must trace every attach/options/reset/resume/text-write,
EOF/error/exception/exit and detach edge. Synthetic tests inject unsupported/masked
EXITKILL, failed SETOPTIONS, partial attach, unreadable enumeration, new threads,
generation changes, server/client/SSH death, early/forged D, release-check failure,
partial detach and exhausted byte/time/evidence budgets. A one-line source grep
or mocked syscall success cannot establish full-path behavior.
Include internal ARM step-over/loader sites, removal and reinsertion, restoration
mismatch at an already-removed site, unknown site/encoding, ledger overflow and
shadowed-memory false matches. Two capture stops remain the evidence limit;
internal stepping must not be mislabeled as a third successful capture.

Main-only owned-target qualification then exercises the exact patched binary and
Windows-client/SSH transport with multiple LWPs, including thread creation, server
SIGKILL before/after protection, with breakpoints active, at each capture stop and
during release; client SIGKILL/EOF; and startup/partial-attach failures. Prove process
death on protected tracer death, absence of post-failure progress in the owned
fixture, successful untouched-word release and complete bounded independent cleanup.
Unsupported kernel behavior refuses the native trial. Host/QEMU evidence is labeled
separately. No tablet attach occurs before source/artifact/guard review.

All existing limits remain: one claim/attach/input, two successful capture stops,
requested target-memory bytes<=1024, private raw evidence<=8192, diagnostics
stdout<=65537/stderr<=1024, proposed5s attach-through-detach inside the unchanged
native lifetime and helper100ms/one-second/50ms limits. Incompatible timing requires
an explicit reviewed amendment, not an automatic extension or second attempt.

Source basis: verified local GNU14.2 source paths above and current Main selection
to author this amendment. Enforcement design and proposed private release command
await independent review; no modified server or actual-kernel result is claimed.
