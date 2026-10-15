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
The end/completion marker bounds this sampled interval; it does not prove that
Qt input queues are drained or impose ordering across the input and filesystem
descriptors. Any accepted/spontaneous/synthesized indicators are sampled values,
not physical-origin or handler-acceptance proof.

## Specialized end protocol and clocks

Use distinct names/purpose: `input-observation-ready`, temporary
`input-observation-end.tmp` and final `input-observation-end`. Ready carries
nonce/PID/start/root device+inode, waiting-input-observation stage, setup elapsed,
120000 setup and fixed `main-dev-input-observation-120s` profile. End carries the
same identity plus end-input-observation stage,120000 and that exact profile.
Records are private0600 regular non-symlink files, maximum256 bytes. Numeric
identity fields admit only positive decimal PID/start/device/inode values of at
most20 digits each; elapsed time is decimal0..119999 and setup is exactly120000.
Maximum-width ready/end records are189/178 bytes respectively. Validate each
numeric field and complete record length; reject overflow/overlength without
truncation. Apply256 consistently to this mode's writer, reader, publisher and
collector. Existing facts protocol caps remain unchanged.

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
wrong/stale/cross-purpose/duplicate tokens, maximum-width identities and
near-setup-deadline ready tokens, overlength/overflow without truncation;
destroyed/replaced/inactive windows;
closure/restore/identity drift; delayed/lost GUI completion and late/empty/oversize
capture; one grab only and no I/O in the filter. Use explicit synthetic fixtures
and real public Qt GUI/offscreen behavior where available, labeled accordingly.
No tests prove target event acceptance, physical EPD or root cause.

Coordinate exact SDK/consumer proposal revisions before code, then independent
source/artifact/packet review before any Main-owned attempt. No nonce, build,
device action, canonical sync, archive or production authority is selected here.

## October 6 source implementation checkpoint

The default-off FactsEntry mode, fixed event storage and capped output stream are
implemented in qt_page_facts_entry.h and qt_input_observation.h. One accepted end
seals storage and queues one grab. A separate queued publication boundary follows
the capture callback's return, retaining the original accepted-end5000ms budget,
QPointer window, unique engine and live closure/root/attempt checks. Facts-reader
construction remains excluded; the existing facts mode and protocol stay intact.

Verified with tools/qt_page_facts_entry_test.sh in the existing vendor SDK image
sha256:416c7a7be0038156797b0892f031f352b841d1921fae83f712d0a272e4724618,
QT_PROBE_SDK_ENV=/opt/codex/rm2/5.8.203/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi,
read-only source mount and network disabled:21 refusal checks,32 existing entry
cases and16 observation cases PASS. Observation fixtures exercise genuine public
Qt GUI/offscreen callbacks plus explicitly synthetic events/clocks: storage/point/
count/JSON/PNG stream caps, accepted-state preservation, duplicate counts, receiver
relationships, inactive readiness, wrong/overlength/cross-purpose ends, restoration,
setup boundary and window loss/destruction/cancellation/deadline at queued completion.

These fixtures do not cover every proposed target condition: actual target image
behavior, window replacement/ambiguous-engine selection, nested render behavior,
full target touch synthesis, exact physical EPD and installed artifact compatibility
remain unqualified. Source inspection establishes the single grab call and no
event-filter output; fixtures cannot prove all possible Qt render reentrancy.
Independent exact source review, artifact/packet selection and Main-owned device
gates remain open. No target payload build, fresh nonce or device action occurred.

## Proposed fixed device and coordinate frames profile (review before implementation)

Main and Astra selected this minimum source-only proposal after spent8b observer
records showed corner-local touch coordinates. The existing observation does not
identify the mapping cause or handler acceptance. Main's valid-mask refinement
supersedes the earlier whole-point nonfinite refusal proposal. No implementation,
new artifact, packet, input or device attempt is selected by this proposal.

Require fixed `device-frames-v1` evidence_profile in SDK completion, consumer
decoder, packet bindings and receipt. A newly selected decoder must refuse an
old payload missing that exact profile; preserve original spent packets/receipts
without rewriting or revalidating them as extended evidence. Default-off fixed
development mode remains mutually exclusive with facts, with no PageFactsSession
or native/render/UI authority. This profile does not change the heap overview
profile, helper, image capture or qualification rules.

