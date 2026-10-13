# Engine-ready pre-token shutdown discriminator

Source preparation only, selected October 8 after the installation-only cohort
recorded a bootstrap-refused callback. Preserve that evidence and its frozen
source. No fresh nonce, matching native build or device operation is selected.

## Question and smallest change

Observe successful original engine bootstrap before admitting a later sampled
render as the STOP trigger. Keep original startup/signal/window order, queued
bootstrap, synthetic configuration, both request-admission fences, single owned
root, independent actor and all budgets unchanged. This changes exposure phase
and timing; it cannot isolate an individual engine connection, watcher or filter.
No active page getter, request publisher, contact, forced rendering, event pump,
manual cancellation or application quit is added.

`facts-waiting` alone cannot order a render inside the trace. Add one diagnostic
event, `entry-engine-ready`, synchronously after the original successful waiting
file write and before the existing queueRequest call. Require actual successful
engine-destroyed connection, valid current entry/engine context, live root/identity,
no closure and unexpired setup budget at that boundary. Capture connection success
only in diagnostic builds; ordinary operations and their ordering remain intact.
The callback is installed before start and captures only the process-resident
recorder. Invoke it at most once, with no extra queued turn or retry if recording
is dropped or fails. Missing evidence remains incomplete.

## One selected proof interface

Use diagnostic kind `pretoken-engine-ready-v1` for the new source/proof variant.
Retain the ten-field v1 wire, 96 records, fewer than 192 bytes per record, 18432-byte
trace cap and first 32 normal frame pairs. Record real counters and flags.

The consumer requires startup, exactly one entry-installed with frames0, exactly
one later entry-engine-ready on the same startup GUI TID, then a before-render
with greater sequence and strictly greater frame count than engine-ready. Identity
must match throughout and all admission evidence must have quit_seen0/dropped0.
Earlier renders cannot qualify. Window/entry order otherwise remains unrestricted.
Unknown events, duplicate readiness and malformed values cannot admit. The
installation, readiness and qualifying render must each carry quit_seen0/dropped0.
A later about-to-quit or destruction record does not retroactively change that
earlier qualifying prefix; no additional global terminal-event veto is implied.
This is observed scalar order, not an atomic lifetime or
display fence. A concurrent render can advance the counter before its record;
strict higher-frame admission may therefore refuse an otherwise interesting trace.

Preserve `facts-waiting` as optional corroboration only. Its absence after recovery
does not negate a valid trace marker because finish may unlink it. No new actor
phase, file wait or ownership is introduced. The shared-root watcher-stimulus
confound accepted for P1-SINGLE remains explicit.

## Bound and refusal behavior

Keep observation cutoff min(now+30 seconds, absolute deadline minus the existing
185-second reserve) and the existing absolute 360-second budget. Never reset a
deadline on readiness. Thirty seconds bounds an attempt; it does not guarantee
bootstrap or later rendering. No engine, ambiguous engine, refused context,
connection/write/closure refusal, readiness too late, absent later natural render
or exhausted 32-frame sampling gives inconclusive recovery. Do not poll a new Qt
loop, force a frame, expand sample caps or relax recovery to obtain evidence.

## Verification before native selection

- Owned successful bootstrap emits exactly one marker after waiting creation;
  actual pre-ready frame counts are preserved and only a later higher frame can
  meet the proposed proof.
- Failed waiting write and restoration closure before bootstrap emit no ready
  event. Start-refusal and deferred lifetime cases remain covered.
- Valid unexpected normal/source/capture/input requests remain fenced after
  readiness: no owner discovery, page getters, reader or capture output.
- Owned engine destruction preserves queued completion/deleteLater without manual
  cancel or forced Qt quit. No native SIGTERM lifecycle is inferred from it.
- Macro-absent source equivalence, ordinary admission control and exact diagnostic
  include/MOC/compiler evidence remain required. Consumer parser tests separately
  cover wrong order, equal/lower frames, duplicates, unknowns, flags and timeout.

Source checkpoint: all twelve owned ARM/QEMU cases and ordinary admission control
pass. The three new cases verify readiness followed by a higher-frame sample,
failed exclusive waiting-file write, and restoration closure before bootstrap.
The prior request-purpose, start-refusal and deferred-lifetime cases now also
check readiness presence/absence and post-ready admission exclusion. Exact
connection return and post-write predicates are source-reviewed; no artificial
Qt connection failure or native lifetime claim is introduced by these fixtures.

A matching rendering fault after this evidence would show active page getters
unnecessary for that occurrence. Non-reproduction leaves the original cause open.
Independent source/consumer review precedes future artifact selection; production
qualification, canonical synchronization and archive remain open.
