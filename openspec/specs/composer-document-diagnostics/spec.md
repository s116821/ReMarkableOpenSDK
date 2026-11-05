# composer-document-diagnostics Specification

## Purpose
Provide bounded Qt notification diagnostics without granting native composer authority.

## Requirements

### Requirement: Bounded document notification diagnostics
The SDK SHALL offer an opt-in diagnostic observer for an already live exact
QTextDocument supplied on its owning thread. It SHALL use direct Qt notifications,
record actual document revisions only on that thread, and use fixed storage with
at most 256 records and an absolute collection deadline of at most 120 seconds.
It SHALL NOT mutate the document, enqueue native work or issue draft authority.

#### Scenario: Unicode edit followed by Undo and Redo
- **WHEN** an observed Qt document changes and returns to equal text through Undo
- **THEN** the observer records the revisions supplied by Qt without replacing them with text hashes or notification counts
- **AND** the records remain diagnostics rather than an owned-draft lease

#### Scenario: Notification arrives on another thread
- **WHEN** a direct notification runs outside the installation thread
- **THEN** observation terminates with a thread mismatch without reading document revision on that thread

#### Scenario: Bound or deadline reached
- **WHEN** capacity, the original deadline or nonblocking recorder contention is reached
- **THEN** the recorder closes with an explicit reason and cannot silently wrap, extend its deadline or resume

### Requirement: Diagnostic lifetime and retirement
The observer SHALL retain only its own recorder state in connection functors.
Cancellation SHALL serialize terminal closure with record publication before disconnecting; document destruction
SHALL terminate collection without reading destroyed QTextDocument state.
Logical retirement SHALL NOT be advertised as native owner reclamation or safe
payload unloading.

#### Scenario: Handle retired during signal processing
- **WHEN** the observer is canceled or destroyed while Qt activation is underway
- **THEN** any entered functor retains valid recorder storage; an admitted record may publish before terminal closure, but stop returns only after closure establishes a frozen record prefix
- **AND** emitter callbacks never wait for the publication mutex, while caller-side retirement may wait for its short critical section

#### Scenario: Native document is destroyed
- **WHEN** QObject destruction notification runs
- **THEN** the recorder records destruction without document getters and Qt disconnects the sender

#### Scenario: Idle document reaches deadline
- **WHEN** no document signal arrives before the deadline
- **THEN** snapshot or caller polling reports expiry without renewing it, and explicit stop/destruction removes the remaining connections
