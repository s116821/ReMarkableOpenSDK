# Change: Qualify maintained SDK semantic metadata tooling

## Why
The SDK now has independent source/package verification. Maintained semantic version and title tooling must be qualified before candidate builds and a separately authorized final release job can consume it.

## What Changes
Extract the selected hash-locked Python Semantic Release tool/config and bounded existing Git fixtures. Add actual read-only candidate vectors and reuse the maintained title Action with conventional optional scopes, refusing docs titles for application changes. Keep all production tag/publisher behavior absent.

## Impact
No SDK runtime/Cargo/package/consumer/source-version changes. Existing source/release checks remain independent. Current-source metadata activation depends on normalized executable inventory PR17 reaching main; first-release policy, actual-squash eligibility, protected remote authority and publication remain separate.
