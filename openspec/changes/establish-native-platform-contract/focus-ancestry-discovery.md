# Development discovery from a complete focus ancestry

Status: proposal only against SDK71fe829 (owner implementation0e78e2a).
Main and Sol must independently accept this contract and the owning consumer
amendment before ordinary Sol implementation. Astra reviews exact SDK source
afterward. No new trial, artifact, tablet action or native capability is selected.
Reusable evidence and pinned Qt6.10.3 source reasoning live in
[the technical finding](../../../docs/research/qt-focus-owner-discovery.md).

## Explicit contract change

9453 refused the full-window cumulative queue cap before owner matching. Replace
that discovery domain only in a separately selected default-off development mode:
`developmentFocusAncestry=false`, valid only with developmentCaptureObservation,
setup120000/facts5000, and the existing main-dev-facts-120s profile. Exact source,
configuration and artifact manifests must bind the mode. Never select it after a
failed BFS; existing unselected callers retain their current behavior.

The new completeness domain is one stable, complete active-focus leaf-to-retained
content-root ancestry, not the entire window tree. Preserve numeric depth24,
total-item4096 and candidate8-per-role ceilings, original one-shot clocks, root/
generation/closure/input guards, document identity, activeOwner and unique-pair
acceptance. A complete chain of at most24 edges contains at most25 items, so the
4096 ceiling is preserved but not an allowance to grow the chain. The stronger
leaf-depth rule can refuse a case whose scene alone was formerly within depth24.
Do not claim old and new acceptance sets are identical.

## Preconditions and one discovery walk

Use the same current focused, active, visible QQuickWindow and retained engine.
Retain contentItem and activeFocusItem as QPointers. Require same GUI thread,
nonnull root/leaf and window association; do not set focus. The conditional proof
in the technical note covers ordinary stable window focus semantics. Pinned vendor
Qt owned-fixture evidence must support these semantics before artifact selection;
unknown firmware, unsupported subscene ownership, or incomplete root reachability
refuses. Public upstream source alone is not vendor-runtime qualification.

Install bounded lifetime/parent/window/focus invalidation for each retained chain
item before reading its next parent; observe window activeFocusItemChanged before
sampling the anchor and retain sticky change-away-and-back invalidation through
capture and final facts delivery. Use the existing entry event filter for input.
Callbacks only mark invalidation; no getter, I/O or traversal in signal handlers.
Connection/lifetime/context failure refuses. Check the existing progress guard
at each step and after any getter; all time counts against the original deadline.

Walk parentItem once per edge, from retained leaf to the exact retained content
root. Cache QPointers and edges. Reject repeated pointers/cycles, null before root,
wrong window/thread, root replacement, anchor change or any observed invalidation.
At24 edges, absence of the root refuses without following edge25. Accept no partial
chain and do not choose a receiver before root reachability is established.
No sibling/childItems scan, alternate window/root, forceFocus, wait, retry, second
discovery walk or fallback is allowed. Required existing activeOwner ancestry
checks remain unchanged; they are validation, not a second candidate-discovery
walk. Reusing the retained pointer array for classification is not a live rescan.

Once complete, classify cached items with the existing exact engine, focus-scope,
property-type and SceneView-inheritance/signal rules. Retain candidate caps8 and
all activeOwner predicates (including its strict ancestor/document equality).
Enumerate every retained candidate pair in deterministic root-to-leaf order; the
first passing pair is provisional until all pairs finish. Zero matches refuses;
second match refuses ambiguity. No nearest-receiver or first-scene preference.
Lifetime/parent/focus invalidation during classification or document getters
refuses without rebuilding the chain. Validate the same retained anchor/root
at boundaries and keep sticky observers, since matching endpoint pointers alone
cannot establish continuity. No extra document/page mapping before visual review.

## Later facts must use the same guarded owner

Current PageFactsSession::begin independently calls findPageOwner, so changing
only pre-capture discovery would reproduce the full-tree cap after visual review.
For this selected mode only, add a narrowly scoped internal retained-owner path:
FactsEntry supplies its already selected PageOwner and a lifetime-bound validation
callback/ticket backed by the still-live ancestry guard. This is not a public
caller-supplied owner shortcut and cannot outlive or detach from FactsEntry.

After the unchanged visual-before-facts request, the facts session validates that
ticket, context, unchanged anchor/root, sticky guard and activeOwner; installs its
existing required facts observers and performs all existing identity/order reads.
It does not discover candidates again, switch owner, reset capture/facts/host
clocks or turn the capture expectedOrder into observed order. Failure refuses.
The old session constructor/path retains BFS behavior for all other callers.
All existing per-getter, reentrant Scope, epoch, final delivery and capture-binding
checks remain required. Shared ownership/lifetime plumbing and teardown order
must be independently reviewed; there is no retained-owner continuation without
the original guard. Native/render/atomic authority remains false.

## Evidence and schema compatibility before implementation

Do not label ancestry counts as full-window BFS counts or silently reuse v2
topology_limit for ancestry exhaustion. For the selected mode only, propose
owner-refusal diagnostic version3: existing33 field names plus `discovery_scope`,
`chain_items`, `chain_complete`, `chain_failure` (37 total). Scope is the fixed
string `window-focus-ancestry-v1`. Old modes keep their version2 behavior; historical
version1/2 bytes and strict decoders remain unchanged. Callback4 fields, capture
completion36 fields and facts23 fields stay unchanged; exact source/config/packet
bindings identify the discovery scope for successful records.

