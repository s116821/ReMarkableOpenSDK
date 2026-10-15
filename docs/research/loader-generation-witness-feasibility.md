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

## Supported source-anchor investigation and next decision

The scoped follow-up checked creator-controlled public Qt APIs, the saved vendor
creation boundary and two applicable prior-art approaches. None supplies the
missing passive attachment for this candidate. This is a bounded result for the
inspected mechanisms, not proof that such an observation is impossible.

| Checked mechanism | Finding and consequence |
| --- | --- |
| QQmlApplicationEngine objectCreated | Its own finishLoad emits the result of its own component creation. It is not a notification stream for every nested Loader or independent component creation. No evidence establishes that the selected vendor creator exposes MainView through this signal. |
| QQmlComponent create return / owned QQmlIncubator result | These provide a direct definition-to-instance association when the actual creator is controlled or cooperates. Finding a component afterward does not recover that association; a separate helper would create a different instance. |
| Component status, creationContext and URL | Definition readiness is not an instance birth notification. The definition's creation context and supplied URL do not attest arbitrary existing instances or pinned source bytes. |
| Existing WindowNavigator notification | The retained windowOpened/windowClosed metadata carries a QString window name, not the created root, Loader, component or generation. It cannot supply the required instance join. |
| Saved WindowLoader creation boundary | The selected path packages a window contentItem and context for a virtual provider call. The concrete provider/result exposure is not joined by the saved analysis. A private call boundary is not a supported external observer. |

The application-engine finding is grounded in
[Qt 6.10.3 finishLoad](https://github.com/qt/qtdeclarative/blob/v6.10.3/src/qml/qml/qqmlapplicationengine.cpp#L136-L165).
The controlled-creator distinction follows the
[component API](https://doc.qt.io/qt-6.10/qqmlcomponent.html) and
[incubator API](https://doc.qt.io/qt-6.10/qqmlincubator.html).
The component's public signals describe definition progress/status; a caller
can supply both bytes and a URL through setData, so URL agreement alone cannot
attest those bytes.

The pinned [Loader implementation](https://github.com/qt/qtdeclarative/blob/v6.10.3/src/quick/items/qquickloader.cpp#L612-L675)
creates and owns its own incubator and reports the successful item through its
Ready handling and loaded signal. An external incubator subclass does not attach
to that existing object. Upstream also chooses AsynchronousIfNested when the
Loader's asynchronous property is false; that setting is not an unconditional
immediate-completion guarantee. This qualifies timing interpretation without
establishing any asynchronous interval or cause in actual 7a4. The proposed
future witness still requires observed Ready/item state and source identity.

The saved WindowLoader receipt remains SHA256
`aea5dc0c78b1d29cf23f78328eebdef149d3209be4c62c841f35f58fdfe3f200`;
its selected item-creation disassembly was rechecked against SHA256
`f098a47d2984dfe5c77460f14a3062d5f1f3c04e4a4d831066b71198545e6ef0`.
This follow-up reread the creator delegation and wrapper notification metadata,
not the completed registration chain. It neither exhaustively analyzed the
provider nor inferred absence of other interfaces from that unjoined boundary.
Raw private instructions and metadata remain unpublished.

Two existing prior-art approaches do not remove this distinction:

- [Inkling 089efb2](https://github.com/nathanmarlor/inkling/blob/089efb2c9f24ce64127c6fc4d7fd30f93ae5ad94/xovi-ext/inklingfb/main.c)
  discovers objects by tree/class/property relationships. Its comments describe
  stale pooled DocumentViews and it retains a first-match fallback. This is
  traversal precedent, not source-qualified creator observation; no fallback
  or property dump is adopted.
- [qt-resource-rebuilder 6c709dd](https://github.com/asivery/rm-xovi-extensions/tree/6c709dd8416ddf89034831073d76fb4ee3372905/qt-resource-rebuilder)
  overrides resource registration and can modify resource tables before the
  original call. A lexical witness inserted into a pinned QML creation site is
  a possible different design, but would modify the application resource and
  require patch/cache/loaded-byte and lifecycle qualification. It is not an
  untouched public observer or permission to use a held activation route.

**Next decision:** keep this external public-observer candidate unselected.
Its missing input cannot be supplied by adding objectCreated listeners, a new
incubator, context-name queries or more census fields from the assessed APIs.
To reopen it, identify an actual application-facing creator-result reference
with source provenance, or explicitly scope a different creator-cooperation or
interception design. The saved unjoined provider boundary is a precise possible
research lead, not an available API. Resource modification is another effect
scope, not an automatic fallback. Neither is selected by this assessment.

No new scaffold, test cycle, native getter, callback attachment, build or packet
is justified by the present findings. Other SDK work and separately permitted
diagnostics remain open. Source basis for this follow-up is direct saved-source
inspection, pinned public Qt source/docs and the cited prior art, with author/
Astra analysis of their applicability. No live source-to-instance observation
was obtained.

Coordinator review checked the selected upstream creation/incubator paths,
component/incubator APIs, cited prior-art behavior, saved creator receipt and
matching item-creation instructions, and the wrapper's QString-only notification
metadata. It found no actionable inconsistency. This was selected-source
consistency review, not exhaustive provider resolution or runtime proof.
