# Fixed pen holder, handoff and original-byte consumer

The default-off research holder now composes one fixed dispatch with a separate
parent recovery owner and a strictly bound result consumer. Owned FIFO/process
tests pass. The standalone ARM CLI builds and refuses invalid arguments; no
valid CLI request, native event IO or tablet stroke is qualified by this report.
The separate [native read-only profile](pen-device-readonly-native-result.md)
does not fill those gaps.

Later result: the [fresh-image successor](fixed-pen-spawn-owned-result.md)
replaces the native parent's fork route and records actual native owned-file
process checks. The original fork results and hashes below remain historical;
neither report alone qualifies input-device delivery or target ink.

## Ownership and original deadlines

[The holder core](../../scripts/reader_fixed_pen_holder.h) forks one owned child,
gates its first frame through a start pipe, cancels through a separate pipe,
and retains a result pipe and parent-owned recovery descriptor. The child uses
bounded plain IO and `_exit`, without Qt/heap/destructor unwinding. Parent code
fences the child before its single five-record release. It never starts a second
stroke or renews the original one-second dispatch/three-second recovery budgets.
An unfenced child produces a diagnostic, with no late release or normal positive.
Uninterruptible child/holder/kernel state remains an explicit outer-recovery gap.

The caller must be single threaded and fork safe, ignore SIGPIPE, retain the
default SIGCHLD disposition without SA_NOCLDWAIT, and have no competing reaper.
Actual ignored/custom-reaper controls exposed why an unknown wait result cannot
authorize signaling a possibly reused raw PID. The repaired core refuses that
state. The [CLI](../../scripts/reader_fixed_pen_holder.cpp) normalizes its signal
dispositions and checks its fixed device/profile/process/private-root controls.
The planned dedicated unit must remain outside both xochitl and the actor's kill
groups; compiling the CLI does not establish that topology.

[Fresh handoff](../../scripts/reader_fixed_pen_handoff.h) consumes a claim before
callouts, checks a callback-local ready identity, and publishes exactly
`nonce pid start request_ms` plus newline as its terminal action. No borrowed
Reader guard is consulted afterward. It requires more than 3250ms of original
setup time for admission plus recovery. Owned source joins and clocks are models;
real QObjects, file bytes, deletion, reentry and terminal callback controls run.
Each host/ARM-QEMU/sanitizer handoff matrix passes 27 scenarios/33 checks, with
separate production-header host/ARM compilation and independent62-check review.

## Positive result means a conjunction

[The consumer](../../scripts/reader_fixed_pen_result.h) consumes one claim, checks
closed canonical JSON and exact types/keys, binds boot/nonce/candidate/holder/
request/original deadlines, and requires externally supplied independent holder
exit evidence. File appearance alone cannot complete it. Original scalar stdout
must match the saved scalar exactly; the raw trace must match its size/hash.
The consumer independently decodes ARM's sixteen-byte event records and replays
the [sticky stream](fixed-pen-stream-owned-result.md) from the reported initial
snapshot through the exact final released snapshot. It does not grant native,
rendering, durable or atomic authority.

The current result also returns validated binding/timing metadata for a caller's
own comparison. This adds no wire fields. An actual parser counterexample accepted
child PID text with a trailing newline; complete ASCII decimal validation now
rejects it. The current scalar/trace fixture passes each137 scenarios/25093 checks
on host, ARM-QEMU and sanitizers; raw traces and actor completion there are models.

The stronger owned composition runs actual pipes, fork/wait, FIFO bytes and
parent recovery: each27 scenarios/211 checks. Its untouched 2928-byte positive
trace contains 178 dispatch plus five recovery records, SHA-256
`8beddbee85518e624c022eeb6aa97c8cf1931cf45a562f818d610a882a1e0902`.
Separately exported expectations use a real distinct owned parent candidate and
holder PID/start. Rebuilding the current consumer against unchanged positive,
canceled and diagnostic outputs passes each3 scenarios/146 checks. Only the
positive passes; canceled and unfenced-child results refuse. Device effects,
native UI context and independent service completion remain modeled.

## Reproduction and exact evidence

Run the corresponding `tools/reader_fixed_pen_{handoff,result,holder}_test.sh`
with a frozen source folder and fresh output folder, selecting host, arm or asan
through the script's documented mode variable. The holder exports original
scalar/raw/expectation files; `reader_fixed_pen_holder_read_test.sh` consumes that
separate asset folder without rewriting it. The pinned vendor SDK image is
`sha256:416c7a7be0038156797b0892f031f352b841d1921fae83f712d0a272e4724618`.
Host Qt6.4.2 and vendor ARM Qt6.10.3 are distinct environments, not tablet runs.

Core source SHA-256:
`2114a3f5b4d5a89dc74c510d26bd7742a6ec41e0c8f422ff9fc44e2d8c7fdadb`;
CLI `b94ac8939bd62e7b4e4f52a6ce4e5597d3901700f3a5785b6d72ac981b769e2c`;
wire `3f4b41a68140a896f43f5895c7d789601ab0c1f781c032e5552c4c36db575e55`;
handoff `7c03fbc34e2c4d8b48b6715919c8e5a237cd378dbc2f3c074ca1086baa9fafd0`;
consumer `e179a560ec5e07438c50d88bb711463575e25eea000742ee7a6feb2b47ef00cb`.
The compiled CLI artifact is
`1eb53ad37ee6ab10a1fb29d0287b328acea0e3a4a408c92c1752508f35fd7b12`;
only invalid-argument execution under QEMU is recorded.

Private original holder R8 receipt:
`9d9b724c6f289ece33bceffcde27efbc01a6e4cac4853b6e1b4a620194b156c7`;
current original-byte reader R3 receipt:
`8632fc2bc6aa5d36a7b498493c863556f83d102bb29f28efe0de06ffb60c2e87`.
Root's independent296-check original binding audit across holder, reader,
retirement and sequence receipts is
`d6623f0253f2ebe984efbb1b2f4879c7e5b2c366c6df0efec9e47e618d840dc9`.
Private streams have no public download links. Earlier fixture/compile failures,
the initial CRLF expectation-manifest failure and signal/reaper counterexamples
remain preserved separately. Passing successors do not relabel them.

Source basis: current original source, frozen owned artifacts and saved original
process/FIFO/JSON/trace streams, with independent review. Native device delivery,
dedicated service topology and target ink remain future qualification.
