# One-shot pre-handler job snapshot

This supplies the narrow adapter design for the optional attribution callback
in [the runtime contract](native-insertion-guard-runtime-core.md). It is not
implemented native attribution or a Task lookup. The retained static route
supports a coherent snapshot of the existing worker's active unsigned64 ID,
subject to the caller and native-profile conditions below.

## Admission and exact operation

Use the selected exact-build worker queue mutex and its two active-ID words.
The queued count and next-ID counter are distinct fields and are never used as
the active ID. Check pointer arithmetic/alignment before constructing an
immutable snapshot context outside the callback. The native context needs
strong Document/Worker owners, normal/default mutex construction and exact
pthread ABI/provider/code verification. A nonnull context cannot prove these.

The hook core invokes this only after fresh UUID value, worker incarnation,
actual worker thread, original outer caller, carrier and unique-scope checks,
before the original handler can hold its final map lock. All symbol bindings,
including unlock, must already be resolved; no lazy resolver or logger may
run while the queue lock is held.

Try the queue mutex exactly once through the qualified public pthread C ABI.
Busy refuses attribution without retry. On success, copy just the two unsigned
32-bit active-ID words while holding that mutex, then unlock exactly once.
Combine the words without narrowing to signed integers or floating point.
Zero refuses attribution after unlocking. Do not hold the queue mutex across
the original handler, a callback, private exception transport or a completion
wait. The passive final-map predicate never acquires this queue mutex.

Unexpected try-lock results or unlock uncertainty must terminate the candidate
directly without an arbitrary reporter callback, logger or unwind. Even a
reporter that promises to return could block while ownership is uncertain;
preserve an unknown reason rather than making reporting a prerequisite to exit.
Do not assume every nonzero try-lock
return means no ownership: robust owner-death behavior is explicitly outside
this normal-mutex profile. Never initialize, repair or copy the native mutex.

## Verification and interpretation

Use an original owned layout containing an actual default pthread mutex and
two ID words. Verify one successful coherent full-width snapshot, busy and
zero outcomes, exact one-attempt counts, released lock before return, checked
arithmetic/configuration and nonreturning unexpected-error/unlock controls.
There is no need to rerun unrelated Rust/native matrices. Keep the focused
code, actual ARM outputs, failures and input/library/provider hashes.

The snapshot alone does not prove that the immediate caller belongs to this
job: reentrant dispatcher work could reuse its return PC, and the ordinary run
loop has an event-dispatch callout. Require the fresh target UUID, complete
worker/thread/incarnation/scope binding and later exact same-worker unsigned64
jobCompleted, plus the separate operation callback/private-catch outcome.
Buffer early completion; never infer success from a greater job ID. No
all-writer exclusion, continuous lease, native cancellation, owner retirement
or accepted insertion follows from this value.

Source basis: hash-bound retained run/dispatcher/Task/constructor instructions,
public target pthread ABI assertions and exact libc exports. Sequencing and
failure containment above are proposed original control logic.
