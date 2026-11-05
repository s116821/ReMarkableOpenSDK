# Ordinary gesture planner

## Purpose
Provide reusable explicit gesture geometry independently of input dispatch and native completion.

## Requirements

### Requirement: Explicit ordinary gesture geometry
The SDK SHALL provide immutable geometry and timing for explicit RM2 portrait next/previous actions, preserving the existing Reader trajectory.

#### Scenario: Mapped direction
- **WHEN** RM2 portrait and next or previous are supplied
- **THEN** the plan preserves the corresponding endpoints, fifteen steps and timings

### Requirement: Unsupported pairs refuse
The planner SHALL refuse unknown or unimplemented model/orientation pairs and SHALL grant no dispatch or native completion authority.

#### Scenario: Unsupported pair
- **WHEN** another model or orientation is supplied
- **THEN** a typed unsupported result is returned
