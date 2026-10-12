# Design and unresolved gates

Conventional Commit PR titles become squash subjects. The original candidate uses upstream commit-analyzer and semantic-release; their dependency audit remains unresolved. The current maintained alternative proposes Python Semantic Release for tag-only calculation and explicit existing-tag GitHub CLI drafts, with no production tool selected. docs: explicitly suppresses release even if other default conventions would suggest one. Upstream path/title Actions must reject docs: misclassification of application changes; no custom classifier. Relevant build/CI/dependency changes must receive a tested release policy. Breaking/pre-1.0 and first SDK version policy need explicit acceptance before activation.

Initial qualification uses actual pinned semantic-release and commit-analyzer packages, owned temporary bare/work repositories, equal timestamps, immutable fixture tags and dry-run. Fixture results are not hosted CI or production publication proof. The SDK toolchains and downstream Cargo consumer are not prerequisites for semantic qualification.

Official builds must occur after tag creation at the exact tagged source, not semantic-release's pre-tag prepare step. Evaluate an upstream exec publish-stage build hook before GitHub publication; custom commands may build/package/verify identity only. Concurrency (including main advancing) and failed post-tag build/publication recovery must be qualified independently before selecting this mechanism. No private semantic/orchestration workaround is acceptable.

Rust library distribution should use an immutable Git tag/exact locked commit (or suitably qualified source crate package/registry later), compiled into Buddy. Do not assume arbitrary prebuilt Rust .rlib files are reusable across toolchains. Real target helper binaries may be release assets with architecture/model/firmware/runtime/ABI/hash metadata and bundled into Buddy. Local Cargo path overrides are supported but official builds refuse them. Compatibility manifests distinguish architectural compilation from RM1/RM2/Paper Pro native support; no model is qualified by matching another model's CPU.

Future Buddy prerelease promotion is a separate coordinated REM-46/41 decision. GitHub prerelease metadata and SemVer prerelease identifiers are different: promotion must preserve tag/artifact/runtime identity and never relabel/rebuild bytes under an existing tag. Running Manager instances discover stable promotion by refresh/poll and gate installation on manifest integrity/compatibility.

First public SDK release remains gated by accepted shipping source availability, license/dependency/API policy and independent source/artifact review. The current candidate compiler lanes use an explicit experimental snapshot, not accepted shipping source; researching/pinning release tools does not supply that acceptance. Preserve baseline PR #2 and active native research separately.

User clarification: no project version field or placeholder is maintained in source. Git tags alone mint/maintain project versions. Packaging/runtime version fields are generated only in an isolated build staging area from the checked-out tag; never committed. No version-bump commits. This supersedes earlier manifest-placeholder allowances. Dependency pins are upstream identities, not this project version.

Observed owned-repository lifecycle qualification: upstream prepare runs before
tag creation; publish sees the exact release tag. Failed publish leaves that tag
intact, and a plain rerun returns no release without retrying publish. Production
composition therefore needs a qualified maintained existing-tag publication path.
A stale local main checkout is refused before tagging, but post-check advancement
races remain unqualified. These results use no production repository or publisher.

PR classification composition: pinned dorny/paths-filter v4 classifies changes
using a fail-closed application filter: all paths except root Markdown, docs,
and OpenSpec Markdown. Exclusion uses the upstream every quantifier. Pinned
amannn/action-semantic-pull-request v6 receives allowed types from the upstream
path result: docs is allowed only for documentation-only changes. No private
path/title parser is introduced. The job runs on pull_request, including title
edits, with read-only permissions and no pull_request_target code execution.
Owned fixtures execute the pinned paths-filter distribution against staged local
Git changes, including mixed changes, hidden files and a documentation-to-code
rename. Hosted PR checks verify the title Action composition. Required status
configuration remains a maintainer repository setting, not changed here.

PR classification refuses missing/zero or >=3,000 changed-file counts because
the upstream GitHub files API caps its observation at 3,000. Both rename
directions stay application-relevant. Required status settings remain unchanged.

