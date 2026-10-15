# Scene tiles, render correspondence and page geometry

Status: October 13, 2026 static interoperability findings and a proposed
discriminating observation. No native execution, new adapter, producer schema,
consumer pin or qualification follows from this work.

The saved firmware 3.28.0.172 executable provides a concrete path from
`SceneView` through `SceneTileManager` to `DeviceSceneViewport` painting. It also
shows why a page ID, a readiness flag and a Qt frame signal cannot individually
establish that a captured image contains the current page. The next useful
observation must bind this object relationship, accepted tile work, geometry and
recognizable page content in one running interval.

## Evidence and limits

The private saved executable has SHA-256
`071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df`.
Qt metaobject tables were decoded as data, then their actual getter, invocation
and vtable paths were inspected with the existing vendor SDK 5.8.203 ARM tools.
No proprietary executable was run. Raw binary, disassembly and reconstructed
function bodies remain private; none is incorporated here. These are static
observations, not a transcript of live object values. Private evidence has no
public download link. Addresses and layout offsets are not a supported API.

The exact vendor headers identify Qt 6.10.3. Its public upstream
[window grab implementation](https://github.com/qt/qtdeclarative/blob/v6.10.3/src/quick/items/qquickwindow.cpp)
and [software render loop](https://github.com/qt/qtdeclarative/blob/v6.10.3/src/quick/scenegraph/adaptations/software/qsgsoftwarerenderloop.cpp)
explain the generic Qt branch, not an attestation of the vendor render-loop
implementation. The software grab polishes, synchronizes and renders the current
scene, then obtains an image. That sequence does not wait for an independently
managed vendor document tile request to finish. Generic Qt frame signals do not
identify that request or its page.

## Observed relationships

| Observed path | What it establishes | What it does not establish |
| --- | --- | --- |
| `SceneView.tileManager`; manager `worker`, `pageId`, viewport size, center and scale | Concrete source and geometry observation points exist on related objects | Which live view is the selected page, complete ownership, or stable identity across a capture |
| `SceneView.checkInitialTileSetReady` tests exactly the conditions exposed by manager `pendingTiles`; on readiness it clears `awaitingInitialTileSet` and disconnects that readiness connection | An initial-readiness latch is distinct from ongoing observation of pending work | That the latch stays connected forever, that later work cannot start, or that false means every required pixel is current |
| Tile-result handler compares the incoming `Interrupter` pointer with the manager's current request pointer before accepting the result | Result admission uses request identity, stronger than a generic frame counter | A public monotonic render generation, or a page/document identity encoded by that pointer |
| Request path interrupts an existing request, installs a new shared request object and carries request/geometry state into queued work | Request identity can change within one manager lifetime | That re-reading manager pointer and page ID detects all such changes |
| `resetPendingJobs` marks the current request interrupted, clears its shared request holder and emits `pendingTilesChanged` | Pending work can disappear through cancellation | Successful tile completion; this is a concrete reason not to equate pending=false with successful rendering |
| `onWorkerDeleted` clears the worker and page ID, releases associated state and resets the manager | Worker lifetime is relevant to source validity | A barrier that makes a borrowed object safe after destruction or process shutdown |
| Device viewport's manager property and painter path refer to the same manager member; paint retains a list of tile shared pointers, maps tile rectangles and calls `QPainter::drawImage` | There is a tile-to-Qt-painter path, not merely a named framebuffer property | That every page layer, pen overlay or final physical display update appears in `QQuickWindow::grabWindow` |
| Device viewport repaint schedules `QQuickPaintedItem::update`, with render-loop-dependent handling | Scheduling a repaint and actually painting are separate steps | Completion of vendor jobs, a physical display fence, or safe shutdown |

The latch may be rearmed elsewhere; the inspected readiness function does not
establish its full lifecycle. `pendingTiles`, `transactionRunning` and
`preservingTiles` are separate properties. Their values must not be collapsed
into an invented single "ready" state. A canceled request can make the pending
predicate false when its other condition is also clear. No cancellation is
proposed merely to demonstrate that counterexample on the tablet.

## Geometry that can now be tested

The manager exposes both `viewToSceneTransform` and `sceneToViewTransform`.
Their getters use the same viewport-size, center and scale state. The inspected
scene-to-view path includes rounding of the scaled viewport origin before
translation. For ordinary finite, in-range inputs, the candidate relation is:

```text
origin_pixels = round_away_from_zero_at_half((center - viewport_size / (2 * scale)) * scale)
view_point    = scale * scene_point - origin_pixels
```

This is an interpretation of the inspected arithmetic, not a replacement native
implementation or a rule for invalid scale, overflow, NaN or infinity. The
native getter remains the preferred observation. Omitting its rounding can shift
the result by up to half a view pixel per axis in ordinary range. Retaining
actual operation order matters near a rounding boundary.

This transform is **scene to viewport**, not page to captured-window pixels.
It still needs the viewport item's mapping into the selected window, image
pixel scaling, clipping, orientation and a justified page domain. The painter
also aligns tile rectangles to integer edges; transformed mathematical points
and raster tile coverage are not interchangeable. `sceneBoundingRect`,
`sceneExteriorBoundary` and controller paper bounds must not be called the PDF
page box solely because their names sound convenient. Infinite-canvas notes
may not have a single finite normalized page domain at all.

## Concrete proposed discriminator

Hypothesis R is that source identity can change or tile work can be canceled or
replaced while an initial-readiness latch remains false and an older tile image
can still be painted. Hypothesis G is that a window-size-only coordinate mapping
fails to account for viewport placement, rounded translation and clipping. These
are hypotheses to distinguish, not reported tablet failures.

For any future independently permitted observation, Main would first select an
existing non-personal fixture with two pages containing distinguishable content
and known non-collinear landmarks. An existing suitable fixture should be used;
this proposal does not authorize a download, transfer, drawing or document edit.
One page transition and, only if separately selected, one fixed-view geometry
change are enough for a first discriminator. A broad fixture matrix is not the
next step.

The observation needs three linked stages on the same selected objects:

1. Before acquisition, establish the actual `SceneView`/controller/manager/viewport
   relationship and matching source IDs and worker association. Observe current
   pending, initial-latch, transaction and preservation state, viewport dimensions,
   both native transforms, item-to-window mapping and effective clip. Observe
   request replacement/cancellation and accepted completion if the selected method
   can distinguish them; property equality alone cannot supply this history.
2. Capture through the selected acquisition path and immediately own the pixels.
   Retain the same object/source/transform observations after acquisition. A
   request change, cancellation, owner destruction, mismatched IDs, invalid
   transform or changed geometry makes the attempt inconclusive for correspondence.
   Do not actively call reset, reload, readiness checks or paint methods to force
   a favorable state.
3. Compare independently known page landmarks with the owned pixels using the
   observed scene-to-view and item-to-window transforms and actual image scaling.
   Use landmarks inside and near the visible clip. Distinguishable old-page content
   despite matching selected source IDs rejects identity-only correspondence.
   Missing current-page content with pending=false rejects that readiness rule.
   A translation/scale/clip residual rejects the proposed geometry chain. A
   matching observation supports only that observed interval, not universal
   qualification, input authority or physical-panel completion.

If the observation method cannot see request acceptance, cancellation and object
lifetime, its output must say so. It can still falsify a correspondence claim
through visible mismatch; it cannot establish a native render fence through
silence. No new receipt field can substitute for the missing observation.

## Access and recovery dependencies

There is currently no selected, independently permitted running-native method
for the observation above. Held 8112 activation and equivalent replay are
unavailable; spent 0404 is not reusable. This diagnostic does not rename or
repackage either route. Main remains the sole tablet operator.

Before any future implementation/selection, the exact method must explain how
it obtains live typed objects and accepted-request evidence, bounds observation
work and output, and recovers independently of xochitl and the observer. GUI
thread property access is not automatically time-bounded or harmless during
destruction. Retaining tile shared pointers during vendor paint is not evidence
that the observer can retain borrowed manager, worker, window or image storage
through shutdown. Existing deadlines remain absolute; no new retry, wait,
service restart or per-capture recovery workaround is selected here.

The saved 0404 core cannot answer this through `QCoreApplication::self`: GDB
prints zero, but direct ELF program-header inspection finds no captured
`PT_LOAD` covering that symbol's address. That debugger value is not captured
runtime evidence and does not prove application teardown. This closes that
specific saved-core anchor route without guessing other heap addresses. The
[original shutdown limits](shutdown-core-extent-limits.md) remain unchanged.

The pinned community [rendering-pipeline investigation](https://github.com/timower/rM2-stuff/blob/451577081b9eaa3d582e50bbca8c4adfd4911b53/doc/RENDERING_PIPELINE.md)
distinguishes canvas, pen and physical update handling and corrects earlier
queue/refresh assumptions. It motivates checking which layer is observed; its
older 16-bit canvas description does not establish this build's pixel layout,
tile lifetime or window-grab behavior.

Source basis: exact-build private static observations, pinned Qt primary source,
the attributed prior-art investigation, existing repository capture/shutdown
contracts and current Main/Root coordination. The proposed observation, finite
geometry interpretation and remaining access dependency are engineering analysis.
