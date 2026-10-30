## ADDED Requirements

### Requirement: Resident protocol correspondence
The SDK SHALL bound and version every resident control frame, bind it to the
expected runtime and connection incarnation, and correlate each response with
the complete immutable request. Peer authentication SHALL be independently
established; echoed fingerprints alone SHALL NOT authenticate the endpoint.

#### Scenario: Duplicate, stale or foreign reply
- **WHEN** a reply has an old sequence, mismatched operation/request digest or runtime/connection identity
- **THEN** it cannot complete the current request or authorize native work

#### Scenario: Incomplete or oversized frame
- **WHEN** the peer disconnects mid-frame or declares a payload exceeding the fixed bound
- **THEN** the frame is never accepted and the decoder closes without resynchronizing

#### Scenario: Unknown mutation after dispatch
- **WHEN** a mutation loses its connection or receives an indeterminate reply
- **THEN** the original operation/digest remains indeterminate across repeated cleanup, the connection is retired and no automatic mutation replay is permitted

#### Scenario: Request expires during native execution
- **WHEN** an admitted mutation reaches the native executor or completes after its original endpoint deadline or connection retirement
- **THEN** the executor retains that immutable deadline, its permit cannot authorize further guarded actions, and late completion cannot publish a response on another request

#### Scenario: Client authenticates the resident process
- **WHEN** a Linux client connects through the supervisor's retained activation-directory descriptor
- **THEN** it verifies directory/socket ownership and permissions, the socket inode, kernel peer UID/PID, current boot and process start ticks against independently supplied expectations before sending Hello or request bytes

#### Scenario: Journal failure or partial transport loss
- **WHEN** a client prepares a mutation
- **THEN** its exact operation, source, target and frame digest are offered to durable journaling before pending admission or transport write; journal failure sends no mutation bytes, and failure after admission retains the original indeterminate identity without replay

#### Scenario: Slow peer drips partial bytes
- **WHEN** a peer makes partial transport progress
- **THEN** the caller's absolute deadline is not extended by that progress

### Requirement: Caller progress during resident transport
The transport SHALL offer caller-thread progress checks during handshake and
request IO using bounded poll slices and the original absolute deadline. A
failed check SHALL retire the connection. Progress SHALL be checked after
durable journaling before pending admission and after reply authentication before
completion. Synchronous callbacks SHALL NOT renew the deadline. Socket retirement
SHALL NOT be interpreted as native physical cancellation or safe replay.

#### Scenario: Cancellation after reservation before dispatch
- **WHEN** caller progress fails after journaling but before pending admission
- **THEN** no request bytes are sent and a new effect is known not dispatched

#### Scenario: Cancellation during a dispatched effect
- **WHEN** progress fails while awaiting or finally admitting an insertion or selection reply
- **THEN** the original operation and digest remain indeterminate without replay, including when complete reply bytes have arrived

#### Scenario: Stalled endpoint or slow progress callback
- **WHEN** a peer stalls or a synchronous progress callback consumes the remaining time
- **THEN** bounded wait slices continue caller servicing while time remains and the original deadline prevents subsequent IO or successful completion

### Requirement: Explicit historical native facts
Native identity/order samples lacking revision or input-epoch producers SHALL
remain explicit historical facts. They SHALL NOT construct operational guards,
durable receipts or advertised native support. The native executor SHALL enforce
its qualified preconditions again at the actual insertion boundary.

#### Scenario: Mutation observed without persistence evidence
- **WHEN** a native executor reports an attributed insertion and exact permitted after-order
- **THEN** the result remains historical insertion evidence, not durable commit, current target selection or writing authority

#### Scenario: Result exceeds bounded representation
- **WHEN** an insertion's resulting complete order cannot fit the protocol bound
- **THEN** the request is rejected before dispatch

### Requirement: Historical native source capture
The resident SDK SHALL return owned image bytes and explicit historical source
evidence from the measured guarded native capture procedure. It SHALL verify the
expected document, selected page and complete order inside both validated source
callbacks, retaining the same live Reader/document/controller and capture context.
Intermediate image bytes SHALL remain private until the final join, original
request admission and deadline checks succeed. This evidence SHALL NOT fabricate
content revisions, input epochs, atomicity, physical freshness, persistence or
continuing capture/write authority. Capture SHALL preserve unresolved effects.

#### Scenario: Final join fails after image encoding
- **WHEN** image encoding succeeds but the second join loses the original objects or expected full order
- **THEN** the request refuses and no intermediate PNG is published as successful capture evidence

#### Scenario: Capture exceeds ordinary control frame size
- **WHEN** a valid bounded PNG needs more than the ordinary 64 KiB frame allowance
- **THEN** only the current capture reply may use its explicit 1 MiB PNG and 1 MiB plus 64 KiB frame payload bounds, excluding the four-byte length prefix, while ordinary control and historical revision 1/2 payload bounds remain unchanged

#### Scenario: Capture delivery loses admission
- **WHEN** cancellation, connection loss or the original absolute deadline interrupts capture publication, transfer or final admission
- **THEN** the image cannot complete successfully, earlier unresolved insertion/selection evidence remains unchanged, and no deadline renewal or effect replay follows

#### Scenario: Capture protocol upgrades historical consumers
- **WHEN** the live protocol advances to support capture
- **THEN** canonical historical revision 1 and 2 frames retain their original tag grammar, bytes, digests and correlation, without becoming valid live requests

