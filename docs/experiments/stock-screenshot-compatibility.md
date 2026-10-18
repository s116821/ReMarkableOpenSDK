# Bounded stock screenshot observation on RM2

October 17, 2026. One maintained Buddy screenshot example completed against the
selected idle RM2 development profile, with full image decoding, process/cgroup
absence verification and removal of its owned staging files. This is a narrow
development observation, not an SDK native adapter or document/page capability.
The active native-platform delivery and its remaining gates stay open.

## Utility names do not establish option support

The selected profile records firmware 3.28.0.172. Actual utility diagnostics
identify BusyBox 1.36.1, but a version string does not identify its enabled
configuration or qualify other firmware builds.

| Actual operation | Observed result | Consequence |
| --- | --- | --- |
| `head -c` | Invalid option; cleanup stopped before unlink | Finding the command is insufficient to qualify byte limiting. |
| `dd bs=1 count=2 status=none` on literal `abc` | Usage refusal, no accepted sample | Do not assume the optional quiet-status operand exists. |
| `dd bs=1 count=2` on literal `abc` | Exact `ab`, exit zero, then exactly two normal count lines | Basic byte-count operands and this diagnostic format were observed separately. |

The [pinned upstream dd implementation](https://github.com/mirror/busybox/blob/1_36_1/coreutils/dd.c)
supports the distinction: `bs` and `count` are base operands, while status
selection is configurable. Upstream source is not proof of vendor configuration.
The small actual operation supplied that missing evidence. Historical local
dummy results had suppressed dd diagnostics and did not establish quiet-status
support; they were not rerun as a prerequisite.

## Preserve the producer log and limiter diagnostics

The corrected capture uses a FIFO reader with basic operands only:

```sh
/usr/bin/dd bs=1 count=16385 < "$owned_fifo" > "$owned_log" 2>&1
```

Both output descriptors share one open-file description. The file retains the
producer bytes followed by dd's diagnostics. The parent must observe successful
completion of both the screenshot process and the reader before accepting a
capture. No diagnostic stream is discarded or filtered from manager stderr.

The producer limit remains 16 KiB; byte 16385 is an overflow sentinel. The combined
raw log has a separate 17 KiB bound, and the capture response retains its 32 KiB
bound. A closed header declares the combined byte count and SHA-256 before the
raw payload. After exact framing, hash and normal manager completion, the host
requires an end-anchored pair of normal count lines. Both counts must equal the
producer-prefix byte length, which must be at most 16384. Leading-zero counts,
mismatches, overflow, extra limiter warnings, an unqualified third status line or a
nonzero exit refuse acceptance. NULs and embedded framing markers in the producer
prefix remain ordinary bytes. Retain the full combined raw/hash and a separate
producer-prefix archive.

The successful observation retained 2057 producer bytes and 2094 combined bytes.
This does not qualify arbitrary future utility output; unexpected output still
requires a separate source/evidence decision rather than a fallback or retry.

## Bound the actual started payload

The private operator reserves seven distinct identities before contact and uses
one exact fixed route and host-key policy. Each phase obtains a fresh same-boot
clock/profile sample. Source bytes are supplied through a short, count/hash-bound
plaintext loader using a regular host input prepared before the clock sample;
no large remote command or on-device script staging is required.

The screenshot runs in an ephemeral oneshot unit with no restart, a 20-second
startup allowance, two-second stop grace and control-group termination with final
SIGKILL. The running-job limit is 24 seconds. The selected capture also checks
CPU, address-space, file-size and core limits. Screenshot execution receives the
remainder of the original startup allowance, not a new allowance after setup.
These controls do not bound manager queue or whole-SSH-launcher lifetime; late
unit/cgroup/journal metadata remains a disclosed residual.

The fixed target entry expiry is sampled uptime plus 150 centiseconds, never
renewed. The clock bracket is at most 900 ms, and immediate parsing must finish
within 100 ms of its receipt. The final local pre-resume deadline was explicitly
amended from a fixed 100 ms to the minimum of receipt plus 250 ms and clock-request start
plus 980198019 ns, additionally capped by admission, phase and overall deadlines.
The latter integer is `floor(99 * 10 ms * 100 / 101)`: with the selected 1.01
relative-clock upper-rate and centisecond precision assumptions, it retains at
least 50 centiseconds at the observed time check. A 900 ms sample bracket leaves
only about 80 ms after receipt, not 250 ms.

The unchanged Windows ownership helper checks time before the thread-owner check
and `ResumeThread`. This is a bound at that observed pre-resume check, not a hard
real-time guarantee against subsequent scheduling or API delay. Remote boot and
expiry guards remain necessary before effects. Guard-to-syscall races, clock
assumptions and kernel/manager failure limits are not eliminated.

Capture validates the full executable digest once in the parent, then rechecks
the same owned directory/file metadata in the child alongside the final stock
identity and expiry guards. Removing the duplicate full digest and redundant
parent identity query reduced setup work without extending the target horizon.
Exact inode, owner, mode, size and link checks do not eliminate concurrent
privileged mutation between validation and use.

After capture, a separately bounded phase verifies the recorded worker,
screenshot and reader start identities are absent or different and both exact
cgroup paths are absent or empty without subgroups. Fetch archives the existing
PNG; a bounded local decoder validates all PNG structure/CRC/deflate data and the
expected normalized 768 by 1024 grayscale image. Those are normalized dimensions,
not the native panel dimensions. Final cleanup rechecks exact owned file
identities/digests and a fresh boot/expiry guard, unlinks only those files and
removes the empty literal staging directory. Unknown outcomes stop later contact.

The separate recovery of an earlier failed attempt used direct SHA-256 after
exact fixed-size regular-file guards and before metadata rechecks. It relied on
the owned file and short unit/CPU/host bounds, not a hard read-byte cap against
privileged file growth. Inert failed-unit metadata was left separately tracked;
successful file cleanup is not an assertion that every historical experiment
succeeded or every metadata record disappeared.

## Result and evidence boundaries

The maintained development executable was built at Buddy source
`8d4be268c3ab08c91b1e45776e628004739ae682`, with manual source/blob/build binding;
the build did not supply an embedded Git stamp. Its 7,097,876-byte artifact has
SHA-256 `36980c0123ab2d2edfafc8a2cf7c7a85f6db9036741a5f5febf8315fc439fdb0`.
See the [ordinary-entry source assessment](../research/reader-public-bootstrap-assessment.md)
for the maintained example and the distinction between entry and identity.

The accepted one-shot completed all six phases and full local decoding. Nineteen
local process trees/readers and all 38 archived raw-stream hashes were accounted
for. Capture manager completion was normal; later process/cgroup absence and
staging-directory absence were explicitly observed. The saved PNG was viewed
once locally, showing the tutorial title **Set a template**, a circled **2/8**,
the drawing toolbar and a swipe instruction. Those are displayed pixels only.
No navigation, recapture, input, native activation, account pairing or sync was
performed by this observation.

Private evidence identities, provided for exact maintainer correspondence:

| Evidence | SHA-256 |
| --- | --- |
| Basic-dd operation receipt | `b0f00512785c65c04151c85f9282090e8030f15b8b5b5322afcf5af33edb706a` |
| Accepted visual operator source manifest | `a8b1e09fd7572fc98b6e51db3677af5852fe17131a518f389980e4619d50d066` |
| Accepted visual execution receipt | `c480d2e3be93f7b8caa3c071ebb0477388817f62cd14a2c91f5034972de119ac` |
| Saved normalized PNG | `62dfc6a10318d2109ba211f07be994c968f92677f5d7d6728b4c3ce5c164bf1c` |

These raw receipts and the image are private and have no public download link.
The operator checked the hashes, decoded the PNG and inspected that saved image;
an independent reviewer also verified the receipt/stream correspondence and
viewed the same saved image. This is captured visual corroboration only. It does
not establish a native document/page ID, selected window, render fence, panel
correspondence, insertion, same-candidate navigation, writing or persistence.
Original failures and spent identities remain immutable. The successful narrow
observation neither releases held operations nor closes native qualification.

Source basis: the actual private operator outputs and saved image described
above, manual source/build/artifact binding, and pinned upstream BusyBox source.
Clock/cap reasoning is the reviewed operator policy under its stated assumptions,
not a measured hard real-time or vendor-wide guarantee.
