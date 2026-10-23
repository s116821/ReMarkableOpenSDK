# Research capture inside a live selected-page join

October 23, 2026 UTC. This is a source/owned proposal; no new native packet is
selected. The [composed acquisition result](../../../docs/experiments/conditional-target-acquisition-native-result.md)
now supplies an actual ready Reader/document/page join. Earlier normal visual
captures returned loading pixels without that join. Reuse those capture primitives
with the now-established relationship rather than repeating creation or treating
a generic frame signal as document readiness.

## One synchronous helper

Add a separate default-unused research helper. Its fixed expected identity is
canonical document/page UUID text, zero-based index and count1..16. Inputs also
include the already admitted QQuickWindow, a bounded PNG Sink, a shared one-grab
claim, caller permission and one absolute cooperative deadline of at most20s.
It opens no document, navigates nowhere and acquires no native worker owners.
The later entry/profile, if separately implemented, owns setup/recovery and the
one owner-only output file. Existing modes and wire schemas remain unchanged.

1. Pin application/window/content-root/engine identities and check GUI thread,
   focus, visibility, activity, caller permission and the original deadline.
   Permission is sticky on first failure. Copy owned configuration before callouts.
2. Call the existing observeCreationJoin on that exact window. Its SourceAction
   must receive the expected ready/coherent document/page/index/count and retain
   QPointers to the joined Reader/document/controller for this helper's lifetime.
   This is the existing public Qt join, not an insertion or worker submission.
3. While that Sample and its borrowed epoch guard are still alive, consume the
   shared claim before exactly one visual_capture::captureObserved call. Its
   BeforeGrab/AfterGrab observer only compares the exact pre-pinned window/root
   and checks permission; it must not recursively start another Sample. The core
   independently enumerates windows, so any ambiguity or different selected
   window refuses rather than selecting a replacement. Keep existing image,
   storage and PNG quotas and GUI-only grab semantics.
4. Check the returned capture context inside SourceAction, copy only owned scalar
   image facts/hash, then clear Result.context before returning. No borrowed
   Sample guard, QVariant, native pointer or callback closure may escape. Do not
   call the old Sample guard after the outer observation has unwound.
5. Start a distinct fresh observeCreationJoin only after that unwind. Require the
   same Reader/document/controller/window/root/engine, expected ready page identity
   and original deadline. The first observation describes pre-image facts; it
   cannot be relabeled as the fresh post-image observation.

Successful helper output means one bounded window image has matching sampled
selected-page relationships before/around/after acquisition. It does not mean
native/render/durable authority, physical-panel freshness, a page-only image or
blank content. The backend's pixels need separate inspection. Full-window image
coordinates are not page coordinates; no page transform is invented.

## Refusal and output handling

The Sample epoch observes delivered QEvents and notifications. A real grab may
itself cause invalidation; whether that occurs is an unanswered diagnostic
question. Preserve and report refusal rather than resetting/filtering the epoch,
extending the deadline, pumping events or retrying a grab. Guard, metadata,
identity, thread, window/root/engine, callback-count or readiness loss is terminal.
A used claim cannot acquire another image, including after failed/throwing grab.
Reentry cannot produce a second accepted operation.

Image bytes may already exist when a later guard, fresh join or publication fails.
Such bytes are an orphan forensic output, never an accepted image. A later entry
must bind original scalar/image bytes to one request/incarnation and verify size,
hash, dimensions and full decode before acceptance. This proposal neither changes
publication nor defines the eventual Buddy IPC protocol.

The QQuickWindow API is GUI-thread-only and expensive. Its documented redirected
rendering limitations and the vendor backend remain relevant. A cooperative
deadline detects late return; it cannot preempt a blocking native grab or promise
a maximum UI pause. Independent process recovery belongs to any later packet.

## Verification and remaining gates

Owned controls must exercise the real capture core and an admitted selected-page
fixture, plus explicit injected boundaries for otherwise difficult failure paths:
exact expected identity, wrong document/page/index/count, unready join, different
Reader/document/controller/window/root/engine, missing/duplicate SourceAction,
nested/reentrant requests, lifetime loss, sticky guard loss/exception, deadline,
used/shared claim, grab failure/exception, epoch change during grab/encoding,
fresh-join failure, and copied-scalar-only result lifetime. Demonstrate at most
one grab on all paths and no accepted image after a failed fresh join. Test host,
ARM and sanitizer controls and production-header compilation separately; mocks
must stay labeled and previous source/transport modes must remain untouched.

Native activation requires its own committed source/profile/artifact/providers,
strict scalar/image consumer, complete current11-page/24-file preservation packet,
fresh admission, absolute deadlines and independent recovery. No earlier spent
packet is reused. A useful source image and intended blank target need separate
content corroboration; writing, reopen, other document types/positions, resident
mutating retirement/reclamation and consumer integration remain open.

The separately proposed [fixed Entry and image-pair continuation](identity-capture-entry.md)
defines that later mode and publication boundary; it is not implemented by the
synchronous helper.

Source basis: current selected-page and capture-core source, actual composed
acquisition and earlier loading-image findings; public
[Qt6.10.3 grabWindow documentation](https://doc.qt.io/qt-6.10/qquickwindow.html#grabWindow).
The composition and expected refusal behavior are proposed design, not native
observations or a diagnosis of the earlier loading images.

Dated later result, October 23, 2026: the [actual identity-capture report](../../../docs/experiments/identity-capture-native-result.md) records one useful fixed idle-RM2 capture with live original-byte collection, independent restoration/preservation and audited cleanup. Earlier unimplemented/unqualified statements describe their source/design checkpoint; this later result grants no supported native capability, safe unload, writer or persistence authority.
