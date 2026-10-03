# Design and unresolved gates

Conventional Commit PR titles become squash subjects. Upstream commit-analyzer determines release type; upstream semantic-release determines version/tag and upstream GitHub plugin publishes declared assets. docs: explicitly suppresses release even if other default conventions would suggest one. Upstream path/title Actions must reject docs: misclassification of application changes; no custom classifier. Relevant build/CI/dependency changes must receive a tested release policy. Breaking/pre-1.0 and first SDK version policy need explicit acceptance before activation.

Initial qualification uses actual pinned semantic-release and commit-analyzer packages, owned temporary bare/work repositories, equal timestamps, immutable fixture tags and dry-run. Fixture results are not hosted CI or production publication proof. The SDK toolchains and downstream Cargo consumer are not prerequisites for semantic qualification.

Official builds must occur after tag creation at the exact tagged source, not semantic-release's pre-tag prepare step. Evaluate an upstream exec publish-stage build hook before GitHub publication; custom commands may build/package/verify identity only. Concurrency (including main advancing) and failed post-tag build/publication recovery must be qualified independently before selecting this mechanism. No private semantic/orchestration workaround is acceptable.

Rust library distribution should use an immutable Git tag/exact locked commit (or suitably qualified source crate package/registry later), compiled into Buddy. Do not assume arbitrary prebuilt Rust .rlib files are reusable across toolchains. Real target helper binaries may be release assets with architecture/model/firmware/runtime/ABI/hash metadata and bundled into Buddy. Local Cargo path overrides are supported but official builds refuse them. Compatibility manifests distinguish architectural compilation from RM1/RM2/Paper Pro native support; no model is qualified by matching another model's CPU.

Future Buddy prerelease promotion is a separate coordinated REM-46/41 decision. GitHub prerelease metadata and SemVer prerelease identifiers are different: promotion must preserve tag/artifact/runtime identity and never relabel/rebuild bytes under an existing tag. Running Manager instances discover stable promotion by refresh/poll and gate installation on manifest integrity/compatibility.

First public SDK release remains gated by actual source availability, license/dependency/API policy and independent source/artifact review. SDK main currently lacks Cargo sources; researching/pinning release tools does not supply them. Preserve baseline PR #2 and active native research separately.

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
