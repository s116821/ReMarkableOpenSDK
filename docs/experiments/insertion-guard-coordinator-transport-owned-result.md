# Owned insertion-guard coordinator and transport result

October 23, 2026 UTC. The frozen coordinator passed **34 scenarios and 376
checks** on host Qt, ARM/QEMU and host AddressSanitizer/UndefinedBehaviorSanitizer.
The separate file transport passed **48 cases and 750 checks** on both host and
ARM/QEMU. Production header branches also compiled without their test macros.
These are owned composition and file-protocol results. No native hook,
Coordinator execution in the stock application or tablet action occurred.

The proposed [coordinator contract](../../openspec/changes/establish-native-platform-contract/native-insertion-guard-entry.md)
and [transport contract](../../openspec/changes/establish-native-platform-contract/native-insertion-guard-transport.md)
remain separate from selecting a native packet. Neither result establishes an
accepted insertion, execution-time native rejection or supported SDK capability.

## Coordinator: real witnesses and modeled native boundaries

The [coordinator](../../scripts/reader_insertion_guard_entry.h) composes two fresh
source joins, one worker-thread probe, one registration and one ordinary
creation-method invocation. Its [fixture](../../scripts/reader_insertion_guard_entry_fixture.cpp)
uses the actual generic worker-callback witness, a value-copied callback on an
owned QThread, and the actual ordinary QML callback witness against an owned,
typed controller method. The first probe records a synchronized thread ID and
requires its exact full-width returned job completion before phase two.

The selected-source Sample, native strong-owner acquisition/submission,
installed Core registration/snapshot/cancellation and attribution context are
explicit test models. A passing modeled Core outcome does not demonstrate a
native private catch, mutex cleanup or zero insertion forwarding. The ordinary
method is an owned implementation, not the stock Task.

The controls cover:

- One start, probe, registration and ordinary invocation; terminal instances
  cannot rearm. Full unsigned 64-bit probe and operation IDs remain exact.
- Fresh second-source checks, changed worker identity, changed real order,
  registration refusal, and a deliberately wrong source confined to the
  predicate configuration. Ordinary submission retains the real source/order.
- Early, missing, foreign and duplicate completions; missing/duplicate QML
  callbacks; false controller acceptance; modeled Core refusal, forwarding and
  zero job ID. These cannot produce the positive diagnostic conjunction.
- Cancellation before or during the probe/Task, permission loss, and deletion
  during the attribution factory or a finished receiver. The finished signal
  is deferred beyond the modeled ordinary invocation stack.
- Original relative and supplied absolute deadlines, including expiry before
  start, a shorter external deadline and its unchanged propagation into Core
  registration. The second phase receives remaining time rather than a new
  operation window.
- An intentionally resident owned holder after facade destruction. This tests
  the retention design in the model; it does not test native last-owner release
  or reclaim a canceled native slot.

Earlier iterations remain preserved. R1 failed compilation on macro
redefinition, misleading indentation and an ambiguous qBound call. R2 failed
on a range-loop temporary warning. R3 then passed 29 scenarios/324 checks on
host; R4 passed 32/355 on host and ARM. R5 moved the `ownersRetained` scalar
assignment immediately after successful acquisition, before the next permit
call. Previously, cancellation at that later check could report false despite
retained storage. The added post-acquisition cancellation control checks that
receipt precision; the fix does not introduce a native release. R5 also adds
deletion from the finished receiver and the sanitizer run.

## Transport: actual files and publication uncertainty

The [transport](../../scripts/reader_insertion_guard_transport.h) and its
[fixture](../../scripts/reader_insertion_guard_transport_fixture.cpp) use real
owned directories/files, the actual process PID/start identity, exact nonce
bytes, no-follow opens, stat checks and exclusive hard-link publication.
Dedicated friend access supplies request/window/deadline state and invokes
the publisher with explicitly non-native test bytes. It never injects or
claims a successful Coordinator/native result.

Startup/request cases reject wrong identity or nonce, unsuitable modes,
symlinks, extra hard links, FIFO/directory requests, partial/oversized bytes,
existing output names, cancellation and restoration markers. Publication
cases check consumed-request/root replacement, window loss, original setup
and request expiry, a positive publication gate after the terminal marker,
4096-byte acceptance and 4097-byte refusal. Existing ready/result/pending files
and a pending symlink's target are preserved rather than overwritten.

