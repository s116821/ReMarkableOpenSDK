# Upstream release qualification

REM-50, machine owner SCRAPPY-DOO. This harness executes actual pinned upstream
semantic-release/commit-analyzer tools against temporary owned repositories.
The runner refuses repositories outside its marked temporary fixture root and has no external publisher. It cannot publish production releases. SDK target builds and publication remain
unimplemented and gated by the active OpenSpec tasks.

Use Node 24 (at least 24.10), then `npm ci --ignore-scripts` and `npm test` in this
directory. No own project version is maintained in either manifest. Upstream
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
