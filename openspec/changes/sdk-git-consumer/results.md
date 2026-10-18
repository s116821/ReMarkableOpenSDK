# Verification

Precode: 3be0d06e8ae0538529a721067b48b5f75dd25a62, based on actual main 1bfd4d44e2eaf6e350cd97ca74d2d3bd40323668.

- Local selected Cargo 1.98.1 controls: six owned source/tag/lock/path tests pass (3.773s); actual-current-SDK transport case passes (2.250s). Only temporary file:// repositories/tags and an isolated Cargo home/target directory are used. No registry dependencies are selected by these consumers.
- Research provenance: all five CargoConsumerIdentity test bodies and its git/run_cargo/metadata/verify methods are AST-exact to selected 72136edc94a95706fe1d1c7cb56a8bb6c5ffee4d. GeneratedGitConsumer test body is AST-exact to that source's test_generated_lock.py. Adaptations are fixture environment isolation and same-module method binding.
- Actual SDK case archives the tracked checkout, preserves the committed default root lock, verifies exact Cargo URL/tag/locked SHA and empty SDK features, runs the ordinary public consumer, proves branch advancement does not change the pin, builds a local path override while official admission refuses it, then restores and runs the Git source again.
- Fixture build.rs verifies generated environment/cache semantics only. The actual SDK has no runtime-version hook; returning SDK_BUILD_* values does not prove runtime embedding. Cargo's implicit 0.0.0 is generated metadata, not a maintained release version.
- Ten existing compact workflow graph controls, actionlint, strict OpenSpec validation (18 passed/0 failed), diff and exact selected-test provenance checks pass.
- Existing stage_source.py, Cargo manifests/locks, SDK runtime/API, candidate build/integration and Windows stage are unchanged. Seven consumer controls run once in the existing Linux staging check; no extra job or target matrix.

Independent source review, hosted current-head checks, canonical sync/archive and normal merge remain pending. No official SDK tag/release, native support or Buddy consumer pin change is claimed.
