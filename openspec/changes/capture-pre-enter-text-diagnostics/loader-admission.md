# Proposed private pre-main installation admission

This reviewed design now has a private owned transaction and eager resident route
implementation; see implementation-notes.md for evidence and limitations. The
production StartupBuilder remains absent and the installer Unsupported. No new constructor, runtime
activation, late patch path or caller-constructible quiescence token is proposed.

## Existing source envelope

bootstrap.cpp initialize(), constructor priority200, claims the existing
Activation and calls installBeforeMain before QCoreApplication startup/backend
creation. reader_insertion_guard_native_profile.cpp already admits the executable
and providers, mapped code and eager bindings, rejects main PREINIT/IRELATIVE/
TEXTREL, and performs existing Reader hook installation. Its tasks() uses
opendir/readdir and merely records taskCount; it does not require one thread.
It allocates State, maps/provider vectors and Facilities and invokes DobbyHook.
None of those operations belongs inside the proposed final capture patch interval.

Extend that private envelope at a named internal call after existing Reader
installation/preparation and before queued startup. Do not infer capture admission
from wantCondition, Facilities::applicationAbsent or observedTaskCount. Keep the
capture selector tied to a reviewed exact artifact/profile; this precode does
not enable a selector or change supervisor authorization.

The existing supervisor launch/Activation and linked initialization ordering must
establish an initial-load constructor call, not a late dlopen or a function
invoked from a handler. Constructor priority200 alone does not establish order
across different DSOs. Keep the entry internal to the reviewed startup callpath
and refuse if that provenance cannot be established; do not add a public
installBeforeMain-style callable as evidence of being before main.

## Phase A: all preparation before exclusion

- Complete Activation/current-directory checks, existing installBeforeMain work,
  provider/file hashing, ELF/dynamic checks, relocation/symbol resolution, loaded
  mappings and expected code inspection. Account for the already installed
  Reader hooks rather than asserting that all executable bytes still equal the
  original file after those known modifications.
- Prepare the finite capture/state, original deadline, cancellation/publication
  storage, loader registration/unwind state, branch bytes and rollback record.
  Resolve and warm every helper called in the final interval; eager binding alone
  does not exclude arbitrary first-use initialization.
- Replace the lazy function-local Route static before production use. Construct
  the retained route/state in private process-resident storage before exclusion,
  with no registered Route destructor or later shared_ptr release. Publish a
  lock-free pointer only after preparation; entry must not call __cxa_guard_acquire,
  register an exit destructor, allocate, lock a preparation mutex or load a DSO.
  The resident artifact/code must be pinned against unloading. No route teardown,
  native owner release or physical unhook is implied by cancel/handle destruction.
- Reserve a near veneer with non-clobbering mapping hints and bounded attempts.
  Accept only an actually returned nonoverlapping mapping within BL range; never
  MAP_FIXED over existing memory. Write veneer bytes while RW, synchronize code
  visibility and make RX before the site can reach it. Failure here leaves the
  original callsite unchanged. Direct BL is allowed only if the admitted A32
  shim itself is in range; otherwise use the admitted tail veneer.
- Record complete affected page intervals, current RX protections and main-file
  device/inode/offset correspondence. Validate overflow, page size and that the
  page containing0xb4a9b0 cannot also contain executing installer/helper code.
  Pin the exact shim/code/veneer mappings and A32 state; require original
  word0xebfffa3a and target0xb492a0, not merely a decodable branch.
- Preopen genuine procfs task/status/maps descriptors, rollback storage and fixed
  parsing buffers. Bound input bytes/iterations and require EOF, complete records
  and unambiguous parsing. All file opens, allocations, Qt/loader calls and
  callbacks happen here. Failure remains ordinary preparation refusal.
- Touch all planned buffers, stack/code paths and patch/rollback inputs before
  exclusion. Prepare an absolute monotonic installation deadline and a reviewed
  raw clock leaf; no clock-library first use belongs in the interval. Check the
  deadline before mutation and at phase boundaries. Expiry after mutation requires
  excluded rollback or fail-closed exit. Bounded loop/syscall counts do not claim
  hard real-time syscall latency; independent startup recovery remains required.

## Phase B: obtain actual exclusion, with no code writes yet

Enter an internal one-use function that itself establishes exclusion. It accepts
only the private prepared capsule; no public bool, same-thread assertion or
externally minted token can substitute for its checks.

1. Save the calling thread's exact kernel signal mask and block all blockable
   asynchronous delivery with the pinned ARM Linux rt_sigprocmask ABI. A reviewed
   raw syscall leaf is proposed: glibc's wrapper deliberately leaves its two
   internal real-time signals unblocked. Query back and verify the effective
   kernel mask, including those reserved signals. Do not confuse glibc sigset_t
   with kernel_sigset_t or guess the kernel argument size.
2. Require getpid()==gettid() for the admitted constructor thread. With delivery
   blocked, read a fresh bounded /proc/self/status and require Threads=1; enumerate
   /proc/self/task with preopened fd, fixed-buffer getdents64 and full parsing,
   requiring exactly this TID. Recheck the kernel count after enumeration.
   No readdir/opendir, malloc, lazy binding, Qt operation or callback is permitted.
