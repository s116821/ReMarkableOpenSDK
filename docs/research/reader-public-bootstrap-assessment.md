# Public Qt entry and current-reader bootstrap

October 13, 2026. Source assessment following the
[owned current-reference result](reader-current-reference-discovery.md).
Public Qt supplies a usable in-process dispatch and discovery surface. It does
not supply an external attachment transport or a vendor-supported reMarkable
extension contract. No currently permitted running bridge has been established.

Later implementation checkpoint: the [independent metadata entry](../../openspec/changes/establish-native-platform-contract/reader-metadata-census-entry.md)
implements this distinct diagnostic. Its original real QML Window fixture
showed QObject-only traversal misses visual children; the reviewed successor
uses one deduplicated QObject/visual-child union with unchanged distinct-node,
combined-child, depth and time bounds. The initial proposal below preserves its
chronology; the linked contract governs the concrete implementation. Owned
tests are not native deployment selection or qualification.

## Ordinary document entry before capture: October 15 follow-up

October 17 follow-up: the sole operator completed one maintained stock screenshot
observation, full decode and owned cleanup. Its saved tutorial pixels are visual
corroboration only; they do not establish native document/page or selected-window
identity. The [compatibility and bounded-operator findings](../experiments/stock-screenshot-compatibility.md)
record the actual utility refusals, corrected byte limiter and evidence limits.
This does not release any held operation or qualify the in-process root below.

The smallest existing setup path is deliberate developer/manual selection of
the fixture in the stock UI, followed by the maintained Buddy screenshot example
for visual corroboration. This is an engineering recommendation for the sole
device operator to select and review, not an admitted run, automated menu flow,
native qualification or release of any held/spent operation. It precedes the
in-process root question discussed below; it does not solve that question.

### Exact maintained sources

