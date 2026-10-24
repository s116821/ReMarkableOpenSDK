# Tablet simulator: evidence and fidelity map

October 23, 2026. This is a searchable handoff over existing findings, not a new
copy of their results or a claim of a complete tablet emulator. Findings remain
in their linked reports; implementation progress belongs to the owning task/PR.
The current native profile is RM2 firmware 3.28.0.172 and the exact recorded
xochitl/Qt build. Another model/firmware needs its own calibration.

## Start here

1. [SDK knowledge index](../README.md) groups the accepted research and experiment
   reports. [Research](../research/) includes mechanisms and remaining unknowns;
   [experiments](../experiments/) preserves successful and failed observations.
2. The [complete derived catalog](research-evidence-catalog.json) lists **every**
   Markdown file under SDK `docs/` and `openspec/`, including archived changes,
   with path, heading/line navigation and LF-normalized source digest. It adds no
   success/coverage classification. It covers the versioned SDK knowledge corpus,
   not private tablet documents, raw binaries or every ecosystem repository.
3. Run `python tools/research_evidence_catalog.py --query "pageAdded"` for literal
   full-text discovery without an external search service. Other useful queries:
   `remembered`, `lexical`, `reconciliation`, `deadline`, `callback`, `retirement`,
   `tile`, `restore`, `pen`, and `unknown`. Use `rg -n -i "term" docs openspec`
   for exact source lines. Regenerate with `--write`; verify with `--check`.
4. Follow dated later-result pointers. Do not read an old "open" paragraph as
   the latest disposition, or turn a later pass into a pass for an earlier failure.
   Exact private evidence locations/hashes remain with their owning investigation;
   do not export user page images, credentials, vendor binaries or core dumps.

## Three existing layers, with different purposes

