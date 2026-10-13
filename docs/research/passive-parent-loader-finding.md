# Exact Loader-wrapper candidate and ancestry limit

Evidence checkpoint: October 13, 2026. The new private locator identifies one
exact static vtable candidate in the pinned QtQuick image. It does not validate
the live parent or justify an automatic type-admission increment.

## Verified retained evidence

Main's selected-parent-provider-r2 observation, 939 bytes, SHA-256
`bc21ddd40c2e2235271b990d468c4686e452a529def85c99702d25999656fe84`,
reports focus-parent / visual-chain, a read vptr, zero fifteen-table admissions,
zero four-candidate hints, QtQuick provider and recorded private locator. OPEN
and 34 READs completed; no CLOSE or visual association was completed and remote
cleanup remains unknown. Main's execution/host timing and process checks remain
operator-attributed. No device operation occurred in this source investigation.

The 548-byte private artifact SHA-256
`f6aeb3b278d0fa745c2f609c997d63d9c17a494cd36ba16ae4297dbcccfb9b05`
matches the public artifact digest. Its original 100-byte prefix is unchanged,
SHA-256 `f6f9f81ecb215ba6842c1d2a3fa89bc9dcf690865702c4a685276d2b0ffce8de`,
matching the private pre-append record. The 420-byte locator record fits the
reviewed private/nested caps and binds the current manifest and QtQuick hash.
These proofs were checked locally; addresses and the locator remain private.

Before/after screenshots show the foreground disposable document canvas and
toolbar, with Page 6 of 8 visible in the pre-image. They are not pixel-identical
or proof of a constant UI interval, and do not join the sampled parent object to
the pictured document. This new observation does not identify older refused
parents retrospectively.

## One-position source finding

At the single recorded image-relative position in QtQuick SHA-256
`b20f9adaefe8cebeca445906e891370f75f0b0af878ac1b35e0c91159778628f`,
the saved primary vtable has signed offset-to-top zero and an ARM RELATIVE
typeinfo reference. The referenced single-inheritance RTTI names
`QQmlPrivate::QQmlElement<QQuickLoader>` and its defined base QQuickLoader.
The binary was read as data; no proprietary body or raw disassembly is published.

Private source receipt parent-exact-position-source-20261013.json, SHA-256
`52837a516f49fab78d922c52c203848ea33bfab921f1ffcf4e4c63c33ae428c8`,
and its one-position script retain the exact assertions. They have no public
download links. This gives a concrete named static pair matching the new
recorded word. The live header, backlink, private layout, object lifetime and
post-read continuity were not checked at this refused parent.

## Does this path establish SceneView ancestry?

Pinned [Qt 6.10.3 Loader source](https://github.com/qt/qtdeclarative/blob/v6.10.3/src/quick/items/qquickloader.cpp#L575-L593)
sets a loaded visual item's parentItem to its Loader. The
[Loader documentation](https://doc.qt.io/qt-6.10/qml-qtquick-loader.html#focus-and-key-events)
describes Loader as a focus scope. A FocusScope followed by this wrapper is
therefore consistent with a loaded component root; it is not evidence of which
application component or Loader instance was sampled.

The required SceneView is a distinct application type. This Loader node itself
cannot satisfy that role. If the displayed SceneView lies inside the loaded
component below the current FocusScope, walking parentItem upward cannot reach
it. Conversely, Qt permits a Loader beneath a SceneView, so the generic type
alone does not prove that the path is incapable of reaching one. Neither a
root/toolbar branch nor a productive document branch is established here.

The existing [SceneView/document relationship](scene-document-controller-relationship.md)
explains how a selected view reaches its backing document and worker. It does
not identify this Loader's loaded component or visual parent, or place the
current SceneView on this sampled ancestor chain. The focus-ancestry completeness
argument remains conditional on that application's scene satisfying its focus
predicates; opening the document does not supply that missing premise.

## Smallest justified follow-up

Do not automatically add the Loader wrapper and repeat the walk merely because
its static identity is now known. First establish the application-specific
containment relationship between the reader component's Loader/loaded root and
SceneView from retained application source or another exact existing source
lead. If SceneView is a descendant/sibling of the keyboard-focus branch, this
ancestry route cannot supply the intended association and needs a separately
justified selection mechanism. A source-only binary/QML relationship gap should
be routed to Root's designated analysis lane with this concrete question, rather
than broad type enumeration or another blind tablet cycle.

Subsequent [retained reader-resource inspection](reader-scene-selection-wiring.md)
establishes the application containment and focus restriction, plus an explicit
current-view reference. It does not identify the sampled live component instance.

No new admission manifest, Loader field read, provider, guard relaxation,
observer implementation or target operation follows from this finding. All
current limits/refusals and held 8112/equivalent/spent 0404 exclusions remain.
Earlier [native insertion, ink and reopen milestones](../experiments/caller-selected-insertion-result.md)
remain established historical evidence; this parent-chain gap does not erase them.

Source basis: retained actual observation/artifact independently checked locally,
one exact saved-image RTTI position, pinned primary Qt source/documentation and
existing SDK relationship research. Execution belongs to Main; no live class,
source association, rendering or lifetime qualification is claimed.
