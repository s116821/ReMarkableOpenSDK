# Fresh fixed-writer image and native owned-file process qualification

The Qt-linked holder may contain library-owned background threads. The native
backend now requires a separately pinned, Qt-free writer image through
posix_spawn. There is no native fallback fork. The historical single-threaded
fork path remains available only to the existing caller-supplied owned fixtures.

The process-only checks in Recorded results qualify process topology and owned
FIFO behavior. They opened no input device and made no UI or stock/Buddy service
changes. The later composed admission trial is reported separately below; it
opened verified device descriptors but sent no input. The originating refusal
and secondary startup abort remain recorded in the
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

## Later composed holder admission, with no GO

Fresh native trial e219cebe48d247e698a965b663dbd3ca used committed source
d34d17bd850eb0f4fcd7c9695afc15d75889184e, payload
720bb5a91f94c9dbff5a99be017dd198363b5cff31f4e302e617a3ada64a1143
and parent721f9b5dc31d72fa432afec538b6985efcda2baf903a815eadc15c59ba65547e.
Holder12924/start135420088 published readiness for candidate12782/start135419843.
Thus the production image pinning and NativeIO preparation gates passed. The
holder was waiting for GO; no fresh writer child was launched.

The application did not publish selected-ready within the original host window.
No selected request or GO was published. After the single stop request, the
original pen result reports go_claimed=false, child_pid=0, child_started=false,
zero dispatch calls, closed device/trace descriptors and an empty original
event trace. Its unstarted dispatch snapshot fields are defaults, not a new
measurement of the device's coordinates. Startup's observed=false result is
startup-admission-or-cap; it did not perform the write/close/reopen sequence.

The exact failed readiness predicate was not captured. The source requires both
the matching holder and inspectLegacyFixture composition. The My files screen
needing a manually opened Reader is a plausible explanation, not an observed
branch. Live cgroup bytes for holder12924 were not collected. A separate fresh
process-only transient service, nonce31d913dc09e24d618f49aaec44ae3b27,
subsequently measured exactly the expected v1-systemd/v2-unified dedicated paths;
that validates this placement pattern, not the exited holder's exact state.
The completed profile unit and exact archived flat RAM root were retired.

The generic recovery fence conservatively withheld Buddy after the canceled
pre-GO holder exited1. Stock16285/start135431414 was restored. A separate
one-time restorative request verified absent requests/GO, original result and
zero-trace bytes, all original test processes gone, both cgroup hierarchies
empty, unchanged service definition and the saved initially active Buddy state.
It restored Buddy without overwriting recovery.failed or retrying input.

Original trial archive SHA-256:
818b56a58b8d9131ce575488f2065594247bec7eca864b3d88e9744eb5c96a81.
The full post-restoration backup remains byte-identical for all26 files and all
12 page IDs/order, including target ink/thumbnail and bookkeeping/unknown fields:
748,544 bytes, SHA-256
b52f4a1b1dd55579069f63389e18d29a525e7b259f4500b8a70fe34216b163f6.
The trial is spent. Native input, target ink and close/reopen remain unqualified.

The successor startup source retains its last observed wait phase in the
terminal refusal: holder-not-ready, composition with the bounded refusal tag,
or request-not-published. The detail is sampled diagnostic provenance, not a
current readiness claim. It adds no input request and changes no existing
readiness condition, guard or original deadline. This cannot reconstruct the
missing e219 observation.

LF-normalized successor startup SHA-256:
8fc827a698b575716a609c4e36d892a651e0774156556eb52ea9bb3abb6a13e0.
Source-delta review verifies that removing only diagnostic assignments and
terminal formatting recovers all previous non-diagnostic tokens. The unchanged
actual admission fragment passes16 cases each on host, ARM-QEMU and sanitizer.
Review receipt9d1744755b49fc572c19012a573cda444d6b38b247739273886803539533a8d6
also preserves the first wrapper's rejected argument assumption; its successor
uses the shipped positional test arguments. The complete ARM link,24 frozen
inputs and8 fixture/11 conflicting-selector refusals pass, receipt
ce1d24a5af28fe2cefff3710bd7665dacb6d4b452e2dddfce59bf3af328141d2.
Its link-only artifact2c824096b6911914b120331bac29b71fc99dd48128a2263c3224ee34514cd0f6
has not been executed on the tablet. A new native candidate must bind this
source and retain the precise wait result before choosing a repair.