This assessment verifies Buddy revision
[`5f89060d5953d4f5367d03913d81a1019fc2cd6b`](https://github.com/s116821/ReMarkableBuddies/tree/5f89060d5953d4f5367d03913d81a1019fc2cd6b),
identified by the coordinator as current main on October 15. The initial lookup
used older local `origin/main` snapshot `ff8ad75bec45fca403d55a7b6d93eb83ab732e3d`;
that snapshot was not live GitHub main. After fetching the exact newer revision,
Git comparison confirmed identical blobs for the example, all five source files
listed below and AGENTS. README differences concern releases/acknowledgments;
the Testing example instructions are unchanged. This is source correspondence,
not a new build or hardware compatibility check.

- [Contributor guidance](https://github.com/s116821/ReMarkableBuddies/blob/5f89060d5953d4f5367d03913d81a1019fc2cd6b/AGENTS.md#predictable-device-interaction)
  permits deliberate developer/manual test setup through menus with advance
  notice, separate from product paths. It does not permit autonomous product
  menu navigation or qualify a blind coordinate sequence.
- [Screenshot example](https://github.com/s116821/ReMarkableBuddies/blob/5f89060d5953d4f5367d03913d81a1019fc2cd6b/examples/screenshot.rs)
  exposes `screenshot FILE [--image-only]`: `Screenshot::new`, then
  `take_screenshot`/`save_image`, or `take_image`/image save. It initializes neither
  input nor a provider. [Testing instructions](https://github.com/s116821/ReMarkableBuddies/blob/5f89060d5953d4f5367d03913d81a1019fc2cd6b/README.md#testing)
  build an ARMv7 development example explicitly; it is not a release asset.
- [Capture implementation](https://github.com/s116821/ReMarkableBuddies/blob/5f89060d5953d4f5367d03913d81a1019fc2cd6b/src/device/screenshot.rs)
  chooses the firmware layout from `/etc/os-release`; RM2 firmware 3.24+ uses a
  unique size/header-matched 1404 by 1872, four-byte allocation in process memory.
  Its first `pidof xochitl` result makes an independently established sole
  instance important. Allocation discovery does not establish physical-panel or
  selected-window correspondence.
- [XochitlIntegration](https://github.com/s116821/ReMarkableBuddies/blob/5f89060d5953d4f5367d03913d81a1019fc2cd6b/src/workflow/xochitl_integration.rs)
  `navigate_to_page` takes only Next/Previous: it swipes and waits a legacy
  500 ms. It neither opens a document nor proves completion.
  [Backend routing](https://github.com/s116821/ReMarkableBuddies/blob/5f89060d5953d4f5367d03913d81a1019fc2cd6b/src/device/backend.rs)
  and [navigation completion](https://github.com/s116821/ReMarkableBuddies/blob/5f89060d5953d4f5367d03913d81a1019fc2cd6b/src/device/navigation_completion.rs)
  have a guarded path using metadata/pixels on an already selected source;
  they are not document-entry readiness APIs.
- [Native page loading](https://github.com/s116821/ReMarkableBuddies/blob/5f89060d5953d4f5367d03913d81a1019fc2cd6b/src/device/native_page.rs)
  retains `load_selected`'s `supported: false`. Saved LastOpen/page metadata must
  not be promoted into native current-page authority.
- Delivered SDK revision
  [`d9be3eb3c8cd997fc33caf817a8ac2ac3f08723b`](https://github.com/s116821/ReMarkableOpenSDK/blob/d9be3eb3c8cd997fc33caf817a8ac2ac3f08723b/src/lib.rs)
  defaults `Platform::navigate` and `Platform::capture` to unsupported. Its
  [capability report](https://github.com/s116821/ReMarkableOpenSDK/blob/d9be3eb3c8cd997fc33caf817a8ac2ac3f08723b/src/capabilities.rs)
  distinguishes synthetic modeling from native availability. No implemented
  public document-opening API was found in these maintained SDK/Buddy sources.
  This is not a claim that every external vendor/community mechanism is exhausted.

### Readiness evidence and remaining decision

Record the actual stock UI selection, then observe the fixture's distinctive
page content and Reader controls replacing the library/loading view. The existing
screenshot example can corroborate this observation in the same xochitl process
and session. Retain contemporaneous independent physical observation if available;
without it, label the result as captured visual evidence only. An unchanged image,
fixed wait, fixture-file hash or LastOpen value alone is not readiness proof.

Any later restart invalidates that prerequisite. A capture in a new process must
establish entry in that new instance; it cannot borrow an earlier stock session's
entry. Visible entry does not prove native document/page identity, exact
QQuickWindow identity, a render fence or capture freshness. This recommendation
adds no polling framework, native hook, SDK API or consumer dependency change.

The retained startup-only capture never established document entry: its selected
operator pinned fixture files, restarted xochitl and requested a frame without
opening/navigation/input. Loading pixels and zero anchors therefore do not prove
that an active Reader cannot be identified. This historical result cannot decide
the current stock state or authorize another run.

Current firmware, sole stock process, UI state, example build provenance and
availability remain unverified here. The sole operator must select any operation
under current-state review, advance notice, bounded output/cleanup and independent
recovery. No new operator packet is selected by this document. Held 8112 and all
spent operations remain excluded, including equivalent replay or rerouting.

Source basis: directly compared committed public sources above; the historical
startup finding and sole-operator constraints are attributed to the current
Project's [retained assessment](https://mem.ai/6eb0ecae-ddeb-5105-a23d-556d392ebb38)
and [Main handoff](https://mem.ai/1c6f5a48-b765-5a56-aaf9-e640a52e3f68).
Those private notes are optional provenance, not public contributor prerequisites
or publicly verified transcripts. The recommendation is inference from these
sources. No device operation, new experiment or runtime modification was performed.

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

Restriction clarification, October 13: Root relayed Main's authority record
version 49: held 8112 transfer/activation/equivalent replay remains excluded,
and spent 0404 is never replayed or repackaged. These are not a blanket ban on
independently distinct useful diagnostics. Classify the requested operations,
effects and failure surface, not merely the presence of activation. No direct
read of that authority note is claimed here; this clarification is attributed
to Root's current-chat instruction.

| Entry or operation | Evidence and disposition |
| --- | --- |
| Already resident, independently permitted typed bridge | Could queue this small observation without replacing the process. No such endpoint or permission is established by the retained evidence. Process/service identity alone does not prove module presence or absence. |
| Direct preload plus public startup callback | Development entry has executed before. Sharing this deployment mechanism does not by itself make a distinct observation equivalent to 8112. The exact observation and whole deployment/recovery packet require their own review; none is selected here. |
| XOVI no-override extension | Eligible architectural candidate sharing the process failure domain. Changing loaders cannot make a held operation permissible, and sharing a loader does not make all operations equivalent. No loader switch or deployment is selected here. |
| Current passive SFTP reader | Can retain bounded memory observations, but cannot execute public Qt calls. Adding a callback or private QML decoder changes its contract. No such expansion is selected. |
| Stock IPC or screenshot | Existing research has not demonstrated a current-reader QObject endpoint. A screenshot cannot supply an in-process root. No claim that all possible vendor interfaces are exhausted. |

The retained 8112 selection is `open-document-insertion-v2`, prepared with
read-only backup and no native execution. It selects SDK 605b1f66, source index 5,
source5-stable-body-idle-v1, AfterOpen95s and an eight-page explicit fixture,
with creation enabled, candidate 130 seconds, absolute 360 seconds and recovery
reserve 185 seconds. Main's retained packet includes staged ready/open/trigger/
result/stop control, library navigation and guardian release/fence. The proposed root
observation has no insertion, source witness, native page getter or capture.
That purpose difference warrants independent operation review; it does not by
itself grant deployment permission or qualify recovery.
Spent 0404's FactsEntry startup and failed restoration cannot be replayed, and
its unexplained rendering fault remains relevant to any process replacement.

### Concrete operation and effect comparison

| Dimension | Held 8112 source5 insertion packet | Proposed root census |
| --- | --- | --- |
| Question | Insert at a specified source in the preserved eight-page fixture and assess the selected workflow | Determine whether the current window's bounded QObject domain contains one consistent reader-anchor pair |
| Application operations | Creation-enabled configuration, source5 witness, staged open/trigger/result flow and library navigation | One GUI callback, window/engine association, names/children/parent/window/effective flags and declared property metadata only |
| Document effects | Native page insertion is intended; target/order/preservation/persistence matter | No document mutation, page opening, navigation, insertion, capture, rendering request or application document/page/controller property-value read |
| Data and output | Explicit document/page inventory, source/target identity and operation evidence | Capped counts, fixed booleans and refusal stage; no document IDs, pointers or arbitrary names; no current-page authority |
| Process effects | Payload loading, setup/trigger machinery and eventual candidate shutdown/restoration | In-process callback, Qt calls/list allocations and owned receipt I/O; any chosen load/restart still changes process state and timing |
| Shared failure surface | ABI/loading, GUI stalls, native lifetime/reentrancy and shutdown/rendering hazards, host/actor loss | Those hazards remain where the chosen entry/recovery uses them; cooperative time limits do not contain a native fault or drain display work |

At the proposed operation level, the census is substantively distinct from the
source5 insertion workflow: it tests root containment without invoking or
preparing the held insertion. It is read-only with respect to document state,
not effect-free or universally safe. Smaller output alone is not the reason for
the distinction. The exact future implementation must enforce the exclusions:
do not instantiate the creation Probe or FactsEntry, include the 8112 creation
configuration/publisher, replay its source witness or navigate to recreate its
workflow under a new label. Do not change executor, model or loader to evade
review. A packet that restores those operations must be reassessed as potentially
equivalent and remain held if it is materially the restricted operation.

No current census implementation or deployable packet is selected. The
comparison supports ordinary independent review of a distinct diagnostic; it
does not release 8112, authorize transfer/activation, establish a resident entry,
or resolve 0404. The existing reviewer must assess the actual source and whole
packet before Main selects any deployment. Successful insertion/ink/reopen
evidence remains separate from this root/lifecycle uncertainty.

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
