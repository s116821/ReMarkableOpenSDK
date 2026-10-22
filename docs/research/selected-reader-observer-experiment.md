# Selected Reader observer experiment

Status: experimental interoperability tooling. No supported native adapter,
qualified stock-Reader capability, page creation, durability receipt or crash repair is supplied
by this increment. Standalone owned fixtures may run on a host, under QEMU or on
an admitted tablet; those scopes must be recorded separately.
The [October 22 stock result](../experiments/selected-reader-observation-result.md)
identified the expected document and page five in one exact admitted sample;
full native adapter qualification remains open.
The original implementation uses public Qt APIs; extracted
firmware source and disassembly are not included.

## Purpose and evidence boundaries

The Reader maintains an application-owned selected-view reference alongside
prefetched neighbors. The experiment follows that reference within the exact
component context instead of using traversal order or keyboard focus. Its
result is a bounded observation, never an action token or an atomic snapshot.
The complete native mapping and getter behavior still need exact-device review.

Scoped id lookup starts from the discovered direct ToolSettings helper's creation
context. A nested composite Reader root can expose its caller's outer context;
bounded ascent from that root alone therefore misses its internal `root` and
`sceneView` ids. The helper is declared inside the Reader component. From its
context the observer ascends at most 16 parents, requiring both ids to equal the
exact discovered Reader and device objects, the same engine and direct helper
ownership. Final checks retain the helper's original creation context and engine.
Owned nested-component tests reproduce the old failure and verify this fix.
The October 22 result completed this path on its exact stock candidate; access
across other documents, states and firmware remains a separate device check.

