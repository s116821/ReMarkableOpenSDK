# Navigation through the linked legacy MainView

The [post-boot census](navigation-ownership-postboot-result.md) found distinct
legitimate navigation wrappers and one linked legacy document-view composition.
The optional `READER_SELECTED_LEGACY_COMPOSITION` startup flag narrows the
[existing-fixture diagnostic](fixture-navigation-source-join.md) to that
composition. It requires `READER_SELECTED_OPEN_FIXTURE_JOIN`; the earlier
global-receiver mode remains available and unchanged.

## Selector and operation

The same bounded eligible visual traversal considers a MainView only when one
guarded creation context, within sixteen parent contexts, resolves both `root`
and `mainView`. Its exact URL must be
`qrc:/qt/qml/xofm/modules/legacydevice/qml/LegacyMainWindow.qml` and the candidate
must be the resolved `mainView`. The selector does not require the root and main
view to have identical `qmlContext()` pointers or use generated class suffixes.

Both objects must be live visible/enabled items in the original engine/window.
The main view must be the root's direct visual child and its `contentItem`.
Their `windowNavigator` properties must agree on one guarded native
`xofm::bindings::qt::WindowNavigatorWrapper`; their `windowLayout` properties
must agree on one guarded native `xofm::bindings::qt::WindowLayout`. Its notified
`ready` value must be true and `activeWindow` must equal
`legacydevice/window/main`.

Borrowed objects, contexts and all relevant visual/property notifications are
guarded before dependent reads or permission callbacks. The search continues
after finding a match. A second qualifying main composition refuses even if
both share a receiver. Unrelated wrappers are ignored. Immediately before
dispatch, the complete scoped IDs, direct/content links, native objects and
layout values are reread. The operation still invokes only the exact public
two-argument slot, frozen existing-document UUID/page and fixed route.

Attempt consumption precedes invocation. The old receiver and discovery epoch
are not dereferenced afterward because the action may destroy or replace them.
One fresh observation is queued five seconds later through the event loop.
This is an opportunity for normal loading, not a readiness promise or polling
loop. Dispatch, queued delay, controller preparation, selected-page join and
unwind share the original twenty-second operation and ninety-second setup
limits. No retry, page creation or library unload is introduced.

The wire retains `navigate-creation-source-join` and adds
`navigation_scope: "legacy-main-composition"` only for this mode. Success still
requires a fresh observed selected page, the expected frozen document/index,
coherent mapping, readiness and the typed creation-Library same-wrapper join.
All native/render/durable authority flags remain false.

## Verification and limits

The added owned suite is `reader_legacy_navigation_test.sh`. It covers unrelated
wrappers, duplicate compositions with shared/distinct receivers, exact scoped
IDs/URL, bad content/parent/native links, wrong or unready layout, hidden
pruning, fresh final reads, destruction, ABA, reentry, cancellation and deadline.
The Entry suite runs both selector modes through admission, stop/expiry,
replacement, wrong-target, shared-budget and no-retry paths, including the real
five-second timer. It compiles all five startup variants. These are owned Qt
fixtures, without stock execution or native authority.

October 22, 2026 verification: all five suites passed with unchanged source
hashes on host Qt 6.4.2 (115.8 seconds) and ARM Qt 6.10.3/GCC 13.4.0 under
QEMU 8.2.7 (173.0 seconds). Independent source and fixture review found no
concrete blocker. Strict OpenSpec validation also passed. Full command/output
and before/after source hashes remain in private local receipts. No native
result is credited to these runs.

The selector relies on truthful public accessors/notifications and live pointer
returns. It cannot preempt blocking getters, establish backing-router
equivalence or detect unnotified context/ID/property ABA. The composition check
is not a native worker lease or a creation-time ordering guard. Native results,
fixture preservation and recovery must be recorded for each separate attempt.

Source basis: retained RM2 census and packaged-resource/native interface
findings, original interoperability code and independently reviewed owned
fixtures. No firmware source bodies are included. Source checks alone do not
establish selected identity, persistence, safe unload or a shutdown repair.