Audit recheck: semantic-release 25.0.9 and commit-analyzer 13.0.1 remain latest
supported registry releases. Normal dependency refresh leaves 30 high/1 moderate
package findings. The braces <=3.0.3 and http-cache-semantics <=4.2.0 advisories
report no patched versions; forced old-major downgrades or waived gates are not
accepted. Evaluate git-cliff 2.14.2 as another maintained semantic calculator,
using its upstream configuration and exact verified release binary in owned Git
fixtures. This is not a selected production tool or a replacement private bump
algorithm. Test graph/equal-time ancestry, docs suppression and explicit pre-1.0
breaking policy; do not store initial/project version constants in its config.

Recovery qualification uses an actual pinned softprops/action-gh-release v3
bundle with a localhost-only GitHub API fixture and non-credential token. Observe
partial upload failure, draft preservation, existing-tag recovery, duplicate asset
behavior and finalization ordering without production publication. This tests
upstream functionality, not selection/security approval. The released git-cliff
Cargo lockfile had seven RustSec findings at database revision
83278802b2c75399c153d84a6de246503b62966a; the inspected publisher also has audit
findings. No candidate is accepted merely for passing functional tests. Existing
assets require direct identity verification before publication; skipping by name
alone cannot establish byte identity.

Independent packaging preparation will use owned source fixtures with no version
field and immutable fixture tags. A build-only staging helper verifies exact
HEAD/tag/SHA and a clean source checkout, exports the tagged tree into a new
staging directory, and injects the official package version there only. It
refuses maintained source version fields and mismatched/missing tags. This is
build/version identity verification, not semantic or release orchestration.
Generated provenance marks all native device qualifications unsupported. Real
SDK builds/releases still require actual reviewed tag/source and license policy.
Compiler qualification can use a tiny explicitly synthetic library for host,
ARMv7 and AArch64; it does not claim full experimental SDK or native adapter CI.

Packaged source-crate verification checks the normalized Cargo package identity,
Cargo.toml.orig and each staged source input, plus the packaged external lock
inventory. Mutated source, a different package version and altered dependencies
are refused. This is a package identity check; the helper does not orchestrate
release publication. Trust in generated staging metadata still depends on a
trusted CI job and the exact-tag staging invocation. No arbitrary artifact is
accepted as a tagged crate just because it has a filename and hash.

Actual experimental Rust crate CI is independent of native/release qualification.
A read-only checkout of an exact owner research SHA runs its declared host fmt,
test and clippy checks and locked library builds for host/ARMv7/AArch64. It does
not edit the owner manifest, tag/release, compile private Qt artifacts or qualify
a native adapter. The existing prototype version placeholder is an explicit
production blocker under the user's tags-only rule; it is not a maintained
version introduced by release CI. CI snapshot pins identify source commits, not
project versions. PR builds use upstream application path classification; main
merges do not compile this separate research snapshot. Shipping CI will need
actual integrated sources and the strict build staging contract.

The declared Rust minimum is independently checked with Rust 1.88.0 host tests
and a library build; maintained-toolchain host linting and all three architecture
builds use Rust 1.98.1. Passing one compiler is not substituted for proof of the
declared minimum. Neither lane creates official versions or distributable assets.

Git-tag dependencies with no source package version do not automatically inherit
the tag as CARGO_PKG_VERSION. Qualify that gap with actual Cargo, then pass
validated tag/SHA-derived build environment to the SDK's ordinary Cargo build
script for artifact/runtime metadata. The build helper must reject source own
version fields, wrong or dirty cached checkout identity and local overrides.
SDK build scripts use standard Cargo rustc-env/rerun-if-env-changed; no maintained
version, bump algorithm or release coordinator is introduced. Development
identity is explicitly non-release; real SDK runtime/API integration remains
coordinated with the source/consumer owner. Cargo's implicit package default is
not evidence of the SDK release version.

