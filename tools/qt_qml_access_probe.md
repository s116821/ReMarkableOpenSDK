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
C++ reads that boolean and, once available, the owned typed reference once for
the bounded M1 metadata observer. It does not read native controller properties,
call controller/page methods or access documents
or account files. QML resolution **may invoke the existing registration functor,
associate the singleton with an engine and set CppOwnership**. Import caches and
that association are not undone by helper cleanup. This is not a no-factory or
pure read-only operation. Access proves neither displayed source nor page identity.

Only the owned helper/component/filter/timer/probe are cleaned up; weak engine
guards and post-call checks do not pin native lifetimes. Supported GUI lifecycle
is assumed. Cancellation during setData/create/property access/metadata observation defers destruction until those
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

### M1 metadata observer

After helper resolution, one read of the owned helper's typed
`observedController` property captures the already resolved QObject pointer.
There is no native property read, coercion or method invocation. One queued
app-context observation then uses the existing Ready-observation plus five-second
access budget; it does not reset the clock or emit an earlier access-only receipt.
Public `resolved` requires that scan to complete. Fixed `metadata-*` refusal
stages remain non-success even when helper/controller availability is true.
Cancellation emits no receipt. Terminal cleanup deadline/engine corrections also
correct the private `metadata_result`.

The observer uses public QtQuick window/content/child topology on the selected
engine and existing public metaobjects. It inspects only these fixed buckets:

- SceneView superclass: pageId QString, document QmlDocumentWrapper pointer,
  controller SceneController pointer, with exact source-known type spelling.
- DocumentView top-class token, excluding Shortcuts: sceneController and
  pageSelection must have the existing PointerToQObject flag; their exact native
  types remain unknown.
- SceneSelectionHandler top-class token: controller PointerToQObject,
  viewSelectionRect and sceneSelectionRect QRectF.
- The resolved controller's exact six-argument
  addPageWithTemplateAndPageSize(entry::Id,int,QString,QSizeF,QJSValue,QString)
  metadata and bool return type. No invocation or method enumeration occurs.

Private output contains fixed keys, aggregate presence/type/readable/notify
counts and candidate counts. `all_required_metadata` counts candidates containing
all required readable, compatible properties together; notify is reported
separately. Mismatches remain counted, and multiple candidates never select a
first view. No native values, raw class/type names, signatures or pointers are
serialized. `source_authority=false` and `owner_relation=unproven` always apply,
including a single complete candidate. Metadata absence, incompatibility and
missing creation signatures can be observed successfully without establishing
a current page, active selection, owner join or permission to create a page.

Caps are 16 windows, 256 distinct Quick items including roots, root depth zero
through depth 16, 16 candidates per bucket and 32 superclass entries per item or
controller. Nonempty children at depth 16, oversized child snapshots, repeated
items and cap violations refuse explicitly. Indeterminate/different Quick-window
engines refuse. Application-thread affinity, QPointer guards and monotonic
deadline checks surround topology/metadata groups; final unique-engine
observation is refreshed. Guards do not pin lifetimes or create an atomic
snapshot. Public list allocations and native call duration are not bounded by
these inspection caps. Existing-interface QMetaProperty metaType flags/typeName
inspection does not call type registration, id/userType/fromName or metatype
metaobject discovery. Payload code uses no private Qt headers.

The test runner now links public Qt6Quick and uses SDK-host moc only for the owned
metadata fixtures, never for the payload. Focused fixtures tripwire native getter
and creation invocation calls, preserve mismatches and multiple candidates, and
exercise caps, incompatible pointer categories, object/thread/deadline loss and
the actual queued Probe boundary (no early receipt, cancellation, lost engine or
controller, expiry and changed engines). These remain synthetic evidence.

### M2 navigation candidate metadata

The same single queued scan adds `navigation_candidate`; no new engine, context,
native property access, signal emission, method invocation or timer is introduced.
One exact `indexOfSignal("requestOpenDocumentOnPage(QVariant,QVariant)")` lookup
per visited item checks the source-backed signal shape. There is no fallback
name search or member enumeration. The returned method must be valid, a Signal,
have two parameters and expose two stored QVariant parameter type names.

