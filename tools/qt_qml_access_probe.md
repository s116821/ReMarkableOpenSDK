# Public QML access development probe

This original, development-only experiment adapts the public window/engine/QML
access concept from [rm-librarian 7d0fe086](https://github.com/rmitchellscott/rm-librarian/blob/7d0fe08678203c129b9d848419da2a7e96a2abe5/src/main.cpp#L41-L113).
No community CRUD, generic invocation helpers, persistent raw caches or endless
polling are copied. SDK license/publication decisions remain open.

The app-context startup callback queues one probe. It inspects at most sixteen
existing windows per attempt, accepts one distinct existing engine on the app
thread, and otherwise retries only after coalesced window Show/Expose/focus
events before selection. Eight acquisition admissions are permitted. Once selected,
the engine is never reacquired. The public root gate accepts only an existing
`QQmlApplicationEngine` on the app thread. It subscribes once to `objectCreated`
before checking `rootObjects`, inspects at most sixteen roots, and waits for
either an already loaded root or a successful root-load signal. A null signal
refuses as `root-load-failed`; a plain engine refuses as `unsupported-engine`.
Too many roots refuse as `root-cap`, with private root_count capped at 17.
Root list copying/allocation is owned by Qt; the cap bounds our inspection.
Window events cannot bypass the pending root gate. No root properties or URLs
are inspected or serialized, and no load, navigation or new engine is requested.

Root and successful helper compilation share twenty seconds from Probe
construction. A preexisting root's timestamp is when observed, not its historical
completion time, and does not start the access budget. Only the first `Ready`
status accepted by queued `componentReady` processing before the absolute
readiness deadline starts one five-second create/access budget at that observation
time. It is not the historical status-signal emission time. Neither queued
processing, duplicate signals nor retries reset either deadline. Expiry before a
root witness refuses as `root-readiness-deadline`; subsequent compilation readiness
expiry uses `readiness-deadline`, and access expiry retains `deadline`. The nominal
combined budget is at most twenty-five seconds. The refusal timer is not a
readiness witness or a hard bound on native work.
The explicitly direct signal handler first compares the current thread with the
thread captured at subscription. Unexpected foreign-thread emission queues only
a fixed refusal without reading root pointers or GUI-owned state. On the app
thread it captures a weak root during the signal and queues processing without
compiling or finishing inline. The queued continuation rechecks app/engine thread, the same
unique engine among at most sixteen windows, and that the exact weak root still
belongs to the bounded root list. Lost roots refuse as `root-witness-lost`.
This continuation belongs to the original admitted acquisition; repeated safety
snapshots do not increment the acquisition admission counter. Before the first
compile the gate is consumed and its signal connection removed. Later root loads
cannot authorize post-failure retries. Cancellation disconnects and invalidates
pending gate work. Root success permits only the helper check: it proves neither
native registration ordering nor controller availability. Existing roots may
precede other unfinished loads. Queued delivery also does not guarantee native
load() has unwound when native handlers enter nested event loops.

Public component Ready/Error status transitions queue processing; Loading waits
within the absolute compilation readiness deadline. There is one create.
Only an exact `missing-library` compilation failure may delete the failed owned
component and arm one context-bound 200 ms single-shot recheck after cleanup.
The timer admits one fresh check on the same guarded app-thread engine, with
fresh deadline/thread and unique-window-engine checks. It proves no availability.
There are at most eight total acquisition/compile admissions and seven timer
rechecks; the eighth exact failure terminates `readiness-cap` without a ninth
timer, admission or compile. Initial discovery admissions also consume that cap.
There is no elapsed-time catch-up burst. The original readiness deadline remains
fixed. Changed, absent or ambiguous engines refuse. Window events are used only
for initial engine discovery and cannot authorize post-failure work.
Queued attempts from before cleanup are invalidated; component callbacks capture
weak guards for both their producer and the queued completion. Events during
compilation or teardown cannot authorize another attempt. Queued retry callbacks
capture the guarded producer and cleanup epoch; cancellation/terminal completion
invalidate them, and queued `Ready` processing consumes the single create.
Other compiler failures are terminal. Accepted Ready permits one create; the
helper's availability bool proves access. There is no cache clearing, new engine,
registration shim or URI change. This finite retry follows the pinned community
check pattern as an experiment, not proof that timing caused earlier failures.

The fixed helper imports QtQml and versionless xofm.libs.library, binds a typed QtObject
reference to DocumentController, and exposes its own availability boolean.
C++ reads that boolean only. It does not retrieve the controller pointer, read
native controller properties, call controller/page methods or access documents
or account files. QML resolution **may invoke the existing registration functor,
associate the singleton with an engine and set CppOwnership**. Import caches and
that association are not undone by helper cleanup. This is not a no-factory or
pure read-only operation. Access proves neither displayed source nor page identity.

Only the owned helper/component/filter/timer/probe are cleaned up; weak engine
guards and post-call checks do not pin native lifetimes. Supported GUI lifecycle
is assumed. Cancellation during setData/create defers destruction until those
calls unwind. The DSO must remain loaded until process exit. Qt allocation,
imports and synchronous calls have no hard execution-time bound; the elapsed
deadline refuses late completion, while the independently reviewed operator
provides external recovery.

The operator precreates an owned 0700 `/run/rmb-qt-probe-<nonce>` directory.
`callback.json` is one exclusive, no-follow 0600 open and one write below 256
bytes. It contains fixed nonce/stage and scalar application_thread,
engine_thread, helper_available, controller_available booleans. It contains no
raw pointers or document strings. Existing receipts are preserved; partial writes
are invalid. A fresh frozen 32-character lowercase hexadecimal QT_PROBE_NONCE
is required for the independently built target artifact. The script nonce is
synthetic and must never be reused on hardware.

The original e017ccc/5ce development run reached `component-error` after engine
acquisition; its saved receipt did not disclose a compiler reason. Removing the
explicit 1.0 module version matches the pinned community import. A version
mismatch remains a hypothesis, not an established explanation of that result.
On a later component compilation failure, the classifier inspects at most eight
errors with at most 256 UTF-16 code units per description. It emits only fixed
`module-version`, `module-missing`, `type-missing`, `property-type` or
`component-error` stages. Unknown, mixed or oversized diagnostics use the generic
stage. It never emits error descriptions or URLs. These English vendor-Qt
patterns are categorical hints, not a general parser or root-cause proof. Qt
still owns/materializes the error list; this limit bounds our inspection only.

Following the distinct 0149b5d/f485 run's `module-missing` refusal, exact missing
module descriptions for QtQml, QML, QtQml.Models, QtQml.WorkerScript and
xofm.libs.library map respectively to fixed `missing-qtqml`, `missing-qml`,
`missing-models`, `missing-worker` and `missing-library` stages. Other module
names retain `module-missing`; mixed categories still use `component-error`.
The classifier itself adds no import or acquisition behavior. The original receipt remains
unchanged and identifies no module; no root cause is established by this change.

For this fixed helper's compilation failure only, the combined refinement also
snapshots actual compiler descriptions privately before cleanup and writes them
after cleanup to exclusive no-follow 0600 `diagnostics.json` in the same owned
0700 nonce directory. It performs one write with no retry; a partial/error write
is invalid and prevents the accompanying callback. The final JSON is at most
8192 bytes, retaining at most eight errors and 256 UTF-16 units per description.
The schema includes `context: last-compile-failure` and `failed_attempt` (the
original failed attempt index), followed by `reported_error_count`,
`retained_error_count`, `count_truncated`,
`output_overflow`, and `errors` containing `description`, `description_truncated`,
`description_redacted`, `line`, `column`. Oversized serialization is rejected
and replaced with a bounded explicit overflow record with no descriptions.
URLs and source snippets are never serialized. Descriptions with /home/, /root/,
/Users/, backslash Users prefixes (case insensitive), or a drive-qualified
absolute path are replaced with a fixed personal-path redaction token. Detection
inspects only the bounded retained text. System plugin paths are retained
privately. The fixed helper has no user/document content inputs; this is not a
general content filter. These private descriptions must never be published or
copied into public logs/commits. The reviewed operator collects and hashes the
single fixed file before removing it during exact cleanup. Public callback
fields and size remain unchanged; prior spent packets are not reused.
The last failed snapshot is retained even if a later compilation succeeds, and
is historical failure evidence rather than the final availability result.

The same private diagnostic also contains a bounded runtime summary at terminal
receipt emission: `attempts` counts acquisition admissions (0..8, excluding
the ninth cap-triggering internal call), `compile_attempts` counts setData entries
(0..8). The historical window-only `admitted_post_failure_events` field is zero
for this candidate: no post-failure window event authorizes work.
`timer_rechecks_admitted` counts only fresh timer recheck admissions (0..7), and
`retry_interval_ms` is the fixed 200 ms interval. Neither is a raw event count.
`component_status` is absent/null/loading/ready/error, sampled before final owned
cleanup. `terminal_stage` matches the corrected callback stage and `elapsed_ms`
uses the same single monotonic sample taken after cleanup for deadline correction.
This timing is an observation, not a filesystem-write or Qt-call upper bound.
When there has been no compile failure, context is `runtime-only`, failed_attempt
is zero and errors is empty; historical compile errors retain their original
last-failure context and index. The 8192-byte serialized cap is reapplied after
the summary merge; overflow removes descriptions but retains counters and
explicit overflow/truncation flags. The root-gate refinement also retains fixed
`readiness_budget_ms`, `access_budget_ms`, `gate_engine_kind`
(unselected/application/unsupported), `root_gate` (waiting/accepted/consumed/failed),
`root_witness_kind` (none/preexisting/signal), and capped `root_count`.
`access_anchor` is `component-ready-observation`. `component_ready_at_ms` is null
until Ready is first accepted in queued processing, and anchors the access budget.
`root_witness_at_ms`, `first_setData_begin_ms`, `first_setData_return_ms` and
`first_error_handled_ms` are monotonic observations from Probe birth, null until
observed and sampled only once. `last_setData_begin_ms` and
`last_setData_return_ms` retain the latest compile's observed span, null before
any compile. Error-handled time is sampled after the expiry
check and before diagnostic materialization. These spans do not identify causes
or bound native Qt work. The same 8192-byte cap is applied after all metadata.
Cancellation still emits no callback/diagnostic.
Every Probe terminal receipt includes this summary. Startup no-gui and
application-thread refusals occur before Probe construction and retain their
existing callback-only path without a runtime summary.

Run `sh tools/qt_qml_access_probe_test.sh` with Qt development/runtime plugins,
or set QT_PROBE_SDK_ENV to the firmware-matched SDK environment file to compile
and run owned ARM fixtures under qemu with the SDK offscreen/QML plugins.
Fixtures cover fixed-category/unknown/mixed/count/description boundaries, actual
owned compiler errors, versionless major-version selection,
immediate/same-engine/late readiness, absent registration,
delayed registration without a window event on the same engine,
stale/reentrant window events, changed/ambiguous engines, finite timer/compile
cap and terminal
wrong-module failures,
conflicting and multiple engines, window cap, engine teardown, event cap,
deadline refusal (including a deliberately nested owned singleton callback) and
application cancellation. Focused application-engine fixtures cover preexisting
and delayed roots, duplicate signals, pending-window activity, failed root loads,
root cap, unsupported engines, lost roots/engines, changed/ambiguous engines,
queued admission after the absolute readiness deadline, and cancellation while waiting.
They also cover foreign-thread signal refusal, a root witness observed too late,
a failure signal after an accepted root, and cancellation before queued admission.
The phase/retry refinement also covers GUI delay past the former root-based
access window while readiness remains open, late Ready observation refusing before
create, accepted Ready permitting create across the readiness boundary within
its independent access budget, cancellation with an armed retry and engine loss
during the retry wait. Phase budgets are scaled in owned fixtures; production
defaults remain twenty seconds plus five seconds.
Queued Ready acceptance also refreshes application/engine thread affinity and
rechecks the same unique window engine immediately before create, without a new
admission. That safety snapshot consumes the already anchored access budget.
Owned fixtures change or add a competing window engine before queued Ready and
verify refusal with zero registration-factory/helper creations.
The nested fixture registration factory is confined
to the owned application; the payload has no such registration. These are synthetic fixtures,
not hardware or production qualification. OpenSpec task 2.5 and native page
creation/recovery gates remain open pending exact source/artifact/operator
review and coordinated evidence.
