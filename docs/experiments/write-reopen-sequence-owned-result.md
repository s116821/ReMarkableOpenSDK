# Queued target write, ordinary close and reopen: owned composition

[The sequence](../../tools/reader_write_reopen_sequence.h) composes the existing
fixed-target mechanisms under one original setup deadline. Each host,
vendor ARM-QEMU and sanitizer matrix passes38 scenarios/1218 checks. This is
owned composition evidence; no tablet write, native close or saved-ink reopen
is established by this result.

One app/window/root/engine scope owns one open, blank capture, fresh callback-local
pen handoff, qualified pen result, ink capture, ordinary close, separately sampled
closure, one reopen and final capture. Each step runs on a later queued GUI
opportunity; phase tickets and consumed claims reject duplicate work. The original
absolute deadline remains fixed. Neither a late file nor a callback renews it.
Uncertain dispatch stops later phases and requires reconciliation, without redraw.

Each capture must consume its claim, return the exact ready target identity and
committed scalar/PNG hashes, and confirm phase-owned descriptor retirement.
The new [Files retirement](../../tools/reader_capture_files.h) revokes publication,
closes owned image/image-pin/duplicated-root FDs once with checked results, and
preserves forensic files and the caller's borrowed directory. A nested retirement
revokes/refuses without closing active IO, allowing cleanup after unwind. New
retirement controls pass each15 scenarios/286 checks plus the unchanged existing
63/607 file regression. Descriptor reclamation does not imply native-owner drain
or safe code unload.

## Counterexamples and fixes

An actual old-source control deleted final UI context after admission and still
allowed a positive final observation. The old-header RED is preserved separately;
the current final boundary checks context/deadline/cancellation after the last
callout. A cancellation during final admission is latched even after the finished
flag is set. Another actual control passed an accepted result from the wrong
holder: the result was rejected but suppressed intended-holder cancellation.
A separate fully qualified pen flag now controls that cancellation. Merely
copying an untrusted accepted flag cannot suppress recovery.

Virtual IO callouts may delete the sequence, consume permission or quit the app.
The sequence copies arguments/claims and tracks itself weakly around callouts;
possible effects are recorded before them. It retains no borrowed source guard
between queued phases. Unknown/pending pen or closure states have finite poll
caps. Positive pen metadata must bind the initial holder, current request, boot
and original deadline; a future completion time refuses.

## What actually runs, and what is modeled

The test uses actual owned Qt/QML contexts and source joins, signals/getters,
focused windows, queued timers, deletion, real PNG encoding, inode-pinned scalar/
image publication and explicit Files retirement. It passes unchanged original
R8 holder FIFO output through the current strict consumer. The fixed navigation,
closed MainView route, actor/cgroup admission, clock and independent completion
facts are explicit models. The separate
[closed-composition observer](closed-reader-view-owned-result.md) and
[holder/consumer](fixed-pen-holder-owned-result.md) have their own evidence limits.

Sequence source SHA-256:
`82c61573795ee66df8671684cdd0c74d8bbe97ab1392e0c0263f9af3824f4488`.
Retirement header:
`25c75671563e2be5c083c32211865611c9ed44b8f420bdc2d84312891984678d`.
Private original sequence receipt (184 source/output/asset checks):
`4477ff6da60efdb082e4159b79119b931922ae5b2f391e2f773a260cf66fa7f2`.
Retirement receipt:
`910c272142ff016b5feddf1f8c2ca64dbfa51a13f8459c6c894eb6402414b930`.
The original sequence ARM compiler stream retains psABI notes and two Fontconfig
cache warnings; host/sanitizer stderr is empty. Earlier fixture failures and
actual final-boundary/wrong-holder counterexamples remain preserved.

Run `tools/reader_write_reopen_sequence_test.sh` with frozen source, original owned
holder asset and fresh output directories; select host, arm or asan with
READER_WRITE_REOPEN_MODE. The vendor SDK image is pinned in the linked holder
report. The test does not open input devices or perform service operations.

The native file backend, fixed-profile startup and independent holder/actor unit
composition require separate exact-input qualification. Native acceptance still
requires intended-target ink, positive release, original frames/trace, ordinary
close and fresh reopen, full original document/order preservation, independent
restoration and collected-unit cleanup. Sampled UI closure and stable reopened
files do not expose a crash-durable storage acknowledgement.

Source basis: current original implementation and actual frozen owned Qt, file,
PNG and FIFO consumer results with independent source/original-byte review.
Native composition remains an explicitly separate requirement.
