# Execution-time order remains a separate contract

The positive ordinary creation/selection experiment does not establish an
atomic source/order precondition. Focused exact-build investigation explains
why GUI readiness and worker queue ordering alone do not close that gap.

## Ordinary task and distinct maps

The ordinary AddPageTask retains its normal bookkeeping and connects the matching
native page-added signal with a queued connection. Its execution lambda packages
stored index, template, size, source index and target UUID into the AddPage
request. The inspected payload contains no expected source UUID, complete order
or compare-against revision. A GUI check immediately before submission therefore
remains earlier than native execution.

The GUI-facing native Document and DocumentWorker have distinct map storage and
mutexes. Document's page-count setter can resize its own map; its pending-map
setter only copies a pending map under another mutex and sets a flag. Neither
establishes a worker transaction lease. Exposed notifications/getters and
lastModified do not provide an inspected revision token tied synchronously to
every worker-map mutation. This is a bounded metadata/call-data result, not proof
that no private predicate exists anywhere in the executable.

## Renderer count is an actual writer

Worker renderer setup installs an explicit DirectConnection from
DocumentRenderer.pageCountChanged(int) to the worker's private Qt slot
doSetPageCount. That slot locks the worker map mutex and resizes only when the
clamped incoming count differs from its stored native/base count, then unlocks
and emits map/count/pages notifications. This is a real stock connection; the
later temporary-observer disconnect does not remove it.

Exact PdfRenderer initialization assembly shows a synchronous branch obtaining
the PDF page count, comparing/storing its cached count and emitting when changed.
The caller's PDF mutex guard is still armed at emission. Under the normal guard
contract this suggests PDF-backend mutex then direct worker map mutex ordering;
the helper's internal behavior was not separately audited, so this is an
inference rather than a complete lock-order proof. DirectConnection introduces
no queued thread handoff. At the initial checkpoint, another metadata branch's
scheduling/emitting thread remained unresolved. The scoped followup below closes
that callback-timing question for the inspected path. Reader-ready state still
does not prove that later page-count emission or other writers are impossible.

The native resize utility can add generated IDs, remove entries and update count
and clock-like fields. Those private fields have not been established as a
complete revision predicate across all writers. The utility depends on caller
coordination; it is not an exported ownership token.

## Scoped PDF count followup — October 23, 2026 UTC

Exact static followup resolves the alternate initialization callback chain. The
current-request IPC routine sends, polls and receives synchronously, matches its
64-bit request ID and discards late results with other IDs. The initialization
decoder invokes the metadata callback inline and returns to that same IPC stack.
The inspected chain does not retain the borrowed metadata callback for delivery
after initialization returns. A load reply is handled separately; a failed load
reply does not invoke the requested command's reply callback. This closes the
specific deferred-callback hypothesis. It does not establish that QProcess waits,
signals or direct application receivers cannot reenter while initialization is
still active, nor does a renderer subprocess restart alone establish a later
count notification.

There are two distinct count comparisons. The renderer emits only when its
cached PDF count changes. The worker then clamps the argument to at least one,
compares it with its stored native/base PDF count under the map mutex and returns
without resizing or notifications when equal. In this exact layout that worker
field is at `+328`, equivalent to PageMap `+128`; it is distinct from the complete
page-vector length. These private offsets describe inspected evidence, not a new
getter or supported cache-field API. The GUI total of nine pages does not prove
that the native/base PDF count is nine.

Two renderer constructors initialize the renderer's cached count to zero. A new
renderer can consequently re-emit the same positive PDF count N, but recreation
and re-emission alone do not establish a worker-map mutation: the worker's own
equal-N guard still applies. Conversely, unchanged PDF bytes do not establish
successful count metadata on every initialization attempt. The synchronous
missing-backend/PDF-handle branch constructs zero count metadata and reaches the
ordinary emission comparison; the worker would clamp an emitted zero to one.
That is a conditional initialization failure path, not evidence that it occurred
or can recur after the admitted ready observation.

The bounded conclusion is therefore conditional: if every later count emission
is the same positive N and this worker's native/base count remains N, this
connection cannot reach resize. An immutable-file check and a GUI-ready join do
not yet establish those conditions. The remaining proof is the relationship
between native GUI readiness and successful initialization of this actual
worker's renderer, its native/base count N, and the fixed-PDF callers that can
reset or replace that renderer afterward. No actual post-ready reset, failure or
concurrent count delivery was established by this followup, and no exhaustive
absence of other map writers is claimed.

