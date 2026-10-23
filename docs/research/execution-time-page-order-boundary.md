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
or can recur after the admitted ready observation. The later readiness followup
below further restricts it: fresh cached zero and failure metadata zero do not
emit a changed count, so this is not an every-failure shrink-to-one mechanism.

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

## Initial readiness/lifetime followup — October 23, 2026 UTC

The actual source admission checks selected SceneView Active/pageStatus Ready,
no initial/pending tiles, no controller work, no facade loading/page error or
document load error, plus the creation Library relationship/readiness. It does
not directly observe PdfRenderer initialization, its cached PDF count, the
worker's native/base count or a renderer generation. Exact status-update code
derives SceneView status from its controller and controller counters; the
page-status setter consumes a supplied Page::Status. Those getter/setter
definitions alone do not establish the producer of every admitted Ready state.

Normal worker ordering gives stronger evidence than those definitions alone:
the inspected processJob path calls `ensureRenderer` before normal work for job
types greater than15 and returns on failure. The page-lines loader also has an
already-Ready fast path. These facts do not establish how every Ready state was
produced or whether it can outlive renderer shutdown/replacement. No actual
ready-but-failed counterexample was observed.

`ensureRenderer` immediately returns when this worker already has a renderer;
that ordinary path does not reinitialize the same instance. A newly created
renderer starts with cached PDF count zero, receives the direct count connection
and is initialized; failed initialization clears/deletes it. Fresh failure
metadata zero compared with cached zero produces **no changed-count emission**.
A fresh replacement emitting the same positive N also leaves the worker map
unchanged when its native/base count remains N. Initialization success checks
reply status, not an independently observed positive-count predicate. Neither
success nor failure can be substituted for an actual count observation.

The failed-init error bridge is also distinct from the count writer. Worker
`documentError` is a signal, connected in retained construction to native
Document `setLoadError` using AutoConnection (0). The setter updates the field
read by the wrapper's `loadError` property. A cross-thread GUI receiver can lag
emission under normal AutoConnection delivery; this is a threading inference,
not an observed scheduling race or an exhaustive affinity/receiver proof.

The inspected reset candidates narrow recurrence further. An idle empty queue
waits; renderer deletion there requires the shutdown flag. The other identified
regeneration branch has an EPUB filetype guard and supplies no fixed-PDF reset
example. The bounded store scan does not cover aliases, all instruction forms or
indirect/external writers. A generic renderer meta-property write arm can change
the count, but no stock caller of that write was established. The password setter
only assigns its string and does not invoke initialize. None of these mechanisms
is a demonstrated fixed-PDF post-ready reinitialization or a proposed setter API.

A concrete remaining indirect consumer is the export path: it ensures the
current renderer, supplies that actual pointer to a factory-created exporter and
invokes the exporter's virtual method. This is not itself a direct second call
to renderer initialization; the downstream exporter was outside the initial
batch. Exact Ready production and concrete indirect renderer/reset callers
remain bounded proof gaps. No post-ready count race, stable-generation invariant,
absence of other writers or atomic creation predicate follows from this batch.

## Final insertion lock and Task outcome followup — October 23 UTC

The ordinary handler takes its final worker-map mutex at `0x841824`, calls
the insertion helper at `0x841984` and unlocks at `0x841a44`, without an
intervening unlock. A direct-map predicate at this call is a possible design
point, not an existing API. Calling the established locking getters there
would recursively acquire a nonrecursive mutex. No existing expected-source
argument or clean predicate-refusal branch was found.

Invalid insertion indexes normalize to append. A requested UUID collision
substitutes a generated UUID instead of refusing the operation. A future guard
must retain outer operation eligibility through both transformations. Earlier
source loading/status effects and later allocation/store effects also mean
that preventing insertion is not a transaction rollback of all effects.

The matching generic job-completion path can finish the Task and invoke its
no-argument JavaScript continuation without a pageAdded event. Without that
event, the onAdded dirty/store path does not run. Therefore Task completion
alone cannot distinguish insertion from an explicit adapter refusal; the latter
would need its own operation-bound outcome channel.

