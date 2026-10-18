# SDK trusted uploader transfer

## Purpose
Preserve the independently qualified uploader identity across pipeline jobs without rebuilding or granting publication authority.

## Requirements

### Requirement: Qualified uploader transfer binds independent provenance
The existing Checks stage SHALL retain the qualified uploader and expose its immutable artifact ID and independently computed provenance SHA. Existing Integration SHALL download that exact artifact and require provenance bytes to match the trusted Checks digest before accepting the full file inventory, recipe, audit and selected bundle identity.

#### Scenario: Qualified tool reaches Integration
- **WHEN** the application-admitted predecessor Checks and Build stages succeed
- **THEN** Integration verifies the exact retained tool with trusted provenance and complete file identity without rebuilding it, repeating loopback behavior or receiving publication permission.

### Requirement: Changed transferred tool refuses before execution
Transfer qualification SHALL exercise valid and modified copies of actual retained uploader output and SHALL refuse changed provenance or payload before any modified tool execution.

#### Scenario: Joint metadata and inventory mutation
- **WHEN** harmless action metadata and its copied provenance hash are changed together while the selected JS bundle stays unchanged
- **THEN** the independent expected provenance digest refuses the modified record before its inventory can supply authority.

#### Scenario: Payload mutation
- **WHEN** a retained uploader payload changes while the trusted record remains unchanged
- **THEN** complete file verification refuses it and no changed payload executes.