The public metaobject superclass chain must contain exact `QQuickFocusScope`.
Qt's pinned [FocusScope declaration](https://github.com/qt/qtdeclarative/blob/v6.10.3/src/quick/items/qquickfocusscope_p.h)
maps that QML element to this private C++ implementation class. The payload
compares existing names through public metaobject APIs; it never includes,
casts to or calls the private class. Qt's pinned
[compiler type mapping](https://github.com/qt/qtdeclarative/blob/v6.10.3/src/qml/compiler/qqmlirbuilder.cpp)
and [property-cache mapping](https://github.com/qt/qtdeclarative/blob/v6.10.3/src/qml/qml/qqmlpropertycachecreator_p.h)
map explicitly var signal parameters to QVariant. Real owned QML fixtures verify
the actual exact signature and ancestry rather than assuming a lookup succeeds.

Every exact signal hit counts as a raw candidate even when other metadata fails.
On that same object, `windowNavigator` must be present, readable and have an
existing PointerToQObject flag. Missing properties contribute presence zero and
incompleteness without invented type classifications. A present property with
an invalid interface contributes `type_unknown`; a valid non-QObject interface
contributes `type_incompatible`. Notify is informational. No cross-object join
or first-match fallback is performed.

Property type spelling is unknown (`exact_type_known=false`). A separate lexical
diagnostic accepts canonical ASCII identifier chains separated by `::`, ending
in the identifier WindowNavigator and one `*`, up to 128 bytes excluding NUL.
No leading global `::`, whitespace normalization, const/reference/template
syntax or other grammar is accepted. Null, empty, unsupported, differently named
or oversized spelling contributes `type_name_unknown`. This spelling check never
rejects an otherwise readable existing QObject pointer; no raw spelling is saved.

After a complete scan, zero raw candidates gives private `location_result=absent`;
one incomplete candidate gives `incompatible`; more than one gives `ambiguous`,
even if exactly one is complete. Only one raw candidate with all required metadata
gives `located` and `located=true`. This locates metadata only: source authority
is always false, and owner and receiver relations are always unproven. No selected
native pointer is retained as callable state. All completed statuses retain public
`resolved`, meaning completed metadata observation and helper availability, never
navigation success or acquisition. Actual context/lifetime/thread/cap/deadline
failures remain public refusals. Terminal corrections align `metadata_result`
with the final stage and invalidate location to `not-checked`/false; prior counts
can remain historical. Cancellation still emits no receipt.

Fixed counts are lookups, candidates, focus_scope, signal_kind, arity_two,
variant_pair and all_required_metadata. The window_navigator subobject contains
presence/readability/notify, pointer category, unknown/incompatible interface and
compatible/unknown lexical counts, plus exact_type_known=false. The scan allows
at most 256 exact signal lookups, 16 raw candidates and same-object property
lookups, and 32 parameter type-name reads. The seventeenth raw candidate refuses
as `metadata-navigation-candidate-cap` before detailed inspection. Existing
16-window/256-item/depth-16/32-superclass guards and the 8192-byte diagnostic cap
remain in force. Public lookup/list allocation duration is not bounded by these
inspection counts; the existing twenty-second readiness and Ready-plus-five-second
access clocks remain unchanged.

Owned fixtures cover real QML signals, namespace aliases, alternate valid QObject
pointer names, wrong function/slot/arity/types, bare fake FocusScope, split objects,
two raw candidates with one complete, inclusive/overflow caps, and historical
complete counts followed by loss/deadline invalidation. Native getter, invocation
and signal-emission tripwires remain zero. The write-only owned negative property
deliberately produces a moc warning while retaining isReadable=false metadata;
it is fixture-only, with no payload moc or native getter. Actual Probe fixtures
verify no early receipt, completed absent/incompatible/ambiguous public resolved,
refusals, cancellation, engine/controller loss and deadline handling. None of this
synthetic evidence qualifies the tablet's active owner, receiver or dispatch.

## Explicit disposable-fixture one-call mode (October 2, unqualified trial)

M3 topology observation is held and is not a prerequisite. The optional
CreationConfig mode reuses M2 existing-engine/root readiness, the owned singleton
helper and twenty-second readiness / first queued Ready plus five-second budget.
It replaces metadata traversal with one explicitly configured insertion trial.
There is no active-view discovery, source authority, navigation or production API.

The private generated header selected by QT_PROBE_CREATION_CONFIG supplies the
inline global creationConfig() factory returning qml_access::CreationConfig:
enabled=true, one canonical lower-case document UUID and five distinct canonical
page UUIDs in baseline order. Keep that header outside Git, under .private/, or
in a separate private build directory. Its values must come from the reviewed
backup; no personal fixture literal appears in this source or documentation.
The source remains metadata-only when this build macro is absent. The test runner
also compiles the configured entry point with generated synthetic values.

The QML helper sets entered before preflight. It checks Entry.Document and
Entry.Exporting are available numeric values, resolves Library.entryForId, retains
native wrapper.id and compares String(nativeId) with the configured UUID. It
requires document type, numeric non-exporting status, count five, and all five
idForPage(i) strings matching configuration with pageForId(key)===i. It accepts
only a native string from templateForPage(0), including an actually returned empty
string; there is no fallback. A missing creation method refuses before claim.
The sole invocation has five arguments: retained native ID, index 1, inherited
template, Qt.size(1404,1872), and a zero-parameter observation callback. pageUuid
is omitted. Paper size is a community-supported trial parameter, not a verified
DeviceScreenInfo value. Inherited background is not necessarily blank.

The owned QObject bridge checks GUI thread FIRST before touching its state or
weak Probe callbacks. Preflight and mutation claim revalidate active context,
selected engine/generation, helper/controller lifetime and thread affinity,
current window-engine selection, cancellation and the unchanged deadline. Claim
sets mutation_attempted before native entry. No false return, throw, cancellation,
missing callback or timeout permits replay. The QML callback captures only its
owned helper, checks armed/helper/bridge, updates fixed owned callback counters,
and observes completion. No native pointer or raw Probe is in that callback.
QML callback execution assumes delivery on its originating engine thread; the
native UI call-site callback suggests that intended route but does not prove
actual callback routing or worker semantics. The C++ bridge thread guard cannot
make a foreign-thread QJSValue invocation or prior QML helper reads safe.
First callback time is fixed; count saturates at two with a duplicate flag.
The callback checks bridge arming before updating helper counters; at most one
queued completion is pending, including a synchronous callback burst.
Synchronous callbacks never tear down inside native invocation. inCall and
settlePending defer terminal teardown; disarming precedes owned cleanup. A native
holder's later callback is inert after cleanup. Cooperative deadlines cannot
preempt a blocked native call; externally armed recovery remains necessary.

Private creation_trial diagnostics contain attempted, returned, boolean-known
and boolean value, exception, callback count/duplicate, fixed guard/phase and
relative timestamps. They expose no IDs, template, pointer, exception text or
paths. Creation-mode compiler descriptions are suppressed because generated QML
contains private configuration. Public resolved means call/return/callback
observation only. It does not establish durable creation even for false return
plus callback; durable_success is always false. Every attempted call requires
filesystem reconciliation. Throw/deadline/cancellation or a false return without
completion observation remains effect-uncertain after claim. False return plus
callback can have phase=call-observed while still requiring reconciliation with
durable_success=false; this phase records observations, not the mutation result.
A pre-call guard failure or exception records attempted=false.
Creation-mode cancellation emits a bounded diagnostic receipt; metadata-mode
cancellation retains its prior silent behavior. Operator collection and stock
restoration remain mandatory even after diagnostic refusal. After disarm,
returned=false or exception=false means that event was not observed before
disarm; it does not prove the native invocation never returned or threw.

Builds now need vendor moc for tools/qt_qml_creation_bridge.h, writing
qt_qml_creation_bridge.moc into the private build include directory, and compile
both tools/qt_qml_access_probe.cpp and tools/qt_qml_creation_bridge.cpp with that
include directory. The generated configuration header is also supplied through
a private include directory; define QT_PROBE_CREATION_CONFIG to its quoted
filename. Do not generate or activate a real trial artifact until exact source,
independent tests, semantic review and operator/recovery review are accepted.

Owned synthetic fixtures cover asynchronous and synchronous callback, duplicate
and retained late callback, repeated entry, all five-page guard failures,
unknown enums/missing wrapper/method, getter exceptions, native throw, false
return with/without callback, unknown return, reentrant cancellation, nested
deadline and foreign-thread bridge entry. The latter tests only C++ slot admission
on a live bridge, not foreign-thread QML/JS safety or actual callback routing.
Existing M1/M2 tripwires remain zero.
Synthetic evidence does not qualify real wrapper loadedness, worker semantics,
PDF/ink preservation, native-assigned ID or persistence. Main alone verifies one
new ID at index 1, five originals in relative order with their content unchanged,
and durability after stock restoration/restart/reopen.

Source basis: current-project frozen interoperability handoff
https://mem.ai/ac4bdd09-f843-5dec-85ca-2722e16fecc5 (private native call-site and
saved metadata), Main's accepted all-five guard refinement, and pinned public
[RM2 size patch](https://github.com/rmitchellscott/xovi-qmd-extensions/blob/67d39e943d8fda30936ad7f51a30f59f0292a9f8/3.28/createPagesRM2Size.qmd#L14-L23).
This is original experimental glue, not copied proprietary source or a shipping
capability. Review/native gates and the full OpenSpec delivery remain open.

### Fixed exception-operation diagnostics after the first failed trial

The spent first creation trial reported a pre-call exception with no mutation
claim; its generic receipt cannot identify the throwing operation. Saved fixture
hashes remained unchanged. That negative result does not qualify an enum's QML
registration URI or justify adding an import or guessed enum value.

The helper now owns a local fixed-string operation marker, initialized before
try and assigned immediately before each potentially throwing expression.
Document/Exporting enum accesses, native ID read/String conversion and forward/
reverse page lookups are separate markers. There are no per-step bridge calls.
Only catch passes the marker to the thread-guarded observeException slot. C++
maps seventeen explicit allowed strings to static literals; all other inputs
become exception-stage-unknown. The private exception_operation field is null
unless an exception was observed. guard_stage keeps its refusal/accepted meaning.
No raw exception, ID, page key, template, native value or path is retained.
The native-call marker is set after successful mutation claim and before native
entry; post-claim exceptions still require effect reconciliation without replay.
Imports, getters, call arguments, cancellation and budgets remain unchanged.
The QML module owning Entry is still unqualified: the fixture registration under
xofm.libs.library is synthetic, not target evidence. New fixtures genuinely omit
Entry or Library registration and verify actual unknown-identifier exceptions,
throwing native-like ID conversion/property and page-index getters, and unknown
marker privacy. Existing native-throw verifies operation=native-call with one
sticky attempted call; existing refusal/completion/cancel cases retain their
previous meanings.

### Source-plausible creation import trial

After the spent instrumented trial localized its pre-call exception to
Entry.Document access (enum-document), Main selected one unaliased
import com.remarkable in the creation helper, matching the already inspected
native view's unaliased import pattern. This is a source-plausible trial choice,
not a qualified Entry registration URI, proven root cause or demonstrated fix.
The metadata-only helper remains unchanged. The fixed diagnostic
com_remarkable_import_selected=true identifies this creation-helper version;
it proves only configuration, not availability or enum ownership. Existing
numeric enum guards, native getters, typed ID, five arguments, fixed markers,
no retry, reconciliation, callback and deadline policies are unchanged.

Owned fixtures register a synthetic com.remarkable module and exercise Entry
exported only by library, only by com.remarkable, absent in both, and with
conflicting enum exports from both. Collision expectations follow the actual
owned Qt import resolution, not a claim about the target's registration owner.
A missing module must fail compilation before claim without fallback imports.
These fixtures do not qualify the real module URI or actual native creation.

### Private opt-in for the supervised exact-fixture feasibility trial

CreationConfig adds developmentExplicitFixture=false by default. Default mode
retains the current Entry.Document/Entry.Exporting and native type/status checks.
The reviewed private six-UUID factory may explicitly opt in for development
feasibility only; this is not a shipping adapter or production source policy.
The fixed development_explicit_fixture diagnostic identifies that configuration.
The opt-in imports only QtQml and xofm.libs.library; the default retains its
com.remarkable import. com_remarkable_import_selected records the actual
selection separately from development_explicit_fixture. No further enum/import
ownership guess is made.

The opt-in omits enum and native type/status reads and instead reads native
isExporting exactly once. It requires typeof value===boolean and value===false;
true, nonboolean or a throwing getter refuses before mutation claim. No truthy
coercion or guessed enum number is used. Saved QmlEntryWrapper metadata declares
isExporting:bool and the already inspected native UI uses that property; the
actual getter/value is still a runtime guard, not qualified by synthetic tests.
Exception operation document-exporting-read is an additional fixed allowlist
value. The exact document/native ID comparison, five-page count, all five keys
and reverse indices, native template, index 1, paper size, five arguments,
thread/lifetime/deadline/one-attempt/no-retry/reconciliation policies are unchanged.
Fixture identity and capability checks in this controlled experiment do not
establish a general document type, active user source or worker serialization.

Owned tests keep all default cases, including absent Entry throwing before claim,
and add opt-in success with neither Entry nor com.remarkable registered,
exporting true/nonboolean/absent/throw refusal,
identity/count/page-order/reverse/template/claim-cancellation refusal, and
property tripwires proving native type/status
are not read in opt-in mode. All opt-in successes still observe only a call and
callback; durable success requires Main's saved-file and stock-reopen evidence.

### Development Library readiness and private exception evidence

The private opt-in now resolves Library once under library-resolve, retains its
QObject reference on the owned helper, checks entryForId under library-method,
and reads isReady under library-ready. Missing Library or method and nonboolean
readiness refuse before lookup. A true boolean proceeds to the same fixed UUID
lookup, native ID and five-page guards, template and one creation call.

A false boolean enables one owned Connections target for readyChanged. The first
signal disables that target and queues one C++ continuation; duplicate signals
cannot queue another. The continuation checks the existing engine, ownership,
cancellation and the development readiness/access deadline before entering
resumeLibrary under the same inCall/settlePending protection as createOnce. It
re-reads readiness and requires boolean true. There is no polling, budget
extension, creation replay or repeated lookup. Completion/cancellation disarms
the bridge and destroys the owned helper/connection. Signal delivery does not
perform creation or tear down the helper on the native signal stack.

Only the opt-in catch additionally extracts error name and message with separate
nested try blocks, retaining string values only, at most 256 UTF-16 units each.
A truncation boundary never retains a dangling high surrogate. The C++ slot
independently caps the values. No stack or string coercion is used. The fixed
category allowlist is Error, TypeError, ReferenceError, RangeError and SyntaxError;
other names or extraction failures retain unknown. Raw values exist only in the
private bounded diagnostics as private_error_name/private_error_message, never
the public callback or logs; actual error text must not be copied to Git, Mem,
Drive or chat. The existing 8192-byte diagnostic limit remains enforced.
Default mode retains its previous lookup and exception observation behavior.

Owned tests cover absent/null-provider Library, missing method, absent/nonboolean
readiness, ready success, false-to-true and duplicate signals, signal after the
deadline, cancellation before signal and between signal/continuation, deadline
between signal/continuation, still-false readiness, lookup error capture, throwing
error property getters, long values, escaped control units and a surrogate pair
cut at the cap. They check zero lookup/calls on readiness failures, one lookup
and native call on successful continuation, bounded parseable JSON, fixed public
markers and inert late callbacks. These synthetic cases do not qualify native
Library timing, UUID conversion, creation effects or stock-reopen durability.
### Private first-observation progress markers

The opt-in diagnostic adds twenty fixed first-observation timestamps, using the
existing Probe elapsed clock and null for an unobserved boundary:
create_once_enter, library_resolve_enter/return, library_method_enter/return,
library_ready_initial_enter/return, ready_signal, continuation_queued/enter,
library_ready_resume_enter/return, library_lookup_enter/return,
helper_create_enter/return, mutation_claim_enter, deadline_observed,
cleanup_enter/return (each key has suffix _ms). There is no event list.
Initial, resumed and current readiness retain only the fixed values unobserved,
nonboolean, false or true; they derive from the already-read local value.
last_entered_stage and last_completed_stage use the same fixed stage names.
Cleanup can be their final values; the individual timestamps preserve earlier
phase evidence. A return marker means control returned, not a valid result.

One armed, GUI-thread-guarded bridge slot records QML observations. Unknown stage
names are ignored and repeated stages cannot overwrite the first timestamp or
readiness result. Observation does not call a getter, check admission, finish,
queue work or change creation state. Signal entry is stamped before disconnect;
queue entry uses the existing one-queue latch. Continuation entry is stamped
after weak/epoch checks and before the existing context/deadline check. Every
existing native property is evaluated once at its prior boundary into a local;
markers bracket that evaluation. The native call and its guards remain enabled.

Internal C++ timestamps bracket actual component helper creation and eventual
non-inCall terminal cleanup. Helper-create return does not imply nonnull success.
The first actual deadline observation is recorded in expired() or the existing
post-cleanup deadline check, including when inCall defers terminal processing.
Internal cleanup/deadline recording can occur after bridge disarm and never
re-enables it. Existing attempted_at_ms is successful mutation-claim evidence;
returned_at_ms and first_callback_at_ms remain the native return/callback
observations, and component_ready_at_ms still means compiled component Ready.
Progress recording does not change deadlines or the public callback. Missing markers do not
establish why a boundary was not observed. These fields are private diagnostics,
with the existing 8192-byte limit, not creation or durability proof.
### Development readiness and access budget partition

For the explicitly enabled development fixture only, compiled component Ready
no longer starts the access budget. The existing absolute readiness deadline
remains 20000 ms from Probe start until the already-evaluated initial or resumed
Library.isReady local value is strictly boolean true. The existing resumed
context check must admit work before that cutoff; a separate armed/thread-guarded
admitLibraryReady step rechecks the live context and cutoff after the read.
At or after cutoff it refuses admission. It reads no Library property itself.

The first admitted true observation records library_ready_accepted_at_ms and
rearms the same timer once for the existing 5000-ms access budget. Repeated
admission cannot reset that anchor or timer. The absolute access deadline is
anchor plus 5000 ms, so the maximum logical lifetime is less than 25000 ms from
Probe start. The private access_anchor value is library-ready-observation;
component_ready_at_ms remains the independent compile-Ready timestamp. Default
mode still uses component-ready-observation plus 5000 ms. The fixed progress
observer, readiness signal, queue entry and unrelated events cannot set an anchor.
There is no polling, sleep, inline creation, replay, repeated lookup or retry.

Continuation posting checks invokeMethod's boolean result. Failure records fixed
library-dispatch-unavailable, disarms the bridge and schedules terminal-only
refusal after signal-stack unwind; it never attempts another creation dispatch.
The zero-delay terminal deferral is not a readiness or access budget timer.

Owned tests retain default deadline cases and add readiness arriving after the
former access cutoff but before the absolute readiness deadline, one-way anchor
retention under repeated admission, expiry before the mutation claim, and expiry
during native completion. Readiness-late and queued-deadline cases now exercise
the absolute readiness cutoff. Successful claims must occur before the anchored
access deadline; expiry never qualifies durability. No actual trial is implied.
The existing operator observation/recovery envelope remains unchanged; startup
or transport time consuming that envelope still requires restoration, never an
operator extension. These are cooperative GUI-thread budgets, not preemption of
busy native calls; the outer operator remains responsible for hard recovery.

## Explicit PageKey development open/observe mode

The October 3 [active plan](../openspec/changes/establish-native-platform-contract/page-key-open-plan.md)
adds an off-by-default `QT_PROBE_PAGE_OPEN_CONFIG` factory, `pageOpenConfig()`.
It supplies a canonical document/source/target identity and six unique expected
page IDs. It is mutually exclusive with the creation factory; no private fixture
UUID is checked into these tools. The successful creation packet is spent and its
expected-five configuration remains historical.

`qt_page_owner.h` checks one focused active visible QQuickWindow, existing engine,
bounded item ancestry and source-backed FocusScope/SceneView properties. It requires
one live visible/enabled focused receiver/scene pair with identical document
QObject; QML lexical ids are never used as objectName discovery. Missing, ambiguous,
changed or excessive topology refuses. These necessary conditions are not general
overlay or input-isolation proof. First native qualification remains limited to
Main's explicitly opened fixture with no concurrent interaction.

`qt_page_open.h` installs relevant page/document/lifetime and focus/containment
observations before dispatch. Document/focus/lifetime loss latches refusal. An
owned QtQml helper verifies document native ID, the full document-owned index/key
round-trip map and actual source aliases immediately before one queued
`receiver.openPage(index)` call, omitting the optional position. An already-open
target produces a verified no-op. Native openPage housekeeping may update
last-opened state; these tools never directly rewrite native metadata or create
another page. A native return or notification is not target proof: queued
observation rechecks the same owner, page aliases and map. Target mismatch stays
unresolved until the existing access deadline; no sleep or retry is introduced.

The `open-observed` callback stage is distinct from existing access/creation
`resolved`. Private `page_open_trial` fields report attempted/returned and observed
target separately, including a no-call no-op. `native_api_qualified` and
`render_authority` remain false. This is development navigation evidence, not a
render-ready lease, fresh pixel capture or qualified Rust Platform API. Buddy
guards and SDK unsupported defaults are unchanged. GUI-thread budgets cannot
preempt native calls; the accepted operator/recovery envelope remains separate.

Compile/link `qt_page_open.cpp` alongside the existing access and creation bridge
translation units, generating its moc from `qt_page_open.h`. The independent
`tools/qt_page_open_test.sh` runner builds the synthetic configured shared object
and owned ARM/qemu cases; it performs no tablet actions. The existing probe runner
also links the new translation unit. Native receiver/callable exposure, actual
opening, fresh capture and preservation remain Main-owned qualification gates.

The open-session call envelope counts nested scopes and only releases the outer
Probe teardown guard at the outermost return. Native owner getters are followed
by fresh sticky/weak/thread/deadline checks without repeating getter evaluation.
Owned integrated regressions pump nested native-open event loops through
cancellation and deadline, proving that terminal cleanup waits for the outer
return. Final-claim getter expiry and focus/document away-and-back refuse before
any native call; post-call getter expiry/invalidation cannot report target success.

### Optional one-time development setup arm gate

An OPEN factory can additionally set developmentSetupGate. Startup derives its
directory and nonce from the compiled packet identity. The owned QFileSystemWatcher
observes that directory before an exclusive open-waiting marker, then immediately
checks the fixed open-arm filename. Marker/token bytes bind nonce, actual process
PID and /proc/self start time, with waiting/open words and one newline. Files are
bounded no-follow regular mode0600 artifacts under a retained owner-mode0700
directory identity. Wrong/stale/partial/duplicate or replaced contexts refuse;
unrelated directory notifications never arm or reset the budget.

The token is consumed once and watcher delivery is disconnected before one queued
owner qualification. It grants development setup permission only. SDK checks
entry.closed/restore.claim absence before acceptance, before queued Session.begin
and through subsequent operation guards. Cancellation disarms acceptance so later
callbacks cannot begin. An accepted token is sampled after fresh context validation
before the original absolute20-second readiness cutoff and anchors one5-second
access phase on the same elapsed clock. No polling, menu automation, sleep, retry
or observation/recovery extension is introduced; non-gated behavior is unchanged.
Private access_anchor identifies setup-arm-observation and page_open_trial records
the accepted timestamp separately from actual target observation and false render/
API authority. No tablet result is implied by owned token or integration tests.

Main separately announces one verified source-page setup/capture and executes the
exact independently reviewed arming helper under the existing live-generation and
restoration guards. Atomic no-clobber publication, marker/token collection and exact
temporary-file cleanup are operator responsibilities. The original spent creation
packet/five-page factory must not be reused. tools/qt_page_open_arm_test.sh covers
owned filesystem/token cases; the open runner includes gated clock/loss cases
alongside the accepted ownership/reentrancy regressions.
