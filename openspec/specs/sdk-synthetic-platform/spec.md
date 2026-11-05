# Synthetic platform contract

## Purpose
Provide scoped in-memory platform modeling and explicit unsupported native defaults.

## Requirements

### Requirement: Scoped synthetic platform
The SDK SHALL provide opt-in in-memory observation, creation/reconciliation and adjacent navigation/handoff modeling with scoped guards, explicit synthetic origin and distinct no-effect rejection versus indeterminate outcomes.

#### Scenario: Stale or foreign guard
- **WHEN** source scope or execution state differs
- **THEN** the model refuses before mutation or reports postdispatch uncertainty as appropriate

#### Scenario: Lost creation reply
- **WHEN** model creation commits and reply is lost
- **THEN** reconciliation identifies the original modeled result without duplicate creation

### Requirement: Native defaults remain unsupported
The default platform SHALL report unavailable native observation and operations; capability reports SHALL grant no operation authority.

#### Scenario: Unqualified platform
- **WHEN** an ordinary caller queries or observes without a qualified adapter
- **THEN** native capability and observation remain unsupported
