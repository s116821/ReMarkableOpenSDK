# One development owner/window capture before facts admission

Status: proposed SDK source refinement against `b8c1d0372c878122a2fb3b76bae88cfc019b1dda`.
Main owns device operation; Astra owns this native-source investigation; Sol owns
ordinary implementation/integration and independent review. This is not a native
capture qualification, a target packet, or permission for another device attempt.
It supports tasks 2.9 and 4.11 without reopening direct PageKey opening or the
debugger investigation. Consumer coordination starts at Buddy `89fd9d23e13bb81493239d28e67e39afcebad754`
and Docs `33a294618b37789a6827df0ea40a25e89e71b714`; a matching consumer amendment
and exact implementation/artifact review remain required.

## Evidence and the question this can answer

Current `FactsEntry::queueBootstrap` emits `waiting-facts` after engine/context
selection, before `PageFactsSession` performs owner discovery. It is not a document
or rendered-page readiness signal. The retained da8 run observed Qt input and
document/page loading before a legacy My Files PNG, but stopped before facts
publication. The owner predicate was unexercised, not demonstrated to fail.

The retained 50d Qt PNG (SHA256 `9cd4be816961c5af2a251ddff7421e38c71a5c849c07bf0c3fe0dd30755d6a2e`)
and later heap PNG (`c1320eee6c47d3df1080a729db09e3490bfdc376e9066abcd3ac28952a996ad4`)
both visually show the annotated PDF. They are distinct acquisitions: the Qt image
includes the page-count footer and the later heap image does not. Neither image
has a native owner witness. Different heap addresses across processes do not
diagnose stale allocation, and this comparison does not establish da8's cause.

