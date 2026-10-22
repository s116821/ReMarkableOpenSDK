# SceneView, document wrappers and the library controller

Status: October 13, 2026 exact-build static findings, supplemented October 22
with the Library wrapper-cache path. These findings justify a source-identity
follow-up; they do not qualify a native adapter. The separate
`passive-visual-sftp-v1` route, manifest, caps and first measurement scope at
`91f0973` remain unchanged.

The saved executable establishes a concrete relationship from `SceneView` to
`QmlDocumentWrapper`, its backing `Document`, and the document's worker. The
library `DocumentController` reaches a document by resolving a supplied entry ID
through its associated `Library`. `SceneController` is a different object type;
its pointer cannot be compared with `DocumentController` to prove identity.

## Source basis

These findings come from the existing authorized saved firmware 3.28.0.172
executable, rehashed for this investigation as SHA-256
`071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df`.
Qt metadata, RTTI/vtables, actual property dispatch, constructors and the
controller's document-resolution call path were inspected as data with the
existing vendor ARM tools. No proprietary executable or getter was executed.
Raw disassembly, exact layout evidence and reproducibility scripts remain in
private evidence; they have no public download link. No proprietary source body
is incorporated here. Layout observations are build-specific, not SDK APIs.

The October 22 follow-up used the same rehashed executable and retained GNU
ARM objdump `2.42.0.20240723` in the existing pinned tool image. Five bounded
disassembly slices, metadata, prior traces and exact tool/input/output hashes
are retained privately. These are normal-tool static findings, **not REA
Evidence records**. No target code was executed or tablet contacted.

The earlier [R2 registered-controller observation](native-platform-ledger.md#completed-development-observation-r2-4096-controller-chain)
is historical runtime evidence for finding an existing `DocumentController`,
not a current address or displayed-document binding. The
[scene tile findings](scene-tile-render-geometry.md) cover the separate rendering
relationship and its limits.

## Observed object relationships

| Path | Evidence and interpretation |
| --- | --- |
| `SceneView.document` to `QmlDocumentWrapper` | The actual getter checks a guarded reference's control record before returning its pointer. A nonzero pointer alone is insufficient; this check does not acquire lifetime. |
| Wrapper to backing `Document` | The wrapper constructor supplies the same document pointer to its `QmlEntryWrapper` base and its own document member, retaining shared ownership in each. Document-specific getters and page-key methods use that document member. This is a constructor relationship, not proof that every later live read is consistent. |
| Wrapper to inherited `id` | `QmlEntryWrapper.id` is declared `entry::Id`. Its dispatch reads the base's backing entry and copies the ID member through `QByteArray::operator=`. This rules out assuming a `QUuid` object layout from the name or UUID-looking text. Encoding, canonicalization and supported external conversion are not established here. |
| SceneView setter to document and worker | The document setter copies the wrapper's backing document into a retained view member, then calls that document's worker-acquisition path and retains the result as the view's worker. It resets/disconnects old state before installing new state. These are staged updates, not an atomic snapshot. |
| `SceneView.controller` to `SceneController` | The view exposes a separate controller member. `SceneController.worker` resolves its own guarded worker reference; its `pageId` is a shared string. These are useful consistency checks on a future fresh relationship, not a library-controller alias. |
| `DocumentController.library` to `Library` | Its property getter resolves a guarded library reference. The traced insertion entrypoint uses that same member when resolving the explicit `documentId` argument. |
| Controller ID resolution to `Document` | The controller resolves an entry through its library and dynamically casts it from `Entry` to `Document`, preserving the resolved shared ownership on success. It does not derive the displayed source from the controller pointer. |
| `Library.entryForId` to the same resolver | Native metadata declares one `entryForId(entry::Id)` returning `QmlEntryWrapper*`. Its actual dispatch calls the same native resolver as the controller, using its own receiver Library. The controller's public Library getter and its document resolver use the same guarded Library member. |
| Resolved entry to returned wrapper | The method first checks that entry's guarded cached QObject. A valid cache is returned; a document cache miss constructs `QmlDocumentWrapper` from the exact resolved shared pair and writes the weak wrapper cache. The wrapper constructor retains the same backing document in its base and derived members. |

The setter returns early when the proposed wrapper has the same backing document
as the view's retained document. Thus wrapper replacement and document identity
are not interchangeable observations. Shared ownership inside the application
also does not pin anything for an external reader.

The library resolver has special branches as well as an ordinary keyed lookup.
An empty ID follows a special branch; it must not be treated as a valid displayed
document ID or silently repaired into one. The ordinary lookup compares key
length and contents before returning the retained entry. None of these paths is
authorization to call the resolver, acquire a worker or invoke insertion.

## Meaning and effects of the wrapper lookup

A fresh positive **same live wrapper pointer** between the selected document and
the current creation controller's `library.entryForId` would support the missing
source join under the inspected constructor/cache invariant. A still-live
wrapper retains its backing native entry, preventing that allocation from being
recycled while the wrapper remains alive under this invariant. Different
wrappers with identical IDs remain inconclusive about backing identity.

This is a bounded static argument. All cache writers, cache initialization and
destruction paths, and wrapper backing-member writers were not globally
enumerated. It is not an unconditional proof that stale-cache relationships are
impossible. A future live diagnostic must keep the exact controller, Library,
selected wrapper and selected chain guarded within its observation interval.

The lookup is stateful: even a cache hit touches shared ownership; a miss
allocates a QObject wrapper, installs connections and changes the weak cache.
The inspected normal document path contains no direct known worker-acquisition
or explicit entry-load request. Generic Qt hooks and secondary effects were not
exhaustively analyzed, so this is not a global absence-of-effects claim.

## What remains missing

The inspected paths do not establish a direct `QmlDocumentWrapper` or
`SceneController` pointer to the library `DocumentController`. This is a bounded
negative finding, not proof that no such relationship exists anywhere in the
program. A direct pointer is not required by the observed controller API: it
accepts a document ID and performs a library lookup.

The join under investigation was **current displayed view -> current wrapper/backing
Document -> entry ID -> the fresh controller's Library resolving that ID to the
same backing Document**, within a justified observation interval. Static
construction and lookup paths establish the meaning of that proposed join;
the [first selected-page observation](../experiments/selected-reader-observation-result.md)
established the selected-view half on October 22, but acquired no creation
controller/Library join. The subsequent [stock navigation/join result](../experiments/legacy-composition-navigation-join-result.md)
observed same-live-wrapper agreement for the fresh selected document and
creation Library lookup on the exact RM2 candidate. This closes that sampled
join under the inspected invariant; worker serialization and mutation-time
ownership remain unresolved. Historical R2 addresses
cannot fill those gaps. Equality of document IDs alone would establish a logical
key match, not current object identity, ownership or a lifetime guarantee.

Even a fresh matching relationship would not prove that the selected page key
belongs to the current page order, that worker execution is idle, that pixels
are current, or that a later mutation uses the same source. `rootDocument` text
properties on `SceneController` must not be substituted for library-document
identity. Guard checks and repeated reads do not eliminate ABA reuse or make
cross-object reads atomic.

The optional follow-up diagnostic uses the actual `id` property's owned QVariant
and an exactly matching Qt meta-method parameter type, without reconstructing an
`entry::Id` layout or dispatching through JavaScript. It watches controller
Library replacement/readiness and the selected chain, pins the lookup return
before subsequent permit callbacks, and accepts only the same live wrapper.
It must not change QObject ownership, acquire a worker, wait/retry, or turn
sampled agreement into permission for a later mutation. Native entry preparation
and admission remain separate and unqualified; no additional process-memory
reads or library-table traversal are introduced here.
