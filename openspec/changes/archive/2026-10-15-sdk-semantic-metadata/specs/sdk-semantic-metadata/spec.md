## ADDED Requirements

### Requirement: Maintained semantic calculations preserve version authority
Read-only candidate calculations SHALL use the maintained pinned upstream tool and Git history/tags without changing source versions, refs or generated release files; qualification SHALL distinguish fixture behavior from official SDK release authority.

#### Scenario: Conventional candidate
- **WHEN** supported feature, fix or breaking fixture history is inspected in read-only mode
- **THEN** the upstream expected candidate is returned and source/refs remain unchanged.

#### Scenario: Documentation and unsafe raw success
- **WHEN** documentation or observed raw-tool edge behavior is qualified
- **THEN** docs do not supply a release bump and raw tool success is not presented as SDK publication or verified remote authority.

### Requirement: Maintained title and path admission precede tool jobs
CI SHALL use maintained current-title validation and path classification, accepting conventional optional scopes, rejecting documentation titles for application changes, and skipping qualification jobs for documentation-only changes while retaining a result.

#### Scenario: Application title
- **WHEN** application paths change
- **THEN** the fresh PR title must use an allowed application type and docs titles refuse.

#### Scenario: Documentation-only title
- **WHEN** only named guidance/documentation paths change
- **THEN** docs titles are allowed and semantic qualification is skipped without creating SDK tags or releases.
