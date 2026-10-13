# Reader anchors and public current-reference discovery

Evidence checkpoint: October 13, 2026. This extends the
[reader containment finding](reader-scene-selection-wiring.md) into a concrete
selection design. It is source assessment, not an implemented native adapter or
authorization to call getters through the passive memory reader.

## Usable anchors and properties

The two exact retained application resources expose these relevant anchors:

| Source anchor | Owning component and usable link |
| --- | --- |
| DocumentView.ToolSettings | Named DocumentToolSettings child of DocumentView; its document binding agrees with the reader root's document |
| SceneViewsRoot | Named Item child of DeviceSceneView; its owner exposes the explicit current sceneView, document, pageId and controller |
| DocumentView root | Exposes document, currentPageId and sceneController; currentPageId aliases the device component's selected page state |

The QML id root is lexical and is not an objectName selector. The separate
objectName scene-view belongs to an Epaper.ScreenModeItem, not the native
SceneView. A generated QML class suffix or this screen-mode name must not be
used as a renderer identity. These facts come from the same hash-checked nodes
92/94, without another resource or type enumeration.

## Smallest public-Qt selection mechanism

For an eventual adapter executing in the application's GUI thread, start from
the already justified current QQuickWindow/contentItem or witnessed application
engine root. Locate the two exact named anchors within that owned UI domain and
retain guarded owner references. Names are firmware/application-profile details,
not public SDK identifiers. Require a unique eligible reader pair; absence,
duplicates, inactive/hidden/detached owners or mismatched parent/window relations
refuse. Do not assume a single window, select the first match or scan arbitrary
heap objects.

Public [QObject child/name and property APIs](https://doc.qt.io/qt-6.10/qobject.html)
and [QMetaObject property lookup](https://doc.qt.io/qt-6.10/qmetaobject.html#indexOfProperty)
provide the symbolic access surface. findChild can select the first among
duplicates; it is unsuitable for uniqueness evidence. An unbounded recursive
findChildren call also cannot inherit the SDK's traversal/time guarantees.
Use an explicitly bounded QObject/visual-item walk or an already retained owner
reference, with complete-domain/ambiguity refusal. Source names do not prove
the live search domain is small enough. No particular new cap is selected here.

The direct owner of SceneViewsRoot is DeviceSceneView. Read its declared
sceneView reference through a readable pointer-to-QObject meta-property. Read
document and pageId from that same owner. Cross-check the DocumentView owner's
document/currentPageId/sceneController against the device owner and selected
native view/controller, including the source-established visual-parent links.
Reject invalid variants, nulls, incompatible types or inconsistent references.
Never substitute nextSceneView/previousSceneView or choose a visible child as a
replacement for the explicit current reference.

QQmlProperty/QMetaProperty reads can handle QML properties and aliases using
Qt's own machinery; they are real target-side operations and may execute getters
or binding code. They require a reviewed GUI-thread entry, lifetime/change guards
and bounded failure handling. Static property names do not establish safe reads.
The existing SDK observeProperty helper checks metadata only and explicitly
does not read native values; its availability is not a qualified reader path.

## Maintainable API versus temporary diagnostic

| Route | Concrete value and remaining prerequisite |
| --- | --- |
| Public-Qt GUI-thread adapter | Follow named owners and explicit meta-properties without exposing raw offsets. Needs an independently permitted entry, exact profile/ownership guards, bounded discovery and qualified recovery; no transfer/activation is selected. |
| Existing passive SFTP diagnostic | Keeps current permitted access and obtains no getter execution. Current fields cover vptrs/backlinks/visual links, not QObject objectName or QML alias/current-reference storage. A precise new read contract and source-layout proof would be required; none is supplied by the name declarations. |

Prefer the public-Qt semantics for an eventual capability-oriented API returning
opaque document/page observations. Cache only guarded owner references with
invalidation; no public QObject pointer, object name or guessed address becomes
the SDK contract. Unknown application profiles remain unsupported. This does not
select a loader, attach mechanism, production XOVI dependency or held activation.

For the temporary passive lane, adding Loader admission cannot locate the
current-reference property and is not the next step. The exact source gap is
how to recognize a reader owner and read its declared current sceneView reference
without calling Qt, including QObject-name and QML-property/alias representation
if that route is pursued. Do not invent QV4/private offsets or add a provider from
QML names. First assess those specific access representations, then freeze and
independently review any genuinely necessary read-definition/provider/budget
delta before Main selects a diagnostic. No broader heap/window-tree sweep or
target operation follows from this assessment.

## Required current-source closure

After explicit current-view selection, the
[native document relationship](scene-document-controller-relationship.md)
supplies meaning for the view's document wrapper/backing Document and fresh
library ID resolution. The selected page key must belong to that same document
and agree with the reader/current controller. Preserve before/after ownership,
reference/page consistency and change invalidation; neither property reads nor
endpoint equality create atomicity, a lifetime lease or a render fence.

The actual sampled Loader instance and live isNotepage result remain unknown.
Earlier insertion, ink and reopen milestones remain established; this discovery
design does not close or invalidate those separate results. Held
8112/equivalent/spent 0404 exclusions and Main's sole operator ownership remain.

Source basis: exact retained resources, reviewed containment/native relationship
research, existing SDK metadata-only helper and pinned public Qt documentation.
The design is an inference from those concrete surfaces; no application getter
execution, private-layout traversal or device test supports it. The subsequent
[owned fixture assessment](../../openspec/changes/establish-native-platform-contract/reader-reference-owned-fixture.md)
passed real QML alias/current-reference selection and focused refusal cases
under the existing vendor ARM Qt emulator. That local result supports the public
mechanism on the explicitly owned layout only; application discovery and native
qualification remain open.
