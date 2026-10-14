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

### Native binding-witness result: deferred then installed, no accepted frame marker

Main's fresh `62416` packet used sourcedc8a4034 and producer artifact
`73efaa5e20d95d35c87488cb2e5348c04c8856a1762aee6daf4bb7facc8097d3`.
SDK directly decoded14 retained same-generation metadata samples. The208-byte
initial waiting witness (SHA256
`c0c76e5acfa15b454b262f9c497508b720d414f5ad62acc65d3be5cd03376f01`)
was present in all14 with false/false fields. A separate208-byte bound witness
(SHA256 `79dc886c0710ef0e84c57ac897d2094c396ed85a99eea70bd4723c228707fc8f`)
was present in12 with true/true fields. Its first recorded positive collection
interval was5.734–6.281 seconds after activation; this is not an exact creation
timestamp or proof of current binding state across the interval.

These files witness historical initial deferral followed by sampled successful
public binding and observer installation. Initial waiting was not overwritten.
No accepted frame marker appeared in the sampled budget; no request, image or
result scalar followed. It is incorrect to say binding never occurred. It is
also incorrect to infer the signal never emitted, that callback context stayed
valid, or that content/panel/current-page/owner/lifetime was qualified.

Original operator receipt SHA256
`7e65dc1c29f9b1586b46451d013ee2ef717cd6e64225070c06a1d55987ccce34`
records PacketUnknown/no request, elapsed60.907s and restored=true/cleanup=true.
Main's live-host-started primary and original cleanup succeeded; subsequent
read-only stock verification retained the original policy, no job, unchanged PDF
and all owned resources absent. Timer expiry, host loss and fallback remain
untested. Earlier failed actor/observer outcomes remain distinct.

Root's existing Sol reviewer independently inspected160 raw streams/80 stopped
jobs and18 selected-source snapshots, plus separate final2streams/1job, and
accepted the retained outcome with no new evidence defect. SDK directly inspected
the narrower sample/receipt/final-stock records; no device execution or full
inventory rerun is claimed. Private records/reviews have no public transcript link.
The nonce is spent; no unchanged experiment is repeated.

The [pinned frame-signal applicability assessment](../research/vendor-frame-signal-applicability.md)
uses retained/static evidence to compare documented public paths. It does not
diagnose this missing marker or select a backend/signal implementation/native retry.

### First native visual result: valid blank pixels, incomplete packet closeout

Main's separately selected fresh `c69` packet used producer source00fac00d and
artifact SHA256
`23f53f6a64cbf3e4636b2d98262807a8fd0fdfa5f101a9bd6b85be05d5ab853d`.
It produced a11,957-byte1404×1872 PNG, SHA256
`0a4385e6b501cd03098de51415aea258ae05558541ee7398c713a2b1accb713a`.
The scalar reported visual-image-observed, application_thread/current_window true,
storage10,513,152 bytes and native_authority=false. Main's guarded pair and
complete RGB decode recorded7,884,864 decoded bytes. SDK directly read the scalar
and both operator receipts, checked the saved PNG hash/size and visually inspected
the image: blank white, with no visible Library/document content.

This is measured native acquisition of valid blank pixels. It does not establish
freshness, requested-window-only pixels, panel state, current page/owner, object
lifetime or that the UI was ready. The cause of blankness remains unknown.

Original operator receipt SHA256
`e997f91e01dd1d098113681ba902dedb66ba8fb221bbc30e0e3caabdb72f8fcb`
ended exit1 after35.406 seconds with restored=false, cleanup=false and
recovery_unknown=PacketUnknown following restore-state verification exit1.
Main reports the configured primary itself exited0 with a restored marker and
inactive fallback; a subsequent exact read-only verification trace passed stock
checks. These actor/verification observations do not rewrite the original receipt
or explain why its earlier verification failed.

A separate cleanup-only continuation, receipt SHA256
`7339c674f8534f70c3332125f4ba57c4eaf3811c8b9aa883faa48675126b7d23`,
records no activation/request, original clock preserved, original receipt unchanged,
restored=true and cleanup=true. Main verified stock, preserved the final raw image
as forensic evidence with accepted_image=false and removed task-owned units/root/
drop-in/parent state. This is separately attributed Main execution evidence;
the complete original packet/recovery gate did not pass. Independent timer expiry,
host loss and fallback remain untested. The nonce is spent and no replay is selected.

