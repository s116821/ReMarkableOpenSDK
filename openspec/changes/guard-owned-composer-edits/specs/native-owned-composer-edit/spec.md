## ADDED Requirements

### Requirement: Genuine native composer range ownership
The SDK SHALL admit an owned composer interval only through the existing live document/page binding and two qualified native LF anchors. It SHALL preserve both anchors, require bounded live identity/position/type/order checks and refuse missing, deleted, recreated or unsupported-namespace identities. Persisted IDs and historical page records SHALL NOT themselves confer live authority.

#### Scenario: Previously provisioned range is reacquired
- **WHEN** the same admitted binding and namespace resolve both still-live LF anchors in the current joined root
- **THEN** the SDK may compare the exclusive interval under a fresh live revision without rebinding by text or offsets

#### Scenario: A separator was retyped
- **WHEN** text looks identical but a saved anchor no longer resolves to that exact live LF identity
- **THEN** the SDK refuses ownership without repairing or changing native text

### Requirement: Private plain-text conditional writer
The SDK SHALL require the already-installed exact default layout/provider, bounded whole-document plain shape and closed edit/IME/undo preconditions before comparing the live owner, range, revision and exact text. It SHALL make no synchronous consumer callout or event-pumping transition between that comparison and supported native apply/settlement. Refusal SHALL perform no native mutation, and uncertain outcomes SHALL preserve uncertainty without retry or compensating edit.

#### Scenario: Current owned range matches
- **WHEN** all private profile, owner and exact live comparison checks succeed
- **THEN** only the admitted native operation may affect the exclusive owned interval and copied settlement evidence must preserve all outside text and separator identities

#### Scenario: Layout or revision is unsuitable
- **WHEN** layout is absent/custom, unsupported content exists anywhere in the document, or the final owner/revision/text comparison fails
- **THEN** the SDK reports refusal before native mutation and preserves the draft

### Requirement: Historical key reads do not authorize clearing
A pre-key snapshot SHALL NOT authorize a later composer clear after stock input changes the document. The SDK SHALL require a genuine causally matched current observation and a new private conditional comparison, without revision arithmetic, replay or public fabricated leases.

#### Scenario: Forwarded Enter changes the draft
- **WHEN** stock Enter inserts a newline or applies formatting after the pre-key read
- **THEN** any later clear requires separately verified current owned state or leaves the draft intact
