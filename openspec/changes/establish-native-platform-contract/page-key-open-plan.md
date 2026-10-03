# Explicit PageKey development open/observe slice

Status: Main-selected source preparation, October 3. No native opening API is
qualified. The successful explicit insertion fixture now contains six pages;
its expected-five creation configuration and spent packet remain historical.
Task2.7 is achieved at that narrow scope; tasks2.6/2.3/4.2 remain open.

## Scope and existing seams

Reuse the accepted existing-engine public Qt access lifecycle, owned QML helper,
GUI-thread weak generation checks, bounded completion and Main-owned recovery.
Add a separate off-by-default development open configuration carrying explicit
document UUID, source page UUID, target page UUID and the six-page baseline.
It never enables creation, draws ink, directly rewrites persisted lastOpenedPage or
manufactures an SDK PageObservation. Rust Platform navigation stays unsupported.
Buddy's sticky RequestGuard, ordered navigation completion and guarded capture
are downstream seams; its current Reader next-page classification/render flow
is unchanged. Host lifecycle and dummy-manager models are not native executors
and do not require a new generic protocol or registry for this slice.

## Finite owner qualification

Saved native source describes a document FocusScope with document/currentPage/
currentPageId/drawingAreaFocused and callable openPage, containing a SceneView
whose document is the same QObject and pageId matches the root alias. QML lexical
ids are not objectName guarantees. Before implementing discovery, directly check
the saved source/metadata for each required property, signal and mapping method.
Stop and report an unavailable boundary if any relation cannot be established.

Within one selected current QQuickWindow and existing QQmlEngine, bound the item
tree traversal and require exactly one compatible receiver/SceneView pair.
Require actual parentItem ancestry, identical document QObject, same window and
engine, effective visible/enabled state and an active-focus relationship agreeing
with drawingAreaFocused. Reject zero/multiple, hidden/background, inconsistent,
foreign-thread or destroyed candidates. These checks are necessary evidence for
Main's explicitly opened development fixture with no concurrent interaction;
they do not prove general overlay/input isolation or product source authority.

Read native document identity and SceneView.pageId; crosscheck root.currentPageId,
document.idForPage(root.currentPage), caller source PageKey and complete baseline
order with document.pageForId/document.idForPage reverse index/key round trips. Derive target index fresh from its UUID.
No metadata-only uniqueness or lastOpenedPage substitutes for the live owner.

## One open and observed completion

Install only this receiver/SceneView document/page/lifetime observations before
dispatch. Retain weak engine/window/receiver/scene/document references and a
transaction generation. Queue work off native signal stacks; revalidate source,
mapping, context and cancellation immediately before one call. The owned QML
helper calls receiver.openPage(derivedIndex), omitting the optional position as
the saved native caller does. Do not guess a C++ one-QVariant invocation of a
two-parameter QML JS method. Native openPage housekeeping may update last-opened
state; this is not a page/content insertion or a direct metadata write. A verified
already-open target is a no-op.

Notifications queue fresh observation of the same owner. Completion requires
the native document ID, scene pageId, root alias and both mapping directions to
match the exact target with unchanged expected order. A signal, JS return or
elapsed timer is not completion. Missing target match becomes unresolved at a
finite deadline. Destruction, cancellation, owner/order change or a proven relevant input condition or duplicate
dispatch cannot authorize a second call. No menus, fixed sleeps or creation retry.
Fresh capture/render authority is a later qualified consumer step; this slice
cannot permit rendering merely because navigation evidence was reported.

## Source and test gates before a Main-only native candidate

Owned synthetic fixtures must cover missing/ambiguous/hidden/foreign owners,
alias/document/mapping mismatch, stale source/order, index remap, duplicate
notifications, receiver/document/window/engine destruction, cancellation before
dispatch and after dispatch, target-already-open, unresolved timeout and no
rendering authority. Verify the exact QML callable representation with owned
fixtures. Freeze one tested delta for Astra semantic review and Main independent
validation; exact source/artifact/operator/six-page baseline gates precede any
separately selected native execution. Preserve M3 stash and original backup.

Source basis: current SDK/Buddy interfaces; Astra's saved native qrc92/SceneView
assessment and Main's selected source-only scope, recorded in the current chats
(no public source body); [scoped insertion/persistence](https://linear.app/magentumdragon/issue/REM-25#comment-7d695be1-1baa-4611-8648-26876db57a6e)
and [independent preservation/Main visual development reopen](https://linear.app/magentumdragon/issue/REM-25#comment-f498c923-e7a1-41c4-ad3b-396891c353c9).
Owner qualification and the next operation remain proposals until implemented,
independently reviewed and separately qualified on the explicit fixture.
