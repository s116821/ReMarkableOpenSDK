# RM2 pen device: read-only native profile

October 23, 2026. One standalone ARM inspector executed on the idle RM2 and
returned a complete checked profile. This was a read-only open and ioctl query:
the program contains no input-event read, write or grab. No drawing, navigation,
preload activation or stock service change occurred.

## Actual observed profile

The device was Wacom I2C Digitizer at the fixed character device13:65, with bus
`0x0018`, vendor`0x2d1f`, product`0x0095`, version`0x1231`. File identity, exact
event/key/axis capability bitmaps and full key-query lengths passed. Two matching
key and complete axis snapshots were read, followed by checked descriptor close
before publishing success. Every queried pen key was up and pressure was zero.

| Axis | Range | Sampled value | Resolution |
| --- | --- | --- | --- |
| X | 0–20966 | 11056 | 100 |
| Y | 0–15725 | 7616 | 100 |
| Pressure | 0–4095 | 0 | 0 |
| Distance | 0–255 | 100 | 0 |
| Tilt X | -9000–9000 | 0 | 0 |
| Tilt Y | -9000–9000 | 0 | 0 |

All queried axes had fuzz0 and flat0. These values support the source-derived
RM2 geometry in the [pen ABI/release note](../research/pen-event-release-boundary.md)
and [fixed-stroke policy](fixed-stroke-release-owned-result.md). The retained X/Y
position is a sampled device state; this result does not attribute its origin
or time to any earlier stroke. It does not establish a completed input frame,
an empty queue, input routing, rendered/saved ink, or release after a failed
writer. The input_event16-byte layout remains a compiler/header fact here;
this inspector does not receive an actual event record.

## Exact candidate and actual closeout

Published source revision was
`d05963c4c33d5f0a413e6ef48a0cc831ca467405`,
[reader_pen_device_inspect.cpp](../../scripts/reader_pen_device_inspect.cpp)
SHA-256`949788e01fa1cc7ba505f0b067cea838a7cfaed33b98fc9306c44ebc697438ad`.
The vendor-ARM artifact digest was
`c9ad7ad00f0d5015affef67978091a5a6bc0df120d8ebf8de006deaaf7ebc0c8`.
Source/build review33 receipt
`61b0e974164c870b82f6627555f80b15beea03e6fc64512e9c62ae0141e3ca6c`
closed the short-query-length and unchecked-close findings before native use.
No-argument/wrong-argument emulator refusals are separate from this native query.

The fixed SCP/POSIX operator digest was
`28a1e980c49e5b4da8461b9238c13cc62fc516eb47e1e675f7cff8dc467fda29`;
its independent33-check delta review receipt was
`3b227af2ebe6e197e5a7bf61697b7e47322c6fc1c03b052e6b6ae026f6036732`.
It required exact committed source/artifact/review bytes, a freshly checked USB
route before each transfer/contact, stock PID/start/boot and live rehash of ten
historical providers. A new one-use execution claim preceded the only query.
The intrinsic three-second alarm did not fire; the original process exited0
with empty stderr. No request or event was retried.

The original stdout digest is
`ecfe94578b7add221387a11d6fb6118154a31f7ac1c1096b39fc0c8083f2ae93`.
The six-file archive digest is
`a15e73ceb42efd9c25d0a77aba05b30ef8e4790d49f405a99d3f3e30d1337506`.
Archive bytes matched owner/claim/artifact/stdout/stderr/status before cleanup.
Exact directory resolution, closed file inventory, modes, UID/single links,
all six hashes and a sampled owned-executable scan guarded flat unlink/rmdir.
The task RAM root was removed. The synchronous nonforking process return is the
primary helper-completion fact; the proc scan is not global absence proof.
Stock PID2792/start133792863 and the recorded boot remained unchanged.
Raw receipts/archive remain private without public download links. Independent
actual-run review passed127 checks, receipt SHA-256
`5f1eda589dc55641818905f01a6bfe3f4d7268cf7c9b0503cc9fb29b4e333be5`.
It reconstructed the reviewed scripts and checked all twelve original
route/SSH/SCP records, original scalar/archive and cleanup acknowledgement.
It did not execute another query. No document backup or broad service-state
comparison was performed; unchanged pinned stock identity and source-scoped
absence of document/input/service mutations are the relevant limits here.

## Preserved staging failures

An earlier fresh staging attempt stopped at shell exit127 because the tablet
lacked `base64`; only owner and an empty inspect file existed, with no execution
claim or inspector invocation. A proposed Python3 cleanup also refused before
mutation because Python3 was absent. These were host/device prerequisite
assumptions, not provider restrictions or failed pen-device queries.

The two inert files were archived, verified and removed by a separate POSIX-only
cleanup; archive digest
`6cc94addb9447c81e9472de649130e153fbb610205bfadbc3f1d5acf5a31da9b`.
The successful successor used the already working SCP transport and shell-only
cleanup under a fresh identity. Earlier failure logs are preserved; the later
success does not reclassify them. Check available tablet utilities before
selecting future transport/cleanup scripts.

Native writer/observer/recovery composition and intended-target close/save/reopen
remain open. A released snapshot before writing does not replace that work.

Source basis: exact published source/build, original live route/SCP/SSH streams,
native stdout and six-file archive, plus separate earlier failure/cleanup logs.
Engineering implications and unmeasured queue/routing/durability limits are
explicit above.
