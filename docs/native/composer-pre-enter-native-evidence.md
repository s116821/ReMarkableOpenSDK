# Bounded RM2 own-memory capsule evidence

The owned capsule binary from SDK source
`9df17f96f4aecffcaf74371c62fdc10a1ea8b0a6` completed on the admitted RM2 runtime.
The binary was unchanged: 368544 bytes, SHA256
`0aeb430995eb00d0aa1af45a5ccb443fb4d23773b74fa08ba6134ca2bca2dc7d`.
It used its own anonymous mappings and disposable child processes; it did not
attach to Xochitl or exercise the stock text callsite.

The source runs ten owned child cases: pending-signal/coherent release,
additional-thread refusal, original-word refusal, four pre/post-write/cache/
protection rollback cases, and three fatal rollback/mask/exclusion cases.
The native run completed the exact entire success output, including the one-use
diagnostic admission, 223 bytes with empty owned stderr. Its original invocation,
complete ExecStart and boot matched before/after. Final process state was normal
exit status0/code1/Resultsuccess/MainPID0, with an active/exited retained unit,
empty ControlGroup and absent owned group views. There were no stop or kill
attempts, so explicit fallback recovery remains unexercised.

The accepted profile has zero capabilities, NoNewPrivs, a read-only home with an
exception for the owned writable work directory, per-process AS128MiB/CPU5s/
FSIZE32768/CORE0 limits and source-bounded concurrent topology of two processes/
three tasks. These limits do not establish enforced aggregate128MiB or a task
controller. The host retained the one transfer/one launch, actual EOF/status for
all 34 streams from 17 operations, six completed transport resources, and an
independently contained/reaped/zero-member/closed host Job.

| Evidence | Established scope | Remaining limit |
|---|---|---|
| Owned Linux host controls | Signal/thread/map/word/rollback/fail-closed source behavior | Host behavior alone is not ARM or stock admission |
| SDK QEMU8.2.7 capsule control | Strict cache-syscall refusal before patching | Preserve refusal; do not weaken the zero-return check |
| RM2 own-memory capsule run | Positive ARM raw-cache/task/transaction behavior for the unchanged owned binary and admitted runtime | No stock memory, actual composer or general capsule qualification |
| Existing compiled ARM forwarding/unwind controls | Owned BL/veneer/shim register and exception route | Not newly exercised inside stock by this run |
| Company kernel source | Candidate count/clone/exit mechanism | Exact shipped v1.6.3 source/build binding remains unverified |
| Production constructor/installer | Private builder implemented in the existing startup path and exercised with owned Linux mappings | Source selection disabled; loaded stock site, loader policy and actual ARM constructor execution remain unqualified |
| Semantic composer operations | No new claim | Submit identity, edit/currentness lease, conditional clear and focus acknowledgement remain unproved |

## Constructor source seam

The completed diagnostic removes the need to repeat the same owned ARM question.
The implemented private constructor path follows
[loader admission](../../openspec/changes/capture-pre-enter-text-diagnostics/loader-admission.md).
Its internal call after Reader preparation and before queued startup binds:

- The claimed current Activation and reviewed fresh-exec initial-load callpath;
  a public callable, phase boolean, Qt absence or observed count alone is insufficient.
- The exact loaded executable/providers, original site/target, nonoverlapping
  near veneer, executable shim and retained ready route, prepared before exclusion.
- The raw fixed-buffer mask/task/map/word checks and kernel assumptions required
  by the existing exclusion contract, retaining refusal when they are unknown.
- The original absolute deadline, one-use state, coherent publication, exact mask
  restoration and already defined rollback-or-fatal behavior.

The route starts dormant and accepts one bounded arming request in source.
Runtime selection and stock target behavior still require qualification. The
earlier on-device owned capsule, the owned Linux builder and the owned ARM
arming fixture cover different artifacts and execution paths.

## Provenance and reproduction limits

The source cases are in `composer_pre_enter_patch_owned.cpp`; the existing
`composer_pre_enter_patch_test.sh` builds the same owned capsule. Its QEMU result
and actual-provider result must remain separate. The consumed tablet run is not
a retry recipe; a later live action requires its own concrete authorization.

Recorded diagnostic identity: `aceb1153-18fd-443c-b204-be9345c6fdbd`. Terminal
archive manifest SHA256:
`e30976eaccb735489506dc4bfe57d722b3afed490b7808d6000863ffff1adb65`.
Whole owned success SHA256:
`2aed9f66f74d66734722939ee6f502800be6e451317fe90dea71aa363d329018`.
Original binary, streams, source pins and receipts remain retained privately;
no proprietary firmware or private connection material is included here.

Source basis: [operator result](https://linear.app/magentumdragon/issue/REM-61#comment-38a3258f-080f-47ef-9315-f8e599dccdd5)
and [independent coordinator acceptance](https://linear.app/magentumdragon/issue/REM-61#comment-bc2b8677-dc53-49a9-8b94-5ce7b9488098),
with the exact selected original binary/output/receipt hashes checked again for
this documentation update. The remaining source seam is an inference from the
current private builder declaration, refusal stub and constructor source; it is
not a newly observed stock/runtime result. Tracking links may require access;
the material scope and limits are stated above without requiring those tools.
