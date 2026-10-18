# Design

Reuse release/stage_source.py without changing its contract or implementing a version engine. Preserve five CargoConsumerIdentity controls from research commit 72136edc94a95706fe1d1c7cb56a8bb6c5ffee4d and its GeneratedGitConsumer control. Isolate Cargo home, target output and Git configuration per owned fixture; pass only required compiler/tooling environment selectors.

The actual SDK case archives the exact current tracked commit into an owned local Git repository, creates a fixture-only stable tag, and consumes that repository using the existing feature-disabled ordinary consumer. Cargo.lock and cargo metadata must agree on Git URL, tag and SHA. The default feature graph remains empty; ordinary public identity/geometry calls execute. A temporary copied local SDK patch builds for development but fails official source admission; removing the patch restores the immutable pin. Existing generated Cargo default lock metadata is preserved. No production remote/tag or source package version is created.

Add the controls to the existing Linux source-staging check step after its selected host compiler is installed, with application admission unchanged. No extra job, compiler matrix or repetition of retained candidate compilation. The fixture's illustrative runtime identity/cache behavior is never attributed to the actual SDK.
