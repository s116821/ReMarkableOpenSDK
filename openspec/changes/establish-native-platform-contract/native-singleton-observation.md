# Candidate R2: observe the registered controller without invoking QML

Status: original R2 executed once on October 1, 2026 and refused at its type-count bound; see [recorded evidence](research.md#completed-development-observation-r2-bounded-refusal). No controller was located. The original scope below is retained for traceability. Main owns implementation, review and device operation. Production insertion remains unqualified. The separate R2D proposal below is not execution authorization.

## Concrete discovery chain

Static analysis of the saved executable finds construction of the document controller with a null QObject parent, followed by QML singleton registration with its known static meta-object. Later reparenting is not excluded. The registration's function wrapper allocates a capture holding a guarded pointer to the existing object, and its invoker directly forwards to Qt's SingletonInstanceFunctor. This is an existing-instance registration, not evidence that a live singleton lookup is side-effect-free.

The matching vendor QtQml library's type-by-ID path obtains a global registered-type list. Its accessor locks a recursive mutex and may initialize global state. The proposed observer must not call it. Host inspection instead identifies the already-initialized storage and confirms list pointer/count use. The vendor private headers plus an original compile-only ARM layout probe identify the type-record registration kind, base meta-object, singleton-info pointer and callback storage. These are exact-artifact research facts, not public or portable Qt ABI promises.

The candidate external read chain is:

1. Verify actual QtQml provider hash and mappings in addition to R1's executable/QtCore checks. Resolve the private registry locator relative to its verified ELF load bias, with current readable-mapping bounds. Refuse uninitialized/destroyed/unknown guard state; never initialize it.
2. Sample the fixed registered-type list descriptor and bounded pointer array. For each non-null record, read only registration kind, base meta-object pointer and singleton-extra pointer. Match the known controller meta-object exactly; do not enumerate strings, URLs, properties or source content.
3. For a matching singleton record only, inspect its singleton-info callback descriptor. Require the exact independently traced manager/invoker identities for the existing-instance functor before interpreting its capture. An arbitrary std::function or other singleton factory must remain uninterpreted.
4. Read only the capture's guarded-pointer control/object pair. Check non-null/aligned/in-range pointers, nonzero sampled strong-reference state, the controller's allowlisted vptr and QObject q_ptr backlink. Do not change reference counts or call the functor. Guarded-pointer checks and repeated reads do not acquire object lifetime ownership.
5. Reread registry/record/callback/capture/object descriptors and process identities. Any disagreement, partial read or ambiguous candidate produces incomplete/ambiguous output. Even matching samples retain non-atomicity and ABA limitations.

The exact provider hashes, ELF locators, compiler measurements and static corroboration stay in the private experiment manifest. The public helper must not embed firmware-specific addresses or accept arbitrary unrestricted memory-dump requests. Input manifests must select only this fixed semantic read shape.

## Bounds and results

Retain R1's 2-second cooperative helper budget, 10-second outer operator deadline, 64 KiB total remote payload and 256 KiB mapping-input cap. Limit the registry to 1024 entries and at most four matching candidates. A larger list or exhausted budget refuses an exhaustive result; do not silently page through more entries or scan the heap. Limit output to 8 KiB of status, checked provider/process identity, counts and matching pointer relationships. No raw memory blobs, arbitrary object names or other registered-type payloads.

The intended result is a sampled route to an existing DocumentController object, or an explicit failure/ambiguity with its stage. This does not prove that the object owns the currently displayed page, that insertion can be invoked safely, or that an SDK lease exists. It does not expose a callable native capability.

## Verification and execution boundary

Use owned fixtures to verify mismatched provider/layout, registry guard states, changed list/record pointers, wrong singleton kind/meta-object, foreign callback manager/invoker, expired guarded pointer, multiple candidates, partial reads, output limits and timeout cleanup. Verify that all failure paths make no target writes/calls and that unknown factories are not traversed. Compiler/header layout measurements must be corroborated by static provider accesses before main uses them on the device.

Reuse R1's advance notice, independent USB SSH recovery, private temporary files, exact-helper-only timeout termination and post-run process/response checks. The observer makes no ptrace attachment, suspension, QML evaluation, getter/factory call, service modification or retry. If a syscall outlives the cooperative budget, do not claim cleanup until the exact helper's termination is verified; inability to verify ends the experiment without modifying xochitl.

Worker/queue/lock fields are outside this proposal. A later manifest may inspect them only after a corroborated root path exists. Native method invocation and any mutation require a separate concrete executor/guard experiment; registry discovery cannot substitute for execution-time source serialization.

Source basis: host-only ELF/control-flow inspection of the privately retained executable and vendor QtQml provider; Qt 6.10.3 public/private headers; original compile-only layout measurements; accepted R1 limits. Raw locators and traces have no public link. The read sequence and bounds are a proposed experiment, not a live result or shipping mechanism.

## R2D proposal: fixed descriptor diagnostic after count refusal

The saved R2 output omitted the rejected raw count. Offline reinspection confirms the manifest's descriptor/guard locators against the exact provider instructions, the compile-only type-list offset, and load-bias arithmetic from the operator's saved mappings. No concrete locator correction was found. This corroboration does not establish the contents of the omitted live descriptor. Raising the existing traversal bound would assume the answer to that unresolved question.

Propose a separately selected descriptor-only diagnostic, implemented and independently reviewed by the ordinary helper lane before one new announced operator run. It must not automatically run after an R2 refusal. Reuse the exact provider/executable/QtCore checks, fixed private registry/guard manifest, mapping validation, process continuity and cleanup rules. The entire remote read set is one guard byte plus the same 12-byte list descriptor, followed by a second guard byte and descriptor: at most 26 bytes. Refuse unknown guard state, partial reads, changed identity/mappings or changed descriptor. No type-array, allocation-header, record, callback or object pointer is followed; do not allocate or iterate according to the sampled count. No target calls, writes, locks, factory invocation or heap scan.

Private output, capped at 1 KiB, should contain the two guard values, the three fixed descriptor words with semantic labels, the count interpreted both as signed and unsigned 32-bit, descriptor equality and incomplete/non-atomic flags. Partial failure must distinguish absent fields from sampled zero. Fixed pointer values remain private and are data only, not addresses accepted for another read. Retain the 2-second cooperative/10-second outer deadlines and 256 KiB maps bound, but tighten total remote payload to 26 bytes. Owned fixtures must prove zero pointer traversal for huge/negative counts, exactly bounded reads, and refusal for uninitialized/changed/partial samples. Independent artifact reproduction and exact operator review remain required.

The diagnostic can distinguish a sampled positive over-cap count from a negative/implausible count and expose the fixed descriptor for offline comparison. It still cannot prove that the registry layout is semantically correct, discover the controller, or justify broader traversal. A later discovery strategy requires its own evidence-based bound and review; R2D neither changes R2's 1024-entry limit nor authorizes a retry of R2. This is a reviewable proposal only, pending the existing coordinator/sole-operator decision.
