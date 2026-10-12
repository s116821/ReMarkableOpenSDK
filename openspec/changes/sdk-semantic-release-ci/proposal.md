# SDK semantic releases and target CI

## Why
REM-50 fills the SDK-specific CI/release gap independently of BYORBSAX's REM-46 work. Semantic PR titles, immutable Git tags and post-tag builds must connect SDK distribution to Buddy consumption and Manager compatibility.

## What Changes
Qualify maintained upstream semantic-release tooling in isolated local Git repositories before selecting the production composition. Define SDK host/ARMv7/AArch64 CI and artifact/provenance manifest. Publish only actual implemented distributions, with unknown device/runtime capabilities unqualified. Release-worthy main squash merges release automatically; docs: has no version/tag/release/main application build. No bespoke semantic calculator, classifier or release coordinator.

## Capabilities
### New Capabilities
- `sdk-releases`: semantic tag authority, source/artifact identity and distribution contract.
### Modified Capabilities
None at bootstrap. Buddy/Manager requirements remain in Docs with exact interface revisions.

## Impact
SDK release/workflow files only. REM-46 owns Buddy/Manager pipelines; REM-41 owns stable discovery/install. Pin SDK Git tags in Cargo and lock exact commits; use local path/[patch] overrides only in development. No production tag/release/settings changes during qualification. First production SDK build/release depends on actual crate sources and settled license/API/dependency/distribution policy.
