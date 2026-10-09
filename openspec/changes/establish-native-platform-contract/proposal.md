## Why

Applications currently mix product behavior with device-specific paths, geometry, input and native-runtime assumptions. Safe automatic page creation needs a reusable, independently qualified hardware boundary; the existence of a Qt method or working simulator cannot establish that boundary.

The October 1 architecture decision commits ReMarkableOpenSDK as an independent project with its own specifications. The October 1 23:40 UTC refinement supersedes September 29's blanket XOVI production exclusion: robust direct/native seams remain preferred, while supervised session-scoped lazy XOVI is an eligible candidate under explicit lifecycle gates. No mechanism is selected by that permission. The first delivery must expose only the capabilities required by native page creation and the current Reader path, with honest unsupported results while research continues.

## What Changes

- Implement reusable ordinary logical gesture mapping for explicitly admitted RM2
  portrait input, consumed by Reader's existing event emitter. Unknown or
  unimplemented tablet/orientation pairs refuse. See [gesture-mapping.md](gesture-mapping.md).

- Propose a separately selected development focus-ancestry discovery domain and
  guarded retained-owner facts handoff; see
  [focus-ancestry-discovery.md](focus-ancestry-discovery.md). This changes discovery
  completeness explicitly, preserves bounds, and selects no native trial.

- Propose version2 finite topology return-site evidence for development owner
  refusal; see [capture-topology-diagnostics.md](capture-topology-diagnostics.md).
  Preserve version1 history, original limits and evaluation counts.

- Propose bounded first-original-evaluation diagnostics for the aggregate
  development capture owner refusal, preserving callback compatibility and all
  existing guards; see [capture-owner-diagnostics.md](capture-owner-diagnostics.md).
  This is evidence-only design, not implementation or another device attempt.

- Propose one default-off development owner/window capture inside the existing
  facts entry, before the consumer's visual-before-facts gate; see
  [capture-observation-plan.md](capture-observation-plan.md). It retains separate
  purpose/admission, one GUI grab and original clocks, without qualifying native
  render freshness or granting facts authority before visual review.

- Propose a private development-only Qt input observer plus one separately
  requested window image, using existing entry lifetime and ownership guards;
  see [input-observation-proposal.md](input-observation-proposal.md). Code and
  target qualification remain unselected.

- Establish SDK-owned OpenSpec and a narrow, versioned semantic contract for device capabilities, page identity, guarded capture/navigation and creation receipts.
- Keep persistent page identity distinct from current runtime/visit ownership and capture evidence.
- Prioritize logical Next/Previous with qualified per-tablet, orientation-aware gestures and verified intended destination. Direct native PageKey opening is deferred, not an MVP prerequisite; preserve its unfinished research and setup findings in [the active navigation plan](logical-navigation-plan.md).
- Require explicit unsupported, stale, canceled and indeterminate outcomes; never equate command acceptance with durable native creation.
- Compare robust direct/native IPC/services with a narrowly supervised XOVI candidate, including actual failure modes inside each abstraction. Qualify the selected mechanism on RM2, with independent Paper Pro qualification; never silently fall back to metadata mutation.
- Keep one mockable contract across RM2/ARMv7 and future Paper Pro/AArch64 adapters; expose per-capability qualification rather than universal support flags.
- Integrate only necessary Buddy seams through a build-time dependency. Consumer conversation binding and its operation journal stay in Buddy.

## Capabilities

### New Capabilities

- `native-platform-contract`: capability discovery, identity/evidence, guarded native operations, qualified creation receipts, adapter and release behavior.

### Modified Capabilities

None. This empty repository has no implemented canonical baseline.

## Impact

Current October7 selection is proposal/preparation for
[maintained launch-and-kill](launch-kill-discriminator.md): unmodified pinned GNU
server launches one disposable process and always kills it after capture or failure.
This supersedes the attach/custom-release mechanism below, preserves its unfinished
work, and requires explicit startup/readiness/diagnostic preparation and actual-kernel
protection qualification. No source implementation or device action is selected here.

The October7 fixed development [normalization discriminator](input-normalization-discriminator.md)
adds a proposed exact-build two-stop debugger measurement for the unresolved RM2
touch-coordinate investigation. It is not a public hook API, coordinate fix or
production runtime dependency. Independent proposal/source/utility/recovery review
and Main-only controlled qualification precede any candidate observation.

The [task-only disposable-server amendment](disposable-debug-server.md) holds the
unmodified GNU14.2 attach path after source review found missing attached-process
EXITKILL and automatic detach on exit. Mandatory checked per-thread protection,
kill-on-failure cleanup and explicit successful release require a narrowly reviewed
GNU-source patch, new artifact identity and actual owned-target failure evidence.

SDK repository gains its own contract and eventual implementation/tests/release artifacts. ReMarkableBuddiesDocs owns the coordinated product delta; ReMarkableBuddies adapts its DeviceBackend boundary and owns any separate Supervisor process within the same Buddy artifact. Manager continues one compatible Buddy installation, not a second SDK runtime or user-managed XOVI prerequisite. SDK owns adapter activation/readiness/invalidation/recovery and logical navigation/device gesture semantics; Buddy owns product trigger interpretation and supervisor scheduling. No cloud sync, firmware update, personal pairing, automatic cold-boot injection, public Buddy admin API or all-at-once platform extraction is included.

Native page creation remains an unpassed research gate. A product-level manual blank-successor fallback is permitted only after the exhaustive investigation required by REM-25; SDK `Unsupported` alone does not satisfy that product gate.

October 8 targeted shutdown research adds a [lifecycle-only discriminator](shutdown-lifecycle-diagnostic.md) after the preserved rendering-path SIGSEGV. It separates FactsEntry participation from native teardown, with a bounded scalar observer and consumer-owned independent failure-guarded recovery. Renewed human authorization permits reviewed fresh diagnostics before the defect is solved; no external reply is required and no spent packet is replayed. This does not qualify a native capability.

The separately selected [pre-token FactsEntry follow-up](pretoken-shutdown-diagnostic.md)
adds only that lifecycle cohort behind a compile-time no-admission fence and a
bounded installation proof. Source preparation and owned fixtures precede exact
consumer/recovery review; active page getters and native execution are not selected.

The [engine-ready refinement](engine-ready-shutdown-diagnostic.md) observes the
original successful bootstrap boundary and requires a later sampled render. It
retains the fences and recovery bounds; source preparation is not native selection.