Read review identified two publication gaps, fixed before the frozen run:

1. A missing permission check after linking could remove the pending name even
   after cancellation, restoration, expiry or binding loss. The corrected path
   checks permission before unlinking. Deterministic post-link controls preserve
   the result and pending names at two links, marking publication uncertain.
2. Checking a pending file only when initially empty did not bind its name to
   the file actually written. A replacement or mode change before linking
   could escape that check. The corrected path records the written file,
   checks the named pending file before linking, checks both linked names and
   their expected link count, then checks the final single-link result.
   Pending substitution and mode-change controls now refuse publication.

These are sampled identity/permission brackets. They are not atomic exclusion
against cancellation or a privileged concurrent writer, and they do not prove
persistence. A consumer still needs its own strict receipt/binding checks.

## Frozen provenance and reproduction

| Input | SHA-256 |
| --- | --- |
| [Coordinator header](../../scripts/reader_insertion_guard_entry.h) | `af1c16421aa417c94af849e755fb01228127721274e74e59fc6e67a6e828b34d` |
| [Coordinator fixture](../../scripts/reader_insertion_guard_entry_fixture.cpp) | `91be4d33a9b531994720fd4aab0eec5bccb36533c0109646ce881c8d462f1edb` |
| [Coordinator script](../../scripts/reader_insertion_guard_entry_test.sh) | `2cd005b8161f430ea353e4598eb24997c5cd8f7bf8e71df65cab097e39f8ba24` |
| [Transport header](../../scripts/reader_insertion_guard_transport.h) | `3c8d8ee4d0c03e33d254a96c9f1e9d0eea620e08c3243fafde38a0bee947a013` |
| [Transport fixture](../../scripts/reader_insertion_guard_transport_fixture.cpp) | `3de51e538e70ea6d1ae73cdcc3a6b7a95d497a443eb8219c5177c3b237380525` |
| [Transport script](../../scripts/reader_insertion_guard_transport_test.sh) | `698f44dc142a0de2f52c52c10ab4a46889f943b13fb78bfd46b9bdfc70c06e31` |

The private coordinator R5 receipt SHA-256 is
`1b4840307804db2205fa361b00a6518841f9bfb5af679d1380bcfd79d72bc255`.
The private transport R1 receipt SHA-256 is
`98f966a1a743396b1f34e19fd4cdbea51e04a57430e165512798896413273483`.
They bind the complete immutable 12-file and 15-file input manifests,
respectively, compiler output, raw results and built artifacts. Working files
still matched those manifests when the receipts were written. Private records
are identified by hash here and have no public download link.

Both scripts accept a fresh writable output directory through
`INSERTION_ENTRY_OUTPUT` or `INSERTION_TRANSPORT_OUTPUT`. Set
`QT_PROBE_SDK_ENV` for the ARM SDK build/run; omit it for host Qt. The coordinator
script enables host ASAN/UBSAN with `INSERTION_ENTRY_SANITIZERS=1`. Leak checking
is disabled because resident holder retention is intentional; other sanitizer
errors halt execution. The scripts retain generated sources and binaries in
the supplied output directory and bound owned execution with GNU timeout.

Actual host tools were GCC 13.3.0 and Qt 6.4.2; ARM tools were GCC 13.4.0 and
Qt 6.10.3 under QEMU. All runs used the local pinned image
`sha256:416c7a7be0038156797b0892f031f352b841d1921fae83f712d0a272e4724618`,
read-only input mounts, no network and dropped capabilities. The image is
local infrastructure, not a published toolchain distribution.

Native startup/install ordering, ABI/provider closure, real source/ownership
joins, ordinary Task attribution, full-packet execution and independent recovery
remain separate gates. These runs establish no native/render/durable authority,
safe shutdown/unload, atomic insertion or native owner reclamation.

Source basis: current original source and precode, frozen owned compiler/runtime
outputs and the two hash-bound private receipts. Applying this composition to
the stock application remains proposed work.
