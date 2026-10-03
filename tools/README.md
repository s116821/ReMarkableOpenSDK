# Development-only Qt topology observer

`qt_topology_observer.c` implements Stage R1 of the active native experiment plan.
It is a one-shot Linux ARM32 little-endian topology sampler, not a native SDK
adapter, page guard, production loader or page-creation mechanism. The operator
must independently verify firmware, executable/provider hashes, ELF COPY relocation,
runtime mapping and the compiler-measured private ABI manifest before execution.
No target process is executed by host tests.

The CLI accepts PID, original process start ticks, checked self-slot address (hex),
and bounded decimal layout inputs: object-prefix bytes, data-prefix bytes, object
data-pointer offset, data backlink offset, parent offset, child-array-pointer offset,
child-count offset. Actual firmware locators/layout manifests stay private. Prefixes
are capped at64 bytes and fields must be aligned/in-bounds and distinct where needed.
The helper requires a readable writable executable mapping with matching device/inode
for the self slot; filesystems reporting inconsistent identities are refused.

Limits:2-second cooperative monotonic budget,64KiB requested remote bytes charged
even on errors,64 visited objects,64 children per node, depth four,64KiB output and
two mapping snapshots of at most128KiB each. Every transfer must be full. Invalid
pointers/backlinks/counts, descriptor changes and truncation mark the result incomplete.
EPERM/ENOSYS stop; there is no policy adjustment or debugger fallback. Root pointer,
process start/state, executable identity and mapping bytes are checked again. Vptr
words come only from already-read fixed headers; classes remain unknown. No RTTI,
document/account fields, strings, whole-memory scanning or target methods are read.
Observations remain sampled and non-atomic; checks do not prevent object-lifetime/ABA
races or confer authority. The cooperative deadline cannot preempt a blocked syscall.

Exit0 means the bounded sample completed,3 means a fatal/preflight/change refusal,
4 means incomplete traversal,2 means invalid invocation/allocation. None means native
qualification. Preserve nonzero status with the bounded private JSON. The operator's
outer10-second deadline must identify and terminate only this helper, verify its
termination and target continuity, collect the result and remove exact staged files.
Do not automatically retry or restart the target. Independent source/artifact review
and advance notice precede any authorized tablet execution.

Host fixtures use original owned byte arrays and an owned process. They exercise
short/denied reads, invalid/aligned pointers, counts, cycles, changing descriptors,
caps/deadline, read-only image mapping, real process_vm_readv, PID-start mismatch,
bounded map parsing and end-to-end preflight. The positive fixture needs a non-PIE
host executable with low addresses and consistent executable mapping identities;
an executable tmpfs is suitable inside a Linux container with overlay rootfs.

```sh
gcc -std=c11 -Wall -Wextra -Werror -O2 -no-pie tools/qt_topology_observer_test.c -o /tmp/observer-test
/tmp/observer-test
gcc -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -no-pie tools/qt_topology_observer_test.c -o /tmp/observer-sanitized
/tmp/observer-sanitized
```

Source basis: approved Stage R1 plan and original implementation/owned fixtures.
The plan references the process_vm_readv permission and non-atomicity contract;
host fixture success does not establish tablet permissions, ABI layout or topology.

## Separate Stage R2 existing-instance observation

`qml_singleton_observer.c` implements the separately approved fixed read chain in
native-singleton-observation.md. It reuses the bounded reader/identity/mapping code;
the byte reader is used only for the declared lifecycle guard. R1 aligned pointer
reads remain aligned. No type names, URLs, properties, factories, callbacks, engine
fields, worker data or reference-count writes are accessed. Matching raw callback
words are compared exactly without ARM/Thumb normalization.

Invocation: `qml-observer PID START_TICKS PRIVATE_MANIFEST`. A manifest is at most
4096 bytes: first line `qml-existing-instance-arm32-v1`, second line the exact
provider file path, then exactly 25 hexadecimal integers in this order:

1. PT_LOAD virtual address, file offset, file bytes, memory bytes.
2. Registry-list virtual address and lifecycle-guard virtual address.
3. Type kind/meta/extra offsets, singleton kind, expected meta-object pointer.
4. Singleton-info offset, callback offset/size, capture/manager/invoker offsets,
   expected raw manager/invoker pointers.
5. Guarded control/object offsets, sampled strong-count offset, expected vptr,
   QObject data-pointer offset and backlink offset.

Actual locators and ABI values stay in the private operator manifest. Invalid shape,
unaligned/out-of-bound fields, duplicate callback slots and extra tokens refuse.
The operator checks exact provider/executable hashes and ELF facts. The helper derives
load bias from file-backed mapping identity/offset, corroborates relevant RELRO-split
mappings and readable segment continuity, and allows BSS anonymous continuation.
It reads only declared discrete type fields; only exact kind/meta/callback matches
permit capture/object reads. Nonzero sampled strong count is not retained lifetime.

Caps: 4096 types, four candidates, 128 KiB remote requests, 8 KiB output, two 128 KiB mapping
snapshots, 2-second cooperative budget and the same 10-second outer operator procedure.
An unknown/expired/ambiguous or changed candidate is incomplete, never a callable
capability. Even a unique matching sample remains untrusted/non-atomic and grants
no current-page authority or safe native insertion. A larger registry is refused
without paging or heap scanning. No retry follows a refused sample.

Owned R2 fixtures cover guard states, wrong kind/meta, foreign callbacks with zero
capture reads, expiry, null/malformed pointers, ambiguity/candidate/type caps,
changed registry/type/callback/capture/object fields, partial reads/deadline/output
caps, manifest rejection and provider identity/RELRO/BSS mapping behavior. Run:

```sh
gcc -std=c11 -Wall -Wextra -Werror -O2 tools/qml_singleton_observer_test.c -o /tmp/qml-test
/tmp/qml-test
gcc -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined tools/qml_singleton_observer_test.c -o /tmp/qml-sanitized
/tmp/qml-sanitized
```

Repeat the R1 owned fixtures when shared reader code changes. This does not request
another live R1 run. Native access/layout results require the coordinated actual R2
operator receipt; host tests do not substitute for it.

## Separate Stage R2D descriptor-only diagnostic

`qml_descriptor_observer.c` is a separately selected executable under the approved
R2D amendment. It does not run automatically after R2 refusal. It reuses the exact
private manifest and identity/provider/mapping checks, but reads only one lifecycle
byte and fixed 12-byte list descriptor twice, at most 26 remote bytes. Allocation
and buffer words are output data only: they never drive reads, allocations or loops.
The type count cannot expand the read set, including huge or negative values.

Private output is capped at 1 KiB and retains each fully sampled guard/descriptor,
unsigned and signed count, equality and incomplete/non-atomic flags. Unavailable
fields are null rather than invented zeros. Refusal preserves preceding complete
samples. The diagnostic cannot establish registry ABI, discover a controller or
justify greater traversal by itself. The separately reviewed October 2 R2 revision
uses 4096 entries and 128 KiB; this diagnostic retains its independent 26-byte cap.

Owned fixtures verify exact fixed reads/no pointer traversal, huge/negative/zero
counts, unknown guards, changed/partial samples, timeout, mapping and overflow
refusals. Build/run `qml_descriptor_observer_test.c` with the same native GCC and
ASAN/UBSAN recipes as R2. Repeat R1/R2 fixtures after shared cap configuration edits.
R2D requires its own independently reproduced ARM artifact/operator review and one
announced run with fresh checks, evidence-before-cleanup and no retry. Process/SSH
continuity is distinct from untested physical UI responsiveness.
