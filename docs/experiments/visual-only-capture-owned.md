# Visual-only entry: source and owned verification

October 13, 2026. The separate
[visual entry contract](../../openspec/changes/establish-native-platform-contract/visual-only-capture-entry.md)
is implemented as an original public Qt/POSIX diagnostic. Its source closure is
visual_capture_startup.cpp → visual_capture_entry.h → visual_capture_core.h.
The existing metadata entry remains unchanged. This source acceptance does not
select a native artifact, callable resident endpoint or device operation.

## Implemented boundary

One exact nonce/PID/start request is consumed before one GUI-thread public
QQuickWindow grab. Retained application/window/engine/root pointers and sampled
focus/thread/visibility/active guards are rechecked around grab, encoding and
publication. Original construction-based 60 seconds, 32 watcher checks, nested
termination and resident app-parent lifetime remain explicit. Qt grab, codec
scratch and IO are non-preemptible; pointer/endpoint equality is not an atomic
change fence or lifetime lease.

Returned image dimensions are positive and at most1872 each; pixels at most
2,628,288 and actual storage at most10,513,152 bytes. DPR must be finite/positive.
No conversion, scaling or crop disguises quota loss. A sequential unbuffered
QIODevice writes directly to an exclusive pending image, caps accepted full
writes at1,048,576 bytes, hashes accepted bytes and permanently refuses after
overflow, short write or IO error. Grab and codec internal allocations precede
or exceed these output bounds and require Main's independent process policy.

Checked-close original inode/size/time snapshots bind pending/final publication.
Exclusive image link, post-link original-file/context guard and pending unlink
precede successful scalar publication. Scalar uses the same guarded protocol.
Image/scalar are not one atomic transaction. Orphan image, pending/two-link
names, path replacement, cancellation or missing scalar are invalid/unknown;
Main's retained-handle pair/hash/decode/context checks remain required.

Scalar schema is fixed: schema1; nonce/PID/start strings; fixed stage;
application_thread/current_window booleans; width/height; storage_bytes;
encoded_bytes; finite device_pixel_ratio; lowercase PNG SHA256 on success;
native_authority=false. Unreached dimensions/storage/hash remain zero/empty.
Success stage visual-image-observed carries no UI/page/freshness claim. A refusal
may retain image diagnostic fields without an acceptable image/scalar pair.

The closure includes no FactsEntry, source5 witness/config/executor, application
document/page/controller/current-reference value reads, input/navigation/creation,
heap selection, forced update or worker-thread grab workaround. Startup explicitly
compile-refuses fixture-only hooks. Main must visually confirm context; nonblank
pixels alone do not establish Library/document identity or native action authority.

## Focused owned verification

The author ran only the new focused suite using existing vendor SDK5.8.203,
GCC13.4.0, Qt6.10.3, ARM qemu with offscreen/software rendering and an isolated
network-disabled container. No dependency installation or tablet operation.

- Original owned QML Window public grab produced a decodable100×100 PNG.
- Exact encoded cap was admitted; cap+1, IO error and deterministic nonblocking
  pipe short write permanently refused the sink. An actual noisy PNG hit the
  encoded cap and refused successfully publishing an image.
- Injected grab cases refused null image, side/pixel/storage quotas, context loss
  and nested expiry. Nested event processing and later watcher checks never
  dispatched a second grab; malformed request refused before grab.
- Post-link image cancellation retained a complete-looking image plus pending
  name with two links and no successful scalar. Scalar post-link cancellation
  retained a single-link image and invalid two-link scalar/pending pair.
- Pending image path substitution preserved the replacement name and emitted
  image-publication-refused rather than unlinking or accepting it.
- Actual test-only shared-object preload/startup/app-queued entry produced a
  matching PNG/scalar through the original public grab. Exact expected compile
  refusal checked the startup test-hook fence.

The encoded-cap case emits the expected implicit `libpng error: Write Error`.
Producer image/scalar quotas do not cap Qt/codec stdout/stderr; Main must suppress
or separately cap them in the whole packet.

Preserved initial failures: the first compile needed an explicit QElapsedTimer
include; the first short-write fixture used a pipe that accepted its full write
and was made deterministic with a4096-byte pipe capacity; the fixture's post-link
cancellation initially fired during ready publication and was limited to image/
scalar publication. The latter two changes concern compile-forbidden fixture
hooks/controls; none relaxes production safety.

Test-only startup artifact SHA256:
`3d1df27a38ef3fe7b4d6cc6656e82b1b73138ac20fe7750cdbf4057ab8fc3fd8`
(70,368 bytes, fixed all-a nonce). It is owned-only, never a native selection.
Source/artifact receipt SHA256:
`ec38ac1a19c893d73c1262c0b8474a10c0516f6edaddba8a2bfe5c3d7a1c26a8`.
The receipt records exact five-file hashes, tool-result pass strings and failure
history; it does not pretend those strings are independent raw log files.

Root's existing Sol reviewer independently verified all five source hashes/sizes
and test artifact hash, inspected lifecycle/sink/publication/scalar/fence source
and fixture assertions, and accepted them with no actionable finding. Execution
remains the author's evidence, not a reviewer rerun. The internal review/receipt
has no public transcript link. Strict OpenSpec and ordinary publication checks
accompany the source checkpoint.

## Remaining delivery

Main owns the separate binary-image/scalar collector, decode resource limits,
visual confirmation, current providers/fixture/process state, transport/error/
copy quotas and independently bounded recovery/cleanup. Complete exact source/
artifact/whole-packet review precedes any later fresh native selection. Prior
capture/census refusals and automatic-recovery failures are preserved in
[the actual census findings](reader-metadata-census-result.md). No held/spent replay
or SourceFactsEntry adaptation follows from this entry.

This closes the new source/owned-verification increment only. It does not qualify
vendor render freshness, current page/owner, native lifetime, independent timer,
host-loss/fallback recovery, production capture or the full SDK delivery. Existing
insertion/native ink/reopen milestones remain credited. No canonical sync/archive
or merge follows from these checks.
