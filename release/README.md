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

The 34 Python tests cover upstream alternative calculation, non-forced tag
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


## Actual experimental SDK compiler checks

PR application changes also compile the owner research snapshot
`f6b7dc8954ba9f5a284d56568765e05c38483924` with Rust 1.98.1. The host lane runs
fmt, all-feature tests and clippy with warnings denied; the three lanes compile
the actual Rust library for host/ARMv7 GNU/AArch64 GNU using its unchanged lock.
This is an exact-source compiler check, not synthetic fixture evidence. It does
not build all experimental Qt helper programs, qualify a model/native adapter,
publish artifacts or integrate source into main. A separate host lane tests
the declared minimum Rust 1.88.0; cross-target checks use Rust 1.98.1. The prototype's existing own version placeholder must be removed by its
owner before strict tag-derived official staging can accept that source.
Research snapshot builds do not run on main merges, including docs-only merges.


The Git consumer fixture also verifies a concrete version gap: omitting the SDK
source package version does not make Cargo infer it from the Git tag. Its
implicit package metadata remains a default, so runtime/artifact identity must
be supplied at build time after verifying pinned source. `sdk_build_environment`
checks root single-package cached source HEAD/cleanliness, own-version absence
and Cargo-reported Git identity, then returns exact tag-derived build variables.
Only Cargo's observed empty cache-completion marker is tolerated; developer
configuration/source drift is refused. An ordinary Cargo build script embeds
the fixture's version and uses rerun-if-env-changed; actual execution proves
both official identity and cache invalidation back to development. This is
build identity preparation, not a implemented SDK/Buddy runtime API.
[Cargo build-script guidance](https://doc.rust-lang.org/cargo/reference/build-scripts.html).

## Python maintained-tool alternative qualification

The separate read-only `python-release-qualification.yml` tests Python Semantic
Release 10.7.0 without replacing or waiving the historical npm candidate's audit.
Install in a new Python 3.12 environment:

```sh
python -m venv .python-tools
.python-tools/bin/python -m pip install --require-hashes --only-binary=:all: -r release/python-tools.lock
.python-tools/bin/pip-audit
PYTHON_SEMANTIC_RELEASE="$PWD/.python-tools/bin/semantic-release" python -m unittest discover -s release -p 'python_semantic_release_fixtures.py' -v
```

The lock was generated with upstream pip-tools 7.6.2 using the command in its
header; versions/hashes identify consumed upstream dependencies, not this SDK.
The ten actual-CLI fixtures run against owned bare repos and loopback HTTP only.
No real credential is passed. HTTP/insecure mode is enabled only in the temporary
publisher fixture config, never the candidate configuration.

Extended evidence: successive equal-time tags and stable breaking major; source
HEAD unchanged; hostile copied body text never executed; failed-push local tag
and no-release replay cannot authorize a remote build; absent/unreachable remote
refused; annotated remote identity verified without rewriting conflicting local
tags. `remote_identity.py` only verifies the exact remote object/source using Git;
it neither calculates a version nor tags/builds/publishes. Verification is an
observation boundary; tag protection/immutable publication must prevent later
remote changes independently. Existing `stage_source.py` still separately requires
the clean checked-out source and matching local tag before build staging.

Observed limits: with no baseline tag, raw version CLI emits v0.0.0 even for a
docs-only initial history. Qualified upstream path/title gating must suppress
the command entirely for docs-only events, including bootstrap; first release
policy remains unaccepted. `publish --tag` exits successfully for a missing remote
release or no matched distributions. Partial-upload retry fills missing files but
errors on retained duplicate names; repeated invocation is not idempotent success.
That command is not selected for production publication. Maintained existing-tag
draft creation/upload/completeness/finalization composition remains unfinished.

## Maintained GitHub CLI and composition qualification

The proposed ordered tag/build/draft/recovery configuration and exact remaining
owner inputs are in [composition.md](composition.md). The separate read-only
`github-cli-qualification.yml` verifies the official upstream 2.102.0 archive
checksum and runs five publisher fixtures plus two actual tag-to-compiled-crate
composition fixtures. It passes only fake credentials to an owned loopback TLS
server. Set `SDK_GITHUB_CLI`, `PYTHON_SEMANTIC_RELEASE` and `SDK_FIXTURE_CARGO` to
the verified/hash-installed/pinned tool paths, then run:

```sh
python -m unittest discover -s release -p 'github_cli_fixtures.py' -v
python -m unittest discover -s release -p 'composition_fixtures.py' -v
```

The binary advisory job is a separate unsuppressed gate: GO-2026-5932 currently
causes exit3. Read the exact version/severity/function/reachability and supported
remediation assessment in [upstream-audit.md](upstream-audit.md). A passing
functional job or command-source scan cannot waive that failure. No production
workflow, tags, release, settings, SDK source or Buddy/Manager files are changed.

Primary command/configuration sources:
[Python Semantic Release CLI](https://python-semantic-release.readthedocs.io/en/latest/api/commands.html),
[configuration](https://python-semantic-release.readthedocs.io/en/latest/configuration/configuration.html).


`verify_assets.py` compares previously downloaded SDK assets with an independently
generated verified-build manifest. It checks exact declared names, regular-file
bounds, byte hashes and complete manifest identity; duplicate JSON fields, stale
source, symlinks, FIFOs, unexpected assets and missing final distributions refuse.
Partial mode only verifies a retained subset, never declares it ready to publish.
It does not download, delete, tag, publish or manage release state. Upstream
download/recovery/finalization and remote immutability/concurrency remain separate
production qualification. Caller-provided expected metadata must come from the
verified build, never from the downloaded release's self-reported manifest.
