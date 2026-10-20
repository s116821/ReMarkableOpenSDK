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
- [ ] Qualify an exact candidate on a disposable device fixture only after its
  activation/recovery admission. This gate remains open without device evidence.

No canonical specification is synced or archived by this research increment.

The separate request entry publishes readiness after file/process binding and
accepts exactly one nonce/PID/start request within its original sixty-second
setup window. Readiness does not observe the Reader. Only that admitted request
calls the accessor diagnostic; the sampling budget is at most one second and
cannot extend setup. Cancellation, restore markers and request replacement
disarm it. It remains resident after terminal completion; no unload is provided.
Owned host and ARM/QEMU fixtures cover admission, reentry, cancellation/expiry
unwinding and publication boundaries. Actual target results must be separately
recorded with their exact payload/operator/profile.
