# Existing-fixture navigation and source-join diagnostic

This separate development mode opens a frozen existing document/page through
the application's own window navigator, then obtains one fresh selected-page
and creation-source observation. It is not the SDK's qualified navigation API.
Native results, when available, must be recorded separately from these source
and owned-fixture checks.

## Mechanism

`reader_fixture_navigation.h` discovers declared readable QObject-valued
`windowNavigator` properties on eligible items in the current focused window.
The bounded visual census includes hidden/disabled boundary notifications before
pruning. All eligible property owners must refer to the same guarded live
`xofm::bindings::qt::WindowNavigatorWrapper`; conflicting receivers refuse.
No generated QML owner class name is guessed. Lifetimes, thread/engine, focus,
visibility, structural/property changes and direct ABA changes are checked
before dispatch.

The selected public native slot is exactly `void open(QString,QVariantMap)`
declared by that wrapper. Its one-argument default-argument clone is allowed in
metadata but never selected. The fixed route is `legacydevice/window/main`,
with exactly `documentId: QString` and zero-based `page: int`. The startup
artifact freezes a canonical nonnil document UUID and a page index in the
research range 0..9999. There is no arbitrary route, creation call or retry.

Navigation can replace the UI or delete its receiver. Dispatch is consumed
before invocation; the old receiver and discovery epoch are not inspected
afterward. A true dispatch result establishes only that Qt invoked the slot.
The application still controls loading, error/modal handling and readiness.

The Entry queues one fresh observation two seconds later through its normal
event loop, without event pumping, readiness polling or reopening. It reacquires
the focused window and requires the original live engine. Navigation, queued
delay, controller preparation, observation and unwind share the original
twenty-second cooperative budget and ninety-second setup window. Pre-action
permission callbacks check the absolute operation deadline. Blocking native
calls remain nonpreemptible. Cancel/restore/expiry while on a call stack defers
publication until unwind; disarm makes queued callbacks inert.

The returned `navigate-creation-source-join` wire kind separates
`navigation_attempted`, `navigation_dispatched`, `creation_join_observed` and
`observed`. Accepted identity must match the frozen document and page index;
wrong-target or deadline results clear identity/readiness. Native, render and
durable authority remain false. Uncertain publication is preserved and never
retried.

## Source basis and limits

The retained target is RM2 firmware 3.28.0.172, executable SHA-256
`071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df`.
Its stock QML uses this existing-document route/map, forwards the opened args
through the legacy window to MainView, and uses page four for human page five.
Native metadata declares the full slot and its clone. Private binary/QML bodies
remain outside the repository. REA native forwarding/registration findings are
a separate evidence item; source/fixture checks cannot substitute for a live
target result.

REA 6.7.0 / Ghidra 12.1.4 evidence
`ev_a0344e931816fa5fa1422e9a00e96d63a6d917f00c1ac5bed8647368a65f83fb`
confirms the inspected wrapper at `0xa5a908` parses the route, copies the map
into QVariant-backed captured storage and calls the underlying navigator's
virtual slot at offset eight before returning. The concrete downstream target
and scheduler remain unresolved: inline wrapper forwarding is not synchronous
UI completion. The `0xcf1708` registration helper retains the `windowadder.cpp`
`addWindow` diagnostic identity; the setup slice pairs literal `main` with
`qml/LegacyMainWindow.qml` through that helper and `0xcdede4`'s QML factory
binding. The route interpretation combines these native findings with the
retained stock QML callsites; it is not a recovered public SDK contract.

The owned temporary analysis session exported its evidence and closed. Unrelated
OpenCode sessions were untouched. No global MCP/client configuration changed.

Normal opening may update visit/last-open metadata. Preserve complete before
and after backups and report actual changes; require immutable PDF/page/ink
preservation without writing saved bytes into a running document cache.

Owned Qt checks are `reader_fixture_navigation_test.sh`,
`reader_selected_view_test.sh` and `reader_selected_view_entry_test.sh`.
They cover typed dispatch/clone selection, receiver agreement/ambiguity,
hidden pruning/ABA, cancellation/destruction/reentry, exact admission,
queued and in-call stop/expiry, original shared budget, target mismatch,
uncertain publication and no retry. They also compile all three startup modes;
they do not inject into a stock process or qualify native navigation.
