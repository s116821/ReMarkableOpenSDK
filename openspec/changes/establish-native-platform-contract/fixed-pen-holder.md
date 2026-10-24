# Standalone fixed-pen holder

## Placement and fixed handoff

The recovery holder runs outside xochitl's cgroup, launched by the independently
scheduled experiment actor. Do not use a QProcess child inside the modified UI:
stock service restoration could terminate that recovery holder. Qualification of
holder/kernel/actor failure remains separate; descriptor close is not pen-up.

The proposed ARM-only CLI is `--wait activation_nonce candidate_pid start boot_id
setup_end_ms`. It uses only the corresponding fixed `/run/rmb-qt-probe-<nonce>`
private directory, its owner/attempt.identity and original actor deadline.
The setup end is absolute CLOCK_BOOTTIME milliseconds and cannot be renewed.
Exact source/artifact/provider/device/route admission remains with the outer
packet. This program is a research diagnostic, not arbitrary input injection.

After checking the measured RM2 profile, independently opened observer, writer
and recovery descriptors must refer to the same character device/inode/ID.
Publish one private pen-holder.ready record containing the holder and candidate
identities and original setup end. A one-use holder claim prevents replay.
Wait for one atomic pen-go.request containing `nonce pid start boottime_ms\n`.
Consume it before parsing; malformed, stale (over250ms), changed or expired
handoffs refuse rather than wait for a replacement. A fresh callback-local
Reader join in the UI publishes that request as its terminal action. No borrowed
Qt pointer or guard crosses the process boundary. This is an idle-fixture
sampled handoff, not atomic kernel routing to a document.

## One child, one independently held recovery

Before child start, require two matching released kernel snapshots and an empty
observer queue. Do not drain old events to obtain admission. Reserve the original
one-second stroke/three-second recovery budget and arm the separate published
Recovery policy before any writer child can act. Use a start pipe so the parent
can record the child PID/start before releasing it. One child uses the published
DispatchClaim and fixed frames, constructs vendor input_event records directly
and makes one nonblocking write per frame. No evdev retry loop, eraser, arbitrary
coordinates, screenshot or navigation belongs here.

The parent retains its own recovery descriptor and continuously observes bounded
event records while checking child/candidate identity, cancellation and original
time. UI stop/cancel or candidate loss stops further child dispatch; it must not
revoke the recovery holder's release permission. Fence a child that cannot finish
within its original stroke window before release. Child failure/short pipe result
keeps possible-dispatch facts and never starts another stroke.

The parent emits at most one five-record recovery group through its retained
descriptor. Confirmation needs the [sticky stream validator](fixed-pen-stream.md),
a bounded20ms quiet observation within the original recovery end, fresh matching
released kernel state and checked descriptor identity. Record actual raw event
bytes with a131072-byte cap; short/malformed/dropped/unknown trace or storage
failure prevents full acceptance. Filtering unchanged events is allowed;178
constructed records is not an observed event-count requirement. A snapshot-only
check after discarding a failed stream cannot become positive release proof.

## Result and failure boundaries

After the child is confirmed gone and all owned descriptors are closed, publish
one bounded (4096-byte) pen-result.json atomically, preserving pending/partial
files as forensic evidence. Bind schema fixed-pen-holder/1, nonce, candidate and
holder identities, original setup/stroke/recovery clocks, dispatch/child facts,
copied stream/snapshot facts, trace digest/count and recovery outcome. Native,
render, durable and atomic authorities remain false. stdout retains the same
original result; stderr is diagnostic only. No result retry is an effect retry.

Holder readiness, write acceptance, coherent contact, positive release, target
ink and saved/reopened ink are separate facts. Any unobserved release prevents
the UI close/reopen sequence. The outer actor must account for a launched holder
and possible contact before restoring the UI; its separate failure/backstop
contract must be reviewed in the complete packet. No native run is selected by
this design or an owned build. Test real owned child/pipe/descriptor boundaries
with modeled device IO before any native writer selection.

