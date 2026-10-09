# Original topology-bound return-site evidence

Status: proposal only against SDK `117fd0e8ccc4954545b33d91900a6ff0d4eabf20`.
This narrowly amends [capture-owner-diagnostics.md](capture-owner-diagnostics.md).
Main selects later work; Sol owns ordinary implementation/consumer integration;
Astra owns independent SDK/native review. No implementation or device trial is
selected by this document.

## Evidence and remaining ambiguity

The retained e997 diagnostic is version1, 753 bytes, SHA256
`480e593b403993d7ec56d6f8b3f4ec123dac35337f19c0029cda4a5606a7a64c`.
It records owner-discovery/open-topology-bound at visited_items3657, with pairing
not reached. Source inspection excludes visited>4096, but cannot distinguish
node.depth>24 from children.size()>4096-queue.size(). The queue includes already
visited entries; the latter bounds cumulative enqueued items, not just the live
frontier. Candidate counts from an incomplete traversal do not establish owner
absence or ambiguity. Source basis: exact SDK source and independently hash-checked
private retained receipt; the private receipt has no public link.

## Smallest source change

Keep the same default-off capture option and optional diagnostic sink. Keep
open-topology-bound as the discovery result and capture-observation-owner-refused
as the callback stage. Preserve all existing limits, clocks, once-only behavior,
Scope/reentrancy, root/generation/closure checks and publication behavior.

Only record which existing return condition first failed:

1. After the original visited increment, evaluate visited>4096. On failure record
   `visited`; leave depth/queue/child scalars null because those terms were skipped.
2. Otherwise read the existing Node depth into a local once and evaluate depth>24.
   On failure record `depth` and that depth, with queue/child scalars null.
3. After the unchanged structural-candidate and candidate-cap checks, call the
   existing childItems() once. Retain its original size operand and the existing
   queue.size() operand in locals, each evaluated once, and perform the unchanged
   comparison child_count>4096-queue_size. On failure record `queue-cap`, the
   already evaluated depth, queue_size and child_count.

There is no second childItems(), node/parent visit, property getter, signal, clock
sample, progress call or traversal. Record only local scalar values from that
evaluation; never reconstruct them during serialization. Preserve C++ short
circuiting of the visited/depth conditions. On success use the same child list
and original append loop; diagnostic collection cannot change acceptance.
Neither proposed branch selection nor extra evidence permits a larger budget.

The existing queue cap normally prevents visited>4096 from being reached: queue
starts with one item and every append is bounded to4096. Retain the defensive
visited return-site enum without claiming a live reproduction. Do not weaken the
queue guard merely to exercise that branch.

## Exact diagnostic version2 compatibility

The private filename, kind, 8192-byte cap, exclusive0600 writer and four-field
callback remain unchanged. Only the owner-refusal diagnostic moves to version2:
exactly the existing29 field names, with version=2, plus these four fields (33 total):

- `topology_limit`: null or the finite string `visited`, `depth`, `queue-cap`.
- `topology_depth`: null or the original nonnegative integer depth.
- `topology_queue_size`: null or the original nonnegative integer queue size.
- `topology_child_count`: null or the original nonnegative integer child count.

All other version1 fields and their branch/nullability matrix retain their meaning.
New scalars are JSON integers (never booleans/floats), within signed64 range; on
this bounded traversal depth is at most25 and queue size at most4096. No scalar is
clamped or truncated. Invalid/unrepresentable diagnostics stay unknown evidence,
without changing the original refusal or triggering another evaluation.

| Original return | topology_limit | topology_depth | topology_queue_size | topology_child_count |
| --- | --- | --- | --- | --- |
| visited>4096 | visited | null | null | null |
| depth>24 | depth | 25 | null | null |
| child_count>4096-queue_size | queue-cap | Original depth, 0..24 | Original queue size, 1..4096 | Original child count, greater than 4096-queue_size |
| Any other result/branch | null | null | null | null |

Non-null topology_limit requires branch=owner-discovery and
discovery_result=open-topology-bound; that branch/result in version2 requires one
of these complete tuples. visited requires visited_items4097; depth and queue-cap
require visited_items1..4096. For queue-cap require visited_items<=queue_size.
All topology cases retain matched_pairs=null, null pair/active-owner/observer
detail, and null predicate/deadline_check_ms. Counts remain partial observations;
depth refusal occurs before candidate classification of the offending node,
whereas queue-cap refusal occurs after it.

Consumer amendment must explicitly decode version1 as exactly29 fields and
version2 as exactly33, including raw duplicate-key rejection and the corresponding
matrix. Old version1 records remain immutable, valid historical evidence with
topology site unknown. Never infer depth from visited_items alone, synthesize new
fields into saved bytes, or relabel old bytes as version2. Unknown versions and
mixed field sets are undecodable diagnostics; preserve bounded raw bytes/hash and
the original refusal under existing ownership rules. A consumer supporting only
version1 is not selected for a version2 artifact. Capture completion version1,
facts, request tokens and callback schemas do not change.

## Review and verification before later selection

Main and Sol review this exact amendment and the coordinated consumer plan before
implementation. Owned fixtures cover depth25 and queue-cap with visited<4096,
valid depth24 and cumulative queue size4096 boundaries, exact33-field/null tuples,
and first failure unchanged by later state. Compare sink/no-sink getter, progress
and traversal counts. Preserve the original42-case behavior, including held
callbacks under reentrancy. Validate the defensive visited tuple synthetically at
the scalar encoding/decoding seam; do not claim it is reachable through ordinary
bounded traversal. Cover version1 readback unchanged, malformed/mixed versions,
foreign/partial output and closure/generation-loss preservation.

Source acceptance, consumer acceptance, exact artifact/packet review and native
selection remain separate. No bound relaxation, owner fallback, extra input,
retry, new trial or native/render qualification follows from this proposal.
