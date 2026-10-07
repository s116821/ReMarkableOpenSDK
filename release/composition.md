# Proposed maintained release composition

REM-50 / SCRAPPY-DOO. Reviewable candidate, not an enabled production workflow.
Own versions exist only in Git tags. Python Semantic Release 10.7.0 calculates
and mints the tag with `python-semantic-release.toml`; checksum-pinned GitHub CLI
2.102.0 manages an explicit existing-tag draft. No private version calculator,
release state machine, source version commit or patched publisher fork is added.

## Ordered release configuration

1. A main squash merge supplies the immutable intended source SHA. Run the
   already-qualified upstream dorny path filter and semantic PR-title enforcement
   before merge. A docs-only event skips **all** tag/build/publication steps,
   including bootstrap. Unknown paths and source/build Markdown count as
   application changes. Do not infer docs-only from an unvalidated subject.
2. Check out that exact SHA with complete reachable tag history. Use upstream
   semantic-release with the candidate TOML and documented
   `version --no-commit --no-changelog --skip-build --push --no-vcs-release`.
   Partial tags remain disabled. No checked-in initial/source version is added.
   A docs-only raw CLI bootstrap is known to emit v0.0.0, so step 1 is mandatory.
   The first official SDK tag and pre-1.0 breaking policy require owner agreement.
3. Treat the upstream CLI's result as a request to observe the exact remote tag,
   not build authority. `verify_remote_tag` verifies advertised object/peeled
   commit, fetches that object without replacing local tags, and re-observes it.
   Require the intended SHA. A failed push's retained local tag or a replay that
   says no new release cannot bypass this check. Use repository tag protection
   and immutable releases separately to prevent later replacement.
4. Stage the clean exact tagged source with `stage_source.stage`. Inject the tag
   version only in the new build directory, compile/package accepted source,
   verify packaged source/dependencies, and generate the independently expected
   distribution manifest. Include target/firmware compatibility from qualified
   domain evidence. Current synthetic fixtures explicitly mark every tablet
   unqualified and native operations unsupported; they cannot ship as production
   SDK compatibility evidence. The current experimental SDK's source version,
   source/license/API and real packaging/consumer acceptance remain owner gates.
5. For an absent release, run maintained `gh release create "$TAG" --verify-tag
   --draft --notes-file "$NOTES" --repo "$REPOSITORY"` **without assets**.
   The default auto-publish path deletes a failed draft and is not used. For
   recovery, observe the existing exact-tag draft through the maintained CLI
   instead of recreating it. An existing published release is an integrity
   verification/replay path, not permission to upload or replace bytes.
6. Use `gh release download` into a new owned directory. Compare any retained
   subset with the independently verified build manifest using
   `verify_downloaded_assets(..., require_complete=False)`. Refuse unexpected,
   corrupt or conflicting assets; never delete/clobber them to make retry pass.
   Upload only missing exact names using `gh release upload`, without --clobber.
   A competing upload refuses rather than overwriting. Recovery uses the existing
   immutable tag and verified build/provenance; it never reruns the version step
   or trusts a downloaded manifest as the source of expected identity.
7. Download and verify the complete declared asset set and manifest. Re-observe
   exact remote tag/SHA before `gh release edit "$TAG" --draft=false`. GitHub
   immutable-release enforcement must be configured and qualified by its owner;
   CLI/API success alone does not prove immutability. The local verifier does not
   prevent concurrent mutation between verification and finalization. Serialize
   same-repository publishers with upstream Actions concurrency and inspect
   permission/tag-protection controls before activation; concurrency does not
   constrain external actors. No repository settings are changed by this PR.
8. Published integrity/provenance verification through maintained `gh release
   verify` / `verify-asset` is a separate qualification boundary. Its attestation
   dependencies and actual immutable release behavior must be assessed before
   those commands are selected. Promotion of Buddy prerelease flags without
   rebuilding belongs to REM-46; this SDK lane does not modify Buddy/Manager.

These are candidate steps to implement with maintained Actions/CLI primitives
after the listed gates, not a parallel coordinator implementation. No token or
write permission is provided to the checked-in qualification workflows.

## What the executable composition proves

`composition_fixtures.py` executes the actual upstream version CLI against an
owned bare Git remote, checks its intended SHA despite main advancement, verifies
the remote tag, stages version-free Rust source, compiles an executable reporting
the injected tag version and verifies a real Cargo source crate. Actual GitHub
CLI commands against loopback TLS then recover an interrupted explicit draft,
download and validate the complete assets, and finalize without retagging or
deleting retained bytes. A local-only tag success fails before staging or any
publisher call. The modeled API's exclusivity and immutability do not prove live
GitHub behavior, protected tags, reproducible production binaries or native
compatibility. No production tag/release is created.

## Exact outstanding decisions

- Main reviews the new PSR/config/identity and publisher/composition delta rather
  than transferring the old NON-production b5725ed acceptance.
- The publisher security owner decides whether the restricted command source
  analysis is an acceptable policy boundary for GO-2026-5932. The full binary
  advisory gate remains failing until a supported fix or explicit reviewed gate
  policy is committed; functional success does not silently waive it. See the
  decision-ready assessment in [upstream-audit.md](upstream-audit.md).
- SDK source owners provide version-free accepted shipping source, license/API,
  dependency/consumer contract, target packaging and compatibility evidence.
  Main/REM-35 owns initial/public release policy and final delivery acceptance.
- Repository owner qualifies protected tag/immutable release enforcement and
  actual recovery/concurrency/finalization before production activation.

Until these inputs exist, a production-enabled release workflow would either
publish an unaccepted source/compatibility claim or bypass an unresolved gate.
