# Fresh fixed-writer image and native owned-file process qualification

The Qt-linked holder may contain library-owned background threads. The native
backend now requires a separately pinned, Qt-free writer image through
posix_spawn. There is no native fallback fork. The historical single-threaded
fork path remains available only to the existing caller-supplied owned fixtures.

This is process-topology qualification and owned FIFO evidence. No input device
was opened in the native checks below; no stroke, UI activation, service change,
page navigation or persistence result is claimed. The originating refusal and
secondary startup abort remain recorded in the
[pre-GO native result](write-reopen-pre-go-native-result.md).

## Mechanism and preserved boundaries

The parent pins fixed-pen-writer and its packet-bound worker.sha256 in the owned
directory before publishing holder readiness or opening device descriptors. It
requires a private single-link executable, a canonical65-byte digest, matching
bytes and stable pinned/named identities. The image is consumed before the first
launch attempt, including a failed attempt.

Four source descriptors are duplicated above15 before any child mappings:
writer3, start4, cancellation5 and result6. The child receives null standard
streams, an empty environment, explicit signal state and closefrom7. The
executable lookup uses the still-open parent descriptor through
/proc/PARENT/fd/FD, so closing the child's extra descriptors does not remove the
lookup pin. A successful spawn always returns its known PID to the original
holder, even if a later close or pin recheck fails.

The fresh image checks its one-thread state, native record ABI, exact descriptor
roles, parent PID/start and original budget. The production device branch
requires the measured character-device identity. It accepts only the existing
single GO byte, emits the unchanged fixed frames and returns the unchanged
36-byte child record. The parent still owns the observer, cancellation, child
incarnation/wait/fence and independent release path. The original one-second
dispatch and three-second recovery deadlines are unchanged.

The native parent has no fork-child device-writing methods. Its compile-time
IO trait requires the new spawn method. The owned fallback is kept so existing
fork, failure, reaping and recovery tests continue to exercise their original
boundary.

## Recorded results

| Check | Observed result | Limit |
| --- | --- | --- |
| Host actual spawn/FIFO, parent with two background threads | 12 cases,183 checks; positive, cancellation, descriptor mismatch, stale budget, replacement/hash/mode/link errors and exec failure | Device snapshots/context are owned models |
| Sanitized host parent | Same12/183; empty diagnostics | The separate plain worker is unsanitized |
| ARM SDK compile | Full parent, production writer, explicit owned writer and fixture compile | A build alone is not execution |
| Existing holder regressions | Each27 cases/211 checks, host/ARM-QEMU/sanitizer | Their historical fork path and device facts remain owned |
| Untouched spawned original-byte consumer | Each1 case/46 checks for host, sanitized-parent and native-owned producer assets, across all three consumer environments | Independent completion is explicitly modeled |
| Actual RM2 production writer profile | One thread,16-byte records; offsets8/10/12; owned_model=false and input_io=false | Profile branch returns before descriptor/device inspection |
| Actual RM2 owned-file process suite | 12 cases/183 checks, actual ARM spawn/exec, background-thread parent, pipes/FIFO/status/recovery | Explicit owned writer model; no native input-device effects |

The native production profile was PID10644/start135296250 on the unchanged
RM2 firmware3.28.0.172. The native suite used fresh nonce
f5079c467825411ab8b8db0114af51fd. Stock xochitl8194/start135057962 and
Buddy8747/start135080740 remained active throughout. The original owned trace is
2928 bytes:178 dispatch records plus five recovery records. Its SHA-256 remains
8beddbee85518e624c022eeb6aa97c8cf1931cf45a562f818d610a882a1e0902.

The compiled parent requires the target libc's
posix_spawn_file_actions_addclosefrom_np at GLIBC_2.34. The native owned suite
executes that actual target path, including the parent-descriptor executable
lookup, fixed descriptor roles and closure of an inherited non-CLOEXEC FD200.
The production worker has no Qt ELF dependency. Its artifact is
a4b768cc39b73dd5912304c06694f290a2fffc3173d6d69b745c0feaf07805eb.
The explicit native owned worker is
0c7d259f2aee435e1282ec48ea4a5214e43a46033dc2048765dbe7b6e5d64b37;
these are distinct artifacts and must never be substituted.