[Qt 6.10.3 QQuickWindow source](https://github.com/qt/qtdeclarative/blob/v6.10.3/src/quick/items/qquickwindow.cpp)
requires GUI-thread grabbing and delegates to a render loop/control. The upstream
[software loop](https://github.com/qt/qtdeclarative/blob/v6.10.3/src/quick/scenegraph/adaptations/software/qsgsoftwarerenderloop.cpp)
polishes, synchronizes and renders into a backing store during a grab; a pending
update can also cause a flush. The
[threaded software loop](https://github.com/qt/qtdeclarative/blob/v6.10.3/src/quick/scenegraph/adaptations/software/qsgsoftwarethreadedrenderloop.cpp)
blocks the caller while the render thread synchronizes, renders and reads back.
These are upstream mechanisms, not proof of the vendor's exact render loop.
Grabbing can therefore change rendering state, but it must not be assumed to
force a fresh render on the selected RM2 backend. Its return does not certify
asynchronous native page-worker texture readiness.

Local inspection of the pinned vendor SDK image established that its
libQt6Quick.so.6.10.3 matches the retained target-admission SHA256
`b20f9adaefe8cebeca445906e891370f75f0b0af878ac1b35e0c91159778628f`.
Its generic grab dispatch agrees with the upstream render-loop/control boundary.
The SDK also contains libqsgepaper.so, SHA256
`4f8a352a45550679bef3e5a800393e7289cbb43377e33b42ceed23f89ca6b831`.
Narrow static inspection of that plugin's grab path finds a copy of the image held
by its framebuffer singleton, without using the requested window or invoking
polish/synchronization/render work in that path. Buffer setup can select a supplied
image or its internal image; this is not an observed native page/frame identity.
Main subsequently reports a fresh read-only target hash matching this plugin;
the stock process mappings did not show it loaded. Runtime plugin selection remains
unverified. If that
path is active, a call on the retained window still acquires framebuffer pixels,
not a window-specific render completion. No private offsets or reconstructed
firmware source are incorporated into the SDK implementation.

The retained xochitl artifact was freshly verified at SHA256
`071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df`.
Narrow local static tracing also identifies an embedded EPRenderLoop: an
initialization branch passes an object with that RTTI/vtable to
QSGRenderLoop::setInstance. Its grab slot likewise obtains a singleton-held image
and invokes the QImage copy constructor, with no requested-window use or direct
render call in that body. This supplies a concrete built-in candidate despite the
missing standalone plugin mapping. It does not establish that the branch executed
or identify the live render-loop object; initialization/getter effects remain
possible. Private locators stay outside Git. No new hook or backend selection is
introduced by the observation implementation.

Qt frame submission/swap signals identify scenegraph stages, not a native page
generation or completed physical e-ink refresh. Adding a frame counter or waiting
for one such signal would not close the page-to-pixels gap. No frame-wait loop,
forced update, fixed sleep or worker-thread grab is selected here.

[Pinned Inkling source](https://github.com/nathanmarlor/inkling/blob/089efb2c9f24ce64127c6fc4d7fd30f93ae5ad94/xovi-ext/inklingfb/main.c)
reports a GUI-grab panel freeze and uses a worker-thread exception, in conflict
with upstream Qt's GUI-thread requirement. This is third-party reported behavior,
not a reproduced local result or justification to adopt its thread/ABI workaround.
Keep the existing independent process restoration; no in-process deadline can
preempt a hung grab. Original implementation only; no prior-art code is copied.

The bounded question is: did one uniquely observed active owner retain its current
identity through one grab requested on that same window, and what pixels did that
backend-dependent acquisition return? Full visual review can corroborate the expected fixture. This is development
correspondence evidence; it does not establish content revision, worker completion,
general render freshness, a qualified CapturedBatch, or physical panel state.
The stronger [capture contract](capture-contract.md) remains unchanged.

## Minimal source seam and purpose

Add `developmentCaptureObservation=false` to `FactsEntryConfig`, permitted only
with the existing development setup120000/facts5000 configuration and incompatible
with `developmentInputObservation`. Retain `main-dev-facts-120s`, original entry
birth, immutable host origin and existing recovery. Old facts/input modes retain
their current behavior. Source/artifact/config bindings must explicitly identify
the new option; the old waiting marker alone does not advertise this feature.

Use the existing held directory and watcher, not another endpoint or loader:

- `capture-observation-request.tmp` / `capture-observation-request`: one private
  purpose-bound request, with `identity("capture-observation") + " 120000 main-dev-facts-120s\n"`.
  Apply the existing positive decimal identity validation and 256-byte cap.
- `capture-window.png`: exclusive private PNG, reusing `InputImageOutput` and the
  existing 4194304-pixel / 16777216-raw-byte / 8388608-encoded-byte bounds.
- `capture-observation-complete.json`: exclusive private completion, maximum8192
  bytes, after the GUI capture callback unwinds. It is not callback.json or a
  facts record. No truncation of identity/status fields is allowed.

Startup rejects any stale new files when this option is selected. The first
observed request consumes a separate capture latch before validation. Never reuse
the facts `consumed_` latch or `acceptedAt_`; capture admission must not consume,
create or accept `facts-request`. Existing `input-observation-end` and its `.tmp`
are wrong purpose. Either `facts-request` or `facts-request.tmp` before capture
completion is terminal refusal. Repeated
watcher notifications for the retained request are harmless and never re-grab;
replaced request identity or an attempted second publication refuses continuation.
Keep the original request file immutable and check its retained file identity
before admitting the later facts request.
Atomic link publication may briefly retain the request.tmp name as an alias of
the exact held final-request inode. Accept only that regular0600/current-owner
alias with identical purpose bytes during publication; a foreign temporary name
refuses. Once absence of the temporary alias has been observed, its release is
sticky and any later reappearance refuses continuation. The selected final facts
publisher still requires the temporary name absent. This exception cannot admit
a second capture or a replaced request.

The selected capture has a fixed5000ms deadline from its own acceptance, additionally
capped by original setup expiry. Queued dispatch, owner reading, grab, encoding
and completion publication all count. Use separate capture acceptance/deadline
members; after successful completion arm only the remaining original setup time.
Never reset entry birth, extend host150000/rollback180s/restoration240s, or call
`finish()` on successful capture. Failure/unknown is terminal and restores stock;
there is no fallback to heap-only, re-grab, re-scan or second input.

## Retained owner and one acquisition

In one queued GUI callback, use `findPageOwner` once with its existing topology
bounds. Require the unique owner window to be the current visible active focused
window belonging to the retained engine. No first-view fallback or window change.
Retain the complete `PageOwner` QPointers. Reuse the required owner/document/scene
invalidation signals enumerated in `PageFactsSession::installObservers`, including
worker and viewport changes, but do not construct PageFactsSession or call mapping
methods before the visual gate. Missing/incompatible required signals refuse.

Install observers before accepting an identity baseline. A small fixed QtQml helper
reuses the established `String(document.id)` conversion and reads only document ID,
receiver currentPage/currentPageId, and scene pageId. Validate canonical nonnil IDs,
matching aliases, integer index within the selected expectedOrder, expected document
and expectedOrder[index]. The expected order is caller input, not an observed order;
do not output page-count/order facts or call idForPage/pageForId here. Revalidate
weak owners, activeOwner, context, deadline and sticky epoch after every getter or
conversion and on helper return. The helper has no completion/destruction handlers,
mutators or arbitrary expression input. These reads can still invoke native code.

Record the baseline only after helper construction/setup and observer installation.
During the capture interval the existing always-false event filter only marks
relevant input invalidation; it performs no traversal, getters or I/O. Touch/mouse,
tablet, wheel and key delivery invalidate this diagnostic continuation, without
claiming complete native input isolation. Focus/owner/lifetime/viewport changes
are also sticky invalidations, including change-away-and-back observed by signals.
Never clear them to obtain a favorable result.

Check image dimensions/DPR first, perform exactly one GUI-thread grabWindow on the
retained owner window. After checking returned dimensions/raw bounds, immediately
make one bounded owned pixel copy with QImage::copy before post-read getters or
encoding. Qt's [version-matched QImage implementation](https://github.com/qt/qtbase/blob/v6.10.3/src/gui/image/qimage.cpp)
normally shares data in its copy constructor; a returned QImage alone does not
establish independent pixels when a backend retains external storage. The owned
copy is not another grab or an atomic framebuffer snapshot. Count its allocation
and execution in the same capture deadline, reject null/late/lost-scope copy, and
include copying in the acquisition interval ending at grab_end_ms. Then reread
the same small identity subset under the same
observers and compare it with baseline. Empty/oversize image, owner/context change,
observed epoch/input change or late return cannot yield usable completion. Preserve
nested-call depth and deferred teardown through getters and grab reentrancy.
Use the existing capped exclusive image writer outside eventFilter. Hash the exact
encoded PNG bytes for completion binding; reject partial, late or replaced output.

Publish after the capture callback returns, with another retained owner/epoch/root/
attempt/deadline check. Completion contains exact attempt/root/profile identity,
accept/baseline/grab/post-read/completion times, observed document/page/index,
local epoch before/after, window/DPR/image dimensions, encoded byte count and SHA256,
`kind=development-capture-observation`, `version=1`, and explicit false
`atomic_snapshot`, `native_authority`, `render_authority`, `ui_acknowledged` and
`observed_order`. No raw QObject addresses or native frame-generation claims.

The fixed version1 field names are `kind`, `version`, `nonce`, `attempt_pid`,
`attempt_start`, `root_device`, `root_inode`, `setup_profile`, `setup_budget_ms`,
`capture_budget_ms`, `accepted_ms`, `baseline_ms`, `grab_start_ms`, `grab_end_ms`,
`post_read_ms`, `completed_ms`, `document_id`, `page_id`, `page_index`, `begin_epoch`,
`end_epoch`, `width`, `height`, `dpr`, `image_width`, `image_height`, `png_bytes`,
`png_sha256`, `image_status`, `gui_callback_completed`, `scope_current`,
`atomic_snapshot`, `native_authority`, `render_authority`, `ui_acknowledged`, and
`observed_order`. Process/start/root numeric identities are positive decimal strings
of at most20 digits. begin_epoch/end_epoch are also positive canonical decimal
strings of at most20 digits (quint64), with the local capture epoch initialized to1.
Times and dimensions are bounded JSON integers, DPR is finite positive, IDs are
canonical strings, SHA256 is lowercase64hex. Conservatively require image_width
equal qRound(width*dpr) and image_height equal qRound(height*dpr); mismatches refuse,
including a backend returning a differently sized framebuffer. Success requires
`image_status=available`, both completion/scope booleans true and the five authority/
order booleans false. No success document is emitted for empty or failed capture.
Consumer validation rejects missing/extra/wrong-type fields, nonmonotonic times,
out-of-budget completion and inconsistent dimensions/bytes/digest. The SDK hashes
the exact encoded PNG stream; the consumer independently hashes retrieved bytes.

Retain the owner watchers after completion through the visual pause and final facts
delivery. Sticky invalidation blocks continuation; no refresh/rediscovery resets it.
At final facts admission verify that capture completed normally, the same retained
owner/identity context remains valid, and the capture token/output bindings match.
Then the original facts request starts PageFactsSession once under its existing
5000ms budget. Its result must agree with the captured document/page/index, and
the retained capture guard must remain valid through facts output delivery.
This conservative dependency is development continuity evidence, not a native lease.
The selected consumer facts publisher must independently require the exact valid
capture completion and image binding plus its recorded positive visual decision;
mere file presence is insufficient. SDK cannot authenticate human visual review;
that gate stays consumer-owned. Capture completion is never facts publication.

## Main-owned bounded recipe and implementation checks

1. Freeze accepted SDK/consumer source, new option, artifacts and exact packet under
   existing baseline/readiness/recovery procedures. Use one preselected changed-axis
   input and verify its complete release. No new timing adjustment or second tap.
2. After that existing setup completes, publish the capture request once under the
   original same-process/root/closure/deadline checks. Await the bounded completion;
   transport timeout or ambiguous publication is terminal, never permission to retry.
3. Retrieve and hash-check completion and native-resolution PNG. Main visually checks
   the complete image against the expected opened fixture, not a library thumbnail.
   Legacy heap acquisition, if explicitly retained for comparison, remains a separate
   image with its own interval/provenance; it cannot grant owner/render authority.
4. Only after positive visual review and fresh unchanged guard evidence publish the
   existing facts request once. Collect original facts and restoration evidence.
   Any unknown/refusal preserves artifacts and ends the attempt under existing recovery.

Before a target packet, extend existing owned Qt fixtures for: ordinary modes
unchanged; wrong/early/cross-purpose/replaced/stale tokens; one capture despite watcher
reentry; no PageFactsSession/mapping read before facts; unavailable/ambiguous owner;
wrong IDs/alias/index; owner destruction or ABA/input/viewport/worker invalidation
during getter/grab/queued completion/visual pause; empty/oversize/late image; capped
encoding and output replacement; original setup expiry despite capture success;
external-buffer alias modification after the owned copy cannot change encoded pixels;
one facts admission after capture, with result agreement and invalidation through
final delivery. Use injected hooks/clocks and owned Qt scene items, not proprietary
objects. Run the existing entry/facts suites plus these focused cases. Host/offscreen
passes cannot qualify the vendor render loop, worker texture freshness or panel.

Keep tasks2.9/4.11 and native capture qualification open. Main persists the accepted
handoff through existing canonical workflows; no archive or shipping claim follows.
