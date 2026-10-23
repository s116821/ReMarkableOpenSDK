# Owned negative-only hook core

The separate original [hook core](../../tools/reader_insertion_guard_hooks.h)
passes 17 normal cases / 209 assertions and 15 fail-stop subprocess controls
on the pinned ARM SDK/QEMU image. Independent source review and an actual
rebuild/replay reproduce the same binary and results. This is owned-code
evidence; no native loader, Entry integration or tablet trial is selected.

The [runtime contract](../../openspec/changes/establish-native-platform-contract/native-insertion-guard-runtime-core.md)
separates installation, one immutable registration, tracked/foreign scope,
private rejection and terminal publication. The tracked inner path never
forwards to the original insertion helper, including unexpected predicate
results. The outer wrapper catches only its own private marker after original
owned RAII cleanup. Unrelated exceptions propagate. Recognized binding failures
and duplicates cannot silently forward another insertion.

Both original eight-byte sequences are checked before mutation; overlapping
overwrite ranges are refused. Each hook's actual entry and 16-byte trampoline
must match, with readable/executable/nonwritable mappings. The complete pair
is checked again after the second installer returns. A failure, partial pair,
early entry or throwing installer terminates the owned process. A returning
or throwing diagnostic reporter cannot prevent that termination.

Controls cover transparent full-width carriers, private cleanup, transformed
arguments, nested foreign work, duplicates, cancellation, deadlines, optional
64-bit job attribution and unrelated exceptions. Independent counts of original
inner calls check the negative barrier; its structural zero counter alone is
not the oracle. Deliberate actual first-entry and first-trampoline corruption
by the second installer is detected before any operation submission. These
fixture writes check mapping and protection calls, restore RX, flush caches
and reread the changed bytes. They are not a native patching implementation.

The [focused script](../../tools/reader_insertion_guard_hooks_test.sh) builds
the [original fixture](../../tools/reader_insertion_guard_hooks_fixture.cpp).
It requires the supplied SDK environment, matching Dobby public headers and
the repaired archive documented in [the earlier owned result](insertion-guard-owned-result.md#later-arm-mode-repair-and-remaining-installation-limits).
Every subprocess is bounded by the verified GNU timeout tool. The image is
`sha256:416c7a7be0038156797b0892f031f352b841d1921fae83f712d0a272e4724618`;
archive SHA-256 is
`622b17dc65d0dcad717d83264f783ad12044565ed8ea86f9e5088d9abf5328ef`.

| Frozen input/output | SHA-256 |
| --- | --- |
| Core | `4543c6b43cf61e27c714b9faca678b38bd42385bb3ec092c3e738c890d01be27` |
| Fixture | `8e866fc95563894048220b1b6ff353250c8caed22d29d72a6ab8419df56a99e5` |
| Script | `37700abceddbcaecacc9223c60bda43fb0ce02125256d8fa070fe70c285eaf80` |
| Binary, both executions | `ba2b3a0d43f3b39bf8ba5e749a074d09a78625aea7fcfef255f47f3a0d68e356` |
| Source-author receipt | `fb788134d50fcbccb0c42a8d77faa47c19ef1f4bd4b1469de29884394fd26e10` |
| Independent review/replay receipt | `ec711597ce4615da075c72f7128ad8175e6f3183ae4d04b46c0b2ade8a8cc461` |

Earlier failures are retained: the first deliberate-corruption attempt stalled
and was terminated without a completed mutation proof; a subsequent fixture
compile failed because dynamic division emitted an unsupported UDIV under its
declared instruction profile. The final controls use checked power-of-two page
arithmetic and explicit bounds. Passing results do not reclassify those failures
or establish their unobserved cause.

Native integration remains open. This core's current exact-pointer binding is
an owned interface: the ordinary native caller constructs its optional argument
on its own stack, so a future caller-borrowed mode cannot require a GUI-known
address. Size values may be compared only when their presence is established.
Native public-Qt ABI, original caller/cleanup, job correlation, providers, loader
ordering and retained owners require separate qualification. A pointer or caller
admission flag does not establish those facts. Completed snapshots are not
native-owner retirement, safe unhook or unload. No accepted native insertion or
supported capability follows from this result.

Source basis: current original source, reviewed contracts, actual compiler/ELF/
ARM/QEMU outputs, independent source review and replay; retained native carrier
evidence identifies the remaining integration limitation.
