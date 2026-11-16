# resident-native-adapter Specification

## Purpose
TBD - created by archiving change integrate-resident-native-adapter. Update Purpose after archive.
## Requirements
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
Native target selection SHALL be a separately correlated effectful request when
a separately qualified profile enables it. The installed RM2 insertion
profile SHALL leave this native selection route disabled. An enabled route SHALL
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

#### Scenario: Repeated insertions on an admitted installed profile
- **WHEN** an installed Reader insertion profile receives a fresh request after the previous request has completed and its native owners have been reclaimed
- **THEN** the executor can admit the fresh request in the same process subject to its ordinary identity, deadline and capacity checks
- **AND** an outstanding or unreconciled request still prevents admission to the occupied executor

### Requirement: Unsupported until qualified
Capability discovery SHALL remain observational and SHALL NOT activate the native
runtime. Unknown model/firmware/profile combinations and unimplemented integration
boundaries SHALL remain unsupported, independently of passing host protocol tests.

#### Scenario: Protocol feature enabled without native executor
- **WHEN** a consumer enables the native-adapter protocol feature alone
- **THEN** UnqualifiedPlatform continues to report unsupported native operations and the default dependency-free build remains available

### Requirement: Reproducible installed Reader bundle
The RM2 installed Reader adapter bundle SHALL bind its exact native library and
profile to a tablet-tested digest pair. The build SHALL validate the pinned Dobby
source archive, reconstruct the required compatibility edits, and verify the
resulting library and profile before packaging. The bundle SHALL include both
licenses and a checksum list, and remain separate from the Rust crate.

#### Scenario: Rebuild for the admitted RM2 firmware
- **WHEN** the pinned source inputs and the matching RM2 SDK are used to rebuild the bundle
- **THEN** packaging succeeds only if both native artifacts match the approved digests

#### Scenario: A changed source or native artifact is supplied
- **WHEN** the upstream source archive, compiled library or profile does not match its pinned digest
- **THEN** the build refuses to emit an installed Reader bundle

### Requirement: Development joined Reader focus receipt

The diagnostic SHALL use the existing authenticated revision-4 resident
connection and the original pending lease. It SHALL accept request kind 11 with
an empty body and return reply kind 13 with exactly one unsigned enum byte.
These kinds SHALL be available only in their respective opt-in native and
client builds. Existing Hello and Ready messages SHALL remain unchanged.
The operator SHALL establish exact opt-in artifact identity before dispatch.
An unrecognized kind SHALL fail closed without fallback or downgrade.

| Value | Receipt |
| --- | --- |
| 0 | Refused |
| 1 | NoLeaf |
| 2 | ExactScene |
| 3 | InsideScene |
| 4 | ScopeOnly |
| 5 | OutsideScope |

The classification SHALL refer to the active focus leaf of the one joined Reader
window at sampling time. ExactScene is the joined selected SceneView itself.
InsideScene is a descendant of that scene. ScopeOnly is within the joined device
scope but outside the selected scene subtree, including the scope itself.
OutsideScope is in the admitted window outside the device scope. NoLeaf is
reserved for a valid join with no active item. The existing focused-window
permit excludes the standalone sampler's NoFocusedWindow and OtherWindow
outcomes. They SHALL never be emitted as wire values.

Both messages SHALL use the ordinary 65,536-byte payload limit. The exact
payload lengths, excluding the existing four-byte prefix, SHALL be 204 bytes
for the request and 237 bytes for the enum reply. The original runtime,
connection nonces, sequence, operation and SHA-256 of the full length-prefixed
request SHALL govern correlation. The only alternative replies SHALL be the
existing Rejected(Unsupported) for unavailable base admission and
Rejected(UnknownIdentity) for unavailable or changed GUI admission, subject to
the original lease. A focus receipt SHALL never complete an ordinary or
pre-Enter request, and their replies SHALL never complete a focus request.

The existing selected Reader join SHALL establish the exact current scene,
device, page relationship and readiness before sampling. It SHALL use the same
GUI permit, join epoch, reentry scope and original absolute request deadline
through sample validation. Focus or relationship drift, including observed
away-and-back changes or reentry, SHALL yield Refused while the original GUI
permit and lease still allow publication. If the deadline has expired, the peer
is stale, or the lease has retired, the server SHALL discard the result.
The client SHALL reject a decoded reply if cancellation, deadline or peer
checks fail before completion. A late or mismatched reply SHALL never satisfy
a successor request.
Lost requests SHALL have no automatic retry and no pending mutation.

The receipt SHALL carry only the enum classification. It SHALL NOT create a
focus lease, page or visit identity, text receipt, edit or Submit authority,
capability advertisement, or mutation effect. The diagnostic SHALL NOT use
creation or source callbacks, activate a route, or consume or clear a mutation
reservation. Existing ordinary reply bytes, semantics and limits SHALL remain
unchanged with either feature configuration.

#### Scenario: Joined focus remains stable

- **WHEN** the same authenticated read-only owner completes a ready selected
  Reader join and its active focus leaf is a descendant of the selected scene
- **THEN** it SHALL return InsideScene against the original pending request

#### Scenario: Focus or the selected Reader changes during observation

- **WHEN** the sampler observes focus, page or ownership drift or reentry and
  the original GUI permit and request lease still allow publication
- **THEN** it SHALL return Refused without a classification

#### Scenario: Retired request cannot publish

- **WHEN** the server observes deadline expiry, peer change or retirement
  before publication, or the client observes cancellation before completion
- **THEN** the server SHALL discard the result or the client SHALL decline
  completion, respectively, and the result SHALL never complete a successor

#### Scenario: Exact shapes and explicit opt-in

- **WHEN** a disabled or older codec receives a focus kind, a frame has extra
  or missing bytes or an unknown enum value, or a reply mismatches the
  pending command or request identity
- **THEN** it SHALL fail decoding or correlation as appropriate without
  interpreting the frame as any other request or reply
