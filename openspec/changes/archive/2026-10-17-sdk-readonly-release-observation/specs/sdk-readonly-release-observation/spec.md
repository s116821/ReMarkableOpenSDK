## ADDED Requirements

### Requirement: SDK main candidate observes an exact merged main PR
Application main candidate calculation SHALL require a successful read-only observation of a merged main PR whose merge source is the exact frozen commit, independently of current-message parsing, and SHALL refuse unavailable or mismatched observations.

#### Scenario: Exact merged source
- **WHEN** an associated merged main PR identifies the exact frozen source
- **THEN** observation returns before release/asset calls and candidate calculation may proceed subject to other gates.

#### Scenario: Missing or invalid observation
- **WHEN** no merged match exists, source/base/state differs, or the read fails
- **THEN** candidate observation refuses without release/tag writes or local retained-output creation.

### Requirement: Read-only release metadata preserves exact observation boundaries
SDK release observation helpers SHALL distinguish explicit absence, exact draft and complete published state, validate declared asset identity/completeness and refuse malformed or conflicting metadata without using provider write APIs.

#### Scenario: Retained release observation
- **WHEN** owned fixtures exercise absent, partial draft, malformed or incomplete published observations
- **THEN** only valid declared bytes are downloaded into a new owned directory and malformed/undeclared/duplicate/incomplete states refuse; payload hash and final publication authority remain separate.
