# Optional mutation request lifetime: owned source result

October 23, 2026. The optional request scope now spans the coordinator and its
owned worker/ordinary-creation/JS callback producers. Host Qt, ARM emulation and
host ASAN/UBSAN controls pass. This is a source/owned milestone; no new tablet
activation, insertion, writing, native retirement or owner release occurred.

## Implementation and repaired boundaries

[Scope](../../tools/reader_request_scope.h) owns immutable hook storage before
acquisition and retains its lease until after hook-capture destruction. It has
no QObject/Fence dependency and cannot be copied or moved. Default-unbound
behavior preserves the legacy path; bound missing/null/throwing acquisition
refuses. [The Fence adapter](../../tools/reader_retirement_scope.h) accepts only
a valid existing Frame, using C++17 elision without making Frame movable.
Production hooks are nonreentrant GUI-only adapter internals, not arbitrary
client callbacks. An entered terminal scope is not active permission.

The coordinator's scheduling depth remains separate. Scopes cover admission,
permission, queued advancement, callbacks and cleanup paths in the coordinator,
both witnesses and CreationBridge. No scope/Fence enters the copied native
worker closure. An outer registration scope must still cover complete
construction/start and deletion expressions, including argument, member and
QObject-child destruction.

Source review found and repaired two defects before running this matrix:

- A supplied Permit could retire its request yet return true. Its copied target
  could also retire during destruction after the return expression. The copy is
  now explicitly destroyed inside the scope before fresh active permission and
  final borrowed checks. Joined guard and Boolean bridge admission also recheck
  after their callouts; bridge callback copies survive receiver deletion.
- Cleanup entries previously constructed a scope but did not require successful
  entry. Bound acquisition failure now refuses reentrant cleanup work rather
  than running it without a lease. Valid terminal cleanup remains permitted.

Controlled normal disarm requires a bound, entered but inactive request and a
finished successful coordinator result. It releases the Permit under its scope
and suppresses only the destructor's later logical Core cancellation. Default
and abnormal cleanup keep their original cancellation path. Resident, Prepared,
Core and native callback code remain retained; no native close/cancel/drain or
module unload is introduced.

## Executed owned controls

| Component | Host Qt | ARM/QEMU | Host ASAN/UBSAN |
| --- | --- | --- | --- |
| Scope/Fence/actual JS bridge | 20 scenarios /143 checks | 20/143 | 20/143 |
| New modeled mutation-retirement sequence | 3 scenarios /100 checks | 3/100 | 3/100 |
| Permit body/copied-target destruction regressions | 2 scenarios /24 checks | 2/24 | 2/24 |
| Existing coordinator sections | legacy537, target321; conditional controls pass | same sections pass | same sections pass |

All transports/processes exited0 without timeout/unknown or sanitizer failure.
The existing conditional section reports566 or567 checks depending on the
number of observed readiness turns; its scenario assertions pass in every run.
Do not manufacture one constant aggregate count across those observations.

The new sequence uses one modeled native acquisition/Core/mutation and modeled
Sample identity. It exercises the real ordinary Qt method, owned callback
witnesses, worker-thread callback copies and public-metaobject full-order helper.
A finishes once; terminal notification's nested loop cannot retire its producer.
Complete facade/child destruction occurs under cleanup frames. A later B gets a
fresh nonce/generation and modeled identity join, then performs real full-order
Qt reads with the same window/engine. A's unresolved-mutation latch remains set;
another mutation refuses. A child producer's nested loop separately proves that
a finished-handler-local scope would be insufficient. An undisarmed cleanup
case retains default cancellation and fails registration cleanup with zero B.

The actual CreationBridge tests exercise all eleven JS-exposed entries, stale A
callbacks during a new generation, invalid/throwing hooks, source Binding
deletion during acquisition, and hook/callback-capture destruction pumping the
event loop while the lease remains held. Both permission regressions require
zero subsequent Sample/acquire/probe/registration/controller/B work.

## Provenance and reproduction

The existing pinned SDK image is
`sha256:416c7a7be0038156797b0892f031f352b841d1921fae83f712d0a272e4724618`.
Host uses GCC13.3/Qt6.4.2; ARM uses the vendor GCC13.4/Qt6.10.3 environment and
QEMU. Scripts build the production coordinator branch separately compile-only;
modeled seams cannot hide syntax errors in that branch. Original source snapshots,
compiler output, artifacts, assertions and stderr are preserved privately.

Run [reader_request_scope_test.sh](../../tools/reader_request_scope_test.sh) with
`READER_REQUEST_SCOPE_OUTPUT` set to a fresh directory, and
[reader_insertion_guard_entry_test.sh](../../tools/reader_insertion_guard_entry_test.sh)
with `INSERTION_ENTRY_OUTPUT`. Set `QT_PROBE_SDK_ENV` for ARM and the respective
`READER_REQUEST_SCOPE_SANITIZERS=1` / `INSERTION_ENTRY_SANITIZERS=1` for host
sanitizers. The first frozen matrix ran both fixtures in all three modes. The
second changed only the coordinator fixture to add the two permission regressions
and reran that component in all three modes; the scope source/fixture remained
byte-identical. Final source/result audit SHA-256:
`1bf1b2f4c3fefd20ed86d705d59e65f1d5313a9fcb83de33bfb9a551ab85c9f4`.

## Remaining qualification

There is no production resident registration/publisher protocol or per-request
FD/writer inventory here. The owned continuation is not an implemented native
reconciliation service. Full resource/failure/deadline composition and a fresh
native mutation/retirement/read-only profile remain separate tasks16.7a/b under
[the selected design](../../openspec/changes/establish-native-platform-contract/mutation-request-retirement.md).
No native-owner reclamation, general repeated-mutation capacity, safe unload,
shutdown-fault repair, supported firmware or native capability follows.

These controls improve the SDK's owned model layer. The Buddy application
simulator has not automatically acquired them; its integration gaps remain in
the [simulator evidence map](../reference/simulator-evidence-map.md).

Source basis: current original implementation, independently reviewed source,
frozen owned fixtures and original host/ARM/sanitizer logs. Modeled and native
boundaries above are deliberate, not inferred from a passing build.
