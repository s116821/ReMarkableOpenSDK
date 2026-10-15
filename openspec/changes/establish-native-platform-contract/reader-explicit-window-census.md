# Explicit retained-window structural census: proposed first increment

This narrow source-only proposal follows the accepted
[current-document assessment](../../../docs/research/current-document-after-loading-capture.md).
It is a source/control checkpoint, not a native observation capability or selected
experiment. Existing capture producer679d and Main runtime remain unchanged.

## Inputs and refusal contract

Add a distinct helper to `tools/reader_metadata_census.h`, conceptually
`censusWindow(retainedWindow, retainedContentRoot, permit)`, using explicitly
guarded QPointer<QQuickWindow>/QPointer<QQuickItem> inputs and caller permit.
Keep existing `census(permit)` API and behavior. The new helper never discovers a
replacement window/root or falls back to the focused window when inputs refuse.

Require nonnull retained objects, application/GUI-thread agreement, the same
current visible active focused window, its exact contentItem root, an existing
unique associated GUI-thread engine and unchanged caller permit. Recheck those
conditions around discovery/metadata operations. Deletion, root/window/focus/
engine/thread/permit change refuses without favorable re-selection or rearming.

Reuse the existing bounded metadata-only traversal, uniqueness/owner/window/
visibility guards and declared-property/type checks. Preserve existing node/
depth/edge/window/property/local cooperative time bounds; caller permit must
retain the original operation deadline, not renew it on each call. No application
property VALUE reads, getter invocation, dynamic property enumeration, private
offset/provider/backend hook or renderer/input forcing is introduced.

Return the existing Result stages/counts/booleans only. False defaults after an
early refusal remain unreached, not measured negative properties. The result is
structural correspondence only, never document/page identity, native authority,
render freshness, lifetime lease or capture admission.

## Exact scope and remaining work

FIRST increment has no startup/producer endpoint, mode, scalar/schema/output
file/quota, Main consumer, native profile or capture integration. Future native
integration needs a separate reviewed contract joining this exact window/root
before and after the same single grab under original clocks/caps, and declaring
any output/schema/quota/consumer change explicitly. Do not infer that integration
from an owned helper result or old2ef/d1 evidence.

The [independent precode review](https://github.com/s116821/ReMarkableOpenSDK/pull/1#pullrequestreview-5512213774)
accepted this exact contract with no findings before continuation. Only new meaningful owned
controls are planned: supplied-window selection without fallback, exact root
mismatch/change/deletion, context/thread/permit loss, traversal-cap refusal and
zero application VALUE-getter calls. Owned fixture setup is synthetic evidence;
no unchanged matrix or native operation is requested. Changed-source/control
review remains separate from that accepted design.

Source basis: accepted assessment/publication4dcc82ea, existing metadata source,
and Main's current-chat exact helper contract. Main remains sole device operator.
No new nonce/artifact/transfer/opening/menu/input/replay or broader native gate
follows. Preserve the account's1% reserve and existing operation holds.

## October15 implementation and owned controls

The preserved8190-byte uncompiled header at the reserve checkpoint was verified
byte-for-byte against its handoff SHA2566e1f9dd047031d3c8ed803f816460464aa33060413c2a53bfcb8bb1f2e34c952.
The new `censusWindow` takes QPointer window/root and caller permit. A shared
internal body keeps the legacy `census(permit)` selection and traversal behavior;
explicit mode never selects a replacement. Its retained engine/root/window/focus/
visibility/thread checks run before and after permit. This matters because a
permit callback can reenter Qt and change context even on the final check.
An empty explicit-mode permit refuses; no application property VALUE is read.

The new owned ARM fixture compiled strictly with existing vendorSDK5.8.203 in
the existing416c7 image, network disabled, source mounted read-only. Only the
new fixture executed under qemu/offscreen/software. Two actual owned QML windows
have distinct node counts; explicit/legacy positive results match in both domains.
The supplied inactive window never falls back to the other focused window.
Mismatched/null roots/windows, empty or revoked caller permit/deadline, wrong
calling thread, duplicate anchors, existing child-cap refusal, final permit
reentry hiding the window, and pre-call/in-callback window/root deletion refuse.
Application-property traps report zero VALUE-getter calls across all controls.
Compiler and control stderr are empty. No previous matrix, startup/native payload,
producer, capture, Main consumer, endpoint/schema/quota or device operation ran.

The first source/control package
`0cce6a8d8741dbb7f5296f30c98d49c6743521ffc0e24f4c72af53352776b835`
remains preserved. Author inspection then moved the initial retained-engine
lookup inside the local timer and first caller-permit interval; that affected
fixture PASS is retained in receipt
`4d114a4c811df1ad0b722227ff117605cd7629f33b19c94b3d34f20425b47df2`.

Independent source review found a separate P2: the final permit could reparent
SceneViewsRoot inside the same retained window/root while stale parent-link
evidence still allowed metadata-observed. One new focused regression actually
fails on the exact reviewed r1 header; RED receipt
`18f8182da44e8c4ee7c7794c69b74e65dc9f162e868b687a5a1234900a82f230`
preserves that exit1 and raw assertion. A prior reproduction on the ordering-only
r2 header, receipt
`2efb04cd161181f9331d3e86206f2c1e9045614e0ff897359127ced58a290018`,
is also preserved; neither failed run is overwritten or reclassified.

The repair affects explicit mode only: after the final callback, re-prove the
complete endpoint owner/visual-parent/window/thread/visibility/enabled chain and
retained context/time, with no later permit or application VALUE getter before
favorable return. Legacy owners()/census behavior stays intact. The same new
fixture, including that single reparenting regression, passes on the combined
ordering/endpoint repair; compiler and control stderr remain empty. No old matrix
was repeated; red runs execute only the new regression.

SDK author's repaired source/control receipt SHA256
`a66a7bdc64c465b6717b71979d1f7b08eef2604cb9a9371990b988206a6d2c1f`
binds three source inputs, LF-normalized shell runner, exact invocation, moc/
compiler/control outputs and the owned fixture ELF/hash. Private evidence has
no public transcript link. Working/Git line-ending mappings must be explicit at
publication; no all-byte-exact claim is made. The existing independent reviewer
verified repaired receipt a66a7bdc/all20 files, exact reviewed-r1 RED18f8182d and
corrected-source GREEN with the same new regression fixture/runner, and all five
live source/reporting files. The P2 is resolved with no remaining findings in
this narrow helper scope. This is inspection of saved owner-executed controls,
not reviewer execution or native qualification; private review has no public
transcript link. Owned structural results grant no native current document/
page/readiness/owner/lifetime/capture authority; future integration stays separate.
