# Maintained launch-and-kill discriminator proposal

Status: October 7, 2026, proposal/preparation only. Main selected comparison and
this amendment, not implementation, utility execution, upload or a native trial.
Main remains sole tablet operator. This supersedes the selected attach/custom-server
lifecycle in [the original measurement plan](input-normalization-discriminator.md)
and [disposable-server amendment](disposable-debug-server.md). Preserve those documents,
the uncommitted custom patch and all evidence as unfinished history; do not discard,
archive, continue expanding or silently reactivate that patch.

## Fixed candidate and source basis

Prefer the existing signature-derived, unmodified GNU14.2 ARM server, SHA256
`06e575662e78e28142decba916b9a684f73ccf30c7ceaffb5873b4bdaf486670`, with pinned
Windows GDB15.2 SHA256
`7f581a2f21a2bde99930f394a5b6b0d9b547e8a9b690fad161eef3faa871f6a2`.
Use direct Windows SSH stdio, `--once`, and a single launched disposable inferior.
No attach, TCP listener, reconnect, extended-remote, multi-process session, private
release command or successful detach is selected. Keep source-only/default-unarmed
entrypoints; imports and ordinary invocations must not launch or connect.

GNU14.2 `linux-low.cc:create_inferior` creates the process with attached=0;
`post_create_inferior` applies options from `linux_low_ptrace_options`, whose
!attached branch requests EXITKILL. The clone path uses existing Linux tracing
machinery; source intent does not prove actual thread protection. `server.cc`
exit cleanup kills created inferiors instead of detaching them, and --once makes
connection loss terminate the session. Protocol k also kills inferiors. The Linux
kill path stops threads, sends SIGKILL/PTRACE_KILL and reaps them. These maintained
paths remove the attached-process detach default, not the need for qualification.

The [official GDB server manual](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Server.html)
documents SSH stdio and a wrapper such as `env LD_PRELOAD=...` that affects only
the child. Exact local14.2 source also redirects inferior stdout to stderr and
stdin to /dev/null. A fixed reviewed child wrapper must preserve intended child
environment and working directory, keep the server outside LD_PRELOAD, and route
child diagnostics through a separately identified bounded private sink. Server,
SSH and child diagnostics must have explicit allocations within the existing
stdout65537/stderr1024 limits; no unbounded file, merged raw console dump or silent
overflow exception. Freeze quoting, paths, wrapper hash and environment before use.
The GNU14.2 wrapper is assembled by the shell-startup path; do not combine it with
`--no-startup-with-shell` and assume it still applies. Qualify the exact fixed shell/
wrapper invocation and sanitize the server/SSH environment separately.

## Explicit protection limitation and refusal gate

Unmodified `gdb/nat/linux-ptrace.c` probes EXITKILL using a temporary child, masks
unsupported options and ignores the final SETOPTIONS return. It does NOT enforce
the prior proposal's checked-every-LWP requirement. This amendment replaces that
claim with qualification limited to the frozen server/client/kernel/profile and
tested launch/clone/reset/failure paths. Never describe it as checked enforcement
or universal protection. Passing one dummy death case does not cover other paths.

Before any UI candidate, Main must qualify actual owned multi-thread launch with
server SIGKILL, client death, EOF, startup errors, clones (including shared-address
space concerns), option resets, active internal/client breakpoints and both capture
stops. Unsupported, missing, ambiguous or lost protection, unexpected surviving
threads/processes, or progress after tracer death with patched code refuses the
path. A delayed external kill does not prove prevention of unsafe progress.
Record inability to establish necessary protection as a blocker, not a caveat that
permits continuing. Independent source and actual-kernel evidence must be accepted
for the exact scope before selecting a native candidate.

If that gate cannot be met, the next possible change is narrowly launched-only
mandatory EXITKILL support and checked SETOPTIONS/fatal cleanup, through another
exact proposal/source/artifact review. It is not selected here and does not revive
the full attach/write-ledger/private-release framework. Never modify or replace the
original server silently or claim unsupported-kernel behavior from a version string.

## Bounded startup and dual readiness

The first program exec stop precedes plugin loading and candidate readiness.
Preparation must specify a finite startup checkpoint sequence, using reviewed
loader/event stops and exact plugin/QtGui/provider mappings before installing the
two normalization capture breakpoints. No guessed loader address, general memory
scan, arbitrary breakpoint search or open-ended continue loop. Unexpected stops
or missing mappings consume the attempt and invoke kill recovery.

