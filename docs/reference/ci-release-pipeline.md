# Ordered SDK CI and release

REM-56, SDK release slice owned by SCRAPPY-DOO. SDK core/native source and the Buddy/Manager implementations retain separate owners. The October 15 instruction replaces tag-before-build ordering.

`.github/workflows/ci.yml` has four stages: checks, build, integration and one final release job. Checks include maintained path/title validation, SDK lint/unit/MSRV checks, maintained semantic-tool audit, explicit candidate/official staging refusals, and real GitHub expression evaluation for predecessor failure/skip/cancellation. The semantic calculator resolves an upstream read-only candidate. Documentation-only main merges never run SDK compilation or release; PRs cannot enter the release job.

Build stages the exact source without maintaining any version in source. Candidate staging records a null verified tag and explicit candidate identity; ordinary official verification refuses it. Cargo generates only build-output package/lock identity. The job emits a verified source crate, host/ARMv7/AArch64 compiler libraries, six current host-test executables and a hashed receipt. Rust libraries require a matching compiler/dependency graph; compilation does not establish a stable binary ABI or tablet support.

Integration downloads these artifacts, validates the trusted build-job receipt and package/source/version hashes, executes the transferred host tests and checks tampered receipt/package/test/symlink refusal. It does not rebuild the release. The inherited ignored native smoke case remains ignored and unqualified; host-test completion is not native proof.

The main-only final job also requires an observed merged PR at exactly the successful source SHA. It revalidates the tested artifacts, resolves the unchanged candidate again, and only then calls the maintained upstream tagger. Existing remote tag/source conflicts refuse. Maintained exact-tag GraphQL lookup and request Actions create/finalize an explicit draft by ID, while the selected create-only uploader must preserve retained bytes. Complete downloaded artifacts and the exact remote tag are checked before finalization. The upload step currently fails closed while Root/Sol qualify the shared release-ID/create-only uploader; it is an external completion dependency, not a passing publisher or an enabled production release.

Retry a failed release job using the same successful run artifacts. A complete rerun after tagging may refuse upstream no-release rather than rebuild recovery bytes or invent another version. Job concurrency serializes this pipeline, without claiming control over unrelated external writers. Production source/first release/protected remote and compatibility acceptance remain coordinator gates. Do not merge/activate the pending uploader or delete the user note before verified delivery.

## Focused verification

Run from this repository with the hash-installed maintained PSR and actual Cargo available:

```sh
python -m unittest discover -s release -p test_candidate_stage.py -v
python -m unittest discover -s release -p test_stage_source.py -v
node --test release/release-assets.test.cjs
npm ci --ignore-scripts --prefix release/fixtures
npm audit --audit-level=high --prefix release/fixtures
node --test release/fixtures/pipeline.test.mjs
```

The first command needs `PYTHON_SEMANTIC_RELEASE` and `SDK_FIXTURE_CARGO`. Transferred-artifact refusal checks require actual output from `candidate_artifacts.py build`, supplied through `SDK_CANDIDATE_FIXTURE` and the trusted `SDK_CANDIDATE_RECEIPT_SHA`; unavailable inputs fail instead of skipping. `candidate_artifacts.py integration` executes those same built test binaries. OpenSpec and required hosted CI are separate checks. Preserve original failures and distinguish owned Git/loopback, frozen compiler-source, current hosted candidate and native evidence.

Maintained tool sources: [Python Semantic Release](https://python-semantic-release.readthedocs.io/en/latest/api/commands.html), [GraphQL Action](https://github.com/octokit/graphql-action), [request Action](https://github.com/octokit/request-action), [github-script](https://github.com/actions/github-script/releases) and [actionlint 1.7.12](https://github.com/rhysd/actionlint/releases/tag/v1.7.12). Exact pins and historical qualification are recorded in the owning OpenSpec design. Root/Sol's accepted source-built uploader recipe remains required before selection; no private release coordinator is substituted.
