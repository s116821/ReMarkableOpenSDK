## ADDED Requirements

### Requirement: Typed scene request submission
The adapter SHALL submit the accepted attachment through the exact typed scene request using an existing validated resident worker owner and copied bounded page value. It SHALL NOT cast or invoke the generic worker callback entry for scene delivery.

#### Scenario: Ready owner and current permit
- **WHEN** the exact profile, ready token, original deadline and current permission guard admit submission
- **THEN** at most one typed request is attempted and its full 64-bit returned job ID is retained as enqueue evidence only

#### Scenario: Request outcome is ambiguous
- **WHEN** a request throws, returns zero, is canceled or reenters during submission
- **THEN** the result remains unknown, observation closes and the request is not retried

### Requirement: Borrow and cancellation across native delivery
The queued callback SHALL retain only adapter state and use Scene only during its live native borrow. Cancellation SHALL prevent later joins and preserve the original deadline without authorizing owner release.

#### Scenario: Retained callback outlives handle
- **WHEN** native callback copies arrive after the handle is stopped or destroyed
- **THEN** they do not inspect the Scene argument or attach a new recorder

#### Scenario: Generic retirement targets a typed request
- **WHEN** a typed request has been attempted
- **THEN** the existing generic retirement path cannot release its document or worker owners, even after logical cancellation or callback return

#### Scenario: Slot cancellation precedes delivery
- **WHEN** the resident slot is canceled before the queued typed callback enters
- **THEN** the shared cancellation gate refuses attachment and polling closes any already-attached recorder
