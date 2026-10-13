# Independent public Qt reader metadata census

October 13, 2026 precode. Root selected concrete source preparation following
the independently reviewed [operation distinction](../../../docs/research/reader-public-bootstrap-assessment.md).
This is a new observation implementation, not a deployment selection.

## Purpose and excluded closure

One explicit request tests whether the focused visible active QQuickWindow has
a bounded QObject domain containing the two source-established reader anchors
with consistent visual owners and declared property metadata. Do not read
application document/page/controller/sceneView property values. Do not include
the owned current-reference selector, creation Probe, FactsEntry, held 8112
configuration/publisher/witness/navigation, any 0404 executor, QML helper/import,
image/render/input/native method, service change or document mutation. Public
QObject names and Qt structural getters are actual target-side calls.

## Separate entry and fixed request contract

New tools/reader_metadata_census.h provides the bounded observation; a separate
tools/reader_metadata_entry.h owns only request/receipt lifecycle, and
tools/reader_metadata_startup.cpp registers one public Qt startup callback and
queues application-context initialization. No native objects are deleted. The
entry is application-parented and stays inert after termination until application
destruction; the module must remain loaded through process exit. No hot unload.

Native compilation requires a frozen 32 lowercase hex READER_CENSUS_NONCE.
The exact root is /run/rmb-reader-census-<nonce>, mode0700, UID0. Open with
O_DIRECTORY/O_NOFOLLOW and retain device/inode; re-open and compare before
admission/publication. owner is mode0600 single-link regular, exactly nonce.
attempt.identity is mode0600 single-link regular, `<pid> <start>\n`, matching
getpid and bounded /proc/self/stat start ticks. No stale identity reuse.
Read /proc/self/stat with at most4097bytes and refuse over4096. PID/start are
positive canonical decimal tokens, at most20digits each, without leading zeros.
owner is exactly32bytes; attempt.identity is at most42bytes and census-request
at most75bytes. Compare exact bytes, including the one required final newline;
no trimmed whitespace/trailing data is accepted. Open each input O_NOFOLLOW,
require regular mode0600/UID matching the root/single-link and bounded size,
then verify retained-handle device/inode/mode/uid/link/size metadata after the
read. Any short/error/overrun/metadata change or close error refuses. Revalidate
root device/inode and cancellation/restore absence before EACH ready/result
publication. Missing or partial output is unknown; close error is never success.

Install a directory QFileSystemWatcher before publishing census-ready, then
perform one queued initial request check to cover watch installation ordering.
Directory events coalesce one queued check; no polling timer or input events.
The first present request is consumed once: census-request must be mode0600,
single-link regular and exactly `<nonce> <pid> <start>\n`. Invalid requests
terminate without census; missing requests wait only for watcher events or the
fixed refusal deadline. Existing census-ready/census-result prevent startup.
census-cancel or restore.claim existence terminates/refuses; metadata errors
other than absent are failures. aboutToQuit cancels; no new callback is admitted
after termination. No watcher reconnection, callback retry or deadline reset.
Allow at most32 queued request checks, including the initial one; further
directory-event activity terminates with a fixed refusal rather than extending
work. Queued callbacks retain QPointer and check sticky terminal state.

Setup lifetime is at most60000ms from entry construction. The accepted census
also has a1000ms cooperative monotonic budget nested inside that original bound.
Public list allocation, Qt calls and filesystem I/O are not preemptible; these
are cooperative limits, not a hard timeout or lifetime/render fence. An external
independent recovery actor and actual packet stop/restoration bounds remain
mandatory. No borrowed pointer crosses an asynchronous boundary except QPointer.

## Observation and wire

Take one allWindows snapshot, maximum16. Require current GUI thread, one
unambiguous associated engine, focused QQuickWindow visible/active on that
engine/thread, retained content root, and repeat governing identity checks at
the endpoint. Traverse one breadth-first deduplicated union of QObject children
and QQuickItem visual childItems, at most128 distinct enqueued/visited objects,
first-discovery depth12, and64 unique combined children per object (not64+64).
Immediately guard snapshots; reject lost nodes and visual nodes belonging to
another window. Nonvisual children are reached only through included ownership;
crosslink duplicates neither consume another node nor duplicate anchors. Collect exact DocumentView.ToolSettings and
SceneViewsRoot names; require one each. Helper remains nonvisual QObject;
container, reader and device are QQuickItems. Require helper parent=reader,
container parentItem=device, device parentItem=reader and those visual owners
visible/enabled in the selected window. Retain QPointers and recheck around Qt
calls; no first-match partial success or expanded search after refusal.

Inspect only metadata for reader document/currentPageId/sceneController, helper
document and device document/pageId/controller/sceneView. Bound each owner's
propertyCount to512 before lookup. Require properties present/readable and
QObject-pointer domains for pointer declarations. Report the two page-key
QString declarations as a boolean observation, without interpreting any value or
claiming a native page representation. No QMetaProperty::read/QObject::property.