The selected no-input diagnostic da27df1121414884af34fb6eb9dfac88 subsequently
used source901e333 and payload
76f8bf5d4864620970105910bcc52ef339fb7af4638760c2ce6dee835a18d09e.
Holder19442/start135719534 reached readiness for candidate19302/start135719286;
live incarnation-bracketed cgroup bytes match both exact dedicated paths.
The terminal tag is composition/canceled. Reaching this phase establishes that
the application accepted holder readiness before attempting UI composition.
It does not identify the earlier composition refusal: stopping during inspection
replaced that tag with cancellation. No selected request or GO was published,
no writer child ran and the original event trace is empty.

Original diagnostic archive:
365ce8a0985b2872282bd12d92e9ac2fc1ba980dcc6474c4267ed358600fcaca.
Stock22840/start135730627 and initially active Buddy were restored through
separately verified no-GO reconciliation. All26 original files and full12-page
order again match the original748,544-byte archive. The canceled diagnostic is
spent and does not qualify writing, page identity or a navigation repair.

A separate StartupWait state helper now retains the last and previous distinct
bounded reasons. Beginning a repeated composition inspection cannot overwrite
its prior refusal with pending; repeated cancellation cannot replace the prior
distinct reason. The actual cancellation/pending/repeated/bound counterexample
passes4 owned cases/6 checks each on host, ARM-QEMU and sanitizer, receipt
e6d61b100252173a0682e3db7c61c1eca0ae743d32094610fb2ba10197a53a66.
Review38 checks recover all pre-diagnostic action/admission/clock source tokens
after removing only diagnostic operations, bind all25 full-link inputs and
confirm the unchanged admission fragment, receipt
8aaa0c360be300c474cbd674d85547c49406c8936915b05245e17a590d89c8b2.
Full ARM link and8 fixture/11 conflicting-selector refusals pass, receipt
715c646dd7e95a48a3143d2e8749370931dcbfd9488224a05ce8cf135202264b.
The link-only artifact is
ef33aed9f1676d86943450be5631ca9a7d193c450fd7b2e027c0cfa722e4635c;
this successor has not yet been selected or executed natively.

An offline REA6.7/Ghidra12.1.4 follow-up against the unchanged saved binary found
no matching procedure names in the stripped target, then used exact metadata
dispatch and wrapper records. Together with hash-verified retained MainView
source, it corroborates the existing stock route and configuration-map call
already used by the diagnostic navigation helper. It adds no runtime discovery,
readiness or invocation qualification. Bundle
dd3599460e3ecba955e7d4d7737e261199298a5ae1802d1194e40aa825b78444
and snapshot
634158dab134cbf2cfa447f5460a8f0c46b8b7f0d9745f3d1c74fd4c7c1e70f0
were exported before closing and verifying absence of the task-owned container.
The two unrelated OpenCode containers and original binary remain unchanged;
no global tool registration or installation was changed.

## Simulator calibration and remaining native work

Model a multi-threaded Qt parent separately from a single-threaded fresh writer.
Preserve descriptor pin/replacement failures, explicit role mapping, extra-FD
closure, parent loss/cancellation, one-use launch, child exit/status, observer and
release completion as distinct facts. Use the untouched183-record stream for
consumer replay. Do not collapse process creation, accepted input, rendered
target ink or durable reopen into one success flag.

Also retain the pre-GO-cancellation path: a fully prepared holder can exit with
an explicit refusal and zero input, causing a conservative generic recovery
failure even though a separate no-GO reconciliation can restore initial service
state. Keep that reconciliation distinct from successful pen release.

The real input test still needs a freshly selected complete packet, exact
production parent/worker/provider binding, intended existing blank-page identity,
one fixed stroke, observed release, blank/ink/reopened captures, ordinary close
and stable storage reconciliation. Product integration and broad
model/firmware qualification remain separate.

Source basis: current source, frozen host/SDK/sanitizer outputs, original native
profile/owned-file/composed-admission streams, the separate transient cgroup
profile, byte-checked archives and the fresh full backup, plus exact retained
QML and the separately exported offline REA/Ghidra records.
Device/context/completion facts in the FIFO suite are explicit models; native
ink and durable authority are not inferred.

