# Change: Observe composer document diagnostics

## Why
The native text owner embeds a QTextDocument, but the SDK has no bounded way to
record its actual revision notifications and emitting thread without intercepting
native edit functions. The delivered generic worker callback cannot be cast to
the separate scene callback ABI.

## What Changes
- Add an opt-in C++ diagnostic observer using QTextDocument's direct signals.
- Record bounded revision, signal arguments, thread and recorder-entry facts;
  implement cancellation, expiry, document destruction and connection teardown.
- Document the retained exact-build scene callback ABI and borrowed lifetime.
- Keep runtime attachment/selection, composer leases, text capture and mutation
  outside this diagnostic contract. No resident protocol/profile changes.

## Impact
Native research consumers can attach to an already live, caller-established
QTextDocument on its owning thread. Source/Qt fixture validation is distinct from
native device qualification. No Rust capability is enabled.
