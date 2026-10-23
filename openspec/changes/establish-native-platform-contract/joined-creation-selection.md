# Joined creation and adjacent selection diagnostic

The selected-page/controller join and native worker no-op callback now have
separate positive results. The next useful native question uses the ordinary
controller's existing AddPageTask path rather than bypassing its preflight,
Document reconciliation and store scheduling with a lower worker handler.
Existing caller-selected UUID allocation and ink preservation evidence remain
historical evidence, not experiments to rediscover.

## Scope and limits

Qualify one fresh selected-source join on the existing RM2 fixture, request one
page at source index plus one with a fresh absent caller UUID, then observe the
actual selected page and order. The ordinary controller call does not itself
guarantee target selection: the stock UI's own add-page callback separately opens
the new page, whereas a controller-only caller can leave the original ID selected.

After one matching ordinary callback, one fresh observation must establish count
increased by one, the exact target at source index plus one, all original IDs in
order and readiness. If that target is already selected, finish with an explicit
after-create observation and issue no Next. Otherwise require the original source
still selected and only then permit one stock adjacent `moveForward()` dispatch.
Its end-of-document branch can
create another page, so count/adjacency and original-source identity are mandatory
immediately before dispatch. A final independent selected-view observation must
match the intended new target and readiness. No second creation or Next follows
refusal, missing acknowledgement, timeout or an uncertain result.

This is an idle-fixture interoperability diagnostic, not an execution-time
source/order exclusion mechanism or a qualified SDK creation capability. Earlier
page facts and controller return/callback are separate facts. Persistence requires
complete post-restoration backup comparison, including original page objects,
PDF mapping, ink, unknown metadata and the exact new target. The direct-handler
REA investigation continues independently.

## Implementation constraints

- Reuse the existing engine and a freshly qualified controller, document wrapper
  and actual typed native ID within the Sample's synchronous guarded action.
  Do not resolve a replacement document from UUID text.
- Verify the exact six-argument Qt meta-method, boolean return and actual ID
  metatype. Use the ordinary native controller so its normal task bookkeeping
  remains in place. Inherit the selected page's native template and use the
  separately explicit RM2 paper size for this development fixture.
- Pre-create one owned callable on the same engine. It captures an owned/disarmed
  GUI bridge, never a borrowed Sample guard or raw Entry/native pointer. Late
  delivery after cancellation/destruction must be harmless. Consume the single
  creation claim before invoking; true/false/exception/callback are recorded
  independently and never grant retry permission.
- Keep callbacks and publication off the synchronous Sample/native invocation
  stack. Cancellation, replaced request, restore, expired deadline and changed
  engine/window disarm subsequent phases. Code stays resident through recovery.
- Record explicit pre-create, post-callback/pre-Next and post-Next phases. A
  void Next dispatch does not prove selection. All native/render/durable authority
  flags remain false in this research mode.
- Give this distinct multi-phase diagnostic one fresh fixed operation budget;
  retain original ninety-second setup and independent 360-second actor/recovery
  bounds. Do not extend or replay any consumed twenty-second worker test.

## Gates

- [x] Implement and independently review the ordinary-controller witness and
  owned fixtures for exact typed arguments, callback timing/lifetime, exceptions,
  cancellation/deadline, absent/duplicate target and no retry.
- [x] Implement and review Entry phase integration, fresh guarded adjacency,
  original absolute deadline and bounded closed wire; rerun affected suites.
- [x] Freeze a distinct exact source/build/package with one absent target UUID;
  independently review artifacts/operator/backup/recovery before target selection.
- [x] Perform one creation/at-most-one adjacent Next and separately
  verify actual selected identity, original preservation, persistence and recovery.

The [actual a7 native result](../../../docs/experiments/joined-creation-selection-result.md)
records the one ordinary creation and one adjacent Next, ready target at index 5
of 9, complete original page-object/file preservation, sampled saved target/order,
original stock restoration and exact owned cleanup. It does not close the
execution-time source/order or resident/failure/consumer gates.

Source/control checkpoint: all ten owned suites passed on distinct host Qt 6.4.2
(272.6 seconds) and ARM Qt 6.10.3/GCC 13.4 under QEMU 8.2.7 (407.1 seconds), with
identical before/after hashes for all 31 inputs. The witness fixture covers 158
checks. The phase fixture models ordinary task effects explicitly and tests
qualified source, preserved order, already-selected target skip, at-most-one Next,
fresh target, cancellation/expiry, false/unknown partial effect and uncertain
post-link publication. It also exercises a greater-than-2-KiB envelope using
explicit owned-only padding that startup builds forbid.

Independent review closed the publication cap at both generator and publisher,
private expected-order configuration, helper-cap versus whole-operation budget,
and original-window identity across phases. After the full run, the same-engine
window regression was strengthened to unconditionally require a refusal receipt
and zero Next; that affected fixture passed again on host and ARM. No production
source changed after the ten-suite freeze. All seven startup variants compile.
Earlier compile/fixture failures and the intentionally stopped pre-window-fix
run remain preserved; no native creation is inferred from these owned tests.

Source basis: current committed Reader and worker results, original creation
bridge source, and retained stock QML/Task relationship evidence. The proposed
integration is design work; no new page mutation follows from this document.
