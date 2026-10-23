# Native worker callback and ownership contract

The retained RM2 executable with SHA-256
`071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df`
contains an internal request that copies a callable into a worker command and
returns a worker-local unsigned 64-bit job ID. This is an exact-build research
interface, not a Qt invokable or a public SDK API. No portable native capability
or insertion authority follows from these findings.

## Delivery and completion

The constructor copies a `std::function<void()>` through its actual native
QVariant metatype into queued command storage. The supplied function object may
be destroyed after normal submission return; the queue retains its own value.
The command dispatcher makes a local callable copy, invokes it, destroys that
local copy and returns. The normal worker loop subsequently clears active-job
bookkeeping and emits `jobCompleted(qulonglong)`.

For this exact command path, matching completion follows callback return. It
precedes destruction of the command's remaining QVariant and is therefore not
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
bookkeeping. On cancellation or exception, Entry disarms observation and releases
them after its synchronous stack unwinds without requiring matching completion;
the plain-state closure can remain queued and the payload stays resident.

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

A successful no-op native witness would establish callback delivery and matching
post-return completion only. Same-turn insertion, GUI visit ownership, actual
inserted/selected target, render completion, multi-file persistence and complete
resident/failure/uninstall lifecycle qualification remain separate work.

Source basis: retained REA/Ghidra records, exact-artifact ARM disassembly,
compile-only ABI checks and original diagnostic source. Lifecycle design and
the proposed next experiment are engineering inferences, not native results.