For each event of the seven existing types, obtain QPointerEvent::pointingDevice()
once and store only fixed scalars: required device_present boolean,
device_system_id signed canonical decimal string or explicit null, device_type
integer or explicit null. Absent pointer means false and both null; present means
true, signed qint64 systemId and numeric type in filter storage; only later JSON
serialization converts the ID with QString::number. Consumer requires canonical
regex `\A(0|-?[1-9][0-9]{0,18})\z` and Int64.TryParse inclusive signed64 range;
refuse plus, -0, leading zeros, overflow, numeric ID and coercion. Type must be
JSON int/long in0..2147483647; retain unknown numeric types without capabilities.
All three properties must exist. No names, seats, unique stylus IDs, raw pointers,
per-point device identity, registries, enumeration, fd inference or retained objects.
SystemId is Qt backend/session identity, not physical origin, evdev fd/inode or
handler identity. Distinct IDs discriminate reported Qt identities; equal IDs
do not establish one handler or physical contact.

Retain existing point id/state and local x/y; add scene_x/scene_y/global_x/global_y.
Touch uses public QEventPoint position/scenePosition/globalPosition; mouse uses
public QMouseEvent position/scenePosition/globalPosition and keeps id0/statebutton
and existing source/buttons semantics. Never normalizedPosition, geometry-derived
normalization, remapping, clipping, screen limits, rounding or zero substitution.
Required per-point valid_mask is JSON int/long0..7: local1, scene2, global4.
Capture each frame independently: both finite doubles mean setbit plus both
numbers; either NaN/Inf means clearbit plus both explicit null. Retain id/state
even mask0. Every coordinate key exists. Decoder accepts setbit only with both
JSON int/long/double finite; clearbit only with both explicit null. Missing keys,
bool/string/one-sided null or mask/value contradiction refuse the completion.
Finite extreme values, including the observed8388608 value, remain evidence.
Invalid frames are represented only by the mask. In this new profile point_overflow
means actual excess capacity; unlike old profile, nonfinite frames do not omit
whole points or set that flag. Old evidence semantics remain historical.

Preserve64 records x4 points, seven saturated counts, fixed receiver relationships,
ms/timestamps, retained scope/lifetime guards, original setup/read/total deadlines,
one end/one queued grab,8MiB PNG and8192-byte JSON whole-record-tail pruning with
output_truncated. Six coordinate doubles per point replace two, adding8192bytes
for64x4 point coordinate arrays; all struct/device metadata remains fixed and
compile-time bounded. Filter stores only fixed scalars, returns false, never
changes accepted state, performs I/O/JSON, dispatches events, traverses windows,
adds timers or creates an instrumentation framework.

Focused owned fixtures must copy deliberately distinct touch/mouse local/scene/
global frames; distinguish repeated and different device IDs; cover null device,
signed64 extrema exact string roundtrip and malformed IDs/types; exhaust all eight
masks with consistent pairs and reject contradictory/missing fields; capture mixed
valid and NaN/Inf frames without losing other frames; preserve finite extremes;
exercise64/65 records,4/5 points, worst-width JSON pruning within8192 with whole
records/overflow; preserve passive false filter and acceptance plus existing
lifetime/deadline tests. Review exact source and consumer freezes before separate
artifact/packet/device selection. No navigation fix or private platform ABI guess.

Interpretation is limited: interior scene/global with corner local is a frame/local
handling lead; corner in all frames points upstream of receiver snapshot without
proving a backend cause. Matching time/global across different device IDs supports
duplicate Qt device delivery only, never two physical contacts or UI acceptance.
Source basis: current Main/Astra coordination and repository source contract;
actual8b reviewed evidence is an observational lead, not a verified root cause.

## Device frames source implementation checkpoint (independent review pending)

