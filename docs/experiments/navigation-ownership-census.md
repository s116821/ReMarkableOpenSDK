# Read-only stock navigator ownership census

The first [navigation/join attempt](fixture-navigation-source-join-result.md)
found different eligible navigator wrappers and stopped before dispatch.
Retained stock resources explain that each BaseView is a QML Control with
injected navigator/layout properties. BaseView is not a native class. The next
separate diagnostic observes ownership instead of calling navigation.

`reader_navigation_ownership.h` inspects the current focused window's bounded
eligible visual tree. It reports up to eight owners with declared QObject-valued
`windowNavigator` properties, presentation-only class/context URL text, and
intra-census navigator/layout identity groups. Missing/null/wrong/foreign layout
states remain observations. Exact native `xofm::bindings::qt::WindowLayout`
objects may supply their notified bool `ready` and QString `activeWindow`.
The current-window string is reported without interpreting its format.

The diagnostic separately reports lexical `root`/`mainView` bindings, visual
parent and content-item links, and same-object navigator/layout equality on
that pair. A composition match requires the linked pair from the same context;
names seen in unrelated contexts do not establish it. These facts are input
to a future composition-based selector, not action authority or proof of a
selected document/page. Distinct wrapper groups do not prove distinct backing
routers; equal groups do not prove routing or worker serialization.
Context reassociation and ID-table bindings have no complete notification
contract here. Final equality checks detect persistent changes, not every
possible silent ABA; this remains sampled metadata, not an atomic action lease.

The census watches lifetimes, relevant notifications, structural changes,
focus/window state, engine/thread and excluded hidden/disabled boundaries.
Traversal remains bounded by 512 items, depth 32, 64 direct children and 640
retained objects. `visited` includes pruned boundaries and is not a count of
all objects in the application. A ninth owner or another exhausted bound
returns incomplete/refusal; a retained prefix must not be called a complete
inventory. Truncated text is explicitly marked and cannot qualify a source
path by prefix. Context ID bindings are checked again before completion.

The separate `READER_SELECTED_NAVIGATION_OWNERSHIP` startup mode consumes one
exact request. Its `navigation-ownership` wire includes `census_complete`,
`visited` and `owners`; ordinary observation wire modes remain separate.
This mode has a sixteen-KiB result cap, including an explicit small refusal
if encoding exceeds it; other modes retain two KiB. The census shares the
original twenty-second cooperative operation limit and ninety-second setup
deadline. Cancel/restore/expiry during getters defers terminal publication
until unwind. Uncertain publication is preserved and never retried.

This mode calls no navigator slot, `contains`, creation controller, document
lookup, QML helper or ownership setter. Selected IDs, readiness and all authority
flags remain unobserved/false. Public getters still execute application code;
the budget cannot preempt a blocking native call. Independent stock restoration
and complete before/after fixture preservation remain separate checks.

Source basis: retained exact RM2 ELF metadata, extracted stock BaseView,
LegacyMainWindow/MainView resources and original interoperability source.
The native layout class has read-only notified `ready`/`activeWindow`, signals
`readyChanged(bool)`/`activeWindowChanged(QString)`, metaobject `0x1338500` and
metacall `0xa388c8`. Live census findings must be recorded separately; static
layout metadata does not establish a currently eligible composition.

All four owned suites pass on host Qt6.4.2 and ARM Qt6.10.3/QEMU, including
standalone ownership/dispatch/selected-observer checks and the request entry.
All four startup modes compile. Focused fixtures cover pre-visit parent and
visibility ABA changes, distinct legitimate wrappers, linked/mismatched
compositions, incomplete bounds, exact admission/no early read, nested stops,
reentry/expiry, greater-than-two-KiB census publication, explicit overflow
refusal and late-expiry data clearing. These are owned-interface tests, not
native receiver ownership findings.