Discovery follows the window's visual item tree and checks direct QObject
children for the Reader helper. A valid helper must have the visual Reader as
its direct parent, so nonvisual service/model descendants are outside this
candidate relationship. The whole bounded census of effectively visible and
enabled branches completes before an anchor is selected; it does not stop at
the first match. Every accepted selected-view chain already requires these
effective states. Hidden or disabled boundaries are watched before their
descendants are excluded, so later reveal/enable, including ABA, invalidates
the sample. Opacity, clipping and off-screen position are not pruning rules.
Limits are 512 queued visual items, 32 levels, 64 direct or visual children
per item, and 640 retained objects including later relationship checks. Node,
depth and child limits have distinct refusal codes. The cooperative research cap
is twenty seconds; direct `observe` callers retain a one-second default unless
they explicitly select a longer budget. The request entry uses at most twenty
seconds within its original ninety-second setup window, never renewing that window.
The owned host/ARM fixtures verify the effective inheritance described by Qt's
[visible](https://doc.qt.io/qt-6/qml-qtquick-item.html#visible-prop) and
[enabled](https://doc.qt.io/qt-6/qml-qtquick-item.html#enabled-prop) contracts.

Visited visual items watch visual-child, name, visibility and enabled changes. Direct QObject
children have separate name/destruction guards, bounded by the same 512-by-64
census, without consuming the smaller relationship-object budget. A late
duplicate-anchor rename or externally owned visual attachment, including an
A-to-B-to-A reversal, invalidates the sample. QObject-valued property results
are guarded before the post-read cancellation callback can run. These guards
detect destruction; they do not extend native ownership or make a getter that
returns a dangling pointer valid.

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
- Completing or canceling owned work leaves the background service and native
  bridge available for later operations. Disarm temporary work and reject late
  callbacks; conversation completion does not require library unloading or a
  xochitl restart. Process restart, disable/uninstall and failure recovery have
  separate preservation obligations. The one-shot diagnostic's stock restoration
  is a test arrangement, not the production conversation workflow.

## Optional creation-controller source join

`observeWithCreationController` adds a separate experimental question to the
same guarded sample: does the supplied existing creation controller's own
Library return the exact selected document wrapper? Ordinary `observe` does not
perform this lookup. The current one-request native entry still calls ordinary
`observe`; no new native join result or activation is supplied by this change.

The optional path checks the exact build's qualified controller/Library Qt class
names, associated engine/thread/lifetimes, Library readiness and notifications.
It passes an owned QVariant from the actual `id` property to one public native
`entryForId` meta-method with the exact same parameter metatype. Return storage
uses the declared return metatype; generic invocation uses the method's declared
type spellings. It does not reconstruct private types, use JavaScript dispatch,
change QObject ownership, or fall back to a string argument. The returned QObject
is guarded before post-call permit callbacks. A positive result additionally
requires the same live selected wrapper, unchanged Library/readiness, selected
relationships and the selected page's index/key round trip. Only scalar
`creationJoinObserved` escapes; all authority flags remain false.

The [static resolver/cache finding](scene-document-controller-relationship.md)
explains the conditional backing-identity interpretation and its limits. This
lookup can create/cache a wrapper, so it is not passive. Different wrappers with
equal IDs are inconclusive. The caller must establish provenance for an existing
controller before this sample; the diagnostic neither constructs a singleton
nor creates a new QML engine. Separate request-entry integration and a fresh
native profile remain unfinished. No page creation or worker access is added.

Owned fixtures cover exact qualified class names, a non-QString ID value and
mismatched argument rejection, same-ID distinct wrappers, absent/unready and
ambiguous lookup, foreign engines, notification ABA, destruction, cancellation,
reentry, deadline expiry and unchanged QObject ownership. These are model checks,
not evidence that the join has run on stock objects.

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
original setup window is ninety seconds, sampling at most twenty seconds within it.
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

Three earlier exact-candidate stock experiments ended without page identity. The
first returned `request-refused` before sampling; interprocess fixtures later
reproduced the direct-publication race, without proving it caused that run.
The second admitted the atomic request and returned `census-cap` after about
40 milliseconds. That earlier guard combined 256 discovered QObject nodes and
16 levels, so its receipt does not distinguish the exhausted bound. These
results motivated visual-only discovery. Its first stock attempt then returned
`visual-node-cap` after about 605 milliseconds, identifying the 512-node bound.
This motivated eligible-branch pruning above; it does not qualify that revised
implementation on stock Reader. All three attempts were consumed,
their independent stock restoration was checked, and their task-owned runtime
directories were archived and removed. Preservation and recovery results are
separate from the failed primary observations.

A later helper-context candidate admitted the manual page-opening step in time
but returned `deadline` after 1007 milliseconds, without any page identity. Its
flat refusal does not identify the internal phase or qualify the context fix.
Owned profiling then reproduced the cost of full request admission checks: a
490-visual-item fixture with roughly 930 direct nonvisual children made 9106
permit calls. With the original checks and tmpfs request files, three ARM/QEMU
samples completed in 8.449, 8.496 and 8.783 seconds under a temporary twenty-second
cap. Host and QEMU timings are not tablet timings. This motivates the research
cap increase while retaining all lifetime, epoch, request and cancellation checks;
it is not a production responsiveness target or native qualification.

The first longer-budget operator attempt published no request or observation.
Its timing was consistent with reaching the host's thirty-second reserved margin
inside the former sixty-second setup window; the exact failed shell guard was
not logged. A fresh profile uses a ninety-second entry/operator window, retaining
the twenty-second sample and independent recovery deadline. The selected actor
requires at least 285 seconds remaining before that window, preserving the
185-second recovery cutoff plus ten seconds of slack. This refuses a late startup
rather than renewing an existing window. Human confirmation is not page proof.

Elapsed-time checks are cooperative: a synchronous native getter is not
preemptible. An operator profile must admit enough time for request publication,
the selected sample budget and result collection before the original window
ends, and retain recovery independent of the GUI thread. A result arriving after
the original deadline is invalid. A timeout never authorizes another request.

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

The retained creation seam belongs to the `DocumentController` singleton from
`xofm.libs.library`, not the Reader's `sceneController`. Its full invokable is
`bool addPageWithTemplateAndPageSize(entry::Id, int, QString, QSizeF, QJSValue, QString)`;
the final argument supplies a page UUID. This is positional insertion, not an
atomic insert-after-source-ID operation. The exposed boolean and JavaScript
callback are not a native job/target receipt. The existing
[caller-selected result](../experiments/caller-selected-insertion-result.md)
records one historical exact-firmware allocation honoring the supplied UUID;
its durable-success flag remained false and lifecycle qualification stayed open.

A future candidate should preselect a fresh absent target UUID, establish the
existing Library/Reader document correspondence, and derive the insertion index
from the retained source ID immediately before its single dispatch. Read-only
reconciliation then requires the target exactly once immediately after source,
and removing that target must reproduce the complete original order. Both map
directions must agree. Separate preservation evidence must cover original page
records, PDF mapping/bytes, ink and unknown fields. A false return, lost callback
or timeout permits reconciliation, never resending. Matching logical IDs alone
does not prove backing-document ownership or exclude another concurrent writer.

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