Propose a maximum30s startup from launch dispatch to candidate-ready plus debugger
armed/resumed as an engineering ceiling pending exact startup timing qualification.
It is not derived from a verified historical startup measurement. Main corrected
the earlier roughly15s attribution: the14999/15066/16197ms observations concern
post-input end-publication GUI acceptance/grab/completion, not candidate Ready or
startup latency. Do not reuse those observations as startup evidence. Validate
the exact30s bound on the owned full path before use; an incompatible result requires
another reviewed bound, not an automatic extension. Keep the existing150s overall
native lifetime/operation bound, including startup, acquisition, kill and restoration.

Retain a separate5s measurement deadline beginning at installation of the first
measurement breakpoint, covering armed readiness, the one input and paired capture
through bounded termination dispatch. The startup and measurement deadlines may
overlap; neither restarts or extends the other. Independent recovery and confirmed
process exit remain inside the overall bound. This explicitly replaces attach-to-
detach5s; it does not claim a launch can finish in5s. Freeze these timer origins and
the complete recovery allocation with the consumer plan before source selection.

Main's single existing input occurs only after BOTH fresh same-generation candidate
Ready and debugger armed/resumed readiness. First-hit id/state/current-caller and
same-call thread/frame/handler/contact checks are unchanged; never choose a hit by
expected X/Y. Startup checkpoints are not additional successful measurement captures.
The exact permitted startup stops/counts must be frozen before target qualification.

## Measure, persist, kill, restore

Capture the two PRE/POST scalar snapshots using the existing exact fingerprint/
layout contract. Preserve raw double bits and bounded partial bytes before semantic
classification, plus durable count/hash receipts including prepared completion.
Explicit acquisition bytes remain<=1024, aggregate private evidence/receipts<=8192;
no fallback reads or automatic second launch/input. One atomic claim precedes launch.

After the second snapshot is persisted and classified, terminate the disposable
inferior through the reviewed standard kill path; NEVER detach or resume it to
complete a success workflow. On any failure, use the same termination policy and
independent exact-generation guard. Persisted measurement success is separate from
confirmed target/tracer exit and successful restoration. Missing kill acknowledgement
or exit evidence remains unknown; never replay the input or infer completion.

Because no instrumented process is released back to service, the custom all-site
restoration ledger and one-use D authorization are unnecessary for this candidate.
Ordinary debugger removal/stepping between PRE and POST may still modify instructions.
Advance notice and qualification must cover implicit ARM single-step and loader
breakpoints; two captures never imply two text writes. The fixed collector still
cannot issue arbitrary memory/register writes or general tracing commands. Removal
of the restoration ledger is not a claim that debugger writes disappeared.

Before stock restoration, confirm BOTH selected inferior and tracer have exited,
and the existing helper has completed its100ms down/release, bounded one-second
owned window and50ms drain/quiescence checks. Killing the inferior while input is
down does not authorize restoring stock onto an unreleased contact. Preserve the
existing independent input-release/recovery policy for helper failure; do not add
another tap or silently replay input. Guard setup precedes launch and cannot depend
on the candidate, GDB, gdbserver, failure markers or the host SSH session. Owned dummy
cleanup never restarts stock; native cleanup restores original service configuration,
fresh stock generation and fixtures under the existing Main-owned procedure.

This produces diagnostic same-call evidence, not a completed navigation action,
native callback/completion receipt, final UI capture or integrated workflow pass.
Any later correction and integrated navigation test need their own causal evidence
and review. Do not promote old unknown receipts or close unfinished native gates.

## Next gates

1. Independently accept this exact SDK proposal and consumer revision; freeze the
   finite startup checkpoints/timers, child diagnostics and always-kill success path.
2. Main selects focused source changes only after preparation is concrete. Retain
   accepted host policy/fixture evidence while adapting the native lifecycle; the
   current host adapter's direct detach is not reusable for launch-and-kill.
3. Independently review exact source, frozen standard utilities, wrapper/guard and
   bounded host tests. No target transfer or launch follows from this proposal alone.
4. Main qualifies the complete actual owned launch/clone/reset/death/EOF/kill path;
   only accepted evidence permits selecting one fresh UI candidate and one input.

Source basis: current Main selection and correction of the startup-time attribution, verified local
GNU14.2 source, official GDB documentation and accepted host-only source/receipts.
All proposed timings and actual-kernel protection remain unqualified here.
