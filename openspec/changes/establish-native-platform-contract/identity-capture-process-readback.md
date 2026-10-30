# Fixed capture process admission and transport readback

October23,2026 UTC. Separate composition of the
[read-only file helper](identity-capture-readback.md) and the original-byte
consumer. This design selects no activation, payload transfer or native packet.

## Exact wrapper

Add a standalone research reader, not another in-process adapter or IPC manager.
Its only production CLI accepts nonce, candidate PID/start, boot ID and the
original pre-request CLOCK_BOOTTIME millisecond observation. Validate canonical
bounded arguments; derive only the fixed /run/rmb-qt-probe-NONCE root. Production
requires effective UID0 and exact private root/control-file ownership/modes.

Retain root, deadline and candidate.window descriptors across collection.
Check their original FD/name identities and original bytes on fresh admission.
Parse the original actor360s and candidate90s clocks and185s recovery reserve;
reject malformed, mismatched, changed or expired controls. Never renew a clock.

The synchronous helper's external Permit freshly checks the original root
pathname, pinned controls, expected boot ID, live candidate PID/start and its
own original collection budget. The helper separately checks pending/cancellation/
restoration markers and its six retained file descriptors. After helper return
and copied-callable destruction, the wrapper repeats fresh external admission
before serializing or writing a frame.

Exact target executable, payload/provider identities/mappings, recovery actor
liveness, service policy and route admission are separately bracketed by the
frozen host/operator packet before and after this reader process. A generic
process check in the reader does not prove xochitl/provider identity by itself.
The composed consumer needs both the reader result and those independently
reviewed original operator observations.

## Conservative absolute operation clock

Entry scalar times use QElapsedTimer relative to Entry construction, not an
absolute boot clock. Do not invent an Entry-to-boot offset or treat the scalar
clock as an absolute current deadline.

The operator records a boot-clock lower bound immediately BEFORE its one original
request publication, bound to the same nonce/PID/start and transport intent.
Consumption cannot precede request publication. Requiring readback before that
lower bound plus30s is therefore conservative for the original30s operation.
Likewise, the candidate window begins before Entry construction, so its original
90s end is conservative for Entry's90s setup. Suspend-inclusive BOOTTIME can
expire earlier than QElapsedTimer; it must never extend these bounds.

The reader requires current boot time at/after that original request lower bound
and strictly before its+30s limit and the original candidate window end. It also
has one original10s cooperative collection budget, including setup/unwind/final
admission. The operator must use the same original lower bound after the reader
returns. No failed/uncertain request or read is automatically republished or
recollected, and no later clock replaces the original lower bound.

## Original byte frame

After successful file collection and final admission, emit one fixed versioned
length frame containing declared nonce/PID/start/boot, exact scalar/image byte
lengths, original scalar bytes and original PNG bytes plus an exact final marker.
No pretty JSON, normalization, repair or decoded image substitutes for originals.
Write the one bounded frame once; a short/error write is refusal with no retry.

The operator archives raw stdout/stderr and exit status and accepts a frame only
after its final process/provider/clock/restore brackets succeed with zero exit.
It checks exact framing/length/caps/binding and then invokes the existing strict
original-byte validator. A partial frame, stale/refused result, lost bracket,
unsupported PNG or orphan remains forensic. No path advertises native, render,
durable, atomic, physical freshness or page-content correspondence authority.

## Owned qualification

Exercise actual own-process PID/start/boot observations, private files and pinned
control replacement, plus explicitly labeled modeled target/provider facts.
Cover conservative request/candidate/local deadlines, malformed arguments and
clocks, process loss, changed root/control names/bytes, cancellation, callback
unwind, framing/original bytes, write failure and no effect/retry. Run host and
ARM/QEMU controls and available sanitizers before any separate actual packet.