Root's existing Sol reviewer independently checked132 captured stream hashes,
66 stopped-tree transport receipts and15 selected-source hashes, and inspected
the blank image. SDK's direct image/scalar/operator inspection is narrower and
does not claim to repeat that full review. Private records/review messages have
no public transcript link. The readiness design review accepted this bounded
result wording while preserving the original closeout failure.

The later [passive frame-readiness increment](../../openspec/changes/establish-native-platform-contract/visual-frame-readiness.md)
is default-off source/owned verification only, including its repaired request-order
guard. It does not select another native attempt; a frame-stage observation cannot
by itself repair blank pixels or supply page/render authority. Initial r3 shared
objects do not bind the repaired r4 source, and natural vendor emission remains
unqualified. See the linked checkpoint for positive synthetic controls, preserved
natural-signal absence and the source-review P2/repair.

### First native passive-mode result: no witnessed frame, original closeout passed

Main's fresh `a6` passive packet used source29a2e8d and producer artifact SHA256
`fea402e9dd8dee26f0cdb5b2ce516e2b53fd58ce0c5ad4803678764ed6ee6155`.
SDK directly inspected14 retained metadata samples: before/after process-generation
checks were true, but frame_ready_hex was empty in every sample. No accepted
frame marker, request, image or result scalar was recorded. Main stopped at the
original host admission/margin cutoff without extending the entry clock. Direct
transport timing places the first sample start4.250 seconds after activation
attempt start and the last sample return14.109 seconds after it; restore-start
began at14.359 seconds. This was not a full50s/60s no-signal observation.

Original operator receipt SHA256
`d65ba3ff782a619d020b6462614908b3df4bfb5dae3c1ec68c8bde8f74243b10`
records activated=true, request_attempted=false, outcome_unknown=PacketUnknown,
elapsed56.890 seconds and restored=true/cleanup=true in that same receipt.
The executed operator source has distinct SHA256
`9292b5bb6a3834d64301260b106bf6667e74bd75caceca067096fd53e48a2b8b`;
it must not be confused with the result receipt's identity.

Restore samples054–059 recorded RESTORE_PENDING, then060 recorded STOCK_VERIFIED
with primary exit0/active-exited and fallback inactive-dead. This attempt did not
exercise c69's precise STOCK_NOT_VERIFIED-under-restored-marker failure branch.
Main's later read-only final stock record shows
the original command/restart/kill policy, no pending job, unchanged PDF hash and
all task-owned resources absent. This is successful original live-host-started
primary restoration/cleanup; independent timer expiry, host loss, fallback and
full production recovery remain untested. It does not retroactively change c69's
failed observer closeout or earlier automatic-actor failures.

Main reports140 streams/70 stopped-tree transport receipts and17 selected-source
snapshots preserved; the subsequent read-only final check is separate. SDK's
direct sample/receipt/stock-record inspection does not claim to repeat the full
inventory or independently execute the device work. Private evidence has no
public transcript link. Root's existing Sol reviewer independently verified those
140 streams/70 stopped transports and17 snapshots, plus the separate final check's
two streams/one stopped job, and accepted the bounded outcome. CLI exit1 is
attributed to Main's owner tool output, not a separately saved CLI-exit transcript.

Missing frame-ready output does not diagnose lack of window eligibility/binding,
vendor emission, signal timing/delivery, callback context, publication or sufficient
admission margin. It proves none of visible content, frame freshness, current
owner/page or lifetime. This nonce is spent and no identical retry is selected.
Infrastructure ready was published before focus-observer installation/binding,
so it does not establish that either occurred. Frame and capture paths use the
same public window/engine/root/focus/visible/active guards; the records do not
support a new-stricter-engine-guard explanation. Earlier c69 window admission was
a different process/time and cannot establish a6 startup binding.
The new [binding-stage witness proposal](../../openspec/changes/establish-native-platform-contract/visual-binding-witness.md)
now has independently accepted default-off source/control implementation and
preserves those unknowns. It supplies historical stage evidence only and selects
no native artifact, nonce or retry; see the linked source/control checkpoint.

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
## Native fixed frameSwapped result: delivery observed, in-flight witness refusal

