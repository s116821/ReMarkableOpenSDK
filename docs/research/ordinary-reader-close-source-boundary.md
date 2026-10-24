# Ordinary Reader close: source boundary

October 23, 2026. Focused static review of the already retained DocumentView,
DeviceSceneView and MainView resources in xochitl SHA-256
`071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df`.
No new document close, pen event or native call occurred in this review.

## Two different close operations

The stock toolbar, keyboard and gesture document-close paths reach the Reader's
DocumentView close function. MainView also calls that function when leaving an
open document. WindowNavigatorWrapper's separately declared close overloads
operate on navigator windows. Their names do not establish a document-save
contract; use the source-backed document action when designing the experiment.

DocumentView first stores its scroll position and exits applicable selection
and text modes. It invokes DeviceSceneView close, clears document UI state and
the document reference, clears remembered restart-open state, then emits its
documentClosed signal. DeviceSceneView close ends selection, clears its current,
next and previous scene references, resets page identity/index and disables
snapping. Its cache removal hides and unparents the corresponding native scene
items before requesting their destruction.

This extends the [retained current-scene wiring](reader-scene-selection-wiring.md).
It establishes the declared source order, including the removal of prefetched
views. It does not establish the exact native destructor execution order,
asynchronous worker completion, storage flush, crash durability or shutdown
repair. The QML close function exposes no storage-completion acknowledgement in
this inspected path. Neither its return nor documentClosed is a durable-save
receipt. No inference about all underlying save mechanisms follows from that
absence in this function.

## Consequence for the fixed writing test

An additional offline REA/Ghidra12.1.4 query confirmed the metadata dispatch:
the named and no-argument navigator close methods reach the same wrapper
implementation, with the no-argument form supplying an empty string. On its
inspected normal branch the wrapper prepares an argument and delegates through
its stored receiver's virtual method. This does not identify that virtual
callee's complete behavior or connect it to a storage fence. A procedure-name
search returned no matches for the wrapper class name in this stripped target;
exact metadata addresses supplied the subsequent bounded decompilation. That
search result is not an absence claim about the class or method.

The full-analysis startup took about24 minutes under the selected two-CPU
container limit. The earlier ten-minute timeout remains a failed analysis
attempt. Successful later analysis does not change that outcome. REA exported
the evidence bundle and snapshot before closing; its task-owned container was
then verified absent. Original binary bytes and the two unrelated OpenCode
containers were unchanged. No tool installation or global configuration changed.

Require positive pen release before closing. Treat one dispatched close as a
possible effect until fresh read-only UI evidence establishes closure. Preserve
the original absolute test/recovery deadlines; do not issue another close to
resolve ambiguity. Observe target-file changes separately, then perform a fresh
ordinary reopen and a fresh document/page join and image capture. Compare full
before/after fixture contents, page objects/order and original immutable files.
This can establish one observed save/reopen result without claiming a general
storage fence.

The [fixed-stroke proposal](../../openspec/changes/establish-native-platform-contract/fixed-stroke-release.md)
is separate implementation work. A direct QML invocation still needs its own
runtime receiver/lifetime/method/callout controls and native packet; these source
findings do not select an invocation or qualify a new SDK close API. The prior
[native writing/reopen observation](../experiments/caller-selected-insertion-result.md#october-9-follow-up-native-writing-and-ordinary-reopen-persistence)
remains a different page and trial.

## Evidence basis

DocumentView output SHA-256
`867b30a725a4ae97e3459f07a60cd72bcafb21db9a0b374f02d75c5cd8d83269`,
lines386–415 and its keyboard/toolbar/gesture callers; DeviceSceneView output
`78df7fe68f0be6e0795cde67bdb5ce5c5f355c9100f0c14d6e8157c688271a0c`,
lines1488–1517; MainView output
`2bc2866c59245e74af769e277e3aae801ad76bc7f816f24ccf1fc7ac4f5e3cfc`.
The existing [navigator metadata findings](window-navigator-provider.md) supply
the distinct close overload declaration. Proprietary resource bodies remain
private. Focused source-locator receipt SHA-256
`37b3603111d7333e6820ba32095a39e32192a42e3f34ea10473f1af38ce0f1e0`
retains exact private line ranges and hashes; its bounded dynamic-symbol search
found no matching defined close/destructor symbols and supplies no native
implementation claim. It has no public download link.

The additional original REA metacall and wrapper results have SHA-256
`5c471a9f994c64aa4eca1d13bd485c66a88693789b563547bf0dffbca78d62c4`
and `32657d9b051ce9527b432f6245a0f62a79ad27aeec2ce6cf77ae62983a139306`.
The exported bundle digest is
`5a23ef9f23bcf8d58ae7badf8d8088bce9eb194bde06cde4f3e4de3abda958c5`;
snapshot digest is
`e7d705d7de8923ba8cc9352688d7a09b81804778f72b8bff1b5c0b10798c8ec7`.
These are private static decompiler records, not source-equivalent code or live
execution evidence. Preserve the recorded terminal-flow-model limitations;
they do not prove either called wrapper is a non-returning operation.

Independent source/publication review checked35 assertions against retained
resource lines and original REA results, receipt SHA-256
`90b77e9a158ef1899cebcd3e275f2432c3450c3eaacba4e7987378d4be7bca0e`.
Container absence and unchanged unrelated resources remain attributed Root
execution checks, rather than a second reviewer execution.

Source basis: exact retained shipped-resource bytes and prior metadata checked
locally. Test requirements are engineering conclusions from that source order;
native completion and durability remain unmeasured here.