Exact-source CI now advances to owner logical-navigation implementation
f6b7dc8954ba9f5a284d56568765e05c38483924. This is a new source revision, so the
host/minimum/cross matrix must verify it separately rather than reusing older
f8e729c compiler evidence. Its SyntheticVerified/SyntheticUnchanged outcomes and
default Unsupported native methods remain synthetic; compilation will not create
native PageObservation, gesture/completion or device authority.

Existing-tag recovery needs artifact-byte identity, not publisher asset-name
matching alone. A build/identity-only verifier consumes an independently generated
expected manifest and files already downloaded by maintained upstream tooling.
It validates exact tag/SHA/version identity and declared file names/sizes/hashes,
refuses symlinks/unknown or malformed assets, and can verify a partial draft's
retained subset without claiming completeness. Final completeness requires every
declared distribution plus matching manifest. It neither downloads nor calls
release APIs, coordinates retries, computes versions, tags nor publishes.
Production composition/concurrency must separately ensure the verified remote
assets stay unchanged before upstream finalization; this local check is not
remote immutability or production acceptance.

Python Semantic Release 10.7.0 is a maintained alternative qualification candidate,
not a production selection. Its documented no-commit/no-changelog/skip-build
version command with empty version locations creates tags on the existing source
HEAD. Its upstream conventional parser excludes docs (including breaking docs)
and disables squash-body parsing; partial version tags remain disabled. No own
source version or private version calculator is introduced. Dependency installation
will use a complete hash-pinned upstream tool/auditor graph and supported pip.

Extend qualification with equal-time/tagged histories, stable breaking changes,
docs-only initial histories, copied hostile body text, and actual failed-push
recovery against owned bare remotes. A failed remote push can leave a local tag;
CLI success/no-release and local tag existence are not publication authorization.
Before any official build, query the exact remote tag, fetch its immutable object
and verify its peeled commit equals the intended job source. Absence, query failure
or mismatch refuses the build; retry cannot force/move a tag or choose a fresh
version for the failed operation. This is source/tag identity verification, not
a custom semantic engine. Existing-tag asset publication will use maintained
upstream tooling and retain separate byte/completeness/concurrency gates. All
candidate remote writes are confined to disposable local fixture servers/repos;
production credentials, repository settings and releases remain untouched.

Observed bootstrap limit: the raw alternative CLI creates v0.0.0 for a docs-only
history with no prior tag. Do not invoke it for docs-only main events: the already
qualified upstream path/title gates must suppress tagging and builds entirely,
including bootstrap. First SDK release policy remains explicitly unaccepted;
an upstream default/bootstrap value is not a maintained project version.

Observed existing-tag publisher limits: PSR publish succeeds with no matching
remote release or files, and retries error on duplicate retained asset names.
It is not selected as the complete SDK publication mechanism. Qualify a maintained
existing-tag draft creator/uploader/verifier/finalizer with explicit required asset
completeness and immutable publication; successful exit alone is insufficient.

Next publisher qualification candidate is maintained GitHub CLI 2.102.0, exact
upstream binary/source, in loopback TLS fixtures only. Exercise --verify-tag,
explicit --draft creation, partial upload recovery without --clobber, byte
verification/completeness before --draft=false finalization, and published
immutability refusal. The CLI's ordinary auto-publish create path may delete an
incomplete draft after failure; test that behavior rather than assuming it retains
recoverable assets. No production selection/settings/release or custom publisher
is authorized by fixture success. Scan the exact upstream binary independently.

The [concrete maintained composition](../../../release/composition.md) specifies
upstream tag-only calculation, remote identity verification, generated build
staging, explicit draft creation, retained-asset verification, missing-only
uploads and complete verification before finalization. Two actual-tool fixtures
exercise the full tag-to-compiled-source-crate/recovery handoff and fail before
build/API calls without remote tag authority. This is not a production coordinator.
The separate publisher qualification workflow has read-only permissions and a
non-suppressed full-binary govulncheck gate. GO-2026-5932 currently fails it despite
restricted release-command source scans reporting no affected calls/imports.
Severity is unspecified in the primary advisory; no high/critical classification
or waiver is inferred. Owner security policy/remediation, protected immutable
remote publication and accepted shipping SDK source/compatibility remain gates.

