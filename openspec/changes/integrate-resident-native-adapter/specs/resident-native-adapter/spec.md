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
