## ADDED Requirements

### Requirement: One receiver-bound key-to-Scene read
The SDK SHALL bind at most one actual Qt key delivery to the existing typed Scene read, validating the exact selected focused receiver and existing worker within the synchronous enqueue guard. It SHALL preserve the event's key, modifiers, repeat, count, native fields and bounded UTF-16 text without assigning semantic Submit or physical-origin authority.

#### Scenario: Event reaches the native borrow
- **WHEN** an armed receiver receives a key press and the selected receiver/worker join succeeds
- **THEN** the read is enqueued once before stock forwarding, the copied key facts remain paired with that request, and the worker uses only the existing borrowed Scene callback

#### Scenario: Ordinary newline or repeated event
- **WHEN** the stock receiver receives ordinary, modified or repeated key input
- **THEN** the SDK does not consume, substitute, replay or submit that input and does not erase its repeat/count/modifier facts

### Requirement: Original lifetime and authority limits
The handoff SHALL retain its original absolute deadline, cancel on subsequent input or receiver/focus changes, and preserve existing one-use native owner retention. It SHALL NOT turn an event or historical read into a draft-clear condition, reusable native owner, production activation or product shortcut.

#### Scenario: Changed or expired input context
- **WHEN** the receiver changes, further input arrives, the handle is destroyed or the original deadline expires
- **THEN** a retained callback cannot publish a usable read, and no retry or second native acquisition occurs

#### Scenario: Receiver destroyed during admission
- **WHEN** admission destroys the Qt event receiver
- **THEN** the pending read is canceled and Qt forwarding is suppressed solely to avoid dispatch to the destroyed object
