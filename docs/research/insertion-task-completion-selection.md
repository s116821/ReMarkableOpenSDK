# Native insertion task completion and page selection

Evidence checkpoint: October 18, 2026. Bounded static analysis identifies task
submission and completion-callback delivery, with no direct current-SceneView or
selected-page update identified in those paths. This does not prove that task
execution or its receivers leave the displayed page unchanged.

## Provenance and scope

The existing authorized RM2 firmware 3.28.0.172 executable is 22,046,100 bytes,
SHA-256 `071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df`.
The retained insertion-entry trace is 13,333 bytes, SHA-256
`08b1c840839bc2b943c446a4b109d0c3a539e7262a67eadb365df72a1586833c`.
The earlier relationship report is 6,367 bytes, SHA-256
`dded131a541bb45b38fb6bb9ba5def7764ed6b1695c48c336626196e5f842c00`.
All three identities were checked locally before analysis. Private evidence has
no public download link; executable bytes, disassembly, layouts and reconstructed
proprietary source remain outside Git.

LLVM 18.1.3 decoded the entry, submission core, immediate completion handler and
JavaScript invoker. ELF load mappings, dynamic imports, RTTI, vtables and Qt
metadata corroborate the interpretation. Nearest exported-symbol labels were
not treated as function identities. The executable was read as data only.
There was no tablet contact, target execution or fresh acquisition.

This follow-up starts at the already identified
`DocumentController::addPageWithTemplateAndPageSize` entry. It does not repeat
registration discovery or the [controller/document relationship](scene-document-controller-relationship.md).
That relationship resolves an explicit document ID through the Library; it does
not identify the current displayed document or selected reader view.

## Observed submission and completion chain

| Stage | Static evidence and meaning |
| --- | --- |
| Entry | Resolves the supplied document ID, captures the supplied `QJSValue` and calls the insertion core. The outer return tail releases temporary ownership and callback storage; no direct view selection is identified there. |
| Task construction | The successful submission branch allocates an `AddPageTask`, identified by its RTTI/vtable and shared-control RTTI, and supplies the resolved Document plus insertion inputs, UUID and optional data. Construction/submission is distinct from completed mutation. |
| Completion connection | `QObject::connectImpl` connects `Task::finished` to a controller-context slot object containing the captured callback, using AutoConnection. The Task metaobject identifies this signal separately from `failed`. |
| Controller dispatch | The controller virtual slot resolves in the retained ordinary DocumentController vtable to the TaskTracker execution wrapper. It passes the task to task execution/tracking, then conditionally emits `isRunningTasksChanged`. Immediate tracking code retains the task and installs finished/failed bookkeeping connections. This is a task-tracking handoff, with no identified display-owner handoff. |
| Slot invocation | The completion slot's call operation logs and invokes its stored function; its destruction operation destroys the function. The separate function manager performs copy/destruction, rather than delivering completion. |
| JavaScript invocation | The actual invoker calls `QJSValue::call` with an empty argument list, checks `isUndefined`/`isError` and optionally logs the error. This wrapper supplies no page ID, Document or SceneView to the callback. |

The [earlier insertion/task findings](native-platform-ledger.md#static-insertiontask-trace-beyond-metadata)
already distinguish generic job completion from proven creation or durability.
The present trace refines the controller-facing task/callback route; it does not
upgrade that evidence to durable success or fresh source attribution.

## Selection boundary and unresolved effects

No direct SceneView document setter, selected-page assignment or page-navigation
call was identified in the inspected submission and immediate completion paths.
The controller's task dispatch can be statically resolved for its ordinary
vtable, but this does not establish the actual runtime instance or exclude a
subclass override. An earlier global virtual guard also remains unresolved.

The supplied JavaScript callback is arbitrary application code: an empty argument
list does not prevent it from navigating through captured state. Other Task or
document/model signal receivers could also change the current page. Asynchronous
task execution and downstream document/job processing are outside this first
bounded trace. Consequently, **no identified direct selection** must not become
**proof of no indirect display effect**, or a guarantee that insertion only
mutates the Document.

The [reader selection wiring](reader-scene-selection-wiring.md) describes a
separate application-owned current SceneView reference with neighboring prefetched
views. Nothing here joins the completion callback to that reference at runtime.
After creation, observe the actual active document/page before deciding whether
navigation is necessary; do not assume the insertion callback selected it.

Runtime receiver identity, thread affinity, object lifetime, source continuity,
completed mutation, persistence and current-page coherence remain unverified by
this analysis. No native capability, primitive API, firmware support declaration
or experiment authorization changes. Source basis: exact retained private
artifacts and local static decoding; the selection conclusion is explicitly
limited to the inspected paths.
