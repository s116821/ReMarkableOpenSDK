# Verification

Precode: 3be0d06e8ae0538529a721067b48b5f75dd25a62, based on actual main 1bfd4d44e2eaf6e350cd97ca74d2d3bd40323668.

- Local selected Cargo 1.98.1 controls: six owned source/tag/lock/path tests pass (3.773s); actual-current-SDK transport case passes (2.250s). Only temporary file:// repositories/tags and an isolated Cargo home/target directory are used. No registry dependencies are selected by these consumers.
- Research provenance: all five CargoConsumerIdentity test bodies and its git/run_cargo/metadata/verify methods are AST-exact to selected 72136edc94a95706fe1d1c7cb56a8bb6c5ffee4d. GeneratedGitConsumer test body is AST-exact to that source's test_generated_lock.py. Adaptations are fixture environment isolation and same-module method binding.
- Actual SDK case archives the tracked checkout, preserves the committed default root lock, verifies exact Cargo URL/tag/locked SHA and empty SDK features, runs the ordinary public consumer, proves branch advancement does not change the pin, builds a local path override while official admission refuses it, then restores and runs the Git source again.
- Fixture build.rs verifies generated environment/cache semantics only. The actual SDK has no runtime-version hook; returning SDK_BUILD_* values does not prove runtime embedding. Cargo's implicit 0.0.0 is generated metadata, not a maintained release version.
- Ten existing compact workflow graph controls, actionlint, strict OpenSpec validation (18 passed/0 failed), diff and exact selected-test provenance checks pass.
- Existing stage_source.py, Cargo manifests/locks, SDK runtime/API, candidate build/integration and Windows stage are unchanged. Seven consumer controls run once in the existing Linux staging check; no extra job or target matrix.

## Hosted source and independent review

- Source e3001e44f88230d6b4f53780cbc598ca6561ef2e has all five required PR jobs successful in run 39104977349. Direct Linux log 117608769923 records the six selected controls passing in 2.009s and actual-current-SDK control in 1.405s; strict 18/0. Windows staging job 117609154563 succeeds.
- Build 117609222808 and Integration 117609511883 identify temporary PR merge 8c0124f28704d962c8bd0565dac5e8c06aa66379, candidate v0.1.0, retained artifact ID 12127219632 and matching uploaded/downloaded outer SHA256 10fad3b4d4ceff88e90d643b512eab36e5b748878d83f57a13801b622d338c6c. The retained consumer executes and all four tamper controls pass (0.104s).
- Receipt SHA256 4d0ff8e0812d960c01a7340cdd26c7bc2bc443da87a5b14ed4922392fd171550; manifest SHA256 6f952461405dafccb7b508857c2094d07ca4d027ac7145116da6865699074f7b.
- Full independent source/spec review 5561801667 was directly read at exact e3001e44. It reports no blocking findings and accepts own canonical sync/archive while retaining final lifecycle/current-head checks and normal squash as separate gates.

## Lifecycle handoff

Only this five-file change is archived. Proposal, design and delta remain byte-exact to accepted source. Canonical specification contains the same two requirements and three scenarios. All other source entries, including implementation, workflow and unrelated active navigation change, remain exact. Strict validation and preservation checks are run for this handoff; no unchanged broad local matrix is repeated.

Final lifecycle review/current-head hosted checks, normal merge and actual-main proof remain pending at author handoff. No official SDK tag/release, native support or Buddy consumer pin change is claimed.
