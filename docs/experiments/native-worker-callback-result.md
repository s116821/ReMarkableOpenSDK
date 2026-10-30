# One native worker callback delivery result

October 22, 2026 (Chicago; October 23 UTC): one exact RM2 candidate opened the
existing fixture, qualified its selected-page/creation-source relationship and
submitted one nonmutating callback to the associated native DocumentWorker.
The callback entered once off the GUI thread and the queued observer matched
its returned job ID to native post-return completion. Native owner release,
stock restoration and complete fixture preservation also passed.

This qualifies that diagnostic delivery path on this exact candidate. It does
not qualify insertion, execution-time source/order exclusion, post-callback
Reader readiness, rendering, durability, safe unload or other firmware/models.

| Input / observation | Value |
| --- | --- |
| SDK source | `18500c6dfa2279af1f871d417f8a019099f1416e` |
| Consumed attempt | `367219f45110476f95160e976e15c3b7` |
| Executable SHA-256 | `071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df` |
| Payload SHA-256 | `fd3ba34b1da694a3d479d62ca9bf35d1624f229de4af163e2f7bd2e71790039d` |
| Preparation SHA-256 | `ead2adf3bfef974068e5ca85cbb2e402d63b88f730dd40888cbc4ff7d1e42c4c` |
| Candidate PID / start | `15902` / `124275836` |
| Diagnostic / stage | `worker-callback-witness` / `worker-witness-observed` |
| Page observation phase | `before-worker-submit` |
| Navigation scope / dispatched | `legacy-main-composition` / true |
| Pre-submission document / page / creation join | Expected document and fifth page, index 4/count 8; matched |
| Pre-submission map / ready | Both true; status 2/page status 8 |
| Pre-submission loading / errors / tiles / working | All false |
| Native preparation / attempt / acceptance / submission returned | All true |
| Job ID / matching completion | `26` / true |
| Entered / returning / off-GUI / matching-completion counts | 1 / 1 / 1 / 1 |
| Canceled entries / canceled / native unknown | 0 / false / false |
| Worker finished / delivery observed / owners released | All true |
| Observation / worker refusal | Both empty |
| Native / render / durable authority | All false |
| Entry begin / end | `33871` / `44563` ms; 10692 ms |

## Mechanism and timing

The [reviewed contract](../research/native-worker-callback-contract.md) ties the
actual selected document ID to native strong Document ownership, acquires only
its already-existing worker and requires equality with the qualified public
SceneController worker. The exact-image loader checks the executable, mapped
function bytes, native type interfaces and machine ABI before those calls.

The closure captures shared plain state and an immutable thread ID only. It
does not inspect Qt/QML objects, read document order, create a page, write files
or call the GUI from the worker. Its `returning` marker is a tail update, not an
after-return assertion. The separate matching signal corroborates return under
the inspected native dispatcher/loop contract. The ID is local to this worker;
26 is not a reusable/global identifier or a success token for another operation.

The selected page/readiness facts precede submission. They are not a second
observation after callback completion. The measured 10692 ms includes normal
navigation, its queued five-second opportunity, source preparation, exact-image
checks and the wait for callback completion. The end marker precedes final
native-owner release and result publication; the Entry separately checks the
original deadline after release. This is not one continuous GUI-thread pause.

The fixed thirty-second host startup opportunity did not renew the original
ninety-second setup or twenty-second cooperative operation cap. Publication had
55 seconds left in the setup window. The independent actor retained its
original 360-second deadline and 185-second recovery reserve. Exactly one
request, callback submission and guarded stop occurred; no action was replayed.

## Preservation and recovery

The guarded stop recorded primary exit zero and restored stock PID `17021`,
start `124282332`. A fresh readback found stock executable identity, all three
services active/running with zero restart counts and no pending jobs, trial
mappings or runtime service shadows. All eighteen fixture files, including
content/metadata, and all twelve provider/service entries were unchanged.

The complete fixture backup remains 733696 bytes, SHA-256
`3f22dd7853c44fd54dc11ac9224d3e27639069860193b4345bfa9aaf7ec255cf`.
The saved runtime archive is 467968 bytes, SHA-256
`f60a723de88b5cd2db7300de033de1d0bee6e98aa09f2710466560e40f2d19c9`.
Only the exact archived, inactive test root was removed from tablet RAM.

An independent saved-evidence audit verified 141 raw streams, the exact frozen
validator/wire, all twenty archived package files, original windows, complete
fixture bytes/provider hashes, normalized restored policy and guarded cleanup.
Six expected pending read-only polls returned nonzero and carry generic
transport `unknown` flags; these are not unknown submissions or permission to
retry. No unknown mutation/publication outcome was found.

The bridge stayed loaded until process restoration. Successful owner release
and this recovery do not prove native queue cancellation, safe library unload,
all shutdown paths, or repair of the historical rendering-related failure.

## Remaining work

Callback delivery gives a way to investigate work on the native worker thread.
It does not turn ordinary AddPage into an atomic operation: that request still
enqueues a later command. A direct handler candidate also has dependencies and
ordinary Task bookkeeping that need separate review. Same-turn source/order
validation, GUI visit ownership, matching inserted identity/selection,
persistence and complete resident/failure lifecycle qualification remain open.

Source basis: exact committed source and rebuilt payload, independently reviewed
package/operator, native scalar receipt and runtime archive, and fresh full
backup/provider/process readback. Static contract claims have separately
attributed REA/Ghidra and ARM ABI evidence; no proprietary bodies are published.
