# Explicit-fixture caller-selected insertion proposal

Task 2.3 follow-up. The [research selection and outcome table](../../../docs/research/client-selected-insertion-discriminator.md)
are the canonical rationale. This proposal adds a narrowly configured one-call
allocation experiment to the original creation Probe, not the held FactsEntry or
shutdown diagnostic. Native capabilities remain Unsupported.

## Source change

Add an explicit optional target UUID to CreationConfig. The existing omitted-target
five-argument mode retains its behavior. The new explicit mode requires a canonical
nonnil target different from the document and every baseline page, and the exact
fresh six-page baseline for the selected existing fixture. Serialize values through
the existing JSON construction. Check all forward/reverse baseline IDs and target
absence before mutation claim, then pass target as argument six exactly once.
Keep the original index, template, size, callback, engine/thread/lifetime guards,
deadline and no-replay semantics. Do not introduce a generic executor or public
native receipt constructor. Private fixture values stay outside Git.

## Focused checks and remaining work

- [ ] Review exact source amendment with Main against the original successful path.
- [x] Verify the explicit mode passes all six arguments unchanged in one call;
  omitted-target behavior remains five arguments; invalid/duplicate target and
  stale baseline refuse before claim. Reuse the existing owned QML fixture.
- [ ] Freeze exact source/artifact/configuration with Main and confirm applicable
  independent restoration without replaying any rejected operation.
- [ ] Main selects at most one new actual call; preserve order, PDF/ink/unknown
  metadata and independently read back allocation/persistence after restoration.

No fresh target build, private packet or device action is selected yet. The owning
change remains unfinished; canonical sync/archive and full native qualification
are not completed by this proposed test or a component success.

## Author source checkpoint

The implementation changes only `CreationConfig` validation and the existing
creation helper's optional sixth argument in `qt_qml_access_probe_core.h`.
Its default empty target retains the original five-page/five-argument mode.
Explicit target mode requires the six-page baseline and development fixture mode.
No FactsEntry, lifecycle recorder, native opening or receipt API is changed.

`tools/qt_client_selected_creation_test.sh` compiles only the existing owned QML
creation fixture. Six focused ARM/QEMU cases passed in immutable vendor image
`416c7a7be0038156797b0892f031f352b841d1921fae83f712d0a272e4724618`, network disabled
and source mounted read-only: invalid-target/config controls, exact sixth argument,
stale count, stale order, legacy five-argument behavior and one-call replay refusal.
No preload/shared-object or selected target artifact was built by that test.

## Build separation and recovery issue

The production creation entry remains `qt_qml_access_probe.cpp` with
`QT_PROBE_CREATION_CONFIG`, plus its creation bridge/MOC. Its current shared source
also links the preexisting `qt_page_open.cpp`/MOC; page-open configuration is absent
and mutually exclusive with creation. Those unchanged dependencies do not select
native opening. The entry/creation closure has no `qt_page_facts_startup.cpp`,
`qt_page_facts_entry.h`, `qt_shutdown_trace_startup.cpp` or engine-ready diagnostic
macro. The rejected diagnostic's entry and behavior are not compiled into this
creation path. Exact dependency/artifact hashing remains necessary at any later
selected build; no new artifact identity is claimed from this source comparison.

Main found that the later read-only R5 restoration actor checks `baseline.files`
before stock start and again before restored confirmation. Its previous packet
included exact fixture content/metadata prehashes, which a legitimate insertion
changes. That packet must not be reused: it would refuse before stock start.
Main is checking existing preparation constraints for an immutable recovery list
plus complete fixture backup/semantic postchecks. Until the applicable recovery
contract is established, no native packet is selected. This is a concrete operator
mismatch, not evidence against caller-selected allocation or a reason to repackage
the rejected stock-shutdown preparation.

Main subsequently confirmed the preparation tool requires all 24 protected paths
and supports only named diagnostic proof modes: this cannot be solved by changing
packet data. Root selected a minimum new insertion-specific source contract.
Retain the complete fresh inventory and preactivation equality checks. Only the
selected fixture's exact content/metadata paths may change as declared; original
PDF/ink comparisons still produce a preservation result. Document comparison
failure must be recorded without short-circuiting restoration of the independently
verified stock executable/policy. Process, owned-shadow, cgroup and service safety
checks remain mandatory. Missing output, crash or host loss still leads to the
existing independent bounded restoration, with no retry and an uncertain operation.
An explicit insertion mode uses its own bounded completion request/deadline, not a
renamed diagnostic trace proof. Main owns that consumer source and exact review;
no rejected command or historical packet is repurposed by this contract.