census-ready and census-result are published from exclusive no-follow mode0600
.pending-ready/.pending-result files after one bounded full write and checked
close. Recheck root/cancel/restore, then linkat to the final absent name,
recheck root/cancel/restore after linking while pending still exists, and only
then unlink the pending name to commit. A failed post-link guard leaves a
fully formed two-link file plus pending; it is unknown, not a published result.
No overwrite or retry; final files must be single-link
regular mode0600 with exact expected identity. Pending or double-link files are
invalid/unknown and retained for Main's exact cleanup. This makes close failure
observable as missing final publication, rather than a full-looking final file.
At most one ready payload (75bytes) and one result (1024bytes), with at most two
names per payload during publication. The entry emits no deliberate log/stderr;
Qt runtime diagnostics still require Main's independent external stderr cap. Ready contains
exact nonce/pid/start binding. Result is one JSON object no larger than1024bytes:
schema1, nonce, pid/start as strings, fixed stage, application_thread,
current_window, visited, helper_count, container_count, owner_links, metadata,
page_string_domain and native_authority=false. Counts on refusal are partial
diagnostics, never partial object references or authority. No addresses,
document IDs, arbitrary names, exception text or external data are emitted.
Missing/partial result is unknown, never success. Failures before a trustworthy
root/identity produce no receipt. A cancel/restore marker or lost ownership
prevents publication, leaving the externally classified result absent/unknown.

## Focused source and artifact verification

Existing Sol reviews this contract before code and actual source after code.
Original owned QML exercises positive metadata/anchor containment, duplicate,
cap and context loss; declared value getters are instrumented so zero calls is
asserted. Owned filesystem/entry cases cover one valid request, invalid binding,
cancel/expiry, duplicate output and root replacement without native effects.
Keep this one small suite; no broad old matrices. Use existing ARM SDK5.8.203,
vendor Qt6.10.3/qemu/offscreen; compile the independent startup shared object with
an explicitly test-only nonce and no tablet transfer. Frozen native nonce,
actual provider/ELF/import hashes and whole packet are a later reviewed selection.

Main supplies fresh process/fixture state, permissible trigger/setup, independent
actor, numeric observation/stop/restoration/outer bounds and preserved original
service policy before deployment selection. Reusing generic recovery machinery
requires applicability review, not reuse of held behavior or old timing proof.
Main remains sole operator; no target action follows from tests or source review.

Implementation finding: the first original real QML Window fixture refused
anchor uniqueness under a QObject-only content-root walk. Its visual reader
children were not QObject children of contentItem. Preserve that negative result.
Sol accepted the single bounded union traversal above before the core change,
with unchanged caps and no artificial fixture reparenting. The positive original
Window fixture now also asserts crosslink deduplication; native topology remains
unmeasured.

The focused fixture also found that renaming the watched root need not promptly
notify this Qt watcher. Root identity is checked on every subsequent queued
check, admission and publication; the real setup deadline remains active. The
root-replacement test uses a fixture-only elapsed-clock offset to exercise the
next deadline check and output suppression, not to claim immediate notification.
The production shared object excludes that test hook.

Publication is not an atomic cancellation/ownership fence. Main's packet
consumer must bracket receipt collection with the original root device/inode,
nonce, current PID/start and owner/attempt records, cancel/restore absence,
pending-name absence and exact final regular-file UID/mode/single-link/size
checks. A fully formed JSON object alone is not success. Cancellation after the
sampled post-link guard, including the final unlink interval, cannot be excluded
by this protocol. Root or guard loss/IO ambiguity remains unknown. The focused
post-link cancellation regression leaves final and pending with two links and
asserts that this complete-looking object is invalid; it never becomes authority.

## Source/artifact checkpoint

The focused ARM-owned suite and actual owned-app preload/startup path passed
using SDK5.8.203, GCC13.4.0, vendor Qt6.10.3 and qemu/offscreen/software. All
instrumented application-value getters stayed at zero. Sol independently
accepted source/assertions, including the repaired post-link guard; execution
is the SDK author's test evidence, not an independent reviewer rerun.
The startup source also compile-refuses READER_METADATA_TESTING so fixture
clock/publication hooks cannot enter that artifact; its exact expected compiler
refusal was checked.

Retained r3 test-only shared object SHA256:
`5c6e9d611267f603a170b05d934dc04384816ca465d92950ca860a056a30852c`.
It is ELF32 little-endian ARM EABI5 hard-float DYN. Direct NEEDED entries are
Qt6Quick, Qt6Gui, Qt6Qml, Qt6Network, Qt6Core, libstdc++, libm, libgcc_s and libc.
QtNetwork linkage is a build dependency, not a diagnostic network operation.
Version requirements include Qt_6/Qt_6.10, GLIBC_2.4/2.34,
GLIBCXX_3.4/3.4.14, CXXABI_1.3/1.3.9 and GCC_3.5.
Private source/artifact and SDK-provider hash receipts accompany the ELF.
These are build-sysroot providers, not a fresh tablet provider inventory.

The fixed all-a nonce and r3 artifact are owned-test inputs only, never a native
selection. A separately frozen nonce/artifact/provider closure and Main's
independent full process/fixture/recovery/transport/cleanup packet remain open.
The entry stays resident and inert after termination; unload requires process
exit, not deleting a library while Qt callbacks/types remain registered.
No device was accessed and no deployment or native qualification is implied.
