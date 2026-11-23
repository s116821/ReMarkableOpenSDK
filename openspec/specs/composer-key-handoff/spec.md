# composer-key-handoff Specification

## Purpose
Bind one real Qt key event to an exact selected Scene snapshot while preserving stock input, privacy and cancellation, and reclaim snapshot owners only through a private typed completion and borrow-retirement proof. This source capability does not provide product Submit, owned-range edit authority or native qualification.
## Requirements
### Requirement: One receiver-bound key-to-Scene read
The SDK SHALL bind at most one actual Qt key delivery to the existing typed Scene read, validating the exact selected focused receiver and existing worker within the synchronous enqueue guard. It SHALL preserve the event's key, modifiers, repeat, count, native fields and bounded UTF-16 text without assigning semantic Submit or physical-origin authority. Key facts SHALL remain private until a joined native request is actually attempted.

#### Scenario: Event reaches the native borrow
- **WHEN** an armed receiver receives a key press and the selected receiver/worker join succeeds
- **THEN** the read is enqueued once before stock forwarding, the copied key facts remain paired with that request, and the worker uses only the existing borrowed Scene callback

#### Scenario: Ordinary newline or repeated event
- **WHEN** the stock receiver receives ordinary, modified or repeated key input
- **THEN** the SDK does not consume, substitute, replay or submit that input and does not erase its repeat/count/modifier facts

### Requirement: Original lifetime and authority limits
The handoff SHALL retain its original absolute deadline and revoke on subsequent input or receiver/focus changes through an immediately sticky thread-safe latch. Input, notification, destruction and polling paths SHALL NOT wait for a worker borrow to settle. It SHALL NOT turn an event or historical read into a draft-clear condition, production activation or product shortcut.

#### Scenario: Changed or expired input context
- **WHEN** the receiver changes, further input arrives, the handle is destroyed or the original deadline expires
- **THEN** a retained callback cannot publish a usable read, and no retry or second native acquisition occurs

#### Scenario: Receiver destroyed during admission
- **WHEN** admission destroys the Qt event receiver
- **THEN** the pending read is canceled and Qt forwarding is suppressed solely to avoid dispatch to the destroyed object

#### Scenario: Unselected receiver supplies text
- **WHEN** a matching signal surface receives key text but the selected receiver join refuses
- **THEN** the public result contains no copied key text or facts

#### Scenario: Foreign notification while a read is held
- **WHEN** another thread emits a watched notification while the native read holds its borrow mutex
- **THEN** admission is revoked immediately without touching GUI state or waiting, and stock input and snapshot polling continue

### Requirement: Private typed snapshot owner retirement
Snapshot-only reads SHALL retain no persistent document observer. Their native owners MAY be relinquished only by the private exact-request completion path after a known native return, one matching full-ID completion from the pinned same worker, closed/drained borrowing and settled GUI frames on a later non-reentrant turn. The path SHALL use the existing opaque generation and atomic nonlast worker-before-document consumption. Generic Task retirement SHALL continue to refuse typed requests.

#### Scenario: Successful snapshot can release capacity
- **WHEN** all private typed-read proof obligations and both nonlast owner releases succeed
- **THEN** the slot may admit another read while late retained callbacks remain closed and cannot touch the old Scene

#### Scenario: Proof or release is incomplete
- **WHEN** submission/completion is unknown or mismatched, borrowing is not drained, or an owner release refuses
- **THEN** the request remains quarantined without ordinary release, retry, fabricated retirement proof or slot reuse

#### Scenario: Snapshot callback arrives on the wrong thread
- **WHEN** a retirement-bound snapshot callback runs on GUI or a thread other than the pinned worker
- **THEN** it refuses before any Scene access, records the thread refusal and preserves the quarantined native owners

