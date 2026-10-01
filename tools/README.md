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
