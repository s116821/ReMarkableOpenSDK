# Resident SDK integration

October 24, 2026. This amendment selects implementation of the resident control
boundary returned by [REM-57](https://linear.app/magentumdragon/issue/REM-57).
The measured native procedures and remaining qualification are described in the
[integration plan](https://github.com/s116821/ReMarkableOpenSDK/blob/05af65656e07c939349c9a10419ba94cd8b7457e/docs/research/native-adapter-integration-plan.md).

## Facts and public contract

Keep the existing synthetic `Platform` contract intact while introducing a
separate, feature-gated `native_adapter` module. A native Reader sample contains
actual document/page identity and a bounded complete observed order. It does
not contain invented content revisions or input epochs. A sample is historical
data, never a lease. It cannot be converted to `PageObservation`, a creation
receipt, a capture permit or writing authority. The native executor must repeat
its live ownership checks and enforce the measured source/count/right-neighbor/
target-absence predicate at the original insertion boundary.

The first integration increment implements the wire contract and session state;
it does not advertise a native capability. Subsequent work must connect it to
the exact native profile and consumer before capability qualification.

## Wire and connection

Select a local Unix stream with asynchronous Qt-side reads. Use a four-byte
big-endian payload length and a maximum 65536-byte frame. All integers are
big-endian and all fields have one binary representation. No pointers, arbitrary
commands, filenames or native addresses are protocol inputs. Unknown versions,
tags, trailing bytes, invalid UUIDs, duplicate order IDs, invalid counts and
oversized frames close the connection. A partial frame remains incomplete;
disconnect or deadline expiry never turns it into an accepted request.

Every frame binds protocol revision, approved profile digest, executable digest,
adapter artifact digest, boot UUID, process PID/start ticks, activation UUID and
fresh client and server connection UUIDs. The caller and endpoint each obtain
fresh nonces from their OS; Hello has no server nonce, Ready supplies it and all
later frames bind both. This rejects old requests on a fresh endpoint connection
even if a caller incorrectly reuses its own nonce. The
supervisor supplies independently checked expected runtime identity. A handshake
echo is correspondence, not proof of trusted peer credentials or Qt readiness;
the Unix endpoint must additionally verify owner/permissions and peer identity.

Handshake sequence is zero. Requests use strictly increasing nonzero sequence
numbers and one active operation at a time. A retired request's sequence is
never reused. Sequence exhaustion closes the session. Stale, duplicate,
out-of-order and foreign-scope replies cannot complete a newer request.
The original immutable request is included in response correlation through a
SHA-256 digest of its canonical wire bytes. The digest is a correspondence check,
not authentication. Local access control and exact runtime qualification remain
mandatory. No reconnect automatically resends a mutation.

## Mutation and lifetime

### Owned Linux/Qt endpoint

`native/resident` implements the same schema in Qt and an asynchronous local
endpoint. Admission checks an existing owner-only mode0700 directory, a mode0600
socket, `SO_PEERCRED` UID/PID and the expected peer process start ticks. The
endpoint also checks its own boot/PID/start identity. The runtime's artifact and
profile hashes still require independent verification by the native bootstrap;
the transport does not turn supplied hashes into verified provenance.

The listener retains its directory descriptor and socket inode. Permission
changes use a pinned inode; cleanup removes only the recorded socket. This
protects against accidental replacement after identification. Same-UID code
with control of the directory is trusted, not an adversarial isolation boundary.
Callbacks bind the original socket. Reentrant backend calls cannot install a
replacement peer until the original read has unwound.

There is one peer and one pending request, bounded input/output, a five-second
handshake limit, thirty-second idle limit and a configured operation deadline
of at most thirty seconds. Partial bytes never renew a deadline. A plain lease
is invalidated on completion/disconnect/deadline. It contains no native owners;
physical worker completion and native reclamation remain separate obligations.
Without an executor, requests return Unsupported. The owned test backend may
retain a plain request to exercise timeout; it never performs native mutation.

The Rust and Qt implementations share generated test vectors, not native ABI
structures. Native source is deliberately outside the Rust source package until
the native distribution/build contract is implemented. Linux CI compiles and
tests it separately, including a Rust Session talking to the real Qt socket.

### Canonical binary schema

The length prefix excludes itself. Each payload starts with ASCII `RSDK` then
u16 protocol revision 1. It continues with three 32-byte digests (profile,
executable, adapter), boot UUID (16 bytes), PID (u32), start ticks (u64), activation
UUID (16 bytes), client connection UUID (16 bytes), server nonce (16 bytes), request sequence (u64), operation UUID
(16 bytes), then one message tag. UUIDs use their 16 canonical bytes and cannot be
nil, except the server nonce is all-zero only in Hello. Digests, PID and start ticks cannot be zero. Header fields are repeated in
each frame. No platform word sizes or native endianness appear on the wire.

Message tags: 0 Hello, 1 Ready, 2 Request, 3 Response. Hello/Ready contain no body
and require sequence zero. Request/Response require a nonzero sequence.
Requests contain a command byte: 0 Observe; 1 InsertAfter; 2 Reconcile.
InsertAfter/Reconcile then contain source facts and intended target UUID.
Facts are document UUID, selected page UUID, u16 count and that many page UUIDs.
Order is nonempty, unique, includes the selected page and contains at most 2048
IDs. InsertAfter must leave room for its one additional result ID. This wire
bound is not a native document-size support claim; the measured native profile
currently imposes its smaller independent diagnostic limits.

Responses contain the 32-byte SHA-256 of the complete length-prefixed original
request, then a result byte: 0 Observed (facts); 1 InsertionObserved (document UUID,
target UUID, order count/IDs); 2 Reconciled (current facts and a strict 0/1
target-present byte); 3 Rejected (reason byte); 4 Indeterminate (no body).
Rejection reasons are 0 Unsupported, 1 BeforeDispatch, 2 ExecutorBeforeMutation,
3 UnknownIdentity and 4 Capacity. Future variants require a protocol revision.
No trailing or optional bytes are accepted. Insertion result validation requires
exactly the intended target and original order with one insertion after source.

## Mutation handling

Persist the original operation/source/target in the consumer journal before
calling create. The transport records possible dispatch before its first write;
a partial write, lost reply, canceled wait or closed connection after that point
is indeterminate. A returned insertion observation reports measured execution
and order only; it does not establish persistence or select the target.
Reconciliation is read-only and uses the original target. Presence alone cannot
prove operation attribution or authorize another insertion.

An explicit indeterminate mutation reply also retires the client session and
retains its original operation/digest across repeated close calls. Client sequence
correlation is not durable operation deduplication: the consumer journal and
native endpoint must enforce original-operation reuse across connections.

Logical retirement invalidates reply/getter authority immediately. Physical
native callback completion, owner release and capacity reclamation are separate
requirements. The production adapter must not retain one new leaked native owner
per conversation or unload itself to end a conversation. Until callback ownership
and bounded reclamation are connected and tested, mutation remains unavailable.

### Exact-profile source integration

Import the measured Reader/QML/native mechanisms from research revision
`05af65656e07c939349c9a10419ba94cd8b7457e`, retaining the original Git-blob
hashes separately from SDK adaptations. The new order observer executes only
inside the original selected-Reader join's guarded callback. It verifies actual
typed document identity, both page mappings, uniqueness and two matching complete
orders. Its current 17-page envelope preserves the measured diagnostic bound;
broader support remains a qualification task. Results contain only historical
wire facts. Reconciliation rejects a different document and reports presence of
the original target without authorizing another mutation.

The read-only backend requires exact-loader facilities, an active endpoint lease
and the live focused Reader window/engine. No borrowed native pointer, QVariant
or guard crosses the synchronous callback. Source compilation and owned models
do not qualify actual native dispatch. Bootstrap admission remains separate.

Independent lifetime review rejects using matching job completion plus zero
active outer-hook entries as a general native-release barrier. Completion
precedes remaining native command cleanup. Last Worker-owner release on GUI can
enter a thread wait; Document destruction has additional command-six waits with
unresolved GUI dependencies. A single successful historical release does not
settle that graph. A production release policy needs stronger evidence before
mutation activation.

Reusable Core registration must bind cancellation, snapshots and retirement to
an opaque original generation. A stale Coordinator must never cancel the next
request. Registry locks must not cover native release or reentrant destruction.
Physical retirement must cover the exact once-submitted Task's future invocation
possibility as well as currently active stacks; clearing a record must not turn
a late formerly guarded call into ordinary unguarded forwarding. Any unknown
physical mutation retains the bounded mutation slot while independent read-only
observations remain possible. This is a design requirement, not implemented or
native-qualified reclamation.

The Core source now uses opaque weak-record tokens for registration, cancellation
and snapshots. Outer entry pins the record and records active frames before
native dispatch; the inner predicate is unchanged. Registry locks cover only
lookup/removal, with native calls and record destruction outside. Its private
retirement method checks the original token and active frame count, but still
requires the future executor's independent no-future-invocation proof. The
Coordinator now passes its original token on every Core operation. No public
no-argument cancel/snapshot or production retirement caller remains.

The imported GUI fence separates immediate logical invalidation, framed cleanup,
and a distinct later retirement notification. Admission is later still, after
the notification stack unwinds. Its scope binding carries no native owner and
does not establish Task completion. The unchanged research implementation has
a 256-nonce history bound; production integration will use one fence per request
inside the single resident mutation slot, rather than treating that history as
a process-wide operation allowance. Durable deduplication remains the journal's
responsibility. This executor connection is not implemented by the import.

Coordinator acquisition and probe submission now require the process-resident
slot. The returned token is retained even if a native callout deletes the GUI
facade; the slot already owns any returned native words. The second source join
checks the same token's Worker incarnation, and Core's owner marker identifies
the slot. There is no per-request Bridge/Prepared fallback in this path. The
Coordinator's plain resident record and profile attribution remain retained
until the normal physical-cleanup executor is implemented.

### Non-last-only owner policy

Exact-artifact analysis supports a narrower alternative to calling the ordinary
native release helper: decrement only while the pinned strong count is greater
than one. The primitive uses at most eight lock-free compare/exchange attempts,
never touches the weak count and performs no native access after success. A
competing owner may destroy the object immediately after that success.

The private profile method verifies the same process, GUI thread, mapped release
code and control-table prefixes, the actual multithreaded libc flag, writable
control storage, exact vptr, raw/control relationship and positive weak count.
Document raw==control+16 is a strict supported-shape gate, not a claim that every
normally returned Document pair has this layout. Refusal retains the reference.

The method is private to the typed resident slot. The slot's private retirement
method has no production caller yet. Acquisition writes into retained storage
before native dispatch; only normal complete returns become owned references.
Exceptional or partial pairs quarantine the slot without interpreting or releasing
them. Successful release consumes only that owner's local words; refusal preserves
them and a later retry skips owners already consumed. No destructor invokes
ordinary native release. Opaque weak-state tokens reject a replaced allocation;
logical cancellation is separate from physical retirement. A single process-wide
slot factory prevents allocating a new retained pair for each conversation.

The executor must prove physical retirement independently before it can call the
private retirement method. Last-owner or uncertain controls keep
the bounded mutation slot occupied. Neither arithmetic fixtures nor offline ABI
analysis establish normal native availability or repeated-operation capacity.

## Verification and rollout

### Admitted native bootstrap

The build-only native artifact embeds the exact profile manifest digest and
requires the retained repaired Dobby archive/header. Its loader has eager
bindings, static TLS and NODELETE. It hashes the file backing its own executable
anchor mapping, checks a one-use supervisor admission, then calls the original
exact-profile verification/hook installer before main. The queued GUI startup
creates the read-only backend and endpoint. Native startup ordering, activation
and recovery are separately unqualified; loading this payload installs hooks
and refusal terminates with code119 even though request operations are read-only.

The [168-byte admission format](../../../native/rm2_3_28_0_172/BUILD.md) binds
approved artifact/profile/executable hashes, boot, fresh activation UUID, direct
supervisor parent identity, an admission window up to 60 seconds and operation
budget. Rust encodes it without file creation or activation. The native consumer
requires an owned mode0700 directory and regular mode0600 file and consumes it
through exclusive hard-link creation. Evidence names remain; they are never
cleared for retry. The endpoint pins the same directory and independently checks
socket credentials. The process derives its own PID/start identity. The directory
owner remains trusted. The launch deadline gates admission consumption, not the
whole native loader/startup duration; Supervisor must bound recovery separately.

Child processes do not inherit the preload, eager-binding or activation-directory
environment variables. Existing hooks and mappings remain resident. No per-request
unload/restart is introduced. This source integration is not a product capability
or permission to activate an unreviewed candidate.

Exercise real encoders/decoders with fragmented frames, invalid lengths and
schema, identity mismatch, duplicate/stale replies, sequence exhaustion and
mutation connection loss. Interoperate the Rust client with the Qt endpoint on
owned local sockets. Then qualify the exact profile adapter and resident lifetime
on disposable native fixtures. Pin the resulting SDK in Buddy PR 60 and update
Docs PR 56. The broader REM-25 acceptance matrix remains open. Default features,
source-package containment and unchanged synthetic behavior must keep passing.
