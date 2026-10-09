# Experimental model: tasks 3.1 and 3.2 reconciliation

**Later October 8 update:** Root selected the independently useful SDK query despite no current Buddy caller. The [bounded capability report](../../openspec/changes/establish-native-platform-contract/capability-report.md) now implements observational contract/profile revisions and per-operation unsupported or synthetic status. Native qualified profiles and release manifests remain absent. The inventory below preserves the pre-implementation finding; it does not describe that query as still missing after this increment.

October 8, 2026; source reviewed at `316fd22` (Rust sources unchanged from its parent `4d98ebd`). This is a bounded source/assertion inventory, not a fresh test run, native qualification or full delivery acceptance. Existing accepted model/capture/evidence reviews remain recorded in [implementation checkpoints](../../openspec/changes/establish-native-platform-contract/implementation.md).

## Task 3.1: implemented types, unfinished capability/profile boundary

`src/lib.rs` already implements validated UUID/PageKey identity, separate opaque device/adapter/session/visit/revision/input scope, immutable creation requests, synthetic evidence, historical receipts, unsupported and unknown-identity distinctions, cancellation, proven no-mutation rejection and indeterminate outcomes. `src/capture.rs` and `src/navigation.rs` add typed capture/navigation requests and results. `src/evidence.rs` supplies bounded one-way historical facts under schema `remarkable-open-sdk/evidence-facts/1`; those facts cannot be reconstructed as live operational guards. The optional `development_capture` module retains explicitly unqualified historical capture correspondence.

The proposed `capabilities`/DeviceProfile/per-operation CapabilityStatus API and versioned runtime qualification/release manifest are **not implemented**. The historical evidence schema is not that capability contract. Design sections “Contract shape and versioning” and “Proposed narrow public operations” already describe this remaining scope; no new public API is selected here.

Main's current consumer feedback: selected historical capture retrieval uses existing typed evidence classification and stored bytes. It needs no new SDK query or fields. Production ReaderContext still requires a qualified verifier and actual adapter bootstrap; a synthetic capability producer cannot supply either. A later profile/status consumer may justify the query, but no current consuming seam was identified. Task 3.1 therefore remains unfinished without speculative implementation.

## Task 3.2: named deterministic cases already present

| Named behavior | Existing implementation/assertions |
| --- | --- |
| Input/order change between dispatch and execution | `mock::tests::dispatch_execution_race_never_inserts` injects ExternalInput/ReverseOrder and requires executor rejection with no inserted page |
| Two devices sharing UUIDs; recreated adapter | `foreign_device_and_recreated_instance_reject_coincident_ids` requires ForeignScope before dispatch and unchanged page count |
| Stale ownership | Navigation predispatch tests cover input/order/session/visit changes with zero gestures; `changed_creation_guard_and_foreign_receipts_never_refresh_handoff` rejects stale or foreign handoff authority |
| Capture races | `tests/capture.rs::stable_owner_with_stale_buffer_and_away_back_input_are_refused` separately rejects stale buffer generation, rendered owner, input change and changed adapter ownership |
| Native-assigned ID recovery; indeterminate result | `native_assigned_lost_reply_reconciles_without_replay` models commit/lost reply, historical reconciliation, synthetic origin and no duplicate insertion |
| Cancellation | `unsupported_and_canceled_do_not_mutate`, navigation predispatch cancellation, and capture deadline/cancellation assertions refuse effects/batches |
| Postdispatch uncertainty | `postdispatch_faults_never_claim_no_effect_or_repeat` covers wrong neighbor, input/session/order/visit changes, cancellation, deadline and unready pixels, retaining one gesture and refusing replay |

The named model behaviors are substantively implemented; no missing case was identified in this bounded inspection. These assertions do not establish persistent native correlation, native clocks, actual cancellation/serialization or device support. The receipt map is in-memory and adapter-instance IDs are process-local. Broad native and integrated consumer gates remain separate. The existing task checkbox is retained for owning review/acceptance reconciliation; its unchecked state must not be read as an instruction to reimplement this model or repeat the accepted fixture matrix.

No production code, public contract, tests or dependencies changed. No tests were rerun merely to rediscover existing coverage. Source basis: current SDK source, test assertions, design/specification/implementation records, and Main's explicit current consumer feedback relayed in this project. That feedback is an internal coordination message without a public link; it is not native evidence.
