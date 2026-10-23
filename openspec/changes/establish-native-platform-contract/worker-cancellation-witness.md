# Cancellation while a native callback is already running

The [positive worker tuple](../../../docs/experiments/worker-local-order-result.md)
exercises normal delivery/reclamation. This distinct default-off diagnostic tests
ownership after logical cancellation. It does not request native queue
cancellation, suppress already-entered work or revive the canceled request.

One frozen parent diagnostic authorizes an ordinary fixture open, fresh GUI
source/controller/worker join, and one worker callback with the same six passive
ID/index reads. Its plain work context first announces that it is waiting, then
waits at most two seconds for the GUI registration's planned logical cancellation.
The waiting loop uses only owned atomics and bounded sleeps. It performs no
QObject/QML/GUI operation or filesystem access on the worker.

After the source Sample and its enclosing quarantine Frame unwind, a precreated
GUI timer may observe that waiting marker. Require the callback entered once
off-GUI, not returning, registration depth zero and native leases not closed or
destroyed. Cancel the original Pool token, record its Quarantined state, then
release the plain work gate. Only that already-authorized callback continues its
fixed read-only work on the retained receiver. Failure to observe cancellation
within the work wait does not authorize the reads or a retry.

The registration retains both native strong references across the cancellation,
later reads and native completion. It must remain Quarantined: no close, release,
holder destruction, phase resurrection or second native submission follows from
late completion. The process-resident storage retains the slot until process exit.
The parent diagnostic may report this expected cancellation experiment; it must
not report the canceled operation as a normal completed request.

The Pool may record a separate bounded late-completion count only for the same
slot/source incarnation, known returned nonzero job ID, exact matching ID and one
off-GUI callback/returning pair. Cap this diagnostic count at two so repetition
cannot wrap into a positive one. Unknown/foreign IDs and early observations that
cannot be attributed do not supply a late completion. Recording never renews a
clock or changes canceled/quarantined state. A notification can reenter; final
parent publication additionally requires depth zero after its Relay Frame unwinds.

WHEN the planned cancellation is observed while work waits, THEN the callback
may finish its preauthorized passive reads while owners remain quarantined. WHEN
exact late completion and matching six-read facts arrive within the original
plan, THEN report `worker-cancel-observed` with explicit canceled/quarantined and
unreleased-owner facts, all authority flags false. WHEN timeout, parent cancel,
root/request replacement, source/window loss, duplicate completion or any other
failure occurs, THEN refuse without retry or native-owner cleanup.

This is compiled self-cancellation within an authorized forensic plan. It is not
a general external transport cancellation protocol, a promise that canceled work
stops, or permission to act after the parent diagnostic is canceled. Preserve
the existing tuple mode as default and its exact wire. Precreate all new QObject
helpers before sampling. Keep the original90s setup/30s plan/360s actor/185s
recovery reserve and final stock restoration; no wait extends those clocks.

- [x] Add/review only bounded late-completion diagnostics to the quarantine and
  verify exact/missing/foreign/duplicate/canceled/nested source cases on host/ARM.
- [x] Add a separate frozen Entry/startup scenario and owned waiting/cancellation,
  continued-read, quiescent-report, deadline and normal-mode regression controls.
- [ ] Freeze/review a fresh source/artifact/profile and run once if concrete
  gates pass; preserve full bytes, actual restoration and exact cleanup.
- [ ] Interpret actual owner retention/late completion separately from normal
  request success, native cancellation, teardown or supported capability.

Source basis: current quarantine/Entry source, exact native callback copying and
post-return job-completion contract, and the prior success-path native tuple.
This cancellation protocol is proposed engineering design, not native evidence.

Owned verification: 318 quarantine checks pass on host, ARM/QEMU and host
ASAN/UBSAN (intentional process-resident records exclude leak checking). The
Entry's 31 modes pass on host and ARM/QEMU, with both default and cancellation
startup variants compiled separately. These include a deterministic grant after
the two-second worker deadline, requiring zero accessor calls, and nested late
notification, parent cancellation, deadline, request replacement, missing and
duplicate completion. The expiry field records the actual clock deadline;
an early parent cancellation does not itself imply that deadline elapsed.
All are owned fixtures. No cancellation-mode native result is claimed here.