## Ordinary-consumer feature boundary integration

The distinct accepted checker belongs to PR1 exact research source
`e408a55f447bfba893842d8bf4527b4fc2a0ac51`, not PR3's version-free release
fixtures or its older compiler snapshot. Keep the existing f6b7dc host/MSRV/ARM
lanes unchanged. Add a separate job to the existing release-qualification
workflow, using the same application-only PR gate and read-only permissions.
Checkout the full immutable e408a55 snapshot without copying or cherry-picking
broad research WIP into this branch; verify HEAD before executing it.

Run host Rust1.88.0 and the existing maintained-toolchain pin1.98.1, with
Python3.12. Prefetch the committed fixture lock separately via cargo fetch
--locked; then call its existing checker, which stays locked/offline. Require
exact SDK path resolution, no SDK features, positive ordinary consumer execution
and separate negative targets with exact E0432/exit101. Never build all fixture
bins together or skip failures. Check tracked source/locks remain unchanged;
untracked target output is build output. No new workflow or release scripts.

This job can run before PR1 merges because checkout references an existing exact
commit, not nonexistent PR3 files. It links PR1 and PR3 without imposing broad
source merge order; shipping integration still needs accepted sources on main.
After source integration, refresh the checkout policy in a separately reviewed
delta. The current job qualifies the frozen snapshot only, not later PR1 edits
or the consumer's current SDK pin. Existing advisory failures, source-version
policy, license/API/distribution, native and public release gates stay open.

Integration verification October12: local Linux Rust1.88.0 and1.98.1 both
pass the exact existing positive consumer plus separate E0432/exit101 targets.
All152 tracked snapshot files, including both lockfiles, remain byte-identical
to e408a55. Fixture lock SHA256 is
192ad860fa77a0a68a346f1e006e41190c81bc5e9853f70ffaa7a1dd69b06d73.
Workflow parses and retains application-only PR control, exact checkout, explicit
prefetch, offline checker and unchanged tracked-file verification. Strict
OpenSpec passes. Hosted jobs and independent CI-delta review remain pending;
earlier author/Sol Windows checks are separate attributed upstream evidence.

## Supported dependency and explicit crate inventory follow-up

After accepted b1966b2 consumer-CI review, apply the supported transitive
Handlebars4.7.9→4.7.10 lock update. Retain direct tool pins and the failing
high advisory gate. No forced downgrade, override or publisher activation.
Use the final SDK owner a3c2037e2c33fb219a64e7d4021c74b17219a665
explicit Cargo/lock/README/eight-source-module inventory to separate all tagged
build-input hashes from selected crate input hashes. Qualify only literal rooted
include paths in this first integration, not an invented Cargo glob engine.
Still verify every staged input; package verification must require declared
source bytes plus normalized Cargo metadata/lock and refuse undeclared members.
Generated VCS metadata, if present, must match clean root exact source SHA.
Legacy minimal owned-fixture behavior stays explicitly non-production.

Source CI transition: first checkout the actual PR merge candidate. If its
SDK Cargo.toml exists, require both checker/fixture and package-list contract,
verify HEAD equals the event merge SHA and execute existing checkers on that
checkout. Missing tools in a candidate SDK fail; do not fall back. While this
release branch has no SDK crate, explicitly select named historical source
a3c2037 in the same job, log that candidate coverage is unavailable and verify
that frozen source identity. Refuse partial SDK source without a manifest.
After PR1 source merges to the base, the same workflow automatically switches
to candidate/current-source execution and fails incoming boundary regressions;
no permanently pinned job is substituted for current-source checks. Required
shipping integration remains open until actual candidate mode runs in CI.

