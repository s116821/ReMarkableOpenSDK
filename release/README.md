# Upstream release qualification

REM-50, machine owner SCRAPPY-DOO. This harness executes actual pinned upstream
semantic-release/commit-analyzer tools against temporary owned repositories.
The runner refuses repositories outside its marked temporary fixture root and has no external publisher. It cannot publish production releases. Real SDK target builds and publication remain
unimplemented and gated by the active OpenSpec tasks. Synthetic packaging and
consumer fixtures are separate from actual SDK/native qualification.

Use Node 24 (at least 24.10), then `npm ci --ignore-scripts` in this directory. For the complete test run,
set `PATHS_FILTER_ACTION` to the `dist/index.js` from dorny/paths-filter exact
commit `ceb8a2b8f2d89434be7ff52d3de7ec3738c5cc9d`, then run `npm test`.
Also set `SEMANTIC_PR_ACTION` to `dist/index.js` from
amannn/action-semantic-pull-request exact commit
`48f256284bd46cdaab1048c3721360e808335d50`. Also set `GH_RELEASE_ACTION` to `dist/index.js` from softprops/action-gh-release
exact commit `efb35369e0ad2afab669f228072c1b0d510eae64`. Hosted CI checks out these
upstream revisions automatically. No own project version is maintained in either manifest. Upstream
dependency pins retain their upstream versions. Actual release artifacts must
obtain their version only from an immutable Git tag during a build.

The fixtures cover feat/fix/perf/breaking/build/ci/chore/docs policy, equal commit
timestamps, docs-only histories, exact release SHA, dry-run repeatability, actual tag/prepare/publish ordering, failed-publication retries, and stale-main refusal.
Synthetic fixture tags are owned test input; lifecycle checks actually mint tags only in marked local bare repositories. The child runner isolates upstream
stdout interception from Node's test reporter. No production credentials are
passed to the child, and fixture repositories are deleted after each test.

The CI audit is a gate, not an informational success. At initial qualification,
upstream dependency advisories include braces <=3.0.3 and http-cache-semantics
<=4.2.0; those were also the latest registry versions. Do not downgrade to old
major releases or waive advisories simply to turn CI green. Tests passing does
not qualify production publication. Resolve/assess upstream advisories, tag-before-
build and failed-publication recovery before selecting the production tool.

Full task/design scope lives in `openspec/changes/sdk-semantic-release-ci`.

Observed lifecycle results: prepare runs before tag creation, while publish sees
the exact release tag/SHA. An injected publish failure leaves the tag in place;
a plain rerun returns no release and does not retry publish. Therefore a normal
semantic-release rerun alone does not satisfy recovery requirements. A maintained
upstream existing-tag publication path must be qualified before activation.
A checkout behind remote main is refused before tagging; this does not yet prove
safety for a race occurring after the freshness check.

PR paths/title checks compose pinned dorny/paths-filter v4 and
amannn/action-semantic-pull-request v6. Unknown paths, dependency files, workflows,
hidden build configuration and Markdown under source/build directories remain
application-relevant. Documentation-to-source renames and mixed changes cannot
qualify as docs-only. The upstream title Action excludes docs from allowed types
when the path Action reports application changes. No private classifier is used.
The checked-in workflow is read-only; maintainers must make its status required
before it can guarantee enforcement for merging. No repository settings changed.

Title fixtures execute the actual upstream bundle against a local read-only API
server with a non-credential fixture token. They cover accepted/rejected docs,
breaking titles, missing scope and invalid headers, and verify that the Action
fetches the current title rather than trusting a stale event. No GitHub API write
or production token is used.


## Packaging and consumer qualification

Run `python -m unittest discover -s release -p 'test_*.py' -v` from the repository
root with these environment inputs (the hosted packaging job installs them):

- `GIT_CLIFF_BINARY`: verified upstream git-cliff 2.14.2 executable.
- `CREATE_TAG_ENTRYPOINT`: rickstaa/action-create-tag entrypoint at
  `a1c7777fcb2fee4f19b0f283ba888afa11678b72`.
- `SDK_FIXTURE_CARGO`: Rust 1.98.1 Cargo with host, ARMv7 GNU and AArch64 GNU
  standard libraries installed.

The 24 Python tests cover upstream alternative calculation, non-forced tag
races, exact-tag build staging, actual synthetic compiler/source-crate builds,
packaged source/manifest/lock verification, Cargo Git tag/commit pinning and
reversible local patches. The 37 JavaScript tests include the actual upstream
publisher's draft recovery against a localhost API fixture. Temporary fixture
repositories and tags are isolated from production.

`stage_source.py` only exports existing tagged source, injects its tag version
in a new build directory, and verifies package/dependency/artifact identity. It
cannot calculate a release version, mint a tag or publish a release. Its current
scope is a single Rust package with an explicit dependency lock inventory; full
SDK/workspace integration requires separate qualification. Runtime compatibility
in the generated distribution manifest remains unqualified for every tablet.

The publisher retries existing-tag drafts after partial upload failure, but its
asset-name reuse does not prove byte identity. A production composition must
verify existing asset hashes before finalization. The tag-entrypoint concurrency
fixtures do not execute or audit its Docker image. No production workflow is
enabled. See [candidate audit and evidence](upstream-audit.md) for findings and
limits; functional success never silently clears a security gate.
