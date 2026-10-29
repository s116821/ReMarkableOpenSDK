# Simplify SDK CI/CD layout and upstream tooling

## Why

Root release/tool directories and custom rebuilding, transferring and testing of
third-party Actions add avoidable maintenance. The upstream distributed uploader
already provides the required explicit release-ID and no-overwrite inputs.

## Changes

- Move `release/` to `cicd/release/` and `tools/` to `scripts/`; update all current
  source, workflow, package and contributor references.
- Consume the already selected immutable upstream uploader directly. Remove its
  custom build recipe, vendor locks, retained-tool artifact and transfer tests.
- Remove loopback tests of upstream title/request/GraphQL/uploader implementations;
  keep meaningful checks of our configured paths, workflow gates, semantic policy,
  package boundaries and exact release state.
- Preserve the SDK's source staging, version injection, finite distribution
  inventory, retained artifact integration and exact-ID/byte recovery guarantees.

The corresponding Buddy/Manager maintenance is coordinated with the central Docs
change of the same name. SDK specifications and this complete lifecycle remain
here. Native source and frozen experiments are outside this cleanup.