Earlier attempts to run an ARM-QEMU parent against a host child were inconsistent
and remain failed evidence. The portable ARM script therefore compiles the
composition and runs only the production profile under QEMU; it does not report
a fresh ARM child as executed there. QEMU's profile observed two threads, while
the actual native production profile observed one. Do not substitute emulator
thread counts for the tablet's measured state.

A first new consumer wrapper generated a CRLF manifest on Windows and was
correctly rejected by the canonical-byte check. Its successor writes the new
manifest with LF; original producer scalar, raw and expectation bytes were never
rewritten. A separate helper's review stopped at a provider content flag and was
not credited as completed; the current source/outputs were reviewed separately.

## Reproduction and evidence

Use a frozen tools directory and fresh outputs:

- Run reader_fixed_pen_spawn_test.sh with READER_FIXED_PEN_SPAWN_MODE=host,
  arm or asan. ARM execution of a newly spawned ARM child needs a real ARM
  kernel; the script deliberately does not configure binfmt or a production
  emulation fallback.
- Run reader_fixed_pen_holder_test.sh with the corresponding
  READER_FIXED_PEN_HOLDER_MODE to retain the original fork regressions.
- Freeze the three spawn-positive originals and their independently prepared
  expectation into a canonical owned-spawn-original-byte-read/1 manifest, then
  use reader_fixed_pen_spawn_read_test.sh with
  READER_FIXED_PEN_SPAWN_READ_MODE=host, arm or asan.
- On an authorized idle matching tablet, the separately compiled production
  writer's --profile performs no device access. The explicit owned ARM writer
  and fixture exercise temporary files/FIFOs. They do not qualify NativeIO.

All builds use the pinned SDK image
sha256:416c7a7be0038156797b0892f031f352b841d1921fae83f712d0a272e4724618.
Private original native archive:1,409,024 bytes, SHA-256
bc580fa4d2b5f8cb79a389e6394e3940a12acdbbb1a4d5289ddd1499805479f2.
Native owned receipt:
5e157a3110647552837b1b32d3cb8c778a366de4a71b12b1d4f8b169808e17db.
Legacy/consumer regression receipt:
30abb0f33941b0d5e5af8028f8b039bcf836fe7a25f119aaa7be4c09b37e3c2e.
The independent277-check current build/original audit is
763c9eee0a2f504bd3296a970c4a119b76612b37493eb4764e169dc2d06db04c;
a later publication check binds current committed files separately. Private
streams have no public download links.

Both older spent RAM roots were fully archived and byte-checked before removal.
The failed trial's final99-file archive, including later restorative records,
is2,998,272 bytes, SHA-256
429abb1cef116006dc90bc6495727e8d79fd4ba3c178f8ffeaff13bb5f818d7a.
The read-only admission profile's five-file archive is48,640 bytes, SHA-256
5f86aaef79e46c3afd89a597f3b7580bd9bc67e11ca2e6fd11f41708bdf002a1.
Only those exact inactive owned RAM roots were removed; all local originals and
the retained failed-unit diagnostics were preserved. A fresh full26-file/12-page
baseline still matches the original document archive byte-for-byte.

## Simulator calibration and remaining native work

Model a multi-threaded Qt parent separately from a single-threaded fresh writer.
Preserve descriptor pin/replacement failures, explicit role mapping, extra-FD
closure, parent loss/cancellation, one-use launch, child exit/status, observer and
release completion as distinct facts. Use the untouched183-record stream for
consumer replay. Do not collapse process creation, accepted input, rendered
target ink or durable reopen into one success flag.

The real input test still needs a freshly selected complete packet, exact
production parent/worker/provider binding, intended existing blank-page identity,
one fixed stroke, observed release, blank/ink/reopened captures, ordinary close
and stable storage reconciliation. Product integration and broad
model/firmware qualification remain separate.

Source basis: current source, frozen host/SDK/sanitizer outputs, original native
profile and owned-file streams, byte-checked archives and the fresh full backup.
Device/context/completion facts in the FIFO suite are explicit models; native
ink and durable authority are not inferred.