Run the package list-only regression on Rust1.98.1; ordinary-consumer boundary
runs on1.88.0 and1.98.1. No actual SDK archive/build/publish or native operation
is selected. The real source still has prototype version and unresolved license
(including gesture provenance), target/native and publication prerequisites.
Public prospective verifier plan preceded implementation at PR3 comment6127283836.

Local follow-up verification: six explicit-inventory fixtures and12 existing
staging fixtures PASS after normalized dependency/feature/target declaration
checks were added; five Cargo consumer fixtures also PASS before that isolated
package-only strengthening. Changed normalized dependency declarations refuse
as well as missing/changed source and unexpected archive entries. Four actual
workflow selection controls PASS: candidate, explicit historical, missing-tools
refusal and partial-source refusal. Exact a3c2037 consumer gates PASS on Linux
Rust1.88.0/1.98.1; its two list-only package regressions PASS on1.98.1. All156
tracked source/lock files remain byte-identical. Strict OpenSpec/diff checks PASS.
Supported Handlebars lock-only correction separately passed17 policy/lifecycle
checks after a clean install. Hosted follow-up and independent delta review
remain pending; actual integrated SDK candidate mode cannot be claimed until
PR1 sources exist on the PR merge candidate.

Prior b1966b2 acceptance was forwarded by Main/Root from the existing independent
Sol chat; that forwarding has no public review transcript link. It does not
accept this new implementation delta or waive the known advisory gates.

Hosted0f1e91e failure is preserved: new declared-inventory packaging fixtures
PASS and both ordinary consumer boundaries PASS, but1.98.1 package-list tests
fail because consumer-only prefetch does not cache optional SDK serde dependency
metadata. Local cache was already populated. Add a separate locked SDK-root
fetch before its offline package-list tests; do not weaken the checker or make
it online. This is cache preparation only, without actual SDK packaging/native
build or source change. Recheck in a fresh owned Cargo cache and current CI.
Cold-cache proof: an owned empty Cargo cache plus locked consumer-subset fetch
reproduces the missing-serde package-list failure. After locked SDK-root fetch,
the same two unchanged offline package-list tests PASS. No cache-dependent
check relaxation or SDK source/lock edit.

## Reviewed GPL package contract integration

Human GPL-3.0-only selection and SDK owner implementation are complete at
6e45c091d5ca2294f170930d6a25141f1c7fea25. Public independent/evidence receipt:
https://github.com/s116821/ReMarkableOpenSDK/pull/1#issuecomment-6128065282 .
No human license choice or general legal approval gate remains. Use the owner's
exact13 selected inputs (including LICENSE and THIRD_PARTY_NOTICES.md) plus
two Cargo-generated metadata members; keep publish=false. Root relayed bytes
were independently compared with committed Git bytes here. Do not duplicate
license/notice generation or alter owner files.

Advance only the explicit historical fallback to this reviewed licensed source,
retain automatic actual-candidate selection, and execute the existing owner's
three list/metadata/notice regressions after complete locked root prefetch.
Ordinary consumer stays locked/offline on1.88.0/1.98.1. Hashes and original notice
mappings remain owned by that source checker. Package artifact verification
additionally compares normalized license/license-file/publish metadata to exact
staged manifest and proves selected LICENSE/notice missing/tampered refusal
using original harmless owned test inputs, not another SDK license implementation.

Actual research source still has prototype maintained own version fields; do
not strip them or stage an official SDK artifact. Actual candidate-hosted mode
requires PR1 source arrival, and advisory/native/public-tag/remote integrity
gates remain distinct. Accepted SDK task1.4 licensing/dependency/experimental API
policy is attributed to the owner receipt; REM-50 artifact integration remains
its separate scope.

