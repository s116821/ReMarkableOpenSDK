# Native worker callback and ownership contract

The retained RM2 executable with SHA-256
`071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df`
contains an internal request that copies a callable into a worker command and
returns a worker-local unsigned 64-bit job ID. This is an exact-build research
interface, not a Qt invokable or a public SDK API. No portable native capability
or insertion authority follows from these findings.

The separate [native no-op result](../experiments/native-worker-callback-result.md)
confirms delivery, matching completion and owner release on one exact candidate.
It does not extend these findings to insertion or full lifecycle qualification.

## Delivery and completion

The constructor copies a `std::function<void()>` through its actual native
QVariant metatype into queued command storage. The supplied function object may
be destroyed after normal submission return; the queue retains its own value.
The command dispatcher makes a local callable copy, invokes it, destroys that
local copy and returns. The normal worker loop subsequently clears active-job
bookkeeping and emits `jobCompleted(qulonglong)`.

For this exact command path, matching completion follows callback return. It
precedes the native pending-changes helper and destruction of the command's
remaining QVariant and is therefore not
a payload-unload or persistence barrier. Completion alone is insufficient:
require independent closure entry/thread/count facts, the exact worker
incarnation, and its returned nonzero job ID. Other job types can emit the same
signal when skipped during shutdown. The counter belongs to one worker, not to
the process or document ecosystem globally.

Enqueue always appends a command. Calling the ordinary AddPage request inside a
callback enqueues a later operation and leaves an execution gap. This seam does
not make source/order validation atomic with insertion. The inspected Entry
lock/unlock pair maintains a nested file-lock count, releases its short mutex
before returning and can trigger deferred saving on unlock. It provides no
demonstrated worker exclusion or expected-source/order predicate.

## Strong receiver ownership

The constant public `SceneController.worker` property checks a weak QObject
control record and returns a pointer. A QPointer detects destruction but cannot
keep an otherwise unowned receiver alive through synchronous enqueue signals.

The exact native creation-controller resolver accepts the document's actual
typed `entry::Id` QVariant data and returns an owned native Document. The native
Document's mode-one worker acquisition promotes its existing weak worker owner;
it neither creates a missing worker nor schedules the mode-two request. Require
the acquired complete-object pointer to equal the qualified public worker.
Both returned strong owners cover submission and normal successful completion
bookkeeping. In the original plain-state no-op Entry, cancellation or exception
disarms observation and releases them after its synchronous stack unwinds without
requiring matching completion; that closure does not use the native receiver.
The plain-state closure can remain queued and the payload stays resident. This
historical policy does not cover a callback that accesses the native worker;
the later ownership fence is distinguished below.

These native owner returns use nontrivial ARM sret output storage. The adapter
adopts only normally returned opaque raw/control pairs and releases each
nonnull control exactly once with the native control-release helper. It never
deletes a raw pointer, invents an ID from its UUID text, or copies a private
object's member fields as a substitute for acquisition. Partial exceptional
return is an unknown outcome, not ordinary owned storage.

Preparation and release stay on the guarded GUI thread. Strong references cover
ordinary reference-counted lifetime; they do not prove protection from arbitrary
explicit deletion, corruption, shutdown races or source-order changes. Native
release can invoke a deleter and reenter the UI. The owning Entry must disarm
callbacks before release and recheck its own lifetime afterward.

## Diagnostic implementation

The original witness captures only shared plain atomic state and an immutable
GUI thread ID in the native closure. It performs no QObject/QML reads or file
writes on the worker. A `returning` counter records a tail marker; the separately
matched native completion supplies the inspected post-return boundary.

The queued GUI receiver connects before submission, buffers at most sixteen
early completion IDs, consumes one submission even on refusal/exception, and
retains the original absolute deadline. Cancellation disarms observation;
it does not retract native work. Copied closures can outlive the observer
without referring to Entry, Sample, QML objects or borrowed guards. The payload
remains resident because native storage may still retain its callable manager.

The Entry records selected-page facts explicitly as `before-worker-submit`.
They remain separate from later callback/completion facts. It does not resample
the Reader or relabel earlier readiness as post-callback readiness. Native,
render and durable authority remain false.

The loader requires ARM32 little-endian hard-float ABI, exact executable size
and digest, expected main ELF mapping, pinned mapped function bytes, actual ID
metatype identity and native worker metadata/base. Unknown builds refuse.
Owned host/ARM tests model the contract; they do not execute these private
addresses or qualify the native adapter.

## Ownership followup — October 23, 2026 UTC

The later [normal six-read result](../experiments/worker-local-order-result.md)
retained the native owners through actual worker access, exact matching
post-return completion, GUI lease close and holder destruction, then a later
zero-depth settled turn. The [already-running cancellation result](../experiments/worker-running-cancellation-result.md)
instead retained them in a permanently Quarantined slot through six reads and
one exact late completion. Normal completion remained zero; no close, holder
destruction, release or settled state followed. Neither result proves destruction
of the shared native Document/Worker objects or general last-owner teardown.

