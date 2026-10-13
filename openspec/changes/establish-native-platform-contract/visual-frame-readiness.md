# Passive frame-stage readiness: source-only proposal

Status: independently accepted source-only readiness design. No source
implementation, build, nonce, device action or selection. Existing visual entry
00fac00d remains unchanged. Main owns any future whole packet and recovery.

## Measured question and limits

The [first native visual result](../../../docs/experiments/visual-only-capture-owned.md#first-native-visual-result-valid-blank-pixels-incomplete-packet-closeout)
was valid blank white pixels; its original operator closeout was uncertain and
later cleanup succeeded separately. There is no measured cause of blankness.
The proposed question is narrower: can a public naturally emitted scenegraph
submission-stage signal be observed on a bound window before the single grab?
It may change acquisition ordering; it cannot certify visible Library, image
freshness, current page/owner, worker completion or physical panel state.

[Qt6.10.3 afterFrameEnd](https://doc.qt.io/qt-6.10/qquickwindow.html#afterFrameEnd)
reports scenegraph frame submission and is emitted from the rendering thread.
[afterRendering](https://doc.qt.io/qt-6.10/qquickwindow.html#afterRendering)
precedes graphics-queue submission;
[frameSwapped](https://doc.qt.io/qt-6.10/qquickwindow.html#frameSwapped)
reports presentation queuing rather than display completion. Prefer one fixed
afterFrameEnd observation for this proposal. Vendor custom-loop emission of that
signal has not been established; absence cannot diagnose the UI or blank image.
Existing [backend capture findings](capture-observation-plan.md) retain the
singleton/framebuffer-copy possibility and missing page-to-pixels relationship.

## Event-only ordering, one original clock

Proposed implementation retains the existing visual source's GUI/root/file/
image/sink/publication bounds. It adds a separately declared frame-readiness mode;
old infrastructure ready is not relabeled as frame or UI readiness. It must be
identified by exact new source/artifact/consumer contract, not enabled by an old
nonce/request. No FactsEntry/source5/held-spent payload reuse.

At the original queued bootstrap, evaluate the existing at-most16-window/
one-engine/focused-visible-active QQuickWindow admission. If no eligible window
yet, observe only the public focusWindowChanged event to attempt the first
binding; no periodic scan, fixed sleep or forced update. Bind the first uniquely
eligible window/engine/content root once and then disconnect discovery. Persistent
ambiguity/cap/foreign-thread/context loss refuses; no alternative-window chase.

Connect that exact window's afterFrameEnd with an app-parented GUI-thread receiver
using queued delivery. The slot only evaluates current guarded context and latches
the first qualifying delivery; it performs no rendering-thread object access,
blocking connection, grab in the rendering thread, forced render/update,
scheduleRenderJob, processEvents loop or thread workaround. Queued delivery is
acceptable here because no operation must finish before the render thread proceeds.
Disconnect after first delivery. Receiver/window lifetimes use context ownership
and QPointers; queued callbacks are inert after terminal state. GUI delivery time
is not the signal's native emission timestamp or a frame-generation ID.

Repeated rendering-thread emissions can enqueue multiple deliveries before the
GUI handles the first callback and disconnects. The one latched observation and
512-byte output cap do not bound that queued-event count or Qt's internal event
allocations. Already queued callbacks must be harmless after latch/termination;
their lifetime and internal memory remain unqualified alongside grab/codec
allocation hazards and require the whole-packet process policy. This proposal
does not substitute a direct/blocking/thread workaround to claim a queue bound.

No prior isSceneGraphInitialized flag, arbitrary signal count or historical frame
may substitute for a delivery after this connection. If a signal occurred before
connection and the scene then stays idle, this attempt can expire without a sample.
That is a permitted refusal, never a reason to stimulate rendering or extend time.

After the latched GUI delivery passes unchanged focus/engine/root/cancel/process
guards, publish a separate visual-frame-ready scalar, proposed cap512 bytes.
Fixed fields: schema1, nonce/PID/start strings, stage=frame-stage-observed,
signal=afterFrameEnd, gui_delivery_observed=true, native_authority=false.
Use the existing exclusive pending/write/checked-close/original-inode/link/
post-link-guard/unlink protocol with .pending-frame-ready. No pointer or frame/page
ID. Receipt proves only a sampled signal-delivery/context ordering claim.

Main may withhold the ONE visual-request until that marker is collected with
retained ownership/context brackets. A premature present request is consumed and
refused, never parked for later fulfillment or retried. At dispatch require the
same window/engine/root still eligible, frame delivery latched and marker binding
valid; then consume the request before the ONE existing public GUI grab. Signal
delivery cannot bypass image/scalar/pair/decode/Main visual checks. Blank or invalid
output never authorizes a second grab or reopening in this entry.

Entry construction's original60s covers discovery, passive event delivery,
marker publication, host collection/request, grab/encoding and successful output.
No timer reset, render-wait loop or nested event pumping is introduced. Missing
signal/request waits only through ordinary events and the existing terminal timer;
shutdown/cancel/expiry/context loss are sticky and disconnect observers. The
original32 request-directory check cap remains; frame-marker writes can consume
coalesced checks and must be accounted for. Non-preemptible calls and all native
lifetime/recovery hazards remain.

## Quota and integration delta, not current selection

The proposed512-byte marker adds unique output quota:1,049,675+512=1,050,187 bytes.
Its pending/final names can coexist as hardlinks to the same bytes. This changes
Main's producer inventory, pending-name rejection, collection/transport/error/
cleanup quotas and scalar validation; it is not hidden inside the old contract.
The image cap, decoded/storage/scratch limits, ready75/result1024 and one-image
restriction remain unchanged. Exact Main collector/consumer amendments and
whole deadline/recovery/current provider/fixture review precede any selection.

Focused source controls, only after design acceptance: no signal yields original
expiry/no grab; one natural/injected owned delivery can authorize one grab;
wrong/lost window and nested shutdown/cancel refuse; premature request consumes
without later replay; marker publication loss is invalid; repeated queued signals
never dispatch another grab. A real owned signal proves public connection behavior
only, not vendor native emissions or rendered content. No broad fixture matrix,
old capture/FactsEntry suite or native attempt is requested by this document.

Source basis: official Qt6.10.3 signal/thread documentation, existing accepted
visual source and backend capture findings, direct retained c69 image/scalar/
operator inspection, and separately attributed Main execution/recovery reports.
The design is inference from those surfaces; it is not evidence that a frame
signal will remove blankness. Full native/render/current-page/lifecycle tasks
remain open and prior insertion/ink/reopen milestones remain preserved.

## Design review checkpoint

Root's existing Sol reviewer independently verified the frozen four-file manifest
eb57133ae2ff4b850ae6a637881ce3dc480e925dc87477b6e024aedd4b92918b,
checked versioned Qt signal/connection/focus semantics and accepted the result
record/proposal with no actionable finding. This is design/evidence review, not
execution or native readiness qualification. Main subsequently raised the queued
event-memory limitation now explicitly recorded above; it adds no queue-bound
claim or connection-model workaround. Internal review messages have no public
transcript link. Concrete implementation and Main amendments require their own
focused review; no native build/selection follows from this publication.