### Requirement: Visual-only window evidence
The resident SDK SHALL provide a separately correlated visual-only window capture
without ReaderFacts or page/write authority. It SHALL retain the original GUI
objects, request admission and absolute deadline through private encoding and
publication, with sticky refusal and a shared source/visual reentrancy guard.
The consumer SHALL fully validate the bounded PNG before exposing immutable
historical visual evidence. Only source/visual image replies may exceed the
ordinary control-frame bound. Visual evidence SHALL NOT settle uncertain effects.

#### Scenario: Library screen lacks Reader facts
- **WHEN** an admitted visual request captures an owned window without an open Reader page
- **THEN** it may return bounded image evidence without inventing page identity or allowing source capture or writing

#### Scenario: Nested capture or lost context
- **WHEN** capture reenters, loses an original object, expires, or loses admission before publication
- **THEN** it refuses even if the condition later recovers, and no partial image is published

#### Scenario: Visual protocol upgrade preserves history
- **WHEN** live transport advances to revision 4
- **THEN** historical revisions 1, 2 and 3 retain exact bytes, grammar, digests and correlation; old revisions reject visual tags and remain invalid live requests

### Requirement: Fresh conditional adjacent target selection
Target selection SHALL be a separately correlated effectful request. It SHALL
revalidate exact document, current page and complete order in a fresh actual Reader
join. It SHALL invoke no navigation when already on the target, otherwise at most
one qualified adjacent transition. A fresh post-transition join SHALL establish
the same actual objects, intended selected page and unchanged order before success.
Uncertain effects SHALL retain original operation/target identity without replay.
Historical selection SHALL NOT grant capture, writing or persistence authority.

#### Scenario: Target already selected
- **WHEN** a fresh joined observation matches the requested target and complete expected order
- **THEN** selection reports that historical observation without navigation

#### Scenario: Adjacent target needs one transition
- **WHEN** the actual source and complete order match and the target is its immediate successor
- **THEN** at most one transition is admitted, and success requires a separate fresh target join afterward under the original deadline

#### Scenario: Selection becomes uncertain
- **WHEN** the connection, deadline, original object identity or expected order is lost after selection admission
- **THEN** the original selection remains uncertain and neither navigation nor insertion is retried automatically; attempted uncertain selection retains the shared process effect gate across connections and conversations until separately admitted recovery or qualified settlement, while read-only requests remain available

#### Scenario: Reconcile an uncertain selection
- **WHEN** the caller requests read-only reconciliation
- **THEN** its payload preserves the original selection source, present target and order without fabricating insertion facts, and reported presence/current selection does not clear effect quarantine

### Requirement: Preserve historical journal wire identity
Historical journal inspection SHALL retain exact canonical original bytes and
their digest across supported wire revisions, independently of strict current
transport decoding. Unsupported or malformed history SHALL refuse without
rewriting or discarding evidence. History SHALL NOT authorize effect replay.

#### Scenario: Consumer upgrades from protocol revision 1 to 2
- **WHEN** a consumer loads a canonical revision 1 insertion request and response
- **THEN** it validates their original bytes, digest and same-revision correlation without re-encoding as revision 2 or clearing unresolved reservations; the live endpoint still rejects revision 1

#### Scenario: New tags appear under an old revision
- **WHEN** historical input uses selection tags under revision 1 or an unknown revision
- **THEN** inspection refuses the input without changing stored evidence

### Requirement: One-use native activation admission
The native bootstrap SHALL independently verify its exact profile and artifact
correspondence before exposing an endpoint. It SHALL consume a fresh bounded
supervisor admission bound to the current boot and parent process incarnation,
retain consumption evidence, and pin the admitted directory through startup.
Encoding admission SHALL NOT itself launch a process or advertise capability.

#### Scenario: Expired, mismatched or previously consumed admission
- **WHEN** launch admission has expired, identifies different artifacts or a different supervisor, or was already consumed
- **THEN** the bootstrap refuses it without accepting a socket request or clearing its consumption evidence for retry

#### Scenario: Directory replaced after admission
- **WHEN** the admission directory is replaced before endpoint startup
- **THEN** the replacement cannot become the admitted endpoint directory

#### Scenario: Payload environment inherited by a child
- **WHEN** the admitted adapter completes pre-main initialization
- **THEN** it removes preload, eager-binding and activation-directory environment variables while keeping existing hooks and mappings resident

### Requirement: Resident lifetime and bounded retirement
Logical request retirement SHALL end that request's publication authority without
unloading the adapter or routinely restarting xochitl. Native owner reclamation
SHALL require separate physical callback-completion evidence and bounded capacity.

#### Scenario: Late completion after cancellation
- **WHEN** a retired native request completes after the caller has canceled
- **THEN** its late delivery cannot authorize another request or erase mutation ambiguity

### Requirement: Unsupported until qualified
Capability discovery SHALL remain observational and SHALL NOT activate the native
runtime. Unknown model/firmware/profile combinations and unimplemented integration
boundaries SHALL remain unsupported, independently of passing host protocol tests.

#### Scenario: Protocol feature enabled without native executor
- **WHEN** a consumer enables the native-adapter protocol feature alone
- **THEN** UnqualifiedPlatform continues to report unsupported native operations and the default dependency-free build remains available
