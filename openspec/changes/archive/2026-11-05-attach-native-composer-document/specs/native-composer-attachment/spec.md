## ADDED Requirements

### Requirement: Exact borrowed scene attachment
The diagnostic adapter SHALL accept a Scene only during the real exact-build scene callback, after loaded-provider qualification. It SHALL validate the focused root type and native joins and attach on the actual QTextDocument owner thread. It SHALL NOT retain native Scene/item ownership or dispatch native work.

#### Scenario: Focus, type or native transaction mismatch
- **WHEN** the native joins, exact text-item type, owner control block or quiescent transaction fields do not match
- **THEN** no Qt recorder is attached and the callback returns false

#### Scenario: Document has different Qt affinity
- **WHEN** the embedded document is not owned by the current callback thread
- **THEN** attachment refuses without connecting or reading its revision

### Requirement: One-use attachment retirement
The adapter SHALL retain a finite recorder under its original absolute deadline, consume at most one callback and close collection on cancellation, expiry or duplicate invocation. Logical retirement SHALL NOT release native owners or permit payload unloading.

#### Scenario: Callback arrives after cancellation
- **WHEN** a retained callback copy runs after its handle was canceled or destroyed
- **THEN** it performs no native join or attachment and returns false

#### Scenario: Cancellation overlaps attachment
- **WHEN** cancellation occurs while the attachment callback is active
- **THEN** stop waits for admitted attachment and recorder publication to settle, any created recorder is closed before stop returns, and callback-owned storage remains valid

#### Scenario: Provider identity is unsupported
- **WHEN** architecture, Qt build or loaded provider correspondence does not match
- **THEN** no native attachment callback is prepared

#### Scenario: Dispatch consumes preparation time
- **WHEN** native delivery occurs after the attachment was prepared
- **THEN** the recorder keeps the original absolute deadline and does not refresh its collection budget

#### Scenario: Retirement is pending
- **WHEN** a cancellation request overlaps publication
- **THEN** only a settled retirement may report retired with a frozen observation prefix; an unavailable snapshot proves no settlement
