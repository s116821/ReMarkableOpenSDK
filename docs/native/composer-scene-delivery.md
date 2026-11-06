# Typed scene diagnostic delivery

composer_scene_delivery.h connects the accepted scene attachment to the distinct
native request at0x6d6ce4. Its request type is:

    qulonglong (*)(void *worker, const QString &page, const Callback &callback)
    // Callback = std::function<bool(Scene*)>

The generic request at0x6d6b7c takes std::function<void()> and is not used or cast
by this path. The exact scene signature,16-byte/alignment4 callback value and
64-bit return are based on the retained callback metatype, request instructions
and Core case0x1b documented in the
[attachment contract](composer-scene-attachment.md). No new binary analysis was
needed for this integration.

## Caller integration

The caller first admits the exact Profile and obtains a ready token from the
existing validated ResidentSlot owner-acquisition path. Delivery::prepare accepts
that token, a copied page QString and a budget of at most120 seconds.
Delivery::submit uses the existing GUI/reentrancy/permit gate and the native
worker owner from that slot. It invokes the real typed request in production;
owned tests substitute only that typed function through a test-only seam.

The page must be nonempty, at most256 UTF-16 units and contain no NUL. These are
bounded-input checks, not page or visible-visit authority. The caller must supply
the page from its actual selected-view join and use the matching current guard.
The copied page survives caller mutation and permit reentry. Owner type,
document identity and worker correspondence remain the existing slot's checks.

A prepared session and its slot are consumed at most once. A refused competing
wrapper cannot cancel the session that actually claimed that slot. Submission preserves
the full64-bit job ID. accepted means the native call returned nonzero without
observed cancellation/reentry; it does not mean the Scene callback ran, a recorder
attached, or native/UI completion occurred. DeliverySnapshot separates this
submission result from callback entered/returned counts and attachment facts.

A throwing request, zero job, or cancellation/reentry during native enqueue is
unknown. Observation closes and no retry is allowed, including when a native
copy was retained before the failure. No stale selected-page guard or QObject
getter is consulted after enqueue.

## Lifetime and cancellation

Serialize handle operations/destruction on the GUI thread. Permit callbacks may
reenter or destroy the handle: submission retains its own shared state before
calling them. The native callback captures only the attachment callback, an
atomic cancellation gate and scalar counters. It never captures the Delivery
handle, owner slot, worker pointer or a Scene pointer for later use.

Each native invocation borrows Scene only synchronously and always returns false.
Stop closes the recorder and gates future joins. A callback copy delivered after
handle destruction safely refuses without reading its argument. Duplicate
delivery reaches the accepted one-use attachment refusal. The original deadline
is unchanged across preparation, enqueue and delayed delivery.

ResidentSlot::cancel shares the atomic gate: cancellation before callback entry
refuses a new join. Delivery::snapshot polls that gate and closes an observer
that was already attached. Slot cancellation alone is a request, not a promise
that an in-flight callback or existing recorder has already settled. Use
Delivery::stop for the frozen-prefix boundary.

## Native owner retention

Immediately before native entry, the slot is marked as holding a typed Scene
request. Existing generic Task retirement cannot reclaim that slot or release
either native owner, even after callback counts balance, all observed copies die,
or logical stop succeeds. The generic attribution route also refuses that slot.

This is deliberately a one-slot diagnostic after attempted dispatch. It has no
typed physical retirement/reuse path yet. A consumer must keep the code and
resident owner slot alive; these diagnostic counters are not an unload permit.
Actual typed task quiescence must be integrated before reclaiming or reusing it.
No new native owner acquisition, ordinary release, mutation, clear or runtime
activation is introduced by Delivery.

The only shared-source seam is the slot's friendship, cancellation gate and
typed-retention refusal. Generic dispatch and historical/RM1 extraction remain
unchanged. No resident bootstrap or capability selection is enabled.

## Verification

Five owned integration groups exercise copied page lifetime, full64-bit jobs,
real Qt attachment from a retained typed callback, stop/frozen records, late
copies after destruction, slot cancellation, duplicate delivery, expired budgets,
throw/zero/canceled/reentered requests, destructive/reentered permits, input and
thread refusal, and generic-retirement rejection. The fake generic dispatcher
and ordinary owner release abort if called. Existing slot controls also pass.

Host tests use Qt6.4.2 with ASan/UBSan; ARM tests use the retained Qt6.10.3 under
qemu. Both compile macro-free production headers. These checks execute the real
adapter with owned typed native-function fakes. They do not invoke Xochitl,
positively admit Profile in the tablet process, or establish native/UI behavior.
Root owns final integration, runtime selection and tablet scheduling.
