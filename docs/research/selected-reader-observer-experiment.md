# Selected Reader observer experiment

Status: experimental interoperability tooling. No supported native adapter,
qualified stock-Reader capability, page creation, durability receipt or crash repair is supplied
by this increment. Standalone owned fixtures may run on a host, under QEMU or on
an admitted tablet; those scopes must be recorded separately.
The original implementation uses public Qt APIs; extracted
firmware source and disassembly are not included.

## Purpose and evidence boundaries

The Reader maintains an application-owned selected-view reference alongside
prefetched neighbors. The experiment follows that reference within the exact
component context instead of using traversal order or keyboard focus. Its
result is a bounded observation, never an action token or an atomic snapshot.
The complete native mapping and getter behavior still need exact-device review.

The observer, notification-order fixture and logical-close fixture answer
different questions. They must not be combined into a claim of native success.

- A document/page sample establishes only the relationships actually read in
  its bounded sampling turn. Its map-coherence flag covers the selected page's
  index/key round trip, not preservation of the entire ordered document.
  Notification epochs reject observed changes, including
  A-to-B-to-A transitions; they do not exclude unnotified worker changes.
- Error, loading, selected identity, item/text emptiness, rendered blankness and
  physical panel presentation are separate facts. A false loading flag can
  coexist with a page error. No empty-scene predicate proves a blank imported
  background or template.
- A matching creation result, generic job completion and its JavaScript callback
  are separate events. Content-saving effects exist in the traced native path,
  but its failure propagation and multi-file crash durability remain open.
- Closing owned work while leaving the process and library alive is a logical
  close. It is not library unloading, stock restoration or proof of safe native
  process shutdown.

## Reproduction on owned objects

Run from the repository root on a host with Qt6 Core/Qml/Gui/Quick development
packages and a C++17 compiler:

```sh
sh tools/reader_selected_view_test.sh
sh tools/reader_selected_view_entry_test.sh
sh tools/insertion_notification_order_test.sh
sh tools/reader_logical_close_test.sh
```

For the retained ARM development sysroot, set `QT_PROBE_SDK_ENV` to its
environment-setup file. The scripts compile with that toolchain and execute
the owned fixtures through its QEMU. The scripts build in a temporary directory;
they do not create an injectable payload, select a device or contact one.
Read-only container mounts need an executable temporary build mount.
The logical-close fixture also needs a writable private `/run` namespace. It
creates only its exact owned test roots and refuses if they already exist.

The separate `reader_selected_view_startup.cpp` compiles only with a frozen
`READER_SELECTED_NONCE`. It targets one private `/run/rmb-qt-probe-<nonce>` root,
bound to its owner and attempt PID/start. It publishes `selected-ready` without
reading Reader properties. A single `selected-request` then admits the diagnostic;
`selected-result` carries either scalar observations or a bounded refusal. The
original setup window is sixty seconds, sampling at most one second within it.
There is no native polling, navigation, creation, arbitrary request executor or
unload operation. The entry's own fixtures compile a test-nonce startup library
without loading it. A real payload needs exact independent recovery and source,
artifact, process, device and fixture admission.

Publish the request only after its complete private file has been closed. Direct
shell redirection exposes the final filename before the write and close finish;
the directory watcher can therefore reject an incomplete or changing file even
when its later archived bytes are correct. Interprocess fixtures reproduce this
failure before any observation is admitted.

The Linux research helper `reader_selected_request_publish.cpp` writes a private
pending file, closes and verifies it, then uses `renameat2(RENAME_NOREPLACE)`.
It refuses an existing destination or unsupported atomic operation without a
replacement or link/unlink fallback. A link/unlink publication would expose a
temporary second link, which the entry deliberately refuses. The helper does
not observe Reader or supply the caller's process/deadline admission. An
uncertain helper exit can follow publication and must never trigger a retry.
Before file operations, the helper restores the default `SIGALRM` disposition
and arms a five-second process lifetime. Signal termination remains an unknown
publication outcome and never permits a retry.

A terminal `request-refused` result with sample timestamps `-1` is a valid
pre-observation refusal, not a page sample or malformed JSON. Unobserved fields
retain conservative defaults; their error/loading flags do not report actual
Reader errors. Keep wire validation, observation acceptance and the independent
recovery outcome separate.

Consumers must reject a result unless it is a regular single-link mode0600 file
under the same mode0700 root, its pending sibling is absent, and fresh process,
request, cancellation, restoration and deadline brackets agree. Publication is a
sampled commit, not an atomic cancellation or durable filesystem fence.

