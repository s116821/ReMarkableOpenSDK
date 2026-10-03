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
