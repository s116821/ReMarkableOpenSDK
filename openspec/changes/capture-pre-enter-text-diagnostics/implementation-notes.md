# Private capsule implementation and remaining admission evidence

The private capsule and eager route are implemented for owned verification.
Production StartupBuilder and its selector remain absent; installProduction
continues returning Unsupported. Exact target admission is not fabricated from
owned-test construction or observed numeric counts.

## Initial-load source path

The existing consumer resident_runtime launch source builds a fresh Command for
the independently pinned executable, clears the environment, supplies the pinned
adapter in LD_PRELOAD, sets LD_BIND_NOW and the one-use activation directory,
and spawns only after renewed current-artifact/deadline admission. Production
entry uses an empty private command decorator; test-only helpers change it.
SDK bootstrap initialize is an ELF constructor that claims Activation, verifies
loaded artifacts and calls installBeforeMain before queued application startup.
Activation binds the direct supervisor PID/start identity, one-use claim, artifact
digests and current child process identity. The loader/profile checks exclude the
main PREINIT/IRELATIVE/TEXTREL paths already listed in the design.

For that reviewed fresh-exec launch, initial preload is a source-supported path,
not a timestamp/Qt-absence assertion. It does not authenticate an arbitrary late
dlopen or an external call to installBeforeMain. A production StartupBuilder must
remain internal to that actual constructor/Activation path, not accept a caller
phase boolean. No new builder is enabled here. Runtime satisfaction of that
selected launch remains separate from this source finding.

## Candidate kernel count mechanism and exact gap

The retained read-only kernel record says 5.4.70-v1.6.3-rm11x and explicitly marks
exact_source_identity_verified=false. No retained kernel image/source-commit
binding was established by this pass.

The company's kernel repository supplies candidate source at commit
d54fe67bf86e918468b936f97a2ec39f4f87a3d9, also the tag RM1XX_5.4.70_v1.6.2:

- fs/proc/array.c task_sig reads get_nr_threads under lock_task_sighand.
- include/linux/sched/signal.h returns the signal group's nr_threads field.
- kernel/fork.c increments that field under the shared sighand lock in
  copy_process before _do_fork calls wake_up_new_task.
- kernel/exit.c decrements it under that lock during exit signal cleanup; that
  path does not resume ordinary userspace execution.

This lock/publication relationship is relevant to the design's sole-thread
inference: with asynchronous delivery blocked, a properly serialized count of1
excludes another userspace creator/entrant under that implementation. It is not
proved merely by two directory scans. The missing link is the exact shipped
v1.6.3 kernel build/source identity and any vendor changes to those paths.
Do not label the v1.6.2 source as target compatibility or issue a ready production
capsule until the missing link is established.

Primary source:
[proc producer](https://github.com/reMarkable/linux/blob/d54fe67bf86e918468b936f97a2ec39f4f87a3d9/fs/proc/array.c),
[count field](https://github.com/reMarkable/linux/blob/d54fe67bf86e918468b936f97a2ec39f4f87a3d9/include/linux/sched/signal.h),
[clone publication](https://github.com/reMarkable/linux/blob/d54fe67bf86e918468b936f97a2ec39f4f87a3d9/kernel/fork.c),
[exit cleanup](https://github.com/reMarkable/linux/blob/d54fe67bf86e918468b936f97a2ec39f4f87a3d9/kernel/exit.c).
Dynamic loader startup semantics are described in
[ld.so(8)](https://man7.org/linux/man-pages/man8/ld.so.8.html).

## ARM cache-provider limitation

The first actual ARM capsule control failed at the preparation cache leaf before
entering the mask/patch interval. A bounded diagnostic identified SDK QEMU8.2.7:
its ARM_NR_cacheflush branch is a no-op and leaves r0 unchanged. The candidate
Linux ARM kernel implementation instead returns the result of do_cache_op,
which returns0 on successful flush. The native zero-return check is unchanged.

The owned ARM capsule reports unsupported in this emulator, preserving the first
failure rather than substituting a callback or accepting the input address as a
native success result. Host Linux signal/thread/protection/rollback controls and
the separate compiled ARM forwarding/unwind controls remain distinct evidence.
Actual ARM cache-maintenance behavior needs a provider that implements the
selected syscall contract; no tablet execution follows from this limitation.

Primary source:
[QEMU8.2.7 ARM dispatch](https://github.com/qemu/qemu/blob/v8.2.7/linux-user/arm/cpu_loop.c),
[candidate ARM cache syscall](https://github.com/reMarkable/linux/blob/d54fe67bf86e918468b936f97a2ec39f4f87a3d9/arch/arm/kernel/traps.c).

## Owned controls and residency

Linux child controls exercise the actual private transaction with raw mask query,
pending signal delivery after coherent commit/rollback, additional-thread and
wrong-word refusal, exact maps/protections, injected cache/protection failures,
verified rollback, fatal rollback/mask/exclusion paths and no C++ allocation in
the excluded interval. Test faults/factories are rejected by production bootstrap
preprocessing. Production kernel/launch admission is not replaceable by this
test-only factory.

Route preparation allocates before interception, retains immutable SDK state in
objects with a deleted destructor and publishes only a pointer. There is no
function-local lazy Route guard or ordinary Route exit destructor. Preparation
pins the already loaded code with RTLD_NODELETE and retains the handle; the owned
shared artifact also has NODELETE set. Actual native ownership remains borrowed.
Physical unhook, reclamation and unloading remain unavailable.
