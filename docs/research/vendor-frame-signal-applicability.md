# Public frame-signal applicability on the pinned vendor paths

October13 source-only assessment following the
[native62416 binding-witness outcome](../experiments/visual-only-capture-owned.md#native-binding-witness-result-deferred-then-installed-no-accepted-frame-marker).
Binding/observer installation was witnessed; no accepted frame marker or image
request followed within the sampled budget. This narrows the question without
proving lack of emission, current callback context or a blank-frame cause.

## Reused evidence and bounded inspection

Retained xochitl SHA256
`071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df`
was hash-verified locally. The existing vendorSDK5.8.203 providers match earlier
records: QtQuick `b20f9adaefe8cebeca445906e891370f75f0b0af878ac1b35e0c91159778628f`
and standalone epaper plugin
`4f8a352a45550679bef3e5a800393e7289cbb43377e33b42ceed23f89ca6b831`.
These are retained/build-provider identities, not a fresh active-module census.

Read-only ELF import inspection found no direct afterFrameEnd/beforeFrameBegin
imports in the application or standalone plugin. Both import public frameSwapped
and the private common renderSceneGraph routine; the application also imports
beforeRendering and QSGRenderLoop::setInstance. Missing direct imports alone do
not prove a signal cannot arise through Qt, an alternate path or dynamic logic.

The bounded plugin static call summary identifies
EPRenderLoop::handleUpdateRequest(QQuickWindow*) references to public frameSwapped
and common renderSceneGraph. This supports a concrete vendor-path candidate.
A linear call catalog does not prove call ordering, branch execution, the live
loop, its window or actual signal delivery. Proprietary bodies, raw locators and
disassembly remain private; no offset admission or hook is introduced.

Earlier [backend capture findings](../../openspec/changes/establish-native-platform-contract/capture-observation-plan.md)
already identified a standalone singleton-image-copy grab and an embedded
xochitl EPRenderLoop initialization/setInstance candidate. Standalone plugin
mapping and the embedded initialization branch's live execution were not
established. The owned software fixture does not identify the native loop.

## What the public signals support

| Signal | Source evidence | Limit |
| --- | --- | --- |
| afterFrameEnd | Qt6.10.3 standard/threaded software loops emit it at their frame-end paths | Vendor custom-loop emission is not established; no-direct-import is not a runtime absence proof |
| afterRendering | The [common Qt6.10.3 renderSceneGraph](https://github.com/qt/qtdeclarative/blob/v6.10.3/src/quick/items/qquickwindow.cpp#L573) emits before/afterRendering when its renderer path runs | Not graphics submission, worker/page readiness or completed display; skipped renderer paths can omit it |
| frameSwapped | Application/plugin imports plus a direct plugin update-handler reference provide a concrete public vendor path | Active branch/window/delivery is unknown; no call-order/panel/content guarantee follows |

[Qt6.10.3 documentation](https://doc.qt.io/qt-6.10/qquickwindow.html#frameSwapped)
describes frameSwapped as presentation queuing. Its
[afterRendering documentation](https://doc.qt.io/qt-6.10/qquickwindow.html#afterRendering)
places that stage before graphics-queue submission. Native adaptation placement
has not been qualified against those semantics. Treat either proposed observation
as public notification delivery only, not current pixels or physical refresh.

The original afterFrameEnd profile remains unsupported for a guaranteed native
delivery claim. Its observed missing marker does not justify another identical
attempt or relaxing context/request guards. The prior owned natural-signal
absence likewise does not show software backends never emit: the pinned upstream
loops explicitly do, and the fixture did not measure a completed new render cycle.

## Smallest distinct source candidate, not selection

For a later independently reviewed contract, frameSwapped is a better-grounded
public vendor-path candidate than assuming frame-end emission. It could use the
existing one queued GUI observation on the already witnessed window, with one
fixed declared signal profile and scalar literal. afterRendering is a separate
conditional common-Qt candidate, not an automatic fallback.

Do not detect/select a backend, query its private instance, add offsets/native
hooks, force rendering/input, extend time or listen to a sequence until one happens
to succeed. Do not quietly reinterpret the existing afterFrameEnd marker. Any
different public signal needs an exact SDK source/profile and Main consumer
literal/budget/whole-packet amendment before implementation or native binding.
Retain one request/grab, original deadline, context/cancel/ownership/publication
guards and no native/UI/page/owner authority. No new file quota is assumed here.

Source basis: retained application/vendor hashes, SDK author's read-only bounded
import/static-call inspection, official pinned Qt sources/docs and earlier
published backend findings. Private evidence receipt SHA256
`2f8333f44a70b868ad2140ca2c62665096df5b7ac15708caa351e9998c5c0b75`
records exact inputs/tools/output hashes; it has no public transcript link.
This is source interpretation/inference, not device execution, active-loop
qualification, root-cause proof or authorization for a fresh attempt. Full
content/current-owner/lifetime/recovery/Reader gates and prior insertion/ink/reopen
evidence remain unchanged.
