# Proposed development Qt input observation and one window image

## Basis and status

Proposal only against SDKb58a59bcaa08362741df0302f0a4f50e1ff98e1a. Main reports
stock56f positive evdev echo with a visible document open, and DEVf1d positive
echo with unchanged saved My Files pixels. Their conditions differ; neither
localizes the failure. Astra inspected the existing startup, FactsEntry filter,
window bootstrap and descriptor/generation checks and recommended this narrow
discriminator. Main selected proposal work only, with no new device attempt.

## Entry reuse and exact readiness

Add one explicit fixed development observation selection to the existing private
FactsEntry config/startup, default off and mutually exclusive with facts reading.
Reuse retained parentless lifetime, queued GUI startup, existing eight bootstrap
attempt/sixteen-window limits, held directory descriptor/inode/owner, nonce,
attempt PID/start, closure/restore markers and setup timer. Ordinary facts mode
and PageFactsSession remain unchanged. Observation mode never instantiates that
session, scans document ownership, calls native OPEN or dispatches synthetic Qt
events. This is experimental tooling, not public SDK capability.

Before publishing readiness, select QGuiApplication::focusWindow as exactly one
visible active QQuickWindow on the GUI thread, consistent with the selected
qmlEngine(window), and retain its QPointer. My Files needs no document owner.
Engine readiness alone is insufficient. Set/reset the bounded observation store
and enable collection for this retained window before exclusive publication of
`input-observation-ready`. No repeated readiness publication or new probing loop.
Existing bounded bootstrap readiness events remain the only readiness attempts.

## Fixed observation memory and filter semantics

Reuse the existing application eventFilter, which always returns false. It never
accepts/ignores/changes an event, writes files/logs, captures images, traverses the
scene/document or invokes application handlers. While ready and unsealed, collect
only TouchBegin/Update/End/Cancel and MouseButtonPress/Release/Move, with fixed
integer type counts and at most64 fixed records containing monotonic entry time,
Qt timestamp, type, mouse source/button flags or touch point IDs/state and up to
four finite position pairs. Record selected-window relationship as direct window,
QQuickItem::window match, other or unknown, using GUI-thread public relationships
only. No object names, titles, property lists, raw pointers or document text.

Use fixed-capacity storage; cap per-event point iteration at four, signal dropped
records/points and saturating-count overflow explicitly. Event and point counts
are not physical action/contact counts: synthesized mouse can duplicate touch.
Other/unknown targets do not establish selected-window delivery. Later installed
application filters may consume events first. Zero events proves only this
observer saw none within its scope, never kernel rejection or Qt-wide absence.

## Specialized end protocol and clocks

Use distinct names/purpose: `input-observation-ready`, temporary
`input-observation-end.tmp` and final `input-observation-end`. Ready carries
nonce/PID/start/root device+inode, waiting-input-observation stage, setup elapsed,
120000 setup and fixed `main-dev-input-observation-120s` profile. End carries the
same identity plus end-input-observation stage,120000 and that exact profile.
Records are private0600 regular non-symlink files, maximum128 bytes.

Directory watcher queues an explicit observation-end handler, never queueRequest's
read-facts dispatch. Any existing end token consumes one admission, valid or not.
Check exact content, held root/path inode/owner, nonce/process identity, GUI/engine/
window scope, closure/restore absence and original setup deadline before/after
reading. Wrong purpose, stale or duplicate tokens fail closed; facts-request
cannot end observation and observation-end cannot start facts.

On accepted end, seal collection on the GUI thread BEFORE capture, record the
accepted timestamp and queue exactly one capture callback. Preserve setup120000ms
from entry birth; completion5000ms from end acceptance includes queued dispatch,
grab, encoding and publication. This is an observation completion budget, not a
claim PageFacts was requested. It cannot reset setup or consumer host150000ms,
rollback180s or restoration TimeoutStart240s clocks.

## One separate GUI capture and completion evidence

Recheck exact retained/focus window, visibility/activity, GUI/engine thread,
root/generation/lifetime and completion deadline around the callback. Perform
one QQuickWindow::grabWindow on that retained window, separately from eventFilter.
It can render/read back, block or allocate; it is not passive or hard cancellable.
Precheck finite positive dimensions/DPR and anticipated pixels<=4194304; after
grab require nonempty image with actual pixels<=4194304 and raw bytes<=16777216.
No scaling, second window selection, retry or capture loop. Empty image is
unsupported; dead/changed window, scope loss or late return is unknown.

Publish at most one private PNG `input-window.png`, encoded through a capped
exclusive output path (maximum8388608 bytes; partial/failed output unusable), and
one bounded JSON `input-observation-complete.json` (maximum8192 bytes). File I/O
and JSON formatting prune only whole records to this cap, retaining counts and
explicit output truncation rather than publishing partial JSON. File I/O
occurs only outside eventFilter. Completion records collected counts/records and
overflow, window metadata, seal/accepted/grab timestamps, image status/dimensions,
actual GUI-callback completion and scope/deadline checks. Observation completion
and image availability are separate facts. Emit no successful GUI completion
marker before the designated callback returns; expired/lost scope grants no
usable completion. No marker means unknown, not no events or no render.

Every result explicitly has native_authority=false, render_authority=false and
ui_acknowledged=false. Observed Qt events establish a delivery boundary, never
handler acceptance. The image is Qt scenegraph evidence, not physical EPD proof
or native page ownership. Separate consumer heap/physical observations require
temporal alignment and stability; agreement/disagreement alone is not causality.

## Verification and coordinated gates

Extend existing owned Qt entry fixtures: mode isolation/zero facts-reader calls;
exact readiness/window selection; unchanged event return/accepted state; storage/
point/output caps and overflow; touch/mouse duplicates; other/unknown receivers;
wrong/stale/cross-purpose/duplicate tokens; destroyed/replaced/inactive windows;
closure/restore/identity drift; delayed/lost GUI completion and late/empty/oversize
capture; one grab only and no I/O in the filter. Use explicit synthetic fixtures
and real public Qt GUI/offscreen behavior where available, labeled accordingly.
No tests prove target event acceptance, physical EPD or root cause.

Coordinate exact SDK/consumer proposal revisions before code, then independent
source/artifact/packet review before any Main-owned attempt. No nonce, build,
device action, canonical sync, archive or production authority is selected here.
