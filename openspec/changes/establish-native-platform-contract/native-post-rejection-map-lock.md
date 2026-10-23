# Observe map-lock reuse after the native rejection

The [actual negative witness](../../../docs/experiments/insertion-guard-native-negative-result.md)
returned to our private catch and matching Task completion. It did not test
whether the original handler released its final map mutex. Test that separately
before treating private-catch delivery as sufficient native cleanup evidence.

## Default-off, one-shot successor

Preserve the negative-only Core and existing default wire bytes. Add an optional
typed, process-resident Probe to Registration; absence leaves its path unchanged.
Only the matching private-Rejected catch can invoke it, after the original stack
has unwound and before terminal publication. Check the existing cancellation and
absolute deadline before and after the probe; never renew either clock. Never
probe on a binding/predicate early return, foreign/unrelated exception or normal
original return. This does not add another Task, registration, retry or permission.

The owned helper namespace is `reader_insertion_mutex_probe`. Its Config supplies
`pthread_mutex_t *mutex`, checked Storage `{begin,end}`, and qualified `tryLock`
and `unlock` pointers. `Probe::create(Config)` runs outside the critical path,
requires complete aligned native mutex storage, distinct required callbacks and
actual nonrecursive/default kind zero, and returns an intentionally resident
typed object. Its `Result` has attempted/acquired/released booleans and integer
tryResult/unlockResult initialized to -1. `Probe::once() noexcept` uses one atomic
consumption flag, one trylock and, on success, exactly one unlock. Reuse,
nonzero trylock or failed unlock terminates119 directly, without logger,
reporter, Qt, allocation, clock calls or another callback. No arbitrary Core
failStop callout may execute under uncertain lock ownership. On successful
acquisition, unlock is the next function call. Return success facts only after
unlock. A busy result is inconclusive about which writer owns the mutex.

Core appends the typed pointer as `Registration::postRejectProbe` and result as
`Outcome::postReject`; snapshot cannot call it a positive witness when requested
facts are incomplete. The existing barrier still never forwards tracked inserts.
The coordinator adds `Config::wantPostRejectProbe` (default false) and appends
the pointer to Attribution; requested mode refuses a missing factory result.

## Exact native factory and distinct wire consumer

Extend Facilities::attributionParts(worker, bool wantPostRejectProbe=false).
Reuse the existing exact retained worker/meta-object/lifetime/thread admission,
but separately map/check **worker+176** for all24 bytes and actual kind at+188.
Do not substitute queue mutex+592. Existing qualified, version-bound libc
trylock/unlock/Exit providers suffice. Add the typed pointer to AttributionParts;
default mode performs no new map-mutex reads. The native startup selects mode
only through frozen `READER_INSERTION_POST_REJECT_PROBE=1`; configuration and
factory request must agree. No native capability flag becomes true.

Only requested mode adds top-level `post_reject_probe` to the diagnostic wire,
with exact keys required/attempted/acquired/released/try_result/unlock_result/
native_authority. Old mode retains the same schema and no new key. A separate
closed consumer must require that object and all successful facts, strict types,
zero return values and false authority, in addition to the complete existing
negative conjunction. A missing extension cannot pass the successor. Keep the
4096-byte cap, one request, original90/30/360-second clocks and independent actor.

## Verification and interpretation

Use real owned default mutexes and an original owned outer cleanup before Core's
probe, plus busy/recursive/invalid-storage/reuse/unlock-failure and exceptional,
canceled/expired/wrong-thread controls. No spin or test-only native macro enters
the real artifact. Review changed source, old default wire behavior and affected
owned controls, then qualify a fresh source/artifact/provider/packet before one
native run. Preserve the spent negative candidate and all failures.

A successful trylock/unlock proves this one nonrecursive native mutex could be
reacquired after that unwind. It does not prove every cleanup, all-writer
exclusion, physical owner release, safe unload or conditional permitted insertion.
