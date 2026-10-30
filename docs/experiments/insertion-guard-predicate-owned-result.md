# Owned passive insertion-predicate controls

The original [read core](../../scripts/reader_insertion_guard_predicate.h) and
[owned fixture](../../scripts/reader_insertion_guard_predicate_fixture.cpp) pass
**203 checks on host and 203 on ARM/QEMU**. Independent source/result review
accepted those controls. No native addresses, hooks, Entry, device objects or
mutators execute in this fixture.

The [contract](../../openspec/changes/establish-native-platform-contract/native-insertion-guard-passive-predicate.md)
requires the actual under-lock span to be valid and capped at 16 records before
any search. It performs only the three frozen real/right/wrong UUID searches,
one effective-target equality and a final unchanged-span check. The result
contains scalar evidence and refusal reasons; insertion permission and native
authority are always false. A wrong-source UUID may equal the right neighbor
only when its frozen index is that right position.

Controls exercise caller assertions, malformed frozen inputs, checked 32-bit
address additions, alignment/stride, cap/current-count mismatch, absent or
mispositioned records, changed subjects, collision target, late span changes,
reentry, unexpected exception propagation and exact borrowed references.
Near-address-limit and cap-16 cases exercise boundary arithmetic. Failed
preconditions stop before further native-like calls. The searches stay fixed;
no discovered ID or position becomes another input.

The C++ new counter and unchanged QString buffers corroborate the modeled read
path. They are not a complete malloc/provider/TLS-runtime trace. The native
caller must prepare worker-local scope and runtime bindings outside the final
lock; injected passive calls retain their own provider/lifetime obligations.
Exception transport, original native cleanup and native pointer validity are
not established by these controls.

Run [the focused script](../../scripts/reader_insertion_guard_predicate_test.sh)
in the repository root with public Qt Core. Its optional QT_PROBE_SDK_ENV uses
the existing ARM toolchain and QEMU path. The recorded runs used SDK image
`sha256:416c7a7be0038156797b0892f031f352b841d1921fae83f712d0a272e4724618`.
An earlier fixture-only misleading-indentation compile failure is preserved.

Core SHA-256:
`27fa7882a445c650f38d3b1be670661c5600c4e8ea1742654d029f61c274416a`.
Fixture SHA-256:
`31109acdb7f01320464c8242299bd0dff530c9d6106428afe1857df0266b0a0d`.
Test-script SHA-256:
`84bcdb2febb2a4c2ad3a34657c2698d80ae668a0ecc40efb01e3d9a4d2f48d19`.
Saved actual-run receipt SHA-256:
`35ccc34fea684ec65f23a853f4c90b87e2a4fe582113fd63d55b678960016ac8`.

CallerAdmission fields are declarations supplied by the future native caller.
These tests do not prove the original lock is held, native owners are alive,
the profile/providers match or a GUI selected visit is excluded. Final span
equality is not a revision, ABA or all-writer proof. The native witness remains
unimplemented and no capability is promoted.

Source basis: reviewed predicate proposal, current original core/fixture,
saved host/ARM outputs and independent source/result review.
