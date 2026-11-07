# Private capsule implementation and remaining admission evidence

The existing initial-load constructor now has a private path to prepare and
install a dormant resident route. The source selection constant is false for
every current build. Runtime admission requires a separate qualification.

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
phase boolean. The implementation uses that boundary and leaves runtime
selection disabled.

The inspected Reader path reaches `Core::install` and two calls to the pinned
`DobbyHook`. The Dobby inline-hook sources route through allocation,
trampoline/relocation preparation and the POSIX patch leaf. Those inspected
calls contain no thread or process creation. The admitted Reader artifact
does contain `pthread_create` call sites in Dobby's
`base::ThreadInterface::Create` and `OSThread::Start`. Their presence establishes
linked creator code. The inspected hook chain supplies no call to either
entry, and no execution of those entries was observed here.

The retained Reader ELF has DT_INIT and DT_INIT_ARRAY plus direct Qt
Quick/Gui/Qml/Network/Core and libc/runtime dependencies. The
[glibc initializer source](https://github.com/bminor/glibc/blob/glibc-2.34/elf/dl-init.c)
explains why dependency initializers can run before the adapter constructor.
The source checks reject PREINIT, IRELATIVE and TEXTREL in the main image.
They do not establish the behavior of the earlier dependency initializers
and resolver paths. The supported profile relies on ordinary creator behavior
and a stable trusted loader configuration as specified in loader-admission.md.
The linked pthread entries are ordinary group-thread creators under that model.

## Candidate kernel count mechanism and production premise

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
inference. With blocked asynchronous delivery, the reviewed count/publication
ordering and no out-of-group address-space sharer, a count of one and a
non-cloning raw path establish exclusion. Two directory scans cannot supply
those premises. The current design states the ordering as a trusted kernel
behavioral assumption. The v1.6.2 source is candidate evidence and the shipped
v1.6.3 binding remains unverified. The separately adopted process model covers
ordinary vfork/spawn and fatal group termination.

Primary source:
[proc producer](https://github.com/reMarkable/linux/blob/d54fe67bf86e918468b936f97a2ec39f4f87a3d9/fs/proc/array.c),
[count field](https://github.com/reMarkable/linux/blob/d54fe67bf86e918468b936f97a2ec39f4f87a3d9/include/linux/sched/signal.h),
[clone publication](https://github.com/reMarkable/linux/blob/d54fe67bf86e918468b936f97a2ec39f4f87a3d9/kernel/fork.c),
[exit cleanup](https://github.com/reMarkable/linux/blob/d54fe67bf86e918468b936f97a2ec39f4f87a3d9/kernel/exit.c).
Dynamic loader startup semantics are described in
[ld.so(8)](https://man7.org/linux/man-pages/man8/ld.so.8.html).

## ARM provider evidence

The first actual ARM capsule control failed at the preparation cache leaf before
entering the mask/patch interval. A bounded diagnostic identified SDK QEMU8.2.7:
its ARM_NR_cacheflush branch is a no-op and leaves r0 unchanged. The candidate
Linux ARM kernel implementation instead returns the result of do_cache_op,
which returns0 on successful flush. The native zero-return check is unchanged.

The owned ARM capsule reports unsupported in this emulator, preserving the first
failure rather than substituting a callback or accepting the input address as a
native success result. Host Linux signal/thread/protection/rollback controls and
the separate compiled ARM forwarding/unwind controls remain distinct evidence.
This remains the QEMU provider's result. The later unchanged owned capsule
completed its raw ARM cache/task/transaction cases on the admitted RM2 runtime;
that positive target evidence is recorded in
[bounded native evidence](../../../docs/native/composer-pre-enter-native-evidence.md).
The run checked the full success output and normal process/group/host/transport
settlement, without stock access. It does not bind the shipped kernel to the
candidate company source or construct a production capsule. No additional tablet
execution, fallback recovery, cleanup or replay follows from either result.

Primary source:
[QEMU8.2.7 ARM dispatch](https://github.com/qemu/qemu/blob/v8.2.7/linux-user/arm/cpu_loop.c),
[candidate ARM cache syscall](https://github.com/reMarkable/linux/blob/d54fe67bf86e918468b936f97a2ec39f4f87a3d9/arch/arm/kernel/traps.c).

## Owned controls and residency

The startup deadline increment retains the authenticated original expiry in
Activation for the constructor boundary. A live capture preparation
request obtains that absolute CLOCK_BOOTTIME value, and an expired or stale
claim yields zero. The private raw transaction uses CLOCK_BOOTTIME and checks
before mutation and commit. The owned fixture's synthetic budget is confined
to that fixture. Host checks covered real claim expiry, endpoint startup after
expiry and refusal of the expired capsule. The updated raw capsule compiled
for ARM hard-float. The earlier exact RM2 result covers the original
CLOCK_MONOTONIC binary and cannot qualify the changed clock leaf or deadline
path on the target.

Linux child controls exercise the actual private transaction with raw mask query,
pending signal delivery after coherent commit/rollback, additional-thread and
wrong-word refusal, exact maps/protections, injected cache/protection failures,
verified rollback, fatal rollback/mask/exclusion paths and no C++ allocation in
the excluded interval. Test faults/factories are rejected by production bootstrap
preprocessing. Production kernel/launch admission is not replaceable by this
test-only factory.

The constructor rechecks the exact main file and mappings for the whole site
page and original target after Reader preparation. The shared builder preopens
genuine procfs files, verifies original bytes and RX spans, prepares an A32
veneer with bounded non-clobbering hints and snapshots mappings. An owned Linux
child exercises a middle-page patch in a file-backed RX span and checks VMA
restoration, permission, original-word, alignment, expiry and occupied-route
refusals, publication, and release or retention of the veneer as appropriate.

Route preparation allocates before interception and publishes only a pointer.
The installed route is retained for process lifetime and begins dormant.
The constructor can delete only unpublished allocations. There is no
function-local lazy Route guard or ordinary Route exit destructor. Preparation
pins the already loaded code with RTLD_NODELETE and retains the handle; the owned
shared artifact also has NODELETE set. Actual native ownership remains borrowed.
Physical unhook, reclamation and unloading remain unavailable.
