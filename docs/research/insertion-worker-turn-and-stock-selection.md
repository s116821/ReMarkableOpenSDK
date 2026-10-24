# Worker-turn insertion and stock selection boundaries

The successful [worker callback witness](../experiments/native-worker-callback-result.md)
provides an exact-build way to execute a diagnostic on the native worker. It does
not make an ordinary AddPage request atomic: that request still enqueues another
command. Focused follow-up separates a lower direct handler from the ordinary
controller/Task path and identifies the UI selection work performed by stock.

## Direct handler candidate

The ordinary worker dispatcher directly calls an internal AddPage handler. Its
inspected body does not enqueue another AddPage request, so calling it during a
callback is a plausible same-worker-turn seam. The actual hard-float machine
argument locations differ from the decompiler's guessed prototype. No public Qt
meta-call for this handler was established, and no direct invocation is selected.

This handler is not a replacement for the whole ordinary controller:

- It drops and reacquires its page-map mutex. The available ID/index accessors
  individually lock a read and release before returning; they do not export a
  transaction. The [execution-time boundary follow-up](execution-time-page-order-boundary.md)
  establishes a normally nonrecursive mutex and an actual renderer DirectConnection
  to the private meta-callable page-count writer. Holding the mutex across an
  already-locking getter/handler self-deadlocks; after-ready concurrent emission
  remains unproven rather than excluded.
- It can load/access Scenes, call a configured Scene factory and invoke a
  configured template callback. Native initialization and filesystem operations
  are not a leaf operation. Absence of blocking/reentrant dependencies is unproven.
- A supplied UUID collision can fall back to a newly generated ID rather than
  failing. Prechecking absence without exclusion does not guarantee the intended
  identity; exact post-operation reconciliation remains necessary.
- Its page-added signal would use the active callback job ID. Direct entry does
  not create an AddPageTask or reproduce the upstream controller guards and that
  Task's Document map/content-dirty, remembered-page and store scheduling work.
  General map/modified connections have not been shown to provide every omitted
  effect. Void return and generic completion are not commit/durability receipts.

Same worker turn excludes another queued command between statements. It does not
prove exclusion of other writers, configured callouts or synchronous reentry.
The current no-op witness's cancellation policy also cannot be copied blindly
into a callback using native pointers: strong ownership must cover actual native
use even after missing acknowledgement, without last-owner destruction on the
worker thread that the native destructor may wait for.

## Ordinary controller and selection

The existing six-argument ordinary controller method preserves native preflight,
normal AddPageTask construction and its reconciliation/store scheduling. It can
consume the freshly joined actual native ID rather than resolving a new object
from UUID text. The retained Task.finished binding invokes the supplied QJSValue
with an empty argument list on the controller's AutoConnection path. That callback
has no boolean success/status argument and is not a durability receipt.

Stock's add-page UI sets an adding-page state and separately opens the new page
in its callback. A controller-only caller does not inherit that UI continuation.
The ordinary page-map-change path instead preserves the existing current ID.
Therefore callback/true return cannot stand in for actual selected target.

The stock Reader's adjacent `moveForward()` method opens current index plus one
when a right page exists. Its end-of-document branch can create a page. Any
diagnostic using this route must freshly verify the original selected source,
complete expected post-insertion order, the intended target immediately right,
readiness and a right neighbor before one dispatch. If the target is already
selected, no Next is needed. A later fresh observation must establish target
identity; method dispatch alone proves neither navigation nor rendering.

The [joined creation proposal](../../openspec/changes/establish-native-platform-contract/joined-creation-selection.md)
uses that ordinary route as an idle-fixture diagnostic. It does not claim an
execution-time source/order guard or expose a general direct-open SDK capability.

## Provenance

Input is the retained RM2 executable SHA-256
`071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df`.
REA 6.7/Ghidra 12.1.4 completed five focused dependency analyses, evidence
`ev_aef2255d064b0536627b36d70b576cc14486ca049cb47b00cb0990600c784afb`.
The private neutral export is 1127052 bytes, SHA-256
`577a38a3f61c1784657eb3981efd73d031a869139e008e34bcee3ac420b680dd`.
The owned analysis container was closed after export.

The direct handler, controller/Task callback and stock UI relationships also use
retained earlier REA records, exact ARM disassembly, metadata and saved QML
inspection. The global native class name `QmlDocumentWrapper` was confirmed from
its exact metaobject bytes, not inferred only from a pointer type name. No
proprietary pseudocode or extracted resource bodies are reproduced here.

Source basis: exact shipped-artifact static records and original SDK source.
The proposed diagnostic is engineering inference; these static findings do not
constitute native direct-handler execution, actual creation/selection or a
qualified atomic adapter.
