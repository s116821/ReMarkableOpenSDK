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
doSetPageCount. That slot locks the worker map mutex, resizes the map, unlocks and
emits map/count/pages notifications. This is a real stock connection; the later
temporary-observer disconnect does not remove it.

Exact PdfRenderer initialization assembly shows a synchronous branch obtaining
the PDF page count, comparing/storing its cached count and emitting when changed.
The caller's PDF mutex guard is still armed at emission. Under the normal guard
contract this suggests PDF-backend mutex then direct worker map mutex ordering;
the helper's internal behavior was not separately audited, so this is an
inference rather than a complete lock-order proof. DirectConnection introduces
no queued thread handoff. Another metadata branch's scheduling/emitting thread
remains unresolved. Reader-ready state does not prove that later page-count
emission or other writers are impossible.

The native resize utility can add generated IDs, remove entries and update count
and clock-like fields. Those private fields have not been established as a
complete revision predicate across all writers. The utility depends on caller
coordination; it is not an exported ownership token.

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

Source basis: exact shipped-artifact records and retained Task evidence,
new scoped REA results with rejected resolutions preserved, and vendor ARM
compile-only layout evidence. Thread/quiescence/lock-order limits are explicit.
No device experiment, hook, queue modification, native execution predicate or
all-writers proof was performed by this investigation.
