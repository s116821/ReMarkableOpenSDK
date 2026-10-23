# Proposed worker callback witness

The [positive Reader/controller join](../../../docs/experiments/legacy-composition-navigation-join-result.md)
establishes a sampled source relationship on one RM2 build. It does not serialize
source/order validation with later native insertion. Retained native dispatch
analysis identifies a callback command as a useful separate investigation seam.
This proposal is not a qualified adapter or permission inferred from that join.

## Current static boundary

The inspected internal callback request copies a function into QVariant-backed
command storage, enqueues command type one and returns a job ID. The inspected
dispatcher directly invokes that function. The ordinary enqueue path appends a
command, notifies its condition variable and returns; it does not establish an
inline same-thread shortcut. Calling the usual AddPage request inside a worker
callback therefore schedules another command. It does not make the callback's
validation atomic with insertion.

The callback request is an internal C++ seam, not a declared Qt invokable. The
[focused static contract](../../../docs/research/native-worker-callback-contract.md)
records its ABI, instruction mode, receiver/base relationship, function-copy
lifetime, native strong-owner acquisition and enqueue/dispatch semantics.
The saved target remains executable SHA-256
`071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df`.
Unknown builds must refuse. Build-specific addresses are not public SDK APIs.

## First useful native question

After the source/control and exact-profile reviews, submit one bounded, nonmutating
callback to the worker associated with a freshly qualified selected document.
Record whether it enters once on a different execution thread and whether the
matching native job completion is observed. This establishes callback delivery
and attribution only. It performs no insertion, document-order access or native
GUI property read from the worker.

The proposed lifetime design is:

- The function captures only shared plain state containing atomics and immutable
  values. It captures no QObject, QML object, Entry pointer or borrowed reference.
  Establish that the native request copies the actual function, not a borrowed
  reference. If that lifetime cannot be proved, retain the function object itself
  for the process lifetime too; plain-state ownership alone is insufficient.
- Worker execution performs only bounded state updates and a current-thread ID
  observation. It does not call Qt/QML getters, write files or invoke GUI code.
- A marker written just before returning is labeled `returning`, not proof of
  having returned. An explicitly queued GUI receiver separately filters the
  exact returned job ID. Generic completion alone is not success: require the
  matching closure's independent entry/count/thread facts as well.
  Only claim an after-return boundary if the exact command's completion emission
  is established to follow callback return; skipped-command completion remains
  insufficient.
- Connect before submission. Handle completion delivered during nested GUI
  reentry before the submission returns its ID, without finishing on the native
  stack or accepting another job's completion. Bound any early-ID storage.
- Consume submission before the native call. False/exception/timeout/lost
  acknowledgement is not a reason to enqueue again.
- Cancellation/deadline disarms the GUI receiver and marks plain state canceled.
  Already queued slots must check terminal/disarmed state and the original
  deadline themselves; disconnecting cannot retract delivered/queued events.
  Native cancellation is not assumed. A late closure can still make its bounded
  state updates; shared ownership keeps those data alive. It must have no path
  back into a destroyed Entry or native GUI object.
- The payload remains resident. Do not unload code while the application may
  retain a copied function or its manager/destructor. Process/service recovery
  is a separate operation, not a claim that the queue was canceled or drained.
  Do not introduce static/atexit cleanup of function state while retained native
  jobs may still reference it.

Fresh window/engine/document/controller/worker qualification precedes submission.
The public worker getter is weak and cannot keep the receiver alive across an
enqueue that emits synchronous notifications. The proposed exact-build bridge
therefore resolves the sampled actual native ID through the current creation
controller into a retained native Document, acquires its already-existing worker
with mode one, and requires equality with the qualified SceneController worker.
Both native strong owners must cover submission. Mode one must not create a
worker or request additional work. Opaque aligned return storage and the native
control-release helper avoid reconstructing IDs or copying private owner fields.
These machine-level contracts require the pinned-image loader and independent
implementation review; a QPointer alone is insufficient.

Observation before submission and completion afterward remain distinct facts;
the earlier selected-page readiness must not be relabeled as a post-callback
observation. Keep exact process/request binding, one attempt, original absolute
deadlines and independent recovery. All mutation/render/durable authority flags
remain false.

## Verification and unresolved work

- [x] Finish focused native constructor/enqueue/receiver/lock review and record
  exact source/tool/evidence provenance, including negative findings.
- [x] Independently review the witness lifecycle and implement only if the
  native call ABI and receiver can be justified. Verify owned worker-thread,
  synchronous/reentrant, early/foreign/duplicate completion, cancellation,
  destruction, timeout and delayed closure ownership cases.
- [x] If selected, freeze one fresh no-op native profile and independently
  verify the exact build/packet/operator before one target attempt. Preserve
  complete fixture bytes, actual process restoration and pending/late outcomes.

Native checkpoint: [attempt 367219](../../../docs/experiments/native-worker-callback-result.md)
observed expected pre-submission identity/readiness, one off-GUI callback and
matching job 26 completion. All four positive counts were one; canceled count
was zero, refusals empty, unknown false and native owners released. Original
request/deadline binding passed. Stock 17021/start124282332 was restored, all
eighteen fixture files and twelve provider/service entries were unchanged, and
the exact archived RAM root was removed. Authority flags remained false.

October 22 source/control checkpoint: all eight owned diagnostic suites passed
on host Qt 6.4.2 (173.5 seconds) and ARM Qt 6.10.3/GCC 13.4 under QEMU 8.2.7
(281.3 seconds), with identical before/after source hashes. The separate native
bridge fixture passed 126 checks on host, ARM and host ASAN/UBSAN; leak detection
was disabled and explicit ownership counters were checked. Production headers
and all six startup variants compile. None of these fixtures calls the pinned
private native addresses.

Independent reviews found and resolved exceptional Submit-capture destruction,
disconnect lifetime checks and the Entry release-result assignment boundary.
Owned integration covers prequeue source facts, off-GUI callback/matching
completion, large lossless job IDs, original budget, cancellation, restore,
request replacement, thrown submission and uncertain post-link publication.
Earlier failed/rerouted test logs remain preserved. In particular, an invalid
test that reentered QObject destruction from disconnectNotify deadlocked both
owned runtimes; its replacement respects Qt's notification-hook contract. A
local runner initially selected ARM twice; the mislabeled duplicate was stopped
and the distinct host run above passed. No failed run is credited as a pass.

This successful witness leaves same-turn source/order validation and
insertion, GUI visit ownership, matching insertion-specific completion, actual
selected target, persistence and full lifecycle qualification unresolved. It
must not be promoted into a worker execution/exclusion lease or repeat the already completed
caller-selected UUID allocation experiment.

Source basis: committed native join and no-op worker results, retained REA/Ghidra
and ARM disassembly findings, original diagnostic source and owned test evidence.
Source/control review and native execution are separate gates. The native
witness did not mutate a page or call document-order APIs on the worker.
