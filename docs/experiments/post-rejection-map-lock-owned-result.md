# Owned post-rejection map-lock controls

October 23, 2026 UTC. Focused controls for the default-off
[post-rejection witness](../../openspec/changes/establish-native-platform-contract/native-post-rejection-map-lock.md)
passed. They establish original owned mutex/Core behavior and coordinator/wire
controls; **no native map-lock probe has run yet**.

| Owned execution | Result |
| --- | --- |
| Helper, host and ARM/QEMU separately | 2 normal cases / 37 checks plus 5 direct-exit controls each |
| ARM/Dobby Core | 61 normal cases / 642 checks, plus 17 fail-stop controls |
| Entry, host and ARM/QEMU separately | 39 cases / 537 checks each |
| Transport, host and ARM/QEMU separately | 50 cases / 795 checks each |

The Core totals retain all original 48/505 normal and 15 fail-stop cases unchanged
in behavior; thirteen new normal cases add 137 checks, plus two new direct-exit
paths. These use the real owned original outer cleanup followed by the typed
probe. A deliberately retained original mutex and failed unlock cannot report
success. Cancellation or expiry observed before the probe prevent invocation;
either observed after it suppresses the witness. Wrong thread/binding, unrelated
exceptions and missing mismatch evidence do not invoke the probe. There is no registration
reset, second Task, conditional forwarding or automatic retry.

The standalone helper uses real pthread mutexes. It rejects invalid storage,
recursive kind, missing/equal callbacks and unsuitable addresses before use.
Successful acquisition immediately invokes unlock, with no intervening logger,
clock, allocator or arbitrary callback. Busy, reuse, thrown callbacks or failed
unlock terminate119 directly. Declared storage still requires a valid, live
caller-owned mutex; this is not arbitrary-memory validation or ownership.

Entry checks requested/factory pointer agreement in both directions and refuses
an incomplete requested result. Actual transport output retains no new key in
default mode; requested mode adds the exact seven-field extension, including the
untouched incomplete sentinel. Production branches also compile on host and ARM.
The existing unconditional negative barrier remains unchanged.

The initial Core run stopped at an overstrict test expectation that fail-stop
stderr be empty. Qt had already emitted its normal locale warning before the
probe. R2 changes only that test-script assertion to require the exact existing
normal stderr and no custom reporter. R1 source, partial passes and failure are
preserved; no production fix was needed for this test issue.

## Binding and separate native integration

Owned receipt: `post-rejection-probe-tests-r2/receipt.json`, SHA-256
`cb2409a75eea5808f5ba01c60feb590ccf22ef70d4019c3ac98042da4c3692d7`.
It binds twelve source/test hashes and twenty-three dependency inputs, with raw
outputs retained privately.

| Production component | SHA-256 |
| --- | --- |
| Typed probe | `ced87749608327cf01e70f8196bbef994ee7386bd78a1ff2241729acbaa818b3` |
| Core | `8cda4fb4098371921501a1a8af8ceb8aa72839e9dc004832b7da896aa2a9707e` |
| Entry | `c48b3aa3a3cb5d41b5037ed217e0d052ebab39e932558086c1789a4c254630a8` |
| Transport | `2ce5b82b89d33ec65ad33240a52b5e8859b4e489e788b4130300813af7300778` |

Root's separate exact native factory admits worker+176 for all 24 writable,
nonexecutable bytes and verifies nonrecursive kind zero at worker+188, only in
requested mode. Queue mutex+592 is not substituted. It reuses qualified libc
trylock/unlock/Exit providers and retained native owners; the factory does not
supply those strong owners itself. A frozen startup macro selects both factory
and coordinator mode. Independent read review accepted that placement and the
helper's uncertainty path; source review is separate from native execution.

The changed nineteen-input composition compiled and linked in default mode,
never loaded. Its dummy-configured DSO SHA-256 is
`c3326416ee330fed172c75513f254438ddec80897e2fd4c03eae60d5f23a8ee5`.
An actual requested-mode artifact still needs its own build, provider/import,
initializer and complete recovery-packet review.

Root's separate closed consumer passes 144 synthetic vectors while requiring
the full existing negative conjunction and a completed requested probe. Review
closed an impossible complete-probe/nonterminal-Core combination; later
cancellation/deadline may still produce a valid incomplete result. Its base
parser remains unchanged. This is consumer-control evidence, not a native probe.

A native success would establish one successful nonrecursive reacquisition and
unlock after the particular unwind. It would not prove every native cleanup,
all-writer exclusion, physical owner release, safe unload or permitted insertion.
Busy is inconclusive about the owner and must not be relabeled a cleanup defect.

Source basis: original frozen helper/Core/Entry/transport fixtures and actual
owned logs, separate ARM composition compilation and independent source review.
Native results and full supported capability remain open.