Main selected source implementation after Main/Astra accepted proposal490fc8f/
0f5c9d5 and clarified cf7ee82/a0bac1b. SDK filter now stores fixed qint64/int/bool
device metadata and six doubles plus mask per point. Public touch/mouse getters
feed independently finite frames; JSON serialization emits explicit nulls for
invalid frames and canonical signed device-ID strings after capture. Completion
requires device-frames-v1. Buddybc957af54ccaa577c3851496e6a95fa0dda419a1 requires
that exact profile, all device and coordinate properties, strict signed64 IDs,
null consistency, integer mask0..7 and finite mask/value pairs; literal profile
is bound in local packet/receipt and decoder SHA frozen. Facts-mode output and
original deadlines/live guards/image capture/legacy overview binding/preservation
are unchanged. No native payload build/private packet/nonce/device selection.

Verification used vendor image416c7a7be0038156797b0892f031f352b841d1921fae83f712d0a272e4724618,
QT_PROBE_SDK_ENV=/opt/codex/rm2/5.8.203/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi,
read-only source mount, network none and synthetic Qt/offscreen via qemu-arm.
The tools/qt_page_facts_entry_test.sh run passed21 refusal and32 facts entry cases;
observation fixture compilation initially found an ambiguous empty touch-list
constructor. Replacing it with explicit QList<QEventPoint>{} changed fixture only.
The observation portion was then compiled/rerun separately: all17 cases PASS.
Windows CRLF was stripped in the shell stream before execution. No failed run is
represented as a complete suite pass. The new frames case verifies public getter
copy for distinct frames, repeated/different devices, signed64 extrema strings,
null device, all eight mixed NaN/Inf masks without point overflow, finite extreme
values,64/65 records and worst-width four-point whole-record pruning under8192.
Existing caps cover4/5 points, counts and PNG; entry cases retain passive filter,
accepted state, sealing, lifetime and deadlines. These are owned synthetic fixtures,
not target input, installed plugin mapping, physical origin or handler acceptance.

Buddy command `pwsh -NoProfile -File tools/native_page_facts_probe/test-input-observation-collector.ps1`
PASS318 mock/no-device assertions: all8 masks and missing/contradictory frame keys,
nonfinite/nonnumeric values, null/present device schemas, canonical signed extrema
roundtrips and malformed/range/type refusal, old profile refusal, plus original
collector/preservation/deadline/legacy shape tests. Strict OpenSpec and diff checks
are required at freeze. Main/Astra exact implementation review remains open; no
sync/archive, merger or spent evidence promotion. Source basis: current coordinated
selection, repository implementation and these local tool outputs.

### Independent review correction: canonical signed ID termination

Astra reproduced a trailing-LF device ID accepted by the proposed .NET `$` anchor
and whitespace-tolerant Int64.TryParse. Use absolute `\A`/`\z` anchors for this
new signed-ID wire field, retaining the exact inclusive Int64 range check. Buddy
adds LF/CRLF/CR/leading or trailing space/tab refusals and updates frozen decoder
SHA. This corrects the shared proposal and consumer only; SDK scalar capture and
serialization are unchanged. Earlier318 assertions alone missed this case.

### Final independent source acceptance (no artifact or device qualification)

Main and Astra independently accepted exact source triple
SDK4f84be45993edbd80f5ea086c023ec947ca96541 /
Buddy21e06d842f44f886061af6b25d0dd2decd23ff07 /
Docs156138ca7024ae9e44d8522abf0f525aed0069f0 after initial implementation and
both narrow decoder repairs. Each reports full vendor suite EXIT0:21 refusal,
32 facts-entry and17 observation cases; collector331 PASS; no remaining blocking
source finding within the reviewed scope. SDK4f84 proposal-only delta preserves
884f141 code bytes. Astra also reran the original LF reproducer, now false.
Main names sdk-fixtures.log SHA8ae060403e7a47aa782763e6e3b9929961cf248617643f6059cbfdcfca7cd03b
under device-frames-main-source-review and main review5939c001 as internal evidence;
those are coordinator-reported references, not hyperlinks or Sol readback.
Close SDK5.8/Docs4.17 for independent source review only. Native packet4.11 and
artifact/private packet/device/physical-input qualification remain open. Fresh
baseline and any later preparation require Main's separate selection. No worker
device contact, nonce reuse, retry, facts promotion, merge, sync or archive.
Original spent8b receipts/false flags and separate cleanup chronology are retained.