The [native worker observation](../experiments/worker-local-order-result.md)
separately qualifies its six individual reads and their matching completion in
one exact candidate. It does not supply this renderer-lifetime invariant, a
shared map version across the reads or an atomic ordinary-creation precondition.

## An external map lock is not the solution

The complete inspected worker constructor zero-initializes its 24-byte map mutex
without a later direct attribute/init call. Exact ARM vendor-header compile-only
assertions distinguish the normal zero/default mutex kind from recursive kind.
Under normal construction the mutex is nonrecursive. Holding it externally and
then calling an already-locking getter or AddPage handler on the same thread
self-deadlocks. It cannot simply wrap the ordinary request or lower handler.

Same-worker-turn execution excludes other queued commands between statements.
It does not by itself exclude direct renderer delivery, configured callouts or
native external writers. A later map notification can make the result uncertain;
it cannot retroactively reject an unfenced mutation.

The smallest useful read-only increment is a retained-worker execution witness
that compares current native IDs/indexes with the GUI-selected subject using
established getter contracts and ownership through actual callback completion.
It would narrow stale-cache/queued-execution uncertainty, without making a later
ordinary insertion atomic. Full-order/count claims additionally need safe,
bounded accessor semantics. No new native getter call is qualified here.

The later [fixed native worker witness](../experiments/worker-local-order-result.md)
observed three expected ID/index roundtrips on the associated strong-owned worker,
with exact callback completion, GUI reference release and unchanged fixture bytes.
That separate candidate result narrows worker/GUI correspondence uncertainty; it
does not establish a full worker count/order, shared read version or insertion
execution predicate. The boundary findings below remain unresolved.

For a supported mutator, the remaining question is a native predicate after
dequeue and before mutation that preserves the ordinary Task's job identity,
onAdded and store path. None is established by the inspected request/lambda.
Queue surgery, guessed mutex ownership and copied Task bookkeeping are not
implemented substitutes. Creation remains unsupported in the shipping SDK.

## Provenance and rejected tool output

Target SHA-256 is
`071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df`.
REA 6.7/Ghidra 12.1.4 corroborated renderer setup and native resize in evidence
`ev_bf4e1de63036432d59d35e0515aeb3d39a91ad21b82ca78463ad5d9669a2d7fb`
and `ev_5428b9d2c041057e6fce15acb9aee26c54559090bb0c207bfeeb1d29abe6142f`.
The private neutral export has 30 records, 1417512 bytes, SHA-256
`2d27920ed0ab4d9bf0c7989af03863ba3b7d5ce637073f2f6d9cc3d1573dd851`.
The owned analysis container was closed after export; no persistent analysis
database export is supported by this pinned provider.

One requested pending-map address resolved only to an enclosing metacall. A
requested PDF initialization address was reported unknown but accompanied by
unrelated pseudocode; that mismatched body was rejected. A recovered renderToImage
body did not answer the count-emitter question. Exact pending-map/PDF branches
and constructor/metadata facts above use separately scoped raw ELF/ARM assembly,
RTTI and compile-only header assertions, not those rejected decompilations.

The October 23 followup used the same exact target with REA 6.7/Ghidra 12.1.4.
Evidence
`ev_dccb9a773a8bc2a86af5af5c7007def7dbfca8cb9a73fd6694f240c8e190b712`
accepted canonical bodies `0x71e178`, `0x704a20` and `0x711160`. The requested
`0x70347c` was reported unknown and accompanied by unrelated `0x711470`
pseudocode; that body was rejected. Separate exact assembly shows the requested
small method copying a refcounted value without resetting the renderer count
cache or mode. Decoder/register-flow and constructor/failure facts use scoped
raw ELF/ARM assembly; incomplete decompiler argument recovery is not treated as
an exact C++ signature.

The new private neutral export has 31 evidence records and 13 workflow entries,
1672238 bytes, SHA-256
`384f0d70ab3db9cfd7cc87b09bbae2d946b6c9b1ea2e42b6523ba9753db19013`.
The private finding receipt has SHA-256
`49c93cf38955e0a87d66fb4b6dd3d0b0ebb3aaa913f3bda2fcd2544d724d18f7`.
The owned session closed at 03:54:42 UTC after export and its containers were
absent. The earlier neutral snapshot restored evidence, not a Program database;
the followup performed fresh analysis. No persistent database reuse, device
action, renderer hook or direct-handler execution is claimed.

Source basis: exact shipped-artifact records and retained Task evidence,
new scoped REA results with rejected resolutions preserved, and vendor ARM
compile-only layout evidence. Thread/quiescence/lock-order limits are explicit.
No device experiment, hook, queue modification, native execution predicate or
all-writers proof was performed by this investigation.