| Layer | Existing entry points | What it establishes |
| --- | --- | --- |
| Buddy application simulator | [Simulator guide](https://github.com/s116821/ReMarkableBuddies/blob/fe7463993b93dedaa57fc80a7fd3f45adc5e084f/docs/simulator.md), [scenario model](https://github.com/s116821/ReMarkableBuddies/blob/fe7463993b93dedaa57fc80a7fd3f45adc5e084f/src/simulator/scenario.rs), device/raster modules and simulator tests in the same directory/tree | Runs real application orchestration against a simulated device. This inspected checkpoint's scenario mode is Reader-only; the broad roadmap is not already implemented by that harness. It is not xochitl, Qt or kernel emulation. |
| SDK synthetic contract model | [mock.rs](../../src/mock.rs), [model coverage inventory](model-task-coverage.md), `tests/capture.rs` | Already models identity scopes, lost replies/reconciliation, dispatch/execution races, navigation faults, and creation that either retains the source or selects the target. Synthetic receipts stay synthetic. Do not reimplement these as assumed gaps. |
| Owned Qt/ARM fixtures | `tools/*_fixture.cpp`, corresponding test scripts and [experiment reports](../experiments/) | Exercise original owned QML/metaobjects, callback/thread/lifetime behavior, files and compiled ARM ABI. Native acquisition/hook effects may be explicitly modeled. These focused tests do not automatically update the Buddy simulator or reproduce vendor internals. |

## Finding to model and coverage map

The requested model behaviors below are engineering implications, not observed
implementation. "Existing" refers only to the cited bounded coverage; integrated
Buddy simulation is not inferred from a nearby SDK fixture.

| Topic/search terms | Primary evidence | Existing model/control and remaining calibration |
| --- | --- | --- |
| Active versus remembered page, creation, `pageAdded`, Next | [Task/selection trace](../research/insertion-worker-turn-and-stock-selection.md), [conditional creation](../experiments/conditional-insertion-native-result.md), [actual target acquisition](../experiments/conditional-target-acquisition-native-result.md) | The SDK synthetic model already has retain-source/select-target choices. Preserve separate remembered and displayed selection plus explicit adjacent navigation in integration scenarios; do not make creation always select the new page. |
| Current reference, lexical root, visible ownership, stale identity | [Current-reference discovery](../research/reader-current-reference-discovery.md), [scene wiring](../research/reader-scene-selection-wiring.md), [selected-page observation](../experiments/selected-reader-observation-result.md) | Owned discovery fixtures cover portions of selection/lifetime. Integrated simulator needs distinct infrastructure-ready, document-open, selected-current and rendered-ready states, including a missing/changed root and prefetched views. |
| Worker enqueue, matching job, callback return, cancellation | [Worker contract](../research/native-worker-callback-contract.md), [worker delivery](../experiments/native-worker-callback-result.md), [resident retirement design](../../openspec/changes/establish-native-platform-contract/resident-request-retirement.md) | SDK models ambiguity; Qt witnesses exercise copied callbacks and queued completion. Model independent submit-return/callback/completion/observer-retirement transitions. Canceling an observer cannot undo a committed mutation or authorize duplicate creation. |
| Request completion versus resource retirement, late callbacks, nested loops | [Resident read-only result](../experiments/resident-readonly-pair-result.md), [actual mutation Entry owned result](../experiments/resident-mutation-transport-owned-result.md), [one native mutation/readback](../experiments/resident-mutation-readback-native-result.md), [optional mutation scopes](../../openspec/changes/establish-native-platform-contract/mutation-request-retirement.md) | Fence/scope/coordinator/Entry fixtures exercise owned GUI stacks, QObjects, request descriptors, A cleanup then fresh read-only B under the original clock, and unresolved mutation refusal. One native idle-fixture A/B now matches that sequence; general repeated mutation and native-owner reclamation remain open. Integrated simulation must retain resident adapter lifetime while retiring per-request resources and rejecting stale generations. |
| Insertion source/order race, target collision, unknown effect, duplicate avoidance | [Conditional controls](../experiments/conditional-insertion-owned-result.md), [actual creation](../experiments/conditional-insertion-native-result.md), [SDK ambiguity contract](../../openspec/changes/establish-native-platform-contract/specs/native-platform-contract/spec.md) | SDK already covers execution races, canceled/no-effect operations and lost-reply reconciliation. Add calibrated event sequences rather than weakening uncertainty into success or unconditional retry. Other-writer exclusion is still a native qualification gap. |
| Loading/blank/useful pixels, image identity and physical freshness | [Useful native capture and distinct failed attempts](../experiments/identity-capture-native-result.md), [capture readback](../experiments/identity-capture-readback-owned-result.md), [frame applicability](../research/vendor-frame-signal-applicability.md) | Synthetic capture tests already reject stale owner/buffer/input. Add loading, toolbar-only/blank and useful-content fixtures with separate logical identity and pixel readiness. A valid PNG or frame signal is not proof of current content or physical panel freshness. |
| Original deadline, slow transport, partial output and unknown collection | [Native capture collection protocol](../experiments/identity-capture-native-result.md), [original-byte readback](../experiments/identity-capture-readback-owned-result.md) | Dedicated collectors have owned refusal controls. Simulate work/transport cost against one monotonic absolute budget; complete-looking bytes plus timeout/post-check failure remain failed live collection. Recorded durations are samples, not universal timeout bounds. |
| Native file persistence, PDF mapping, original ink and unknown fields | [Caller-selected writing/reopen](../experiments/caller-selected-insertion-result.md), [target acquisition/preservation](../experiments/conditional-target-acquisition-native-result.md), [ordinary close source boundary](../research/ordinary-reader-close-source-boundary.md) | Historical stroke/persistence and current creation/capture results are separate observations. Model deferred save/close/reopen and stable page IDs/order/PDF mapping. Stock QML close removes current/prefetched scene references before documentClosed; that signal is not a storage fence. Full intended-target write/reopen is still open. Use original synthetic documents, preserving unknown fields in contract tests. |
| Pen coordinates, contact release, partial frames | [Gesture component](../experiments/rm2-portrait-gesture-component.md), [pen release and vendor ABI](../research/pen-event-release-boundary.md), [fixed-stroke owned controls](../experiments/fixed-stroke-release-owned-result.md) | Vendor ABI remains compiler/emulator-only. New plain policy covers one-use dispatch, partial writes, original clocks and one serialized parent recovery holder; actual pipe/child controls do not qualify evdev release. Model dispatch/partial-write/release independently of drawing success. Device-FD close or helper death must not silently synthesize proven pen-up. Full kernel/input-device behavior is outside the product harness. |
| Document-close dispatch, UI closure and storage completion | [Stock source boundary](../research/ordinary-reader-close-source-boundary.md), [terminal owned helper](../experiments/ordinary-reader-close-owned-result.md) | Actual owned QML method controls now cover claim reuse, final metadata drift, deletion and overrun. Native joins are modeled. Simulate a dispatched close separately from observed closure, persisted file changes and fresh reopen; documentClosed must not automatically flush every model store. |
| Activation, stock restore, process generation, shutdown fault | [Mechanism comparison](../research/runtime-mechanism-comparison.md), [shutdown comparison](../research/shutdown-entrypoint-comparison.md), [native capture recovery](../experiments/identity-capture-native-result.md) | Simulate explicit activation, expired generation, supervisor/adapter failure and independent recovery outcomes. A logical cleanup pass cannot model away the historical rendering invalid read, prove safe unload or imply a stock cold-boot qualification. |

## Turning another finding into a simulator case

For each new behavior, attach an exact public evidence/source revision and its
model/firmware/profile; label native-observed, owned-only, inferred or unknown.
Represent the observed event order and independently inject alternate schedules,
late/doubled/lost messages and interruption boundaries. Preserve mismatches as
regressions instead of adjusting expected output until a happy path passes.

Prefer one scenario format shared by the same device-facing application boundary,
with explicit capabilities and deterministic scheduling, rather than a second
implementation of Buddy policy. Compare the trace's identities, dispatch counts,
outcomes and preservation invariants to the original evidence. Use separate
coverage columns for SDK contract tests, Qt fixtures, Buddy simulation and native
calibration. A fixture pass must not fill the other columns automatically.

Preserve abstraction boundaries: the Buddy simulator's intentional
end-of-document `NoSuccessor` behavior does not become stock `moveForward()`'s
possible page-creation branch. Test raw stock behavior at the adapter boundary;
the higher-level logical navigation contract may deliberately refuse it.

Prioritize creation/selection/persistence, unknown-effect reconciliation and
resident request transitions first. Broader keyboard/Writer/conversation/UI and
installation scenarios belong to the product owners and central product specs.
Retain targeted real-device checks for rendering, input delivery, vendor
ownership/ABI, recovery and new firmware even after broad isolated tests exist.

Source basis: inspected current SDK source/reports and the pinned Buddy simulator
checkpoint above. Proposed calibration work is explicitly prospective. This map
and generated catalog preserve pointers; the linked findings remain authoritative.
