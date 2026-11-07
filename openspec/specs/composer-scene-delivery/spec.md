# Composer scene delivery

## Purpose
Deliver bounded diagnostics through the exact typed scene callback while preserving resident ownership and cancellation limits.
## Requirements
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

### Requirement: Historical composer scalar observation
The adapter SHALL offer an explicit immutable scalar-only mode of the existing
typed scene diagnostic. It SHALL require the admitted exact provider/layout,
preserve the default observation and optional text path, and refuse any
combination of scalar mode with a text range.

#### Scenario: Matching identities and scalar values
- **WHEN** two borrow-local joins agree on the exact root, control, participant, document and private identity, focused ID and full scalar tuple under the original deadline
- **THEN** the callback publishes one historical observed sample with undo/redo enablement, current undo/redo availability, maximum block count, revision, blocked signals and the distinct literal undo/redo comparison caches
- **AND** the callback returns false without an observer, text copy, receiver traversal, another enqueue or damage flush

#### Scenario: Scalar sample cannot be established
- **WHEN** provider/layout, document affinity, focus, native scope, IME, Qt edit depth, identity, scalar state, cache encoding, cancellation or the original deadline prevents completion
- **THEN** the result does not contain an observed sample and its scalar status remains distinct from snapshot availability, observer attachment and text status

#### Scenario: Historical sample survives logical closure
- **WHEN** an observed scalar diagnostic is subsequently closed
- **THEN** its historical values remain readable without granting native currentness, a clear operation or native owner retirement
