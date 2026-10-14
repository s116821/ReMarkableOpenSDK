# Explicit retained-window structural census: proposed first increment

This narrow source-only proposal follows the accepted
[current-document assessment](../../../docs/research/current-document-after-loading-capture.md).
It is an unfinished implementation checkpoint, not a native observation capability
or selected experiment. Existing capture producer679d and Main runtime remain unchanged.

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

Independent precode review precedes implementation. Only new meaningful owned
controls are planned: supplied-window selection without fallback, exact root
mismatch/change/deletion, context/thread/permit loss, traversal-cap refusal and
zero application VALUE-getter calls. Owned fixture setup is synthetic evidence;
no unchanged matrix or native operation is requested. No code/build/control run
has been started for this increment; helper implementation and verification stay
unfinished at this checkpoint.

Source basis: accepted assessment/publication4dcc82ea, existing metadata source,
and Main's current-chat exact helper contract. Main remains sole device operator.
No new nonce/artifact/transfer/opening/menu/input/replay or broader native gate
follows. Preserve the account's1% reserve and existing operation holds.
