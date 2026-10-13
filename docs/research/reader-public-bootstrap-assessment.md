# Public Qt entry and current-reader bootstrap

October 13, 2026. Source assessment following the
[owned current-reference result](reader-current-reference-discovery.md).
Public Qt supplies a usable in-process dispatch and discovery surface. It does
not supply an external attachment transport or a vendor-supported reMarkable
extension contract. No currently permitted running bridge has been established.

## Reuse what is already demonstrated

The SDK's `tools/qt_startup_probe.cpp` registers a Qt startup callback and queues
one application-context invocation. The [recorded native callback](native-platform-ledger.md#completed-public-qt-startup-callback-trial)
proved application-thread execution once. Its initial restoration failed; later
stock recovery remains a separate result. The [creation entry comparison](shutdown-entrypoint-comparison.md)
records successful creation and cleanup in another cohort, without establishing
general shutdown safety. Insertion, ink and reopen evidence remains preserved.

Qt documents [startup registration](https://doc.qt.io/qt-6.10/qcoreapplication.html#Q_COREAPP_STARTUP_FUNCTION)
before GUI initialization and recommends deferred GUI work. A
[context-bound queued invocation](https://doc.qt.io/qt-6.10/qmetaobject.html#invokeMethod-7)
provides dispatch after entry exists. These APIs are not a way for an external
process to call into stock xochitl. A new process or an independently permitted
resident integration still has to load the code.

The pinned [rm-librarian engine lookup](https://github.com/rmitchellscott/rm-librarian/blob/7d0fe08678203c129b9d848419da2a7e96a2abe5/src/main.cpp)
uses application windows and qmlEngine, then creates a helper for library
singletons. SDK Probe already implements bounded window/engine readiness.
Reuse that public mechanism's semantics, not its first-engine global cache or
the entire SDK Probe: the latter proceeds into QML helper creation and optional
creation/open operations unnecessary for a reader-root observation. The
[runtime comparison](runtime-mechanism-comparison.md) preserves XOVI/Inkling
prior art without requiring another loader or dispatch engine.

## Application readiness is not a current document root

`qt_qml_access_probe_core.h` uses QQmlApplicationEngine root objects as readiness
witnesses before singleton resolution. The [rootObjects API](https://doc.qt.io/qt-6.10/qqmlapplicationengine.html#rootObjects)
lists instantiated roots; choosing its first object does not select the current
document. That existing readiness witness must not become reader identity.

The minimal candidate starts on the GUI thread with the current visible, active
QQuickWindow, retaining its identity and [contentItem](https://doc.qt.io/qt-6.10/qquickwindow.html#contentItem-prop).
Window focus selects a UI domain, not a page or the keyboard-focused item.
Require its [associated existing engine](https://doc.qt.io/qt-6.10/qqmlengine.html#qmlEngine)
to share the GUI thread, and refuse missing/ambiguous/changing context. No new
engine, component import, resource rewrite or helper is necessary to inspect the
source-established named owners.

The content item roots the visual scene. It does not by itself prove that a
QObject-only census contains every visual descendant. The owned fixture had to
establish both kinds of parenting. Consequently the next falsifiable question
is whether the actual focused window's bounded QObject domain contains exactly
one DocumentView.ToolSettings/SceneViewsRoot pair with the already established
reader/device visual relationships. A negative or over-cap result rejects that
bootstrap on the sampled state; it does not authorize a larger sweep. A positive
result supplies a root candidate, not document/page identity or a render fence.

## Entry comparison against the current exclusions

| Entry or operation | Evidence and disposition |
| --- | --- |
| Already resident, independently permitted typed bridge | Could queue this small observation without replacing the process. No such endpoint or permission is established by the retained evidence. Process/service identity alone does not prove module presence or absence. |
| Direct preload plus public startup callback | Development entry has executed before. A new transfer/activation is still a new in-process deployment; calling it a root observer does not release held 8112 or an equivalent workflow. Not selected. |
| XOVI no-override extension | Eligible architectural candidate, but also needs loading/activation and shares the process failure domain. It cannot be used to reroute the held operation. Not selected. |
| Current passive SFTP reader | Can retain bounded memory observations, but cannot execute public Qt calls. Adding a callback or private QML decoder changes its contract. No such expansion is selected. |
| Stock IPC or screenshot | Existing research has not demonstrated a current-reader QObject endpoint. A screenshot cannot supply an in-process root. No claim that all possible vendor interfaces are exhausted. |

The retained 8112 selection is `open-document-insertion-v2`, prepared with
read-only backup and no native execution. It selects SDK 605b1f66, source index 5,
source5-stable-body-idle-v1, AfterOpen95s and an eight-page explicit fixture,
with creation enabled, candidate 130 seconds, absolute 360 seconds and recovery
reserve 185 seconds. Main's retained packet includes staged ready/open/trigger/
result/stop control, library navigation and guardian release/fence. The proposed root
observation has no insertion, source witness, native page getter or capture.
That purpose difference does not make its transfer/activation permissible.
Spent 0404's FactsEntry startup and failed restoration cannot be replayed, and
its unexplained rendering fault remains relevant to any process replacement.

Main's retained postchecks identify a stock xochitl process and no service jobs;
the locally inspected passive preparation snapshot corroborates its recorded
process generation. This is historical operator evidence, not a fresh module
inventory or proof that a callable bridge exists. No tablet query was made for
this assessment. Raw identities and maps remain private.

## Concrete next observation, conditional on a permitted entry

Purpose: falsify or support the bounded QObject containment needed to supply the
reader root to the public selector. One accepted trigger, one GUI callback,
one window snapshot capped at 16 and one census capped at 128 objects, depth 12,
64 children per object, with a fixed one-second cooperative observation budget.
Observe only the two exact anchor names, parent/window/effective flags and
declared property metadata. Do not read application document/page/controller
property values in this first root observation. Emit one capped 1024-byte scalar
receipt: process/attempt binding, stage, GUI-thread and current-window checks,
visited count, anchor counts, owner-link/type-domain booleans and refusal reason.
No pointer, document identifier, arbitrary name list or native authority output.
Absence, duplicates, a changed window, lost object, cap or deadline gives refusal;
there is no timer polling, expanded traversal or retry. A successful receipt is
root-containment evidence only. It does not execute the full owned selector or
prove its synthetic QString page domain matches the application.

Before selecting any device packet, Main must identify an actual permitted
entry and fresh process/window/fixture state, freeze exact source/artifact and
dependency hashes, and supply the independent recovery actor and actual numeric
stop/stock-restoration bounds. A resident endpoint also needs a demonstrated
cancellation/failure path independent of the observer; it cannot be presumed
safe because no restart is initially requested. Native Qt calls and list copies
are not preemptible by the cooperative budget. No new device packet, native
build, transfer or activation is authorized by this source proposal.

If a separately permitted future activation is considered, retain the existing
[guarded recovery obligations](../../openspec/changes/establish-native-platform-contract/shutdown-lifecycle-diagnostic.md):
recovery armed before changing the process, exact baseline policy preserved,
candidate gone before removing protection, one stock start, no deadline reset,
and stop/crash outcome separate from subsequent stock recovery and cleanup.
Do not copy historical numeric budgets as a fresh recovery proof. Until the
entry and whole packet meet those conditions, this is reviewable source design,
not an executable operator instruction or a request to release held 8112.

Source basis: current SDK startup/Probe/FactsEntry code, pinned upstream Qt and
rm-librarian sources, retained 8112 configuration/selection, passive preparation
snapshot and Main's current-chat attribution. Private receipts/chat have no
public transcript link. The recommended minimal observation is engineering
inference; live containment, integration entry and lifecycle remain unqualified.

Retained private evidence digests: 8112 main-final-selection.json
`6c300d652e3867f508d36b654e195e8548240b73fede113e0addc6196570eeaf`;
passive selected-parent-provider-r2/prepared.json
`881fbddc46b93c370932f62228eb1565f57eed19a99219758a73fec9fafd0c72`.
