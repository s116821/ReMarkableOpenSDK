# Selected Reader observation experiment

This increment investigates existing tasks 2.6 and 2.9. It introduces no public
capability or supported native profile. Each native experiment requires its own
current authorization and exact activation/recovery admission. Owned-fixture
verification alone does not authorize loading a payload into xochitl. Historical
held or consumed attempts are not reusable candidate admission.

The experiment follows a supplied current window to a unique Reader component,
resolves that component's own scoped `sceneView` id, and reads its selected
`sceneView` property. It never selects a view by child order, keyboard focus,
remembered index, or the `scene-view` object name. A change counter installed
before access rejects notified A-to-B-to-A changes as well as final mismatches.

The sample must agree on document/controller/current-page references and the
page-index round trip. Page error and loading are independent observations.
Neither a sample nor a ready state proves blank content, rendering, persistence,
an atomic native snapshot, or a lease permitting a later mutation.

Implementation and checks:

- [x] Implement a bounded, same-GUI-thread observer using public Qt accessors.
- [x] Exercise actual QML contexts, external document/controller ownership,
  prefetched siblings, error/loading disagreement, identity switches and
  reentrant A-to-B-to-A notification on owned fixtures.
- [x] Build on the retained ARM toolchain; distinguish QEMU from native results.
- [x] Review callback lifetime, failure/retry and integration requirements.
- [x] Record one admitted stock observation of the expected document/page five,
  sampled readiness and selected mapping, with independent restoration and
  unchanged fixture. See the [exact result](../../../docs/experiments/selected-reader-observation-result.md).
- [ ] Qualify an exact candidate on a disposable device fixture only after its
  activation/recovery admission. The single positive observation above is a
  bounded checkpoint; broader native operation/state qualification remains open.

## Creation-controller source join follow-up

The next distinct question is whether the selected document wrapper is the same
live wrapper returned by the creation controller's own Library lookup. The
[static relationship findings](../../../docs/research/scene-document-controller-relationship.md)
describe the shared native resolver and the stateful wrapper-cache path. This
follow-up does not create a page, acquire a worker, navigate, or grant a mutation
lease. Different wrappers with matching IDs remain inconclusive.

Implement the optional join using the actual `id` QVariant and an exact matching
Qt meta-method argument type, rather than reconstructing `entry::Id` or assuming
it is a QString/QUuid. Guard the supplied controller and its Library before
lookup, watch Library replacement/readiness and the existing selected chain,
require one exact native method, pin its returned QObject before further permit
callbacks, then recheck the relationships. Return scalar facts only; keep
existing observation behavior unchanged when the optional join is absent.
Any singleton/helper preparation must precede this guarded sample, remain on the
same GUI engine/thread, and retain the original operation deadline. No event
pumping, waiting for readiness or automatic retry is permitted.

- [x] Implement the optional same-wrapper join without an action API.
- [x] Verify typed invocation, a same-ID distinct wrapper, missing/wrong method,
  unavailable/unready Library, replacement/ABA, destruction, cancellation and
  reentry on owned Qt fixtures; preserve the existing observer checks.
  Host Qt 6.4.2 and ARM Qt 6.10.3 under QEMU pass the observer and one-request
  entry suites. Qualified native class names and notification guards preceding
  optional getters have independent review and regression coverage. This does
  not qualify stock execution of the new join.
- [x] Implement and review the separate one-request join entry on owned fixtures.
  Resolve the registered controller once through a fixed import helper on the
  admitted window's existing engine, before the guarded sample. This may lazily
  initialize the singleton and is an explicit stateful preparation step, not
  evidence that it existed beforehand. Destroy only the owned helper; never
  change the controller's ownership or create an engine. Keep preparation,
  cleanup and the sample inside the original operation/setup deadlines. Expose
  the join only through a separately compiled entry mode and explicit result
  kind; preserve the ordinary entry's wire shape and no-access-before-request
  behavior. Verify preparation cancellation, reentry and one invocation on
  owned objects before an exact new native profile.
  Host Qt 6.4.2 with its QtQml/Models/WorkerScript runtime modules and ARM Qt
  6.10.3/QEMU pass both observer and entry suites, including both startup builds.
  Entry fixtures cover preparation stop/restore/expiry, request replacement,
  reentry, uncertain result publication and terminal no-retry behavior. Fixed
  helper source uses a local URL; an unknown synthetic scheme had left imports
  pending and was refused without a lookup. No asynchronous wait was added.
- [ ] Freeze and review a separate exact native join profile. The successful
  selected-observation candidate is spent; this entry mode has not run on stock.
- [ ] Record any admitted native join separately; a positive sampled join is
  conditional on the inspected wrapper/cache invariant and is not atomicity,
  worker serialization, creation, rendering or persistence qualification.

No canonical specification is synced or archived by this research increment.

The separate request entry publishes readiness after file/process binding and
accepts exactly one nonce/PID/start request within its original ninety-second
setup window. Readiness does not observe the Reader. Only that admitted request
calls the accessor diagnostic; the sampling budget is at most twenty seconds and
cannot extend setup. Cancellation, restore markers and request replacement
disarm it. It remains resident after terminal completion; no unload is provided.
Owned host and ARM/QEMU fixtures cover admission, reentry, cancellation/expiry
unwinding and publication boundaries. Actual target results must be separately
recorded with their exact payload/operator/profile.

The longer research cap follows an owned 490-visual-item ARM/QEMU fixture that
needed about 8.5 seconds with exact request checks, and a stock attempt that
returned only `deadline` at the old one-second cap. All request, cancellation,
lifetime and notification checks remain. Direct callers retain a one-second
default. The operator must separately budget publication and collection inside
the original setup deadline and retain independent recovery; cooperative checks
cannot preempt a blocked getter. One exact native sample now established the
expected selected identity in 1028 ms; timing across other states and full native
qualification remain open.

The operator's matching ninety-second window requires a 285-second startup
reserve within its original 360-second actor deadline, retaining the 185-second
recovery cutoff plus ten seconds of slack. The thirty-second request margin
reserves publication, sampling and collection. These are fresh profile bounds,
not extensions or retries of a consumed sixty-second attempt.

One possible resident operation-completion diagnostic adds a queued checkpoint
and later GUI acknowledgement after an admitted sample has unwound. It must
retain the original request/result/process/root and deadline binding, terminal
disarmed state, unchanged observer invocation count, and the same guarded
window/engine lifetimes. It must perform no new Reader property reads, rearm,
navigation, unload or private module teardown. A changed result, cancellation,
restoration, expired deadline or destroyed context must prevent acknowledgement.
This is a proposed diagnostic, not native operation-completion qualification.
Buddy and bridge code remain available for subsequent operations. Conversation
completion does not require unloading the bridge or restarting xochitl. Actual
process restart, disable/uninstall and failure recovery are separate transitions.

This is one way to measure the existing lifecycle obligation, not an additional
requirement to unload libraries or stop Buddy after a conversation.

- [ ] If selected, implement and verify the queued acknowledgement on owned fixtures.
- [ ] If selected, qualify its exact stock Reader candidate separately.
