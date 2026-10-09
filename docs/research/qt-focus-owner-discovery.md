# Qt focus ancestry and bounded owner discovery

Evidence checkpoint: October 8, 2026. Source interpretation, not native owner or
render qualification. The proposed application is owned by the
[focus ancestry change](../../openspec/changes/establish-native-platform-contract/focus-ancestry-discovery.md).

## Pinned upstream evidence

In Qt v6.10.3,
[QQuickWindow::activeFocusItem](https://github.com/qt/qtdeclarative/blob/v6.10.3/src/quick/items/qquickwindow.cpp#L1293-L1306)
returns the window delivery agent's selected item; contentItem is the window's
retained scene root. These are observations, not lifetime leases.

[QQuickItem::hasActiveFocus](https://github.com/qt/qtdeclarative/blob/v6.10.3/src/quick/items/qquickitem.cpp#L7268-L7304)
reads the active-focus flag. Its documented meaning includes the selected item
and ancestor FocusScopes. Thus several nested scopes may simultaneously have
active focus; the nearest scope is not necessarily the unique document receiver.

[QQuickDeliveryAgentPrivate::setFocusInScope](https://github.com/qt/qtdeclarative/blob/v6.10.3/src/quick/util/qquickdeliveryagent.cpp#L367-L490)
follows enabled scoped-focus descendants, clears the previous active path, then
marks the selected item and ancestor FocusScopes. It subsequently delivers events
and notifications that can change focus again. Subscene agents also delegate to
the window agent; this is not proof that arbitrary subscene roots have a complete
parentItem path to the window content root.

In that pinned source, focus events precede the window notification.
[notifyFocusChangesRecur](https://github.com/qt/qtdeclarative/blob/v6.10.3/src/quick/util/qquickdeliveryagent.cpp#L591-L610)
compares notified and final flags before signaling. Reentrant event handlers can
restore the original anchor before outer notification; endpoint signals alone
therefore do not prove continuity. The proposal requires conservative application
event-filter invalidation before receiver FocusIn/FocusOut handlers, plus explicit
same-runtime tests. Filter ordering/suppression and unsupported subscene paths
remain qualification limits, not an assumed universal observation surface.

## Conditional completeness argument

For a stable ordinary window focus hierarchy, a scene satisfying the SDK's
existing hasActiveFocus predicate lies on the selected item's ancestor chain.
The existing owner predicate additionally requires the receiver to be a strict
ancestor of that scene. Therefore a complete retained leaf-to-content-root chain
contains every pair eligible under those predicates in that hierarchy. All pairs
must still be checked; nested valid receivers can produce ambiguity.

This is a conditional deduction from upstream semantics and SDK predicates, not
a statement that the vendor runtime or a sampled native tree satisfies those
premises. Focus transitions/reentrancy, detached or embedded subscene roots,
vendor changes, focus leaf depth, pooled views and overlays require explicit
qualification or conservative refusal. Keyboard focus is not page/render/UI
acknowledgment. Endpoint equality cannot detect an unobserved change-away-and-back.

## Why cumulative tree scanning refused

The independently hash-checked 9453 version2 receipt (854 bytes,
SHA256 `199e39bfbe6acbe3473fe20990c07e003003bbd5d5cb1a8b1f3b2e4db99d99fb`)
reported visited3656, cumulative queue4096, depth19 and child_count3. The append
guard refused three children against zero remaining capacity, leaving440 queued
entries unvisited. Pairing was not initialized. One receiver and two scene
candidates were structural partial counts, not unique/absent owners. Private raw
receipts have no public link. Earlier e997 version1 evidence remains ambiguous;
similar counts do not identify its return site retrospectively.

Changing discovery to focus ancestry changes its completeness domain. It is not
an optimization justified merely because a wider search ran out of budget.
Selecting the first partial candidate, pruning arbitrary invisible branches,
increasing4096, or restarting a second search cannot inherit the old acceptance.
Use an explicit reviewed contract with unchanged refusal/recovery boundaries.
