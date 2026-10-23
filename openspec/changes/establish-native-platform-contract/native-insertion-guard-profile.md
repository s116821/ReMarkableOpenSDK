# Exact-build pre-main facilities

This specifies the native-only boundary consumed by the proposed
[negative Entry](native-insertion-guard-entry.md). It is not an independently
supported adapter or an execution packet. The owned startup study cannot
substitute for the actual payload's ELF, provider and initializer review.

## Main image and startup

Require Linux ARM mode, ARMv7 hard-float little-endian and the justified public
Qt/pthread sizes. Before installing hooks, require no QCoreApplication and
verify the actual running executable descriptor: stable identity, exact saved
size/SHA-256, ELF32 ARM ET_EXEC and hard-float flags. Bind main PT_LOAD ranges,
disk offsets and actual mapped code, not merely the executable filename.
Check current readable/executable/nonwritable mappings before reading code.
Bound file/parser sizes and avoid symbol setup inside a native critical section.

The exact saved image has no PREINIT_ARRAY section/dynamic entries, dynamic
IFUNC symbols or ARM IRELATIVE relocations. Preserve those verified facts in
the static profile. They narrow possible executable activity before the DSO
constructor; they do not prove the entire process has no threads. Record task
observation and application absence separately. Native target-use ordering is
an exact composition requirement, not a generic constructor-priority promise.

Pin original outer/inner bodies, their exact entry prefixes, dispatcher callsite,
final-lock/callsite context and the passive search/comparison code against the
same file image. Retain the private profile's exact ranges and evidence; do not
export a general native-address API. Install the reviewed complete pair only
after these preconditions. Unknown/partial state terminates without unhook.

Keep the library's selected plugin initializer before the install constructor
in the actual linked init_array. Add explicit dependencies, resolve bindings
before use and inspect the final artifact. Constructor priorities alone do not
order independent shared objects; see the
[GNU dynamic-linker guidance](https://www.sourceware.org/glibc/manual/latest/html_node/Dynamic-Linker-Hardening.html).
The [ELF initialization specification](https://gabi.xinuos.com/elf/08-dynamic.html)
also distinguishes executable preinitialization from shared-object initialization.
These are semantic references, not device-specific runtime evidence.

## Runtime facilities

Require LD_BIND_NOW=1 in the temporary candidate process and BIND_NOW on the
payload itself. Pin the actual main-image comparator JUMP_SLOT to the qualified
QtCore Qt_6 export before installation. Payload link flags alone do not resolve
the executable's previously lazy slots. Never warm up a native helper to cause
that resolution, or change systemd's global environment.

Supply the installed Core, qualified passive read/search/equality calls, exact
original return PCs and explicitly scoped caller admission. A Boolean admission
is not newly established ownership or selected-view authority. Passive word
reads use only the normal retained worker/map under the qualified original call;
the predicate's range/count checks still precede the three native searches.

Provide an outside-critical-section factory for an immutable job snapshot
context. From a strongly retained, exact-meta-object Worker, checked arithmetic
derives the selected queue-mutex and active-ID locations. Pin normal mutex kind,
public ABI and actual libc implementations/versions; the libpthread shim alone
is insufficient. Factory failure must refuse before submission. Do not create,
copy, repair or lock the native mutex during factory construction.

Resolve try-lock, unlock and exit before a snapshot can own the mutex. The native
uncertainty path exits directly without callbacks/loggers. Preserve the strong
owners/context through candidate exit. No renderer/GUI/IO call belongs in the
final-map predicate, and the job snapshot is outside that predicate and lock.

The actual payload must isolate its internal symbols from accidental external
preemption and verify its imports against the current provider closure. Retain
actual library/archive hashes, compiler/flags, mapped-code checks, init_array
and relocation evidence. No claim of general relocation, all writers, native
unwind, accepted creation, physical reclamation or safe unload follows.

Source basis: exact saved executable/retained ABI and provider records, reviewed
owned controls, current native proposals and primary ELF/GNU references. Native
startup/worker conditions remain candidate-qualified requirements.