Chain fields: items is null before discovery initializes, otherwise0..25;
complete is false until the retained root is reached, then true; failure is null
or a finite label `anchor-unavailable`, `item-context`, `root-unreached`,
`depth-bound`, `cycle`, `anchor-changed`, `guard-invalidated`, `observer-unavailable`.
Failures during chain construction use branch owner-discovery and discovery_result
`open-focus-chain-refused`; predicate remains null except an original failed
progress subcondition, which retains open-context-lost and its original predicate.
No topology fields are populated for ancestry records. Before a complete chain,
candidate/match/pair/active-owner detail is null. On complete-chain classification,
visited_items means classified retained-chain items, with existing candidate/
match/first-pair semantics and existing non-chain discovery result enums. A chain
observer failure uses chain_failure, not the document/scene observer tuple.

The selected version3 matrix is normative:

| Original failure | chain_items / complete / failure | Other diagnostic fields |
| --- | --- | --- |
| initial-progress | null / false / null | Existing initial-progress matrix, all discovery counters null |
| chain construction failure | 0..25 / false / exact chain label | owner-discovery/open-focus-chain-refused, null predicate/deadline operand and all classification/matching/observer detail |
| failed original progress during construction | 0..25 / false / null | owner-discovery/open-context-lost, original predicate/deadline operand; all classification/matching detail null |
| original classification or matching failure | retained count1..25 / true / null | Existing owner-discovery matrix with reached classification counters, original return enum and original failed progress or final activeOwner detail only |
| capture observer installation failure | retained count1..25 / true / null | observer-install/open-owner-observed and existing fixed observer tuple; classification counters and pair detail null |
| final owner revalidation failure | retained count1..25 / true / null | owner-revalidation/open-owner-observed and original allowed predicate/active-owner detail; classification counters and pair detail null |

Initialize chain_items=0 on entering discovery. Append/count an item only after
nonnull/thread/window/cycle checks; an observer failure after append retains that
count. Mark complete only after the root item and its required observers pass.
Before classification, visited_items and candidate/match counters remain null;
initialize classification counts to0, increment visited_items for each cached
item classified (maximum chain_items), and initialize matched_pairs only when
pairing begins. First-pair ordinals refer to root-to-leaf classified candidates,
not BFS positions. Preserve null versus evaluated0. A failure after completion
never masquerades as a construction failure: sticky chain/anchor loss then uses
open-context-lost with the original progress predicate, or the existing later
capture/facts continuation refusal stage, as appropriate. Do not reset complete.
All four version2 topology fields are always null in version3, including when
chain_failure=depth-bound; depth24 here is an explicitly separate ancestry bound.

Initial-progress refusal has null chain_items, complete=false, failure=null;
observer-install/revalidation preserve complete=true and retained chain_items,
while old branch-nullability rules clear unrelated candidate detail. Chain change
after discovery is sticky continuation refusal and cannot overwrite earlier
latched evidence. Consumer must reject unknown/mixed version fields, inconsistent
chain/branch tuples and raw duplicate keys while preserving bounded raw failure
bytes under existing ownership/cleanup rules. No addresses/object names/tree dumps.
Sol/Main must settle the full finite version3 decoder matrix and retained-ticket
API together before implementation, including how post-discovery guard refusal
maps to existing callback stages. This proposal grants no partial wire rollout.

## Required proof and fixtures

Pinned Qt6.10.3 source demonstrates ordinary nested-focus ancestry, not atomic
native focus state. Before code selection, review that conditional completeness
argument and the retained-ticket design. Before artifact selection, execute owned
fixtures under the exact vendor Qt runtime and independently inspect results:

- Leaf SceneView and nested focus-scope SceneView ancestors; multiple nested
  receiver candidates; two valid pairs must refuse rather than choose nearest.
- Pooled sibling SceneViews and a tree exceeding4096 unrelated items: chain
  discovery touches no sibling and still requires all original owner predicates.
- Overlay steals focus, popup changes focusWindow, no active leaf, root-only leaf,
  wrong engine/window/thread, detached/subscene root, cycle hook and deleted item.
- Complete root at edge24 succeeds; root beyond24 refuses without following edge25.
  Candidate9 refuses; full-chain enumeration/getter counts remain bounded.
- Focus/reparent/window/lifetime changes during every getter boundary, including
  away-and-back and nested event delivery; retained guards must refuse and defer
  teardown. No final pointer-equality substitution for sticky invalidation.
- Later facts receives the identical retained owner without another discovery;
  invalid ticket, dropped entry, changed focus or stale capture cannot read facts.
  Ordinary facts/input modes retain their existing behavior and fixture coverage.
- Strict historical v1/v2 plus selected v3 evidence, original callback compatibility,
  partial/foreign output and restoration/preservation failure paths.

Native leaf depth, whether SceneView is leaf or ancestor, pooled/overlay topology,
subscene behavior and focus transitions remain unknown. Owned fixtures cannot
establish native page readiness, physical-panel pixels or navigation. Main alone
may later select an exact reviewed packet;9453 stays spent with its original
queue-cap refusal, and e997's topology site stays unknown. No canonical sync/archive
or completion claim for native tasks2.9/4.11 follows.