3. The kernel count is a required real observation, not an old numeric fact.
   Before accepting it, verify the pinned target kernel's count publication and
   clone/exit ordering support this use. Repeated non-atomic directory enumeration
   alone cannot prove exclusion. Unsupported procfs/ABI/kernel semantics, extra
   tasks or ambiguous enumeration must refuse before any site write.
4. The intended inference is explicit: once the current blocked constructor is
   the sole task, and the remaining leaf-only path cannot clone or invoke a
   handler, no in-process peer can enter the site or create a peer during that
   interval. This depends on the trusted-process boundary: debugger/ptrace,
   external memory mutation, kernel defects and unreviewed interposers are not
   neutralized by a signal mask. Do not claim global stop-the-world behavior.

SIGKILL/SIGSTOP cannot be blocked; they do not run application handlers.
Synchronous instruction/memory faults are not recoverable installation branches.
Do not install replacement fault handlers or change application dispositions.
The design must not promise continuation after such a fault. Blocking is temporary
and successful exit restores the exact previous mask, preserving pending signals.

## Phase C: fixed bounded patch transaction

Use pre-resolved/pinned leaf syscalls, fixed byte checks and lock-free scalar
publication only. No DobbyHook, allocator, C++ exception, Qt call, logger,
dlopen/dlsym, filesystem traversal or arbitrary application callback occurs here.
This is a new capture-only interval, not a claim about existing Reader hooks.

1. Re-read mappings using fixed storage and verify site, original target, shim,
   veneer and installer pages against the prepared spans/protections/identity.
   Verify the exact original instruction again immediately before permission
   change. Missing/different code is refusal, never permission to overwrite it.
2. Confirm the already prepared resident route is coherent and initially disabled
   for capture. Its empty/expired/canceled state must still forward stock once.
   Prepare every invariant needed by a signal handler that could run after mask
   restoration. Do not publish an incompletely constructed shared_ptr.
3. Change only the admitted site page from RX to RW (not RWX); write the aligned
   four-byte encoded BL; synchronize the affected instruction range using the
   reviewed ARM cache-maintenance leaf; restore exactly the original RX flags.
   Verify the instruction and mapping afterward. Installer/helper execution and
   rollback code must remain executable on unaffected pages throughout.
4. Recheck the bounded kernel task count/mask invariants and publish installed
   state with release ordering before restoring the previous signal mask.
   On restore, a pending handler may run immediately, so no half-committed route
   or held SDK lock may remain. Successful continuation requires confirmed mask
   restoration. No later startup/GUI caller may reuse this capsule to patch again.

### Failure and rollback

Before any site write, restore the prior mask and refuse; an unreachable owned
veneer can be retired outside exclusion. After a write but before publication,
rollback is allowed only within the same proven-excluded interval: restore the
recorded original word, synchronize its instruction cache, restore original RX
protections and verify old code/mappings. This is bounded startup rollback, not
physical unhook of a route that might already be executing.

If exclusion, original identity, cache/protection restoration or rollback is
uncertain, terminate through the existing fail-closed startup policy with a
pre-resolved allocation-free exit-group leaf. Do not return into startup with
uncertain executable bytes. Failure restoring the signal mask likewise cannot
continue normally. A committed/published route is never rolled back in response
to later capture refusal, exception, cancellation or missing acknowledgement.

## Implementation/review split

Proposed implementation files are the existing private loader/profile source,
bootstrap integration and a small private fixed-profile patch capsule/leaf unit.
Do not expose an installer or general patch/token framework in public headers.
Route eager resident storage replaces the earlier lazy Route::instance lifetime.
The private owned transaction does not issue real launch/kernel admission;
production construction/selection still requires the unresolved target evidence.

Focused owned controls must establish: extra-thread refusal with unchanged bytes;
mask verification including libc-reserved RT signals; pending async signal held
through the patch and delivered only after a coherent commit/rollback; no
allocation/lazy guard/exit-destructor registration in the final interval;
original-byte/mapping/range refusal; injected failures at each protection/cache
and write stage; verified rollback or fail-closed exit in disposable child
processes; original-once/unwind behavior through the installed route; and later
capture handle destruction without route/state destruction. All negative
process tests are owned executables, not tablet experiments.

Remaining actual target evidence is whether this exact pre-main profile meets
the stronger thread/signal/kernel/mapping admission and preserves its prior mask
and stock input on the real device. Source controls cannot grant that claim.

## Primary references and source limits

The existing local loader/bootstrap establishes the source facts above. The
protocol is a proposed inference requiring implementation and qualification.
Linux documents a per-thread signal mask and the glibc/kernel mask distinction,
including ignored internal RT signals in the wrapper:
[sigprocmask(2)](https://man7.org/linux/man-pages/man2/rt_sigprocmask.2.html).
Linux signal semantics, unmaskable signals and pending delivery are documented in
[signal(7)](https://man7.org/linux/man-pages/man7/signal.7.html).
The kernel documents Threads as a process thread count in
[/proc](https://docs.kernel.org/filesystems/proc.html); that description alone
does not qualify the target kernel's concurrent count/clone implementation.
Fixed-buffer directory parsing uses the kernel interface documented by
[getdents(2)](https://man7.org/linux/man-pages/man2/getdents.2.html).
