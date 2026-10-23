# Owned pre-main loader study

This is a separate, original owned experiment for the unresolved startup gate
in [the runtime contract](native-insertion-guard-runtime-core.md). It is not a
native loader or execution packet. The existing exact-pointer hook core and
its tests stay frozen; this study does not revise native carrier policy.

## Question and controlled composition

Can the pinned repaired interception archive and the reviewed hook core install
both entries from a loaded DSO constructor before the owned host enters main,
with actual linked initializer ordering and ordinary post-startup forwarding?

Use only original ARM entry veneers in an owned host executable, the pinned
GCC hard-float/Qt SDK, and repaired archive
`622b17dc65d0dcad717d83264f783ad12044565ed8ea86f9e5088d9abf5328ef`.
The host exports distinct owned entry addresses; the DSO resolves only those
explicit names. It must never resolve a native document/worker address or read
the proprietary executable. Compile in ARM mode; reject an unsupported compile
profile rather than pretending this is a portable loader.

Compile the archive-referencing core object and a separate startup object. Link
the selected library objects ahead of the startup object, then inspect the
actual DSO init_array, relocations and corresponding constructor symbols. Link
order is a proposed method, not evidence by itself. Required dependency
initializers must precede this DSO's installation; no QCoreApplication or worker
thread may exist at installation time in the owned experiment.

Use checked mapping/byte readers and the existing complete-pair installation
verification. Keep the DSO, Core and originals resident through process exit.
There is no unhook, dlclose or runtime teardown claim. An installation failure
must terminate the owned process before main, not fall through to its host.

## Observable result

The host must corroborate that installation completed before main. After normal
QCoreApplication construction, call the owned ordinary entry on a real QThread
with no tracked registration: the outer and inner paths must transparently
preserve the original public carriers and execute the owned originals once.
Keep this focused on DSO startup/forwarding; do not recount the existing hook
matrix as new coverage or claim native rejection from an owned host.

Retain exact build inputs, archive/image identity, compiler/link command, ELF
and init_array evidence, actual execution output and bounded failure/timeout
logs. No network, tablet or global configuration changes are necessary. Run
under the existing ARM emulator with absolute per-process limits, and preserve
failed attempts before fixes.

## Native transfer remains separate

A future native payload must independently establish exact target/providers,
its own linked initialization order and installation before native workers.
It must check actual native entry/trampoline bytes and mapping protections and
provide candidate-independent recovery. The owned host's exported names,
layout and initialization observations cannot substitute for those facts.
The ordinary caller's ephemeral optional carrier, Task/job attribution, native
owner lifetime and original unwind remain separate unresolved obligations.

Source basis: current runtime/refusal contracts, pinned interception source
review and existing owned hook results. The proposed startup composition is an
inference to test, not an already demonstrated native method.
