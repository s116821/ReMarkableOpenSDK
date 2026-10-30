# Private file backend and fixed-profile startup: source and owned checks

The new [Directory](../../scripts/reader_write_reopen_files.h) and
[NativeFiles backend](../../scripts/reader_write_reopen_backend.h) compose the
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

The separate [startup](../../scripts/reader_write_reopen_startup.cpp) keeps the
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

## Later hybrid-profile correction

A fresh read-only RM2 query observed both
`1:name=systemd:/system.slice/xochitl.service` and
`0::/system.slice/xochitl.service`, with separate systemd-v1 and unified-v2
mounts. It confirmed firmware3.28.0.172, systemd255(255.21^), unchanged stock
PID/start/boot, and the same26-file/twelve-page archive and providers as the last
restoration. No input, navigation, transfer or service action occurred. Private
152-check baseline review:
`820226685796d662fcdc97d1b7e3e41691d33eda90ba148eaaf9db0a430d1921`;
original cgroup query:
`8ecd9244041f6595d7bbfe907af498715e5e2ae4257d89745ad761a48deca110`.

The earlier readiness predicate counted exactly one matching service suffix and
would refuse that normal hybrid pair. The successor instead requires the exact
two observed hierarchy labels to name the same dedicated pen unit, rejecting
missing/duplicate/extra lines and UI/actor coupling. Source:
`d7546d3d5552f9d7e579780575c102625fd30ed4877c3e59930b3ad93f8eed56`.
The successor backend controls pass303 checks per host/ARM-QEMU/sanitizer run,
all stderr empty. This is a source repair and owned topology model; a new pen
unit's actual placement still needs observation. The earlier297-check source and
link result above remain historical, rather than being credited to this header.

The current24-input ARM startup relinks to
`646a7b494a393df977e6e572d98b7ece3ef0b818ad0ff07265385ff0cd9e4b50`,
with eight fixture and eleven conflicting-selector preprocessing refusals,
private exports and unchanged frozen source. Receipt:
`4c1f2cf446f50f1f348f146ac269a1af58b74889e910345119ebc7f9ab24196e`.
The immediately preceding wrapper timed out while repeatedly checking negative
selectors after its default link completed. Its unknown/incomplete receipt is
preserved; the successor checks selector guards during preprocessing and still
performs the full default compilation/link. No device action occurred in either.

The independent-service plan now retains a completed unit with RemainAfterExit
until collection. The [systemd255 primary reference](https://www.freedesktop.org/software/systemd/man/255/systemd-run.html)
documents retention of runtime information with --remain-after-exit and immediate
unloading of successful transient units otherwise. Native help confirms that
option and --no-block. The completed no-process state is active/exited with
MainPID zero; it is not proof that the unit has already been stopped. Both
hierarchies, known exit, original holder identity and guarded eventual cleanup
remain separate native checks.
