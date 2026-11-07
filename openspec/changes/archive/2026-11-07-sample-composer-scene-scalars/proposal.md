# Sample historical composer scalars

## Why

The exact Scene callback can borrow the focused document long enough to compare
its public undo/redo predicates with the private comparison caches. This gives
the source investigation a bounded historical sample of both kinds of state.

## What Changes

- Add an explicit scalar-only mode to the existing typed Scene diagnostic.
- Report a distinct sample status and publish only after a full identity and
  scalar recheck within the original borrow and deadline.
- Extend the owned Attachment and Delivery fixtures for the new mode.

## Capabilities

- Modify `composer-scene-delivery` with a historical scalar diagnostic under
  the existing exact provider and typed owner admission.

## Impact

The default observation and exact text-copy modes retain their existing
behavior. The sample carries no pointer, currentness lease, clear authority or
resident wire representation. Live selection and execution remain separate.
