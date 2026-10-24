# Private file backend and fixed-profile startup: source and owned checks

The new [Directory](../../tools/reader_write_reopen_files.h) and
[NativeFiles backend](../../tools/reader_write_reopen_backend.h) compose the
private research protocol for the [queued sequence](write-reopen-sequence-owned-result.md).
Real isolated file tests pass; a separately frozen ARM startup links. No dedicated
holder unit, native positive capture/pen path or tablet activation was executed
by these checks.

Directory opens a 0700 current-user root without following its final symlink,
pins its device/inode/mode/owner, and brackets bounded 0600 single-link regular
reads by metadata and path checks. Publications use a new exclusive pending file,
checked write/close, pinned inode, exclusive hard-link publication, then checked
pending unlink and final single-link state. Failures preserve partial evidence.
It rejects unsafe names, over-cap input, overwrite, FIFO/directory/symlink and
hard-link reads, changed permissions, replaced roots and operations after one-use
retirement. Each host/ARM-QEMU/sanitizer run passes76 checks using actual files.
These are sampled metadata checks, not a general filesystem transaction or
arbitrary-writer exclusion.

NativeFiles pins original owner, candidate identity, actor deadline and candidate
window files. It checks its own actual process start, boot, original BOOTTIME,
cancel/restore markers and, after one request consumption, the exact request's
inode/bytes on each later admission. It never renews the 90-second candidate or
360-second outer window. The planned positive path requires a live separately
bound holder in the dedicated pen service, strict holder exit/stdout/trace facts,
three retired capture pairs and fresh admission before final publication.

The combined selected-result alone is forensic. A separate selected-complete
binds its exact original SHA-256, nonce/PID/start, completion clock and original
end. Future collection still must validate original scalar/image/trace bytes,
independent unit state, current process/source profile, document preservation
and restoration. The marker does not replace those checks.

Owned backend runs pass297 checks in each mode using an isolated container's
private `/run` tmpfs and its own live process/boot/clock. They exercise valid
initial controls, missing/wrong/two-link holder refusals, FD counts after failed
ready publication, sole request consumption, request/control inode replacement,
mode/boot/process/window mismatch, cancellation, exact refusal-result/completion
hashes and no overwrite. They do not exercise a positive holder/cgroup/pen/capture
path. A failed Control open now stages its FD until all checks pass, preventing a
temporarily two-link readiness file from leaving a sticky descriptor. Earlier
217- and248-check versions remain source-specific historical results.

The separate [startup](../../tools/reader_write_reopen_startup.cpp) keeps the
existing exact xochitl/Qt/ARM fingerprint and pre-main actor claims, clears inherited
injection variables, then queues one controller. It waits for a read-only known
legacy composition and qualified holder before selected-ready, refuses an early
request, pins one matching request and starts the sequence. Fixed polling and the
original absolute deadline bound readiness. Owners remain resident.

The startup's24 exact working/LF-normalized inputs compile/link in the pinned
vendor SDK; the artifact has no defined dynamic exports, and all eight fixture
and eleven conflicting-selector controls refuse compilation. This is a link-only
artifact with synthetic compile-time IDs, not a selected tablet candidate.

Directory SHA-256:
`8a4e6890aebb74dfc52eba5715770213cc5fe370a7bcfe0938e4c21cdea76e7f`;
backend `687b2d640ebae2c8157c7afe908dd88b7748029b7d4b50199ca4eb60626f164d`.
Startup link-only artifact:
`25da1d4d315a0cd21cc018a815501551891d30a125989da33691cc265e6c3d86`;
original compile receipt:
`0074ba583714ce8486498b9ebb1e937ea1e84a617e058499f3dd379553a09b0e`.
Independent199-binding source/output/startup review:
`29b220202d3085522a1a2dbce91ef7e12a43ca7144d4d6e35d41074d7ac08b9a`.
Private original streams have no public download links. Only the Directory ARM
run retains a locale fallback warning; final backend stderr is empty in all
three modes. Earlier review prose misattributing that warning was preserved and
explicitly corrected in the final review, rather than silently rewritten.

Run the corresponding `tools/reader_write_reopen_{files,backend}_test.sh` against
frozen sources and fresh output folders with host/arm/asan mode variables. The
backend fixture requires container root and private writable `/run`; it must
never run against a tablet or shared host `/run`. No input device is mounted.
The pinned SDK image is recorded in the linked holder report.

Source basis: original current source, real isolated files/process/clock checks,
saved host/vendor ARM-QEMU/sanitizer streams and independently checked frozen
startup build. Native service topology and end-to-end operation remain open.
