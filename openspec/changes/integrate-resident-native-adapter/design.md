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

## Verification and rollout

Exercise real encoders/decoders with fragmented frames, invalid lengths and
schema, identity mismatch, duplicate/stale replies, sequence exhaustion and
mutation connection loss. Interoperate the Rust client with the Qt endpoint on
owned local sockets. Then qualify the exact profile adapter and resident lifetime
on disposable native fixtures. Pin the resulting SDK in Buddy PR 60 and update
Docs PR 56. The broader REM-25 acceptance matrix remains open. Default features,
source-package containment and unchanged synthetic behavior must keep passing.