Main's spent d578e98006d34a3ab3de22644af94020 packet used the independently
accepted exact build/profile closure ([artifact receipt](https://github.com/s116821/ReMarkableOpenSDK/pull/1#issuecomment-6158725527)).
Original execution1791940805365692800 receipt SHA256
`2ccc40f5819727d0512fae65069227ac9cd7824321549fd6f04d8b5a629213ed`
records activation, one request attempted, ValueError unknown outcome and original
live-host primary restoration/cleanup true, elapsed56.532s. It is not replayed.

Candidate14935/start46762940 produced initial waiting then bound witnesses and
the exact198-byte GUI-delivered frameSwapped marker, SHA256
`6e263dff6655fe36d048b401886a2f1de46608d4be998bf2daff1343aa1dc645`.
This is a new observed diagnostic delivery milestone; it does not qualify active
backend details, current useful pixels/document/page/owner or physical refresh.

The last pre-restoration044 metadata SHA256
`8fd4eb55d0fc2bf0214c61bb5e50d030b7b2a24efe9b0aadae2118f5b102138c`
contains empty result bytes and null result/image metadata in both brackets,
pending_image=true, pending_result=false, process_current=true and no cancel/
restore. Host collect_visual_metadata invokes optional historical witness
validation before returning to the result loop. That unchanged validator rejects
pending_image; the result loop never parses an empty scalar. A narrow offline
call of the exact original consumer against retained044 reproduced ValueError
"witness pending/cancel ambiguity" for both witnesses. This demonstrates the
concrete source/raw refusal path, not a separately captured original traceback.
SDK assessment SHA256 `c2e8542343d7cc341bcef295024476a112546cd80b60518817931cf3f94923ee`
also preserves initial offline dependency/import-alias setup errors; no original
source or device state changed. Internal raw evidence has no public transcript link.

Recovery-verified forensic retention produced a21066-byte PNG SHA256
`3a899156cd9084e093aed66260332968eae2c6447b8040e1591d3df8a79372f4`.
Main, Root and SDK visually inspected a reMarkable loading screen. It remains
accepted_image=false: no accepted scalar/image pair or normal bounded decode was
completed. Producer image-before-scalar publication is non-atomic; later context
loss or publication refusal can leave orphan pixels. The record does not establish
the scalar's exact publication time or why the image shows loading.

Later post-STOCK_VERIFIED053 metadata SHA256
`3bff2a09f2ba32e704138ad051db86064306abd2bdd95d84cf470202016d75ca`
retains a371-byte success-shaped scalar SHA256
`cfbf6685ed04d34daf28ddee1c941a08155d322a3e848f2b8d800c8bb8e52c5e`,
stage visual-image-observed,1404x1872/DPR1/storage10513152 and the forensic PNG
hash above. Both brackets have process_current=false and restore=true.
Those bytes are forensic only, despite their matching hash and success-shaped
fields: no normal accepted current scalar/image pair or decode followed. Thus
empty044 does not establish that no scalar was ever published. No producer
repair or replay is justified by the host's premature refusal alone.

Separate final read-only verification shows stockPID15242, original
/usr/bin/xochitl --system, Restarton-failure/direct/control-group, no Job, three
active checks, fixture PDF SHA256332446943966f6c3111a09c2df05673a94282ea247bf78c946f2c6e6d479e1f6
and all owned resources absent. Original closeout and separate final verification
are distinct; timer-expiry/host-loss/fallback qualification remains open.
Root's existing independent reviewer checked the original receipt,18 selected
snapshots and156 streams/78 stopped jobs (53520 bytes), and the separate final
receipt `88a58b25eeae2860690a888a4e66e8a2080b962062e5ffa55d6bfe11c0843753`
with two streams/one stopped job. The concrete host P2 is confirmed; original
closeout and separate final verification are verified without reviewer device
operation, execution or rerun. Private review has no public transcript link.
Prior insertion/ink/reopen milestones and full content/lifetime/Reader gates remain.

The [nonadmitting in-flight polling proposal](../../openspec/changes/establish-native-platform-contract/visual-fixed-signal-profile.md#proposed-main-only-nonadmitting-in-flight-poll-phase)
addresses the concrete host phase mismatch without loosening final acceptance.

## Native in-flight host repair result: normal capture of loading pixels

Main's fresh, now-spent d1e03cc3ff3042d29897ee8c86468145 used unchanged accepted
producer679d, fixed frameSwapped/all three flags1 and the distinct accepted
nonadmitting polling repair. Exact [fresh artifact/packet receipt](https://github.com/s116821/ReMarkableOpenSDK/pull/1#issuecomment-6158984543)
records source binding; no old d578 artifact/identity was replayed.

Original execution1791942094423158700 receipt SHA256
`8bf2fb628d48c368b1946826bea100cb72069c887f01f2d79ba4ad3c7ea074d7`
records activation, one request, original restoration/cleanup true,59.859s and
no unknown outcome. Main reports original CLI exit0; SDK read the receipt and
verified82 recorded jobs all exit0/tree_gone/readers_stopped. Independent actual
review accepted164 raw streams/19 selected snapshots,82 stopped jobs/86586 bytes
with no actionable findings. No reviewer rerun or device operation occurred.

Candidate16082/start46891849 delivered the198-byte selected frame marker SHA256
`84410e0ac5a77b7a7b998375f6bca6064b9719b5c96f63dbf5cd919d0b236f5f`.
The normal strict371-byte scalar SHA256
`d420e123b4bc5ddc2aa9901159a74c7bcb86662be62673c81708452d261a3b2c`
matches the21066-byte PNG SHA256
`3a899156cd9084e093aed66260332968eae2c6447b8040e1591d3df8a79372f4`:
1404x1872,DPR1,storage10513152,visual-image-observed,application-thread/current-
window true,native_authority=false. The original normal-pair path completed and
its separate bounded owned decoder reports full RGB7884864 bytes, matching hash/
dimensions and complete_pixel_decode_verified=true. That final decode outcome is
receipt-reported; no independently retained raw decoder transcript exists. The
pair header's earlier
false decode flag describes its pre-decode stage, not a conflicting final result.
No decoder or native operation was repeated by SDK.

Main and SDK visually inspected the normal image: reMarkable loading logo/text
and progress line. This advances guarded capture/pair/full-decode integration;
it does not establish useful document pixels, current page/owner, physical panel
state or freshly produced content. Its identical hash to d578's forensic image
does not prove caching, emission timing or freshness. d578's original failure
remains a failure; d1's normal success does not retroactively accept it.

Separate final receipt SHA256
`64ff3ea992d138514a127592bc85482f1eb6282f459e7991025c8655060240e1`
and final read-only output verify stockPID16402, original command/restart policy,
no Job, three active checks, unchanged fixture PDF hash33244694... and all owned
resources absent. Original live-host primary closeout and separate final checks
are distinct; independent timer-expiry/host-loss/fallback qualification stays open.
Private execution evidence has no public transcript link.

The remaining content gate is a supported join between the observed window/frame,
useful current-document pixels and an independently qualified document/page/owner
and lifetime context. A first eligible window/first latched notification can be a
loading scene; choosing another signal, waiting a guessed interval, replaying the
spent packet or treating any nonblank image as a document would not supply that
join. Existing bounded metadata/no-anchor findings do not prove global document
absence. No producer repair, new getter/hook, forced input/render, clock extension,
new build/nonce or fresh experiment follows from this source-only assessment.
Prior insertion/ink/reopen milestones stay credited; full Reader gates stay open.

Source basis: exact preserved Main receipt/scalar/PNG/decoder records, SDK local
hash/readback and visual inspection, and current fixed-window/signal contracts.
The distinction between capture plumbing and missing content/owner qualification
is source interpretation, not a diagnosis of vendor loading/caching behavior.
