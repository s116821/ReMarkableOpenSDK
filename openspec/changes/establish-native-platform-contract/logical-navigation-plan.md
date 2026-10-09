# Logical navigation priority and preserved research

October 3 user direction: swipe is acceptable; expose logical Next/Previous in
the SDK with per-tablet implementations. Direct native PageKey opening is deferred
unless a later concrete requirement needs it. This is an active plan, not native
qualification or authorization for new hardware work.

## Smallest dependency route

1. Establish a qualified current document/page owner, session/visit and ordered
   PageKeys. Preserve pre-dispatch and execution-time input/ownership checks.
2. Perform native insertion only under the existing creation/receipt contract.
   The demonstrated fixture does not waive durable operation correlation or
   production compare-and-act requirements.
3. Observe the current page again against the expected after-order. If it is the
   intended target, do not navigate. If it is the verified source and the exact
   target is adjacent, issue at most one logical Next. Any other state refuses
   continuation; do not infer auto-selection or search by repeated gestures.
4. Require intended native target identity and fresh qualified rendered pixels,
   then consumer binding and a fresh write guard. Native commit or gesture return
   alone grants no rendering authority. Previous uses the same logical contract
   for the opposite neighbor, not an unconditional recovery swipe.

The legitimate insertion changes order and may change the active visit. A fresh
observation must enter through an explicitly qualified acquisition/handoff that
preserves the retained observer, input epoch and runtime/session continuity and
checks the correlated target and exact permitted after-order. Do not silently
repin the old source guard, clear sticky invalidation or relax its order checks.
Lost continuity or uncertain creation attribution requires reconciliation and
cannot authorize the gesture merely because the expected UUID is now visible.

Device adapters own the mapping from logical direction to supported physical
gesture and orientation. RM2 coordinates are not an RM1/Paper Pro contract.
Unknown profiles remain unsupported. Main coordinates the independent RM1 owner;
Sol owns ordinary implementation. This research lane supports only native owner,
mapping and insertion questions required by that abstraction.

## Reuse and limits of current source

SDK checkpoint `f8e729cbdaa1f2776b482e6d12eccf45457aa305` contains
`tools/qt_page_owner.h`: bounded focused-window topology, exactly one compatible
receiver/SceneView pair, ancestry, engine/window/document equality and active
focus. These are development evidence, not general overlay/input isolation.
`tools/qt_page_open.h` crosschecks native document identity, current page aliases,
full page order and both index/key mapping directions. These checks can inform
observation independently of opening a page.

Do not invoke PageOpenSession as an observation-only substitute: its observe
helper requires a callable openPage, begin queues openOnce, its configuration is
fixed to the six-page fixture, and pre-claim page changes invalidate the session.
Reuse requires a separately reviewed observation/expected-transition contract,
including the existing nested-call lifetime and post-getter deadline checks.
Rust Platform still has no qualified native observation/navigation adapter.

Buddy checkpoint `9d5908777ba6d51448519c8d3da077de234f520f` already separates
logical direction, gesture dispatch and completion in
`src/workflow/xochitl_integration.rs` and `src/device/navigation_completion.rs`.
The latter derives the adjacent target from ordered identity, sends one gesture,
and checks fresh identity/order and stable pixels within a five-second completion
bound. It uses a fixed supported image layout and is not general adapter proof.
Preserve request guards and consumer policy when moving device semantics to SDK.
In particular, visually indistinguishable pages cannot be promoted by pixels
alone, and existing persisted metadata candidates are not native UI authority.

## Preserved evidence and unfinished findings

- Native insertion was demonstrated on one backed-up RM2 fixture at
  SDK `97726f3` / Buddy `01213c6`: one new page at index 1, original five pages
  preserved, six-page order persisted through stock restoration and subsequent
  manual reopen. [Insertion evidence](https://linear.app/magentumdragon/issue/REM-25#comment-7d695be1-1baa-4611-8648-26876db57a6e)
  and [independent preservation / manual reopen](https://linear.app/magentumdragon/issue/REM-25#comment-f498c923-e7a1-41c4-ad3b-396891c353c9).
  Whether insertion auto-selected its new page before restoration is UNKNOWN.
- The direct-open source and operator were independently reviewed at SDK
  `f8e729cbdaa1f2776b482e6d12eccf45457aa305` and Buddy
  `53e94cc630d7ba06fcf407a227b7326135e0f15f`. Spent nonce
  `604d9d8e17c046fe84fea4e44e608165` reached component readiness at 3794 ms,
  expired at 20002 ms, accepted no arm token and attempted no native open.
  Restoration and exact cleanup succeeded. This is neither native-open success
  nor an API/source-owner failure verdict. The old five-page creation config and
  all spent packets remain historical and must never be replayed.
- Local ready-to-source capture timestamps span about 50.4 seconds; at most
  16.206 seconds remained after helper readiness in the fixed setup window.
  These are local ordering evidence, not exact remote action timestamps. Negative
  absence checks admitted later setup after the stage was deleted; positive
  stage/current-generation checks are required for any future such setup.
- Buddy guard preparation `9d5908777ba6d51448519c8d3da077de234f520f` is held,
  unintegrated and unfinished. Review reproduced two blocking defects: locked
  admission validation lies outside its timeout; explicit flock unlock releases
  the shared lock while a surviving descendant can still perform a late effect.
  A proposed whole-scope timeout/supervisor repair was not accepted or implemented;
  immediate-supervisor SIGKILL remains unresolved. Do not expand this framework
  for an optional direct-open experiment. Exact deployed helper source binding
  also remains unqualified; host tests cannot qualify those binaries.
- Preserve M3 stash `dc5269ccf6a01d36e174049e6fbc2cb0ae26ece4`
  (`rem25-m3-held-a6253e87`) untouched. It is incomplete and is not a prerequisite
  for logical navigation. Nothing here marks M3, direct opening, runtime recovery
  or the full native platform change complete.

## Prior art and revisit boundary

The [existing research](../../../docs/research/native-platform-ledger.md) and
[mechanism comparison](../../../docs/research/runtime-mechanism-comparison.md) retain pinned community
contracts: rm-librarian existing-engine QML access, Library.entryForId/native
document identity and event-driven readiness; Inkling's dispatch/visual-tree
precedent; and XOVI's loader/lifecycle tradeoffs. They informed discovery, not
blanket runtime or mutation qualification. Saved native metadata informed the
receiver/document/page mapping contract; proprietary bodies remain private.

Revisiting direct opening requires a concrete need beyond the selected gesture
route, fresh source/artifact/operator review, resolved setup/recovery defects,
exact helper provenance if used, a justified finite setup procedure and separate
Main-owned native qualification. No timer extension, new nonce, runtime build or
device action follows from this checkpoint. Runtime/source continuity, durable
insertion correlation and fresh render ownership remain open product-path gates.

Source basis: current-project user steering relayed by Main/coordinator; source
reads at the exact SDK/Buddy revisions above; linked insertion evidence; saved
604d diagnostics/receipt and local reproduction outputs reviewed in this chat.
Private evidence has no public transcript link. Sol independently reported
preserved six-page objects/order after604d; that report is attributed, not a new
hardware observation by this reviewer. Proposed reuse is design inference.
