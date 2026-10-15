> Current ordering: REM-56 `sdk-gated-release-pipeline` supersedes prospective tag-before-build plans below. See [the ordered SDK pipeline](../docs/reference/ci-release-pipeline.md). The former candidate workflows are retained as [historical evidence](../docs/research/release-history/README.md); their unselected-tool advisory failures are not waived. This work does not activate production tags/releases during development.

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
dependency pins retain their upstream versions. Builds may embed a read-only upstream candidate in generated output; only the final gated release job binds it to an official immutable Git tag after successful artifact qualification.

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

## Distinct ordinary SDK consumer check

The existing release qualification workflow additionally checks PR1 source
[e408a55](https://github.com/s116821/ReMarkableOpenSDK/commit/e408a55f447bfba893842d8bf4527b4fc2a0ac51)
as an ordinary feature-disabled dependency, on host Rust1.88.0 and1.98.1.
Its [canonical guide](https://github.com/s116821/ReMarkableOpenSDK/blob/e408a55f447bfba893842d8bf4527b4fc2a0ac51/docs/reference/consumer-build-boundary.md)
owns the commands and checker. Explicit locked prefetch is followed by the
locked/offline positive consumer and separate exact E0432/exit101 negatives.
The workflow verifies exact checkout identity and unchanged tracked files/locks.

This source is separate from the preserved f6b7dc experimental compiler matrix
and synthetic release/version fixtures. PR3 does not copy broad PR1 WIP, update
Buddy pins or activate shipping CI. No PR1-before-PR3 merge is required for the
frozen checkout to exist; shipping source integration remains its own gate.
These results qualify only the identified source and host boundary. They do not
waive advisory failures or qualify any native operation/model/firmware/release.

## Current-source transition and declared crate inputs

The consumer job now first checks out the current PR merge candidate. If its
SDK crate exists, required boundary tools/fixtures must exist there and HEAD
must equal the event SHA; tests run on that candidate. Missing required files
fail rather than selecting historical source. Until PR1 integrates the crate,
this SDK-free release branch explicitly logs HISTORICAL QUALIFICATION ONLY and
checks owner snapshot
[a3c2037](https://github.com/s116821/ReMarkableOpenSDK/commit/a3c2037e2c33fb219a64e7d4021c74b17219a665).
Partial SDK source without a manifest refuses. Rust1.98.1 additionally runs the
owner's package-list containment regression. It lists files only, without
creating a real SDK archive/build or clearing license/distribution gates.
Successful historical mode does not close current-source integration.

Staging still verifies every tagged build input. An explicit literal rooted
Cargo include list defines the distinct crate source inventory; unsupported
patterns/excludes, noncanonical/missing/duplicate paths refuse. Package checks
require selected source bytes, normalized manifest identity, original manifest
and dependency lock, reject undeclared members and verify generated VCS metadata
if present. The existing minimal owned-fixture default remains non-production;
real SDK packaging is gated on accepted source/license/version-free inputs.
No SDK owner's allowlist or consumer pin is edited.

The supported normal audit refresh changes only Handlebars4.7.9→4.7.10. This
removes critical findings but retains the failing high advisory gate; see the
dated [audit evidence](upstream-audit.md).

Package list-only checks additionally need a separate `cargo fetch --locked
--manifest-path Cargo.toml` prefetch, because the minimal consumer lock subset
does not cache optional SDK dependencies such as serde. Both checkers stay
offline/locked; a previously warm cache is not cold-run preparation evidence.

## Reviewed licensed source inputs

Current historical qualification source is
[6e45c091](https://github.com/s116821/ReMarkableOpenSDK/commit/6e45c091d5ca2294f170930d6a25141f1c7fea25),
superseding a3c2037. GPL-3.0-only and owner license/dependency/experimental API
policy are resolved; [owner review receipt](https://github.com/s116821/ReMarkableOpenSDK/pull/1#issuecomment-6128065282)
records13 selected inputs plus two generated Cargo members, with LICENSE and
THIRD_PARTY_NOTICES.md included. Existing owner checks verify canonical LICENSE
bytes, all notice texts/mappings and current lock-inventory freshness; REM-50
reuses them without generating another grant/notice bundle. Artifact identity
also verifies normalized license/license-file/publish metadata and selected
license/notice bytes. Actual candidate mode and remaining version-free-source,
advisory/native/publication gates remain open.

## Generated own lock defaults and actual source qualification

Source manifest/workspace release versions remain absent. The unique optional
own Cargo.lock node may contain only generated implicit0.0.0/name/dependencies,
without source/checksum. It is development resolution metadata, not release
authority. Preserve every external lock entry. In new staging only, generate
the manifest from the exact existing tag, run upstream Cargo's offline lock
generation, and require the own node match that tag and external entries remain
unchanged before verification/package. Source/consumer lock bytes do not change.

Current explicit historical source71325818 includes the reviewed161b29 version
removals plus documentation-only results; tests do not call it actual PR source
coverage when this release branch has no crate. `qualify_sdk_stage.py` constructs
a marked temporary original Git fixture with an exact source-tree comparison
and an isolated fixture tag, builds/verifies the real SDK source crate and
prints original source SHA/tree separately from fixture SHA/tag. It does not
mint an official tag, select a public SDK version, publish or grant native
compatibility. The existing workflow performs that qualified host check only
after source selection/identity and locked graph prefetch.

Verified-build source checks now require a regular non-symlink staged Cargo.lock
and its tag-derived own/external identity; deleting or substituting it refuses.
An explicit `source_only=True` byte-inspection phase returns
`verification_phase=source-only` and `lock_verified=false`, and cannot replace
default locked-build/package verification. Optional source own-node metadata is
unchanged; it does not make the staged build lock optional.
