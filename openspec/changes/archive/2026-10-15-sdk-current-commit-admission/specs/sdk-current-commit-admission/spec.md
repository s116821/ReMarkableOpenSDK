## ADDED Requirements

### Requirement: Current SDK main message must supply its own eligibility
Application main-push candidate calculations SHALL admit the exact current source message through the maintained configured parser rather than borrowing eligibility from older untagged history, accepting Conventional Commit optional scopes and refusing documentation, unsupported or malformed headers.

#### Scenario: Older failed history
- **WHEN** older untagged application history would yield a candidate but the current message is documentation, copied breaking documentation or nonconventional
- **THEN** current-message admission refuses without changing source or refs.

#### Scenario: Supported optional scopes
- **WHEN** the current actual source has a supported application header with an absent or valid scope
- **THEN** the configured parser admits it; an explicitly empty or whitespace scope refuses.

### Requirement: Candidate admission observes exact source and bounded main push
The candidate workflow SHALL require exact current checkout identity and one observed first-parent main-push step before calculating application candidates, while preserving documentation-only skips and separating this admission from final official release authority.

#### Scenario: Wrong source or ambiguous push
- **WHEN** source identity differs, the before SHA is unavailable, or a push includes multiple first-parent steps
- **THEN** candidate calculation refuses and does not create an SDK tag or release.