The post-completion helper `0x6df78c` can emit Worker local signal20 while holding
two native locks. Retained metadata identifies it as `hasPendingChanged(bool)`,
stub `0x6cd698`. One exact constructor-installed route at `0x6b8ff4` forwards it
to native Document signal14, `hasPendingStoreLines(bool)`, stub **`0x68865c`**,
with explicit **QueuedConnection (2)**. This route queues receiver delivery; it
does not establish the full receiver set or exclude other synchronous callouts.

Exact Worker shared-control RTTI/vtable bytes identify disposal at `0x6acb30`.
It compares the current thread with `QCoreApplication::self`'s thread. Its
application-thread branch can call Worker destructor `0x6de7e4` synchronously;
the other branch uses `deleteLater`. That destructor can call `QThread::wait`
at `0x6dedb8`. Native Document's destructor is `0x6a6dec`; deleting wrapper
`0x6a735c` calls it then frees the object. The first call from the destructor to
`0x6b4240` was unresolved at that checkpoint. A later bounded ARM lookup shows
it promotes the existing worker control under a Document mutex, unlocks, then
calls `0x6d62b8`. The latter's running-worker path constructs command six and
reaches an explicit std::condition_variable wait at `0x6d6468`, with a conditional
second wait at `0x6d65bc`. Thus Document destruction has an additional wait
dependency before the separately known Worker-destructor wait. Its complete
notification/termination/GUI dependency remains unresolved; this is not an
observed deadlock or a quiescence proof.

Consequently, matching completion is not proof that GUI release of the last
Worker/Document owners cannot wait for native work needing GUI progress. The
known queued signal route narrows one concern without proving either a deadlock
or its absence. A live Reader does not prove that another strong native owner
will prevent last-reference disposal. The two results alone do not justify a
native canceled-slot reclamation experiment.

Mechanically, the retained `Prepared::close` releases its owned controls on GUI
without consulting the old Reader/controller permit. That separates cleanup of
existing ownership from authority to perform or publish a canceled operation;
it is not a production cleanup protocol. A future physical finalizer would need
independent resident ownership, exact original worker/job/generation attribution,
post-return facts, deferred zero-depth cleanup and separately settled capacity
reuse while logical cancellation stays terminal. Unknown/zero submissions and
partial cleanup cannot be rehabilitated by guessing a later job ID. Callback
managers/passive history and payload residency remain separate from native-owner
release. No native cancellation, queue drain, general reclaim or unload is qualified.

## Provenance and limits

REA 6.7 with Ghidra 12.1.4 completed four scoped constructor/enqueue/Entry-lock
analyses, retained as evidence
`ev_72169b9bd61da5d3cd09af70eba9ef185efed4b286a99b2c23400e5d696210b7`.
The prior dispatcher/loop analysis is
`ev_607722522c769a1424258116eb919622bde709ff101df224b91a8f7ed8e5f093`.
The exported private snapshot has SHA-256
`40ac6bfa1719e4f3cd07ede595a2ac309dbc1183c390d310a97857539edb1e92`.
No proprietary pseudocode or resource bodies are reproduced here.

The strong-owner ABI findings also use retained GNU ARM disassembly and an
owned **compile-only** GCC 13.4 hard-float ABI corroboration, not additional REA
execution. Exact assembly preserves both return words for the job ID; it takes
precedence over the decompiler's guessed 32-bit prototype. The evidence supports
the machine-level call contract, not recovery of original C++ const/ref spelling.

The ownership followup uses separately scoped exact ELF metadata/RTTI and GNU
ARM disassembly. Its private lookup receipt has SHA-256
`e32a9a0263818b3eac75dd4add5f084c2778aaddbc9a79b97d4e6982900e4a49`;
the bounded findings have SHA-256
`48e775f767fea1d6fef7c7498d866d6c3b70f01d38c9311a08cc9922e10baf4c`.
An initially misread `0x6b865c` receiver range is explicitly rejected; the accepted
`0x68865c` stub independently emits Document local14. No unrelated containing
body supplies that result. These lookups started no new analysis engine and
performed no target/device execution.

The later two bounded destructor-dependency slices have SHA-256
`48cea6d63a3641ab7366f436c9fa782f180c5c60796ac0bbcc71f266ad61f50b`
and `23aeb103772e5aa53bef977b7a03a28d9a572397a1e9913ca7a9a65d30f9fd49`.
Their independent retained-artifact review receipt has SHA-256
`556a64a93167827093f861dd852a09a0a1da2c1de56b5b3d233a72ffc911092a`.
These are offline disassemblies, not a native destructor experiment.

The separate successful no-op native witness establishes callback delivery and
matching post-return completion only. Same-turn insertion, GUI visit ownership, actual
inserted/selected target, render completion, multi-file persistence and complete
resident/failure/uninstall lifecycle qualification remain separate work.

Source basis: retained REA/Ghidra records, exact-artifact ARM disassembly,
compile-only ABI checks and original diagnostic source. Lifecycle design and
the proposed next experiment are engineering inferences, not native results.
