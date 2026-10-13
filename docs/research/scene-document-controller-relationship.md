# SceneView, document wrappers and the library controller

Status: October 13, 2026 exact-build static interoperability findings. This is
preparation for a separately justified source-identity follow-up, not an observer
implementation or native qualification. The accepted `passive-visual-sftp-v1`
route, manifest, caps and first measurement scope at `91f0973` remain unchanged.

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

The setter returns early when the proposed wrapper has the same backing document
as the view's retained document. Thus wrapper replacement and document identity
are not interchangeable observations. Shared ownership inside the application
also does not pin anything for an external reader.

The library resolver has special branches as well as an ordinary keyed lookup.
An empty ID follows a special branch; it must not be treated as a valid displayed
document ID or silently repaired into one. The ordinary lookup compares key
length and contents before returning the retained entry. None of these paths is
authorization to call the resolver, acquire a worker or invoke insertion.

## What remains missing

The inspected paths do not establish a direct `QmlDocumentWrapper` or
`SceneController` pointer to the library `DocumentController`. This is a bounded
negative finding, not proof that no such relationship exists anywhere in the
program. A direct pointer is not required by the observed controller API: it
accepts a document ID and performs a library lookup.

The unresolved join is **current displayed view -> current wrapper/backing
Document -> entry ID -> the fresh controller's Library resolving that ID to the
same backing Document**, within a justified observation interval. Static
construction and lookup paths establish the meaning of that proposed join;
this investigation supplies no live values for it. Historical R2 addresses
cannot fill the gap. Equality of document IDs alone would establish a logical
key match, not current object identity, ownership or a lifetime guarantee.

Even a fresh matching relationship would not prove that the selected page key
belongs to the current page order, that worker execution is idle, that pixels
are current, or that a later mutation uses the same source. `rootDocument` text
properties on `SceneController` must not be substituted for library-document
identity. Guard checks and repeated reads do not eliminate ABA reuse or make
cross-object reads atomic.

A later proposal would need to justify its lifetime/coherence and ID-validation
rules and its exact bounded access mechanism. It must not call the property
getters under a passive-read claim: the inherited ID and page-ID paths copy
shared data, while worker acquisition can create/start work. No additional
process-memory reads, library-table traversal, API/DTO, native payload or device
operation is introduced by these findings.
