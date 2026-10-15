# Owned public-Qt reader-reference assessment

Status: precode implemented in one SDK-owned local Qt fixture, October 13, 2026. It assesses
the public mechanism in [reader discovery](../../../docs/research/reader-current-reference-discovery.md).
It does not load code into xochitl, change the passive observer, select a tablet
operation or replace any held activation. Existing insertion/ink/reopen evidence
and all held 8112/equivalent/spent 0404 exclusions remain.

## Intended source increment

Build a small development-only selector in tools/ and one owned Qt/QML fixture.
The selector accepts an owned QQuickWindow with a retained content root and runs
in its GUI thread. This fixture caller supplies that window; current native
window/engine discovery is not proved by the test. It returns a guarded current
reference and logical document/page values for owned objects only, with false
native/source/render authority. No Rust API, live profile or consumer pin changes.

Use original QML authored for the fixture, not proprietary application bodies.
It declares a reader root with document/currentPageId/controller properties, a
named DocumentView.ToolSettings helper, and a device owner with sceneView,
document/pageId/controller references and named SceneViewsRoot Item. Several
same-type view children exist, including separately retained neighbors. The
current reference is deliberately not the first child. Include a real QML alias
for the page-key observation to assess public alias reads.

## Small bounded selector

Inspect only the owned window content root's QObject hierarchy using public
children/name APIs, with maximum 128 visited objects, depth 12 and 64 children
per object; stop/refuse rather than keep a partial first result. Collect only
the two exact source-established names. Require one of each and consistent owner
relations: helper QObject parent is reader root, scene-container visual parent
is device root, and device root's visual parent is reader root. The reader,
device and container visual items and current view must belong to the supplied window; reader/device/current view
must be effectively visible/enabled. Names are fixture/profile details.

Use QMetaObject/QMetaProperty lookup and read only the declared document,
currentPageId/pageId, sceneController/controller and sceneView references.
Require readable properties, correct QObject-pointer/string domains, nonnull
objects and the exact ownership/document/page/controller equalities implied by
the application resources. The fixture's QString page domain is synthetic and
does not prove the application's native page-key representation. Require the
current native-style view's visual parent to be SceneViewsRoot.
Do not enumerate renderer children to pick a page and do
not use keyboard focus to select it. Reject missing/duplicate anchors, invalid
variants, wrong types, null or inconsistent references.

Use QPointers for the window, root/owners/anchors and returned object references.
Bracket public list/name/property operations with the fixture's monotonic
one-second cooperative deadline and a cancellation guard; do not renew it.
Native Qt calls are not preemptible, so this is not a hard runtime guarantee.
Revalidate the governing reference/value/parent/window relationships before
returning an owned result. This endpoint check does not exclude ABA/reentrancy,
create a lifetime lease or qualify application change-notification ordering.
No mutation, focus assignment, worker acquisition or arbitrary getter discovery
is part of selector execution. Fixture setup is independently owned.

## Focused local proof only

One fixture executable should verify the following bounded cases:

1. Same-type current/neighbor objects are resolved by the explicit current
   reference, with a real alias page key and keyboard focus elsewhere.
2. A duplicate named anchor refuses rather than choosing the first.
3. Mismatched page/controller/document references and a missing current reference
   refuse, preserving no-authority output.
4. A concrete hierarchy cap and cancellation/deadline refuse without returning
   partial references.

Do not repeat native capture/insertion, vendor ABI/type, passive observer, job or
large QML matrices. Build/run through the existing vendor SDK/emulator/offscreen
workflow; no dependency installation or tablet transfer is needed. If its QML
modules/backend cannot execute this fixture, retain a concrete build/runtime
limitation instead of substituting mocked property reads for alias evidence.

Existing Sol reviews this exact small precode and resulting source/assertions.
Successful owned results only show that these public APIs can implement the
selection semantics on this controlled layout. A shippable adapter still needs
an independently permitted entry, exact native profile/root discovery, GUI/life/
change guards and integrated recovery. This does not authorize a new in-process
diagnostic or private QQmlVMEMetaObject reader in the current SFTP lane.

## Local result

`tools/reader_current_reference_test.sh` builds the original fixture with
warnings treated as errors and runs it using the existing ARM SDK 5.8.203,
vendor Qt 6.10.3, qemu and offscreen/software backend. The focused run passed:
real alias propagation, non-first explicit current reference, duplicate anchor,
inconsistent/null references, concrete child cap, cancellation and cooperative
deadline refusal. Every refusal retains no partial references; successful
results retain `nativeAuthority=false`.

The fixture explicitly establishes both QObject and visual parenting under its
owned window. An initial setup with only visual parenting correctly refused
because the QObject census could not reach the anchors. This emphasizes the
remaining application-profile obligation: neither containment nor a current
window witness follows solely from visual ancestry. No live application getter,
tablet operation, render fence or lifetime qualification was exercised.