The dispatcher and worker-loop exception regions around this handler are
cleanup-only, not normal rejection handlers. Unrelated catch regions elsewhere
do not cover these calls. At the exact insertion call, the handler's LSDA row
routes through temporary QString destruction, mutex unlock and unwind resume.
That establishes original cleanup coverage, not a safe native interception ABI.

The proposed [owned interposition study](../../openspec/changes/establish-native-platform-contract/insertion-predicate-interposition-study.md)
tests a private rejection exception caught by an owned outer wrapper. No native
hook, executable predicate, refusal protocol or supported capability is
established by this design or its original-callsite cleanup evidence.

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

The later initial readiness/lifetime checkpoint used twelve procedure requests
and two xref queries on the same exact target. Its frozen private findings have
SHA-256 `959bf83c5ca94559df404809eafb8e6d68e45742215d0fb464d875b410a08039`;
the initial receipt has SHA-256
`16c355457a823eef70e4bdb54e37ab77cc236e656d500a63451b270480276f67`.
Ready-propagation/caller evidence is
`ev_4e63a937778f180a643d0942868b4a967490c0c62fa3ca9a39a99a8e6d67a07b`;
SceneView evidence is
`ev_365a6d011b7fd7274c2117ef56ed21bd0f1c51953a5c580c3d261873bd4b711c`.

Seven requested canonical starts matched. Requested SceneView update/check
entries resolved to a merged metacall, and two worker callers resolved to the
dispatcher; these are context rather than standalone requested bodies. An
unknown load-error setter request returned an unrelated body, which was rejected;
its small actual setter and the exact status branches use separate ARM slices.
All raw replies and accepted/rejected mappings remain retained. This initial
receipt records raw evidence export and a warm session reserved for the parent's
separate followup, **not** a final neutral snapshot export or session closure.
The earlier closed-session receipts above remain historical facts for their
own batches.

The final-lock batch accepted canonical procedures `0x841728`, `0x7b4bb4`
and `0x3e6d94`, with evidence
`ev_f0fba5e379a8c3beef6e0e5eff04a041279214e5cc47c25d44c7195590564518`.
Its private receipt SHA-256 is
`440bbba4308f62a0de1fe85827bf043d25f7305c7ecfb7772782b1aa00ffc8ae`.
The Task-terminal batch accepted `0x6b2580` and `0x6ba4e8`, with evidence
`ev_6fa3d9382816bdba9b6d56ab656aa189d551730dce358306df7a72450e2ab76e`.
An unknown `0x7c040c` request returned unrelated `0x7cfdb8` pseudocode,
which was rejected; the actual small finish stub uses retained ARM/RTTI.
The terminal receipt SHA-256 is
`b84a06a49b21706e6bae116e0c4dc1452dcc7c4a745a8478c0f8d3487898ca33`.

The later retained ABI/LSDA review receipt SHA-256 is
`8c0993ad8730109cde2635da001e59f45df99be5e903ae0c630337d0c55b9f32`.
At the insertion BL, the decoded LSDA row is start `0x25c`, length four,
landing offset `0x978`, action zero, leading to cleanup at `0x8420a0`.
Relocation evidence independently binds its personality to
`__gxx_personality_v0`. Machine argument locations and selected callsite
behavior were recovered; original C++ type spellings, native trampoline
relocation, VFP forwarding and wrapper unwind behavior remain unqualified.

The final neutral export has 3166590 bytes, SHA-256
`98c23a474c5eede18f78098a809b3f8f0a937378ce368b2ff9c69059f4356a76`.
The owned session closed at 05:48:43 UTC and its container was verified absent.
Closure receipt SHA-256 is
`02dbaed1cd0fd8122b0e9cc3caaaef3ff61b74cd3872a9fbeaa5ac24090f6567`.
This closes the warm followup recorded above without rewriting that initial
checkpoint or claiming a persistent Ghidra Program database export.

Source basis: exact shipped-artifact records and retained Task evidence,
new scoped REA results with rejected resolutions preserved, and vendor ARM
compile-only layout evidence. Thread/quiescence/lock-order limits are explicit.
No device experiment, hook, queue modification, native execution predicate or
all-writers proof was performed by this investigation.
