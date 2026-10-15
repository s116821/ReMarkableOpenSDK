# Reader component containment and current-scene selection

Evidence checkpoint: October 13, 2026. The necessary application resources are
already retained. Their static wiring explains a focus-ancestry limitation and
supplies a more specific source-selection relationship; no new observer, type
admission or tablet operation is implemented or selected here.

## Exact retained resource evidence

Two existing private QML outputs, resource nodes 92 and 94, were inspected only
for reader-root, scene, focus and current-page relationships. Their SHA-256 values
are `867b30a725a4ae97e3459f07a60cd72bcafb21db9a0b374f02d75c5cd8d83269`
and `78df7fe68f0be6e0795cde67bdb5ce5c5f355c9100f0c14d6e8157c688271a0c`.
Existing extraction receipts bind the outputs to single bounded compressed
frames. Focused checks reconfirmed their compressed payload hashes at exactly
those two nodes of retained resource registration 64 in the saved xochitl image
`071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df`.
The registration index has SHA-256
`84e70940ce55c8aa648498fc075ed801c1d173f0b05fe50884cf9a4554423c3e`.
No resource-tree sweep, fresh extraction, QML execution or target read occurred.

Private reader-containment-source-20261013.json, SHA-256
`b45438b854dbf3b88abacee8c3f9cfec3e4f599ea1f9d8b5d1e5e3416a2afe80`,
records the checks and source conclusions. Private filenames/receipts have no
public download links; proprietary resource bodies are not reproduced here.

## Concrete containment and focus restriction

The reader DocumentView component has a FocusScope root and contains a
DeviceSceneView component. DeviceSceneView also has a FocusScope root; its native
SceneView instances are created below a descendant SceneViewsRoot item.
Consequently the native SceneView is below these reader scopes, not on either
scope's upward parentItem path.

This is visual parenting, not merely QObject ownership: pinned
[Component creation](https://github.com/qt/qtdeclarative/blob/v6.10.3/src/qml/qml/qqmlcomponent.cpp#L1602-L1620)
uses registered parent functions, and the
[Quick item parent function](https://github.com/qt/qtdeclarative/blob/v6.10.3/src/quick/items/qquickitemsmodule.cpp#L86-L118)
sets parentItem for an Item created under an Item.

DocumentView gates DeviceSceneView's focus on document.isNotepage(currentPage),
as well as dialog, popup, search and layer-tool conditions. The native view's own
focus binding selects its current instance, but does not remove that enclosing
scope restriction. When the notepage predicate is false, this documented reader
structure does not supply the active native SceneView ancestry assumed by the
passive focus method. Adding its Loader ancestor cannot make a descendant occur
on an upward path.

The new [Loader-wrapper finding](passive-parent-loader-finding.md) is consistent
with a loaded reader root, but the generic vptr does not prove which component
instance was sampled. Main's current pre-navigation metadata and images support
an original PDF-page context. A retained older metadata file corroborates the
same page's PDF redirection, while its six-page count/last-opened record differs
from Main's newer eight-page report and must remain historical. Neither metadata
alone nor the images provide a live reader currentPage pointer join or the
actual result of document.isNotepage. The source limitation above is therefore
conditional on that predicate, not an invented measured Boolean or proof that
every possible focus branch is unsuitable.

## Application-owned current-view reference

The resources define a stronger semantic path than generic keyboard focus:

| Relationship | Source-established meaning |
| --- | --- |
| DocumentView.document | Reader's selected document wrapper passed into DeviceSceneView |
| DocumentView.currentPage / currentPageId | Aliases of DeviceSceneView's selected page state |
| DeviceSceneView.sceneView | Explicit current native SceneView reference selected or created for the requested pageId |
| Native SceneView creation | Receives that document and requested pageId and is parented beneath SceneViewsRoot |
| nextSceneView / previousSceneView | Separate prefetched native views with neighboring IDs, hidden by this selection path |

The current reference is explicitly assigned by requested pageId. This matters
because several native SceneViews may coexist; an arbitrary child, first type
match, Loader identity or worker pointer cannot replace that selection rule.
Current-view visibility and state updates are staged, not a render fence or
atomic relationship. No inference about completed rendering follows.

## Grounded next discovery question

The useful source route is to identify the actual active reader component and
observe its declared current sceneView/document/currentPageId relationship,
then validate the native view and its viewport/manager/worker edges. The existing
[document relationship](scene-document-controller-relationship.md) supplies the
next join from native SceneView to wrapper/backing Document and entry ID, and
from a fresh DocumentController's Library lookup to the same backing Document.
Current page-key membership and source continuity still need explicit evidence.
The library controller alone is not a displayed-document selector.

What remains missing is a reviewed, exact read/selection mechanism for that
runtime reader root and application-owned current reference. Static QML IDs,
aliases and pointer meaning do not supply native offsets, actual values or a
lifetime lease. Do not invoke their getters under a passive-read claim, assume
the captured FocusScope is DocumentView, choose a prefetched view, or substitute
a last-opened file field for current UI ownership. Any different discovery/read
definition needs its own scoped precode and review; this finding adds none.

Do not add a sixteenth Loader admission simply to continue the existing upward
walk. The exact next investigation is the grounded reader/current-reference
access gap, preserving old source-pointer limits and refusing unclear ownership.
No guard relaxation, new provider/reader, whole-binary type scan or blind tablet
repeat is justified. Earlier [native insertion, ink and reopen milestones](../experiments/caller-selected-insertion-result.md)
remain established; source association, lifetime/render and remote cleanup gaps
are separate. Held 8112/equivalent and spent 0404 exclusions remain unchanged.

Source basis: exact retained application resources and extraction proofs checked
locally, their declarative bindings, existing SDK native relationship findings,
and explicitly attributed Main metadata/image evidence. No runtime component
identity, live predicate value or native/source qualification is claimed.
