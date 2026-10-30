# Release maintenance

The workflow owns job setup, ordering, artifact transfer and publication steps.
It uses maintained semantic-release, path/title, artifact and release Actions.
Actions are consumed at reviewed immutable revisions, including the upstream
distributed uploader; we do not rebuild their dependency graphs or re-test their
implementations in loopback servers.

The retained helpers implement SDK-specific work:

- `stage_source.py` creates a versioned distribution from the versionless source
  manifest while preserving package admission and ordinary Git consumers.
- `candidate_artifacts.py` builds the finite target inventory and external consumer
  probes, then verifies and exercises those exact retained bytes in Integration.
- `artifacts.py`, `verify_assets.py`, `remote_identity.py` and `release-assets.cjs`
  bind source, manifest, remote tag, original release ID and uploaded bytes so a
  retry cannot overwrite or finalize conflicting assets.
- `check_commit.py` applies our current-commit release policy through the maintained
  semantic parser; the adjacent configuration keeps documentation release-free.

Tests here cover those project contracts. `scripts/ci-fixtures/` checks our workflow
gates and `scripts/ci_paths.test.mjs` checks our actual path configuration. Historical
OpenSpec archives describe earlier tool recipes at their original revisions;
their custom bundle audits do not qualify the current upstream distribution.

Generated candidate directories named `release/` and Cargo's release build profile
are unchanged artifact formats. They are distinct from this repository directory.