Licensed-input local verification: exact owner6e45c091 ordinary consumer PASS
on Linux1.88.0 and1.98.1; all three existing owner package/metadata/notice tests
PASS on1.98.1. No owner files were edited. All160 tracked owner source/lock bytes
remain unchanged; canonical LICENSE and full notice-bundle hashes match the
public review receipt. Plain export has no Git-generated VCS member; the owner's
Git checkout15-member result is attributed until hosted checkout verification.
Eight REM-50 inventory fixtures plus12 existing staging cases PASS, including
license/notice omissions/changes and normalized grant/license-file/publish
relabeling refusal. An initial test-rewriter optional-selector None error was
corrected without product-check weakening. Hosted successor/review pending.

## Generated Cargo lock metadata versus version authority

Coordinate the version-free owner increment without deleting dependency pins.
Manifest/workspace version declarations remain refused. A source lock may
contain one unique unsourced/checksum-free own-package node at Cargo's implicit
0.0.0, with only name/version/dependencies fields; it is generated development
metadata, never release authority. Nondefault/ambiguous/foreign own nodes refuse.
SDK Git-consumer identity retains exact tag/SHA and source-cleanliness checks.

Stage the tagged source unchanged and inject tag version only in the build copy.
Record external dependency entries separately from the validated own placeholder.
The caller uses upstream cargo generate-lockfile --offline in staging; source
verification then requires its own node match the injected tag version and
external entries remain exactly unchanged. Package verification likewise checks
packaged own lock identity. No generated version goes back to source; no custom
Cargo resolver, bump/tag/release engine or schema-version interpretation.
Add owned Cargo proofs for generated-default acceptance and tag regeneration,
source immutability, nondefault/duplicate/source-marked placeholder refusal,
registry drift and packaged lock identity. Actual SDK staging waits for its
owner's reviewed source freeze; no source owner manifest/lock edit here.
Public handoff before code: PR1 comment6129204263 / PR3 comment6129204437.

## Sol6008083 P2 repair plan

Independent review final requires two fixes: use POSIX relative inventory keys
in stage and verification, including target exclusions, because Windows string
keys use backslashes; refuse partial candidate package-checker/other src module/
consumer fixture inputs when Cargo.toml is absent. Normalize filesystem-derived
keys with as_posix, without loosening declared traversal/member protections.
Move the existing source selector into one owned bash script for actual direct
fixtures and require the entire SDK src tree, both checker paths and consumer
fixture subtree absent before historical fallback. Existing unrelated research
tools are not SDK crate inputs.

Add one genuine Windows hosted original tagged/version-free fixture exercising
stage, selected /src/lib.rs, target exclusion, source drift and unchanged source.
Do not rerun an SDK Windows matrix. Add Linux direct-selector fixtures for the
reviewer's three partial cases plus valid candidate/SDK-free controls. Keep
6008083 and its failing Windows/selector review evidence identifiable; final
successor review covers repairs plus separately identified future generated-lock
work, not silently inherited acceptance.

P2 implementation local proof: one original tagged portable stage case PASS
on Linux (genuine Windows hosted counterpart pending), direct actual selector
three test methods PASS including every reproduced partial input and valid
SDK-free/candidate controls. Source-relative keys and target exclusions now use
as_posix throughout. Selector's complete absence guard also includes root lock
and nonregular/symlink manifest inputs. No traversal/member rule weakened.
The additional generated-root-default plan remains prospective, not a claim
that this P2 source already fixes lock handling; stage/runtime owned-version
refusals at6008083 remain until that separately identified implementation.

Hostedc3f7dba Windows portability fixture fails before inventory at the original
regex's unsupported-structure refusal for valid CRLF manifest. Preserve that
failed run38285057113. Follow-up recognizes CRLF package headers and writes
injected manifest bytes without OS double-newline translation, preserving the
source header line ending. Explicit LF/CRLF original fixture controls exercise
both inventory and target exclusion on each host. This is a concrete Windows
manifest portability repair, not weakened inventory/traversal or version rules.
