## ADDED Requirements

### Requirement: Explicit bounded diagnostic text range
The adapter SHALL copy only an explicitly requested half-open range in UTF-16
code units during an accepted native document borrow. It SHALL cap output at
4096 units, exclude the final document sentinel, reject negative/overflowing/
out-of-bounds ranges, and preserve an empty admitted range as successful empty
data rather than treating it as a refusal.

#### Scenario: Range exceeds its bound
- **WHEN** a range exceeds 4096 units or cannot fit within the admitted document
- **THEN** it is refused before copying and no partial text result is published

#### Scenario: Empty range at document end
- **WHEN** start equals characterCount minus one and length is zero
- **THEN** it may return an empty copy after the same thread, composition, structure and publication guards pass

### Requirement: Exact UTF-16 and bounded supported structure
The copy SHALL preserve raw code units and explicit length without normalization
or conversion. It SHALL reject split surrogate endpoints and preserve otherwise
present isolated surrogate units. It SHALL admit only supported root paragraphs
and refuse selected/boundary tables, child frames, lists, inline objects and
preedit. Structural inspection SHALL stop after at most 8192 visits, including
prefix traversal and both passes, without flattening the full document.

#### Scenario: Special text units
- **WHEN** an admissible range contains NUL, NBSP, paragraph separators, combining marks or surrogate pairs
- **THEN** the exact code units are returned, with U+2029 retained as the native paragraph separator

#### Scenario: Unsupported or over-budget structure
- **WHEN** range or endpoint structure is unsupported, preedit is present, or inspection exhausts its budget
- **THEN** an explicit refusal is returned with no partial accepted text

### Requirement: Opt-in matching Qt build
The text helper SHALL require matching private Qt headers and an explicit build
opt-in. Production copying SHALL require the accepted Qt6.10.3 profile; ordinary
builds SHALL remain compatible and refuse requested copying as Unsupported.

#### Scenario: Private Qt read support is absent
- **WHEN** the adapter was built without the private-Qt copy opt-in
- **THEN** a requested text copy reports Unsupported without exposing text

### Requirement: Borrowed owner-thread coherence
The adapter SHALL reuse exact profile/native join admission, actual document
owner-thread and type checks, native transaction/composition and Qt edit-depth
guards. It SHALL record native revisions before and after the read and recheck
document/root correspondence and quiescent state before publication. It SHALL
perform no text edit, event pump, queue wait, IPC or application/virtual layout
callback inside the read.

#### Scenario: Revision or native join changes
- **WHEN** the before/after revisions differ or revalidated join/composition state changes
- **THEN** the copy is refused and no text is published as accepted

#### Scenario: Affinity does not match
- **WHEN** the callback is not on the actual document owner thread
- **THEN** no text or revision is read

### Requirement: Existing cancellation and lifetime settlement
The copy SHALL use the existing Attachment claim, cancellation, original
deadline and publication mutex. It SHALL publish at most once, retain no borrowed
native/document pointers, discard data refused before publication admission, and settle publication
before stop returns. Cancellation requests SHALL prevent later admission; an
already-admitted result may publish before terminal closure. Previously published copies SHALL remain
frozen historical diagnostics rather than revocable live authority.

#### Scenario: Cancellation precedes publication
- **WHEN** cancellation or expiry is observed before final publication admission
- **THEN** the local copied text is discarded and stop settlement prevents later publication

#### Scenario: Callback survives its handle
- **WHEN** a retained callback runs after stop or handle destruction
- **THEN** it refuses before inspecting the Scene argument or performing another text read

### Requirement: Diagnostic authority boundary
The result SHALL NOT issue owned-range, page/visit, Submit, edit/currentness,
clear, focus or owner-retirement authority. Equal finite revisions and copied
text SHALL NOT substitute for native writer serialization or a genuine consumer
lease. The diagnostic SHALL neither enable a consumer revision guard nor release
or reuse a typed resident owner slot.

#### Scenario: Downstream consumer requires a genuine lease
- **WHEN** only this diagnostic copy and revision pair are available
- **THEN** a consumer requiring owned draft/currentness authority continues refusing