The notification fixture deliberately models a QThread object whose QObject
affinity stays on the application thread. Its worker execution emits a queued
page result followed by AutoConnection completion. It also tests GUI emission,
nested event processing, a skipped page result and a foreign job identifier.
These expose assumptions that a native integration must establish, rather than
encoding an assertion that callback completion is a receipt.

Qt documents that AutoConnection depends on the thread executing the emission
and the receiver's affinity, and that equal-priority posted events are processed
in posting order. FIFO delivery alone cannot exclude nested event handling or
prove persistence. See [thread connections](https://doc.qt.io/qt-6/threads-qobject.html#signals-and-slots-across-threads)
and [posted events](https://doc.qt.io/qt-6/qcoreapplication.html#postEvent).
Scoped id lookup uses [QQmlContext::objectForName](https://doc.qt.io/qt-6/qqmlcontext.html#objectForName),
available since Qt6.2; newer recursive context APIs are not required.

## Proposed SDK integration

1. Admit the exact firmware/provider/payload profile and an independently
   recoverable session. Retain process-start identity, engine/window generation,
   original deadline and source document/page. A restart invalidates old facts.
2. Observe the current Reader component and its application-owned selected view.
   Require document/controller/page agreement, map round trips and no observed
   generation change. Refuse missing or ambiguous relationships.
3. Let Buddy recognize a correctly bound current Buddy first, then the immediate
   right Buddy. Unknown/unbound content is not positively ordinary content.
   Preserve the original source anchor if a new page must precede an ordinary
   right neighbor. The SDK does not infer product bindings.
4. Before one guarded creation, record operation identity, expected source/order
   and target correlation. Recheck the same source at dispatch. A caller-chosen
   identity or durable native-assigned correlation needs exact candidate proof;
   a predicted numeric insertion index alone is insufficient.
5. After ambiguous completion, stop mutation and reconcile read-only. Check the
   intended target, exact ordering and preservation before issuing any new
   request. Missing evidence is incomplete, not permission to repeat creation.
6. Observe actual selection. If the intended target is already active, do not
   navigate. Otherwise only verified adjacency from the retained source permits
   the existing logical next-page operation. Re-observe before writing. Direct
   native opening is not introduced by this experiment.
7. Bracket capture/write with fresh identity. Require independent persistence
   evidence after navigation, document reopen and process restart at the
   appropriately admitted stages. Do not promote a content-save notification or
   screenshot to a durable native receipt.
8. Disarm and drain owned work before its deferred destruction. Retain code
   while native callbacks or process-lifetime registrations can still reference
   it. A later controlled stock transition must separately establish render and
   native-reader quiescence before storage release. This experiment supplies no
   fix for the historical shutdown fault.

## Required candidate qualification

| Scenario | Required observation |
| --- | --- |
| Disposable annotated PDF and notebook, middle/end | Exactly one intended adjacent target; original ordered IDs, PDF mappings/bytes, ink and unknown metadata preserved |
| Current Buddy and immediate right Buddy | Correct existing binding reused; repeated requests create no duplicate |
| Ordinary right neighbor | New page follows original source and precedes that neighbor |
| Unknown/unbound page | Refusal or explicit product reconciliation; no ordinary-page assumption |
| Newly created target | Exact selected document/page and ready/error state; intended blank independently corroborated |
| Write, next/previous, reopen and restart | Correct target survives; content/order/ink and operation correlation remain attributable |
| Callback loss, duplicate completion and stale result | Read-only reconciliation; no blind replay or duplicate creation |
| Cancel before dispatch and after possible dispatch | Zero mutation when proven pre-dispatch; uncertain effects retained for reconciliation otherwise |
| Window/document/source change and A-to-B-to-A | Old observation invalidated; fresh source acquisition required |
| Engine/session/Supervisor failure | Independent recovery outcome measured; logical close distinguished from stock restoration |
| Blank start and Writer refinement | Existing product semantics and bindings preserved |
| Simulator and unsupported firmware | Explicit synthetic origin; unsupported native operations remain unavailable |

These are required future checks, not results claimed by the owned fixtures.
The active [investigation plan](../../openspec/changes/establish-native-platform-contract/reader-selection-investigation.md)
keeps native gates open. No canonical specification or unfinished delivery is
archived here.

Source basis: current SDK sources and active contract, exact retained private
artifact analysis (private raw evidence has no public link), the owned Qt
experiments, and the official Qt API references above. The integration sequence
is a proposed engineering design, not observed device behavior.