The selected native packet must launch the holder in a dedicated transient unit
outside both xochitl and the outer actor/supervisor cgroups, with no BindsTo or
PartOf coupling that kills it on their failure. Its absolute original candidate
window and one GO remain unchanged. Record the independently observed holder PID,
start, cgroup/unit identity and exit status. UI restoration may cancel the writer
but must leave the holder alive to fence the child and attempt original-budget
release. A holder/kernel failure or uninterruptible unfenced child is explicit
unknown; no normal positive result, late release, renewed deadline or universal
OS recovery claim follows. Bound and review transient-unit cleanup separately.

The UI consumer must require original result bytes equal the completed holder's
stdout and the canonical compact serializer output (including its one newline),
a closed schema, canonical decimal identity/clock strings, bound trace bytes/hash
and every positive dispatch/stream/release/closed-descriptor fact. Reject duplicate
keys, unknown fields, JSON type coercion and overlong inputs. A separate outer
actor completion observation must bind the expected holder identity, report its
known exit0 after it is gone, and confirm no pending result file. The result file
alone cannot advance UI phases. Consume one admission claim even on failure;
never renew original deadlines or interpret diagnostic/refusal bytes as success.
Include the exact matching pre-arm initial snapshot as well as the final snapshot
in the closed result. Independently replay the original trace from that initial
state and compare every copied stream fact; unchanged-value filtering prevents
inferring an initial coordinate merely from the first observed event.
Owned callers supply modeled file/process observations; native ownership,
no-follow inode checks and independently observed process exit still need the
separately reviewed outer actor. A JSON assertion is not authentication.

The current native process uses both the systemd-v1 and unified-v2 cgroup
hierarchies. Require both to name the same dedicated holder service and inspect
both for remaining processes before completion/cleanup. Retain successful
transient-unit exit information with RemainAfterExit=yes, since the manager may
otherwise unload a completed successful unit before it can be witnessed. The
positive completion state is active/exited, no MainPID/job, exact ExecMainPID,
known exit0 and no members; later guarded stop/remove is a separate cleanup step.
Never confuse active/exited with a still-running holder or inactive/dead with
retained authenticated exit information. Keep --no-block for oneshot launch so
readiness/GO cannot deadlock on a client waiting for the holder's final exit.

## Native parent threads and a separate writer image

The first composed trial refused before holder readiness or any GO. A later
read-only native profile observed three threads in the Qt-linked helper while
its original file/process/clock controls passed. This contradicts the proposed
single-threaded Qt-parent prerequisite; it does not establish the exited trial
process's exact thread count or qualify fork behavior on the tablet.

The successor design keeps the existing fork path for owned fixtures and makes
the native backend require a separately pinned, Qt-free POSIX writer image.
The native parent uses posix_spawn with explicit descriptor actions, never a
fallback fork. Duplicate source descriptors above the reserved child roles,
map writer/start/cancel/result to fixed descriptors3/4/5/6 and close all other
nonstandard descriptors. Resolve the executable through the parent's pinned
file descriptor while it remains alive across spawn; qualify the exact libc
spawn/closefrom support rather than guessing its availability.

Pin and hash the writer image against a private packet-bound digest before
holder readiness. The fresh writer validates its one-thread state, vendor ABI,
descriptor/device roles, parent identity and original budget before the sole GO.
It reuses the fixed DispatchClaim/frames and existing child result framing.
Only after this backend is mandatory may the native Qt-parent single-thread
gate be replaced. No guard is relaxed while the old native fork route remains.
The parent retains the existing child-incarnation/wait/fence, observer and
independent recovery behavior; wire facts and all original deadlines stay fixed.

Add actual owned exec/descriptor/status and failure controls, then separately
measure the same production writer's read-only profile on the tablet before a
newly selected stroke. A profile branch must return before any device access.
None of this design qualifies native writing or changes the failed trial into
a pass. See the [pre-GO result](../../../docs/experiments/write-reopen-pre-go-native-result.md).
