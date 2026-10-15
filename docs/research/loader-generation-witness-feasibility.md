# Loader generation and synchronization witness: feasibility decision

2026-10-15. Source-only mechanism assessment. No implementation or native
operation is selected. This consolidates the next decision after the
[counter-only diagnostic was declined](capture-loader-correspondence-assessment.md).

## Decision and specific observable

Do not implement this candidate yet. Its first prerequisite, a source-qualified
owning MainView instance/context, is unresolved. Status reads and frame hooks
cannot compensate for observing an insufficiently identified object.

The proposed observable is: the exact Loader of a source-qualified MainView
instance has successfully instantiated the expected component generation; that
generation is valid at the beginning of synchronization for the same captured
window and remains unchanged through the explicitly paired frame and grab.

This is a prospective observation, not reconstruction of actual 7a4. It would
establish construction before the identified synchronization, not necessarily
before all frame preparation. It would not establish documentLoaded, visible
Reader pixels, image freshness, physical presentation or current-page authority.

## One candidate mechanism

1. **Resolve the exact owner on the GUI thread.** Start from a qualified
   creation/ownership record linking the pinned MainView definition to an actual
   instance/context and the retained capture window/root. Within that context,
   resolve the Loader and expected component identities. The existing bounded
   traversal and lifetime guards can constrain reachability, but cannot supply
   the missing source qualification. No arbitrary context or window search is
   a substitute for this record.
2. **Arm once from present successful state.** After establishing complete
   invalidation coverage, sample the exact Loader's Ready state, non-null loaded
   item and expected source-component identity on the GUI thread. Recheck
   guarded identities, containment and generation around those reads; refuse
   on inconsistency or reentrancy. A qualified present Ready observation can
   precede a future synchronization without witnessing the earlier `loaded`
   event. It cannot explain historical frame order. Non-ready or unknown state
   refuses this attempt; there is no wait-until-ready, retry or application write.
3. **Publish one bounded generation token.** Retain GUI-owned guarded identities
   and a separate lifetime-safe synchronization record. Invalidate permanently
   on unloading, source/item replacement, destruction, containment/context loss
   or loss of observation coverage. Same pointer values at two snapshots do not
   establish continuity: an intervening transition must not disappear. All
   relevant invalidations must be observed before they can yield a false token;
   late queued notifications are insufficient. The precise notification and
   reentrancy coverage remains a qualification requirement, not an implemented
   guarantee.
4. **Observe a subsequent synchronization.** A direct `beforeSynchronizing`
   callback for the retained window consumes the valid token using an explicitly
   specified cross-thread protocol. It records a bounded local frame/generation
   identifier without QObject lookup, property reads, GUI census, allocation,
   blocking waits or file I/O there. The protocol must define publication,
   revocation, callback lifetime and teardown; copying a Boolean is insufficient.
5. **Pair the frame, then grab once.** A qualified backend/render-loop rule must
   associate that synchronization with the selected later frame emission.
   Repeated, missing, ambiguous or out-of-order events refuse. Queued GUI delivery
   carries the already-paired token; it does not create ordering evidence.
   Revalidate the same instance/generation with the existing window/root/context
   conditions around the one grab. Any generation invalidation refuses joined
   success, even if the old values later return. Exact frame-image equivalence
   remains a separate claim.

These steps describe one contract candidate, not five separate experiments.
No forced update, rendering trigger, source assignment, scene modification,
rearm or alternate window is proposed. If a natural qualifying boundary does
not occur within the existing outer deadline, the attempt would refuse.

## What public APIs do and do not establish

`QQmlEngine::contextForObject` can return the object's assigned context. Qt
automatically assigns an internal context when it instantiates a QObject, but
that API alone does not attest the pinned source definition. See
[QQmlEngine context lookup](https://doc.qt.io/qt-6.10/qqmlengine.html#contextForObject).

`QQmlContext::objectForName` searches only its supplied context. Results may
come from ids, context properties or a context object's properties. `baseUrl`
may be inherited or explicitly overridden. Thus name resolution, URL resemblance
and Loader/source-component pointer agreement within a chosen context do not
independently prove MainView provenance. See
[QQmlContext semantics](https://doc.qt.io/qt-6.10/qqmlcontext.html).

Qt 6.10.3's `objectForName` fallback calls `readObjectProperty`, which reads a
meta-property on the context object. This lookup must not be smuggled into the
metadata-only census as a harmless id lookup. These new identity/state reads
require an explicit scope and behavior review. See
[Qt source, objectForName](https://github.com/qt/qtdeclarative/blob/v6.10.3/src/qml/qml/qqmlcontext.cpp#L358-L374)
and [property-read helper](https://github.com/qt/qtdeclarative/blob/v6.10.3/src/qml/qml/qqmlcontext.cpp#L282-L291).

Loader Ready/item semantics provide a possible present-success observation;
source changes can destroy the old item. The current native Loader ABI, exact
getter behavior and complete invalidation coverage are not qualified by public
documentation. See [Loader semantics](https://doc.qt.io/qt-6.10/qml-qtquick-loader.html).

Qt documents a blocked GUI thread at direct `beforeSynchronizing` delivery and
render-thread emission of `frameSwapped`. This motivates the token boundary,
but does not establish one-to-one synchronization/swap pairing on the retained
backend. It also does not make the GUI-thread-only census usable there. See
[QQuickWindow semantics](https://doc.qt.io/qt-6.10/qquickwindow.html).

## Next decision and contract boundary

The next decision is whether a concrete supported creation/ownership observation
can supply the source-qualified owner/context record in step 1. The accepted
capture instrumentation supplies no such record. No supported attachment point
has been qualified by this assessment. The completed static registration mapping
is not that runtime observation and should not be repeated to manufacture one.

If such a source anchor is identified and accepted, assess this whole candidate
together: exact allowed reads, invalidation coverage, paired frame semantics,
cross-thread lifetime, bounded output and recovery. If the anchor is unavailable
under permitted mechanisms, retain the no-go decision rather than implementing
status reads and callbacks on a merely named candidate. No counter-only packet,
new getter or hook is an authorized fallback.

A future implementation would require an explicit contract/schema and consumer
revision; the accepted nine-field schema cannot silently acquire these claims.
Reuse existing process/context refusal, one-grab, inode/publication and outer
deadline mechanisms where their guarantees apply. They do not qualify the new
cross-thread token or generation coverage. Focused owned controls would need to
exercise invalidation during reads/grab, stale queued delivery, in-flight frames,
missing/duplicate pairing and callback teardown. None were implemented or run.

Main remains sole operator. Fresh artifact/provider/current-state/packet review
and independent recovery requirements remain unchanged. Prior live-host-primary
recovery does not qualify timer or host-loss fallback. Actual 7a4 is spent and
loading-only; held/spent exclusions remain in force. This assessment authorizes
no build, profile, nonce, activation, getter execution or device action.

Source basis: current Entry/core/census source and accepted contracts, retained
MainView wiring, and linked official Qt documentation/source. Private source
evidence has no public transcript link. The candidate and no-go decision are
author/Astra analysis; no runtime correspondence or ordering was observed.

Coordinator review read the complete assessment and independently checked the
linked Qt 6.10 documentation and v6.10.3 context lookup/property-read source.
It found no actionable inconsistency within this source-only feasibility
decision. The prior bounded queued-emission/frame-boundary review also applies.
This does not establish that a supported source anchor is impossible, qualify
an implementation, or block unrelated SDK work and future diagnostics.
