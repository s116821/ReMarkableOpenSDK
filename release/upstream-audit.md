# Candidate audit and qualification evidence — 2026-10-03

These are candidate results, not production security acceptance. Upstream
lockfiles may contain target/feature-specific dependencies; these scans do not
establish exploitability in a given binary. No gate was waived, ignored or forced
to an old unsupported major. No production version/tag/release was created.

## semantic-release

Latest supported registry versions checked: semantic-release 25.0.9,
commit-analyzer 13.0.1. Normal npm dependency refresh retained 30 high and one
moderate vulnerable-package findings (including cascaded package findings, not
31 independent advisories). Two blockers have no patched upstream package:

- braces <=3.0.3: [GHSA-vfj7-8cjw-p6xm](https://github.com/advisories/GHSA-vfj7-8cjw-p6xm).
- http-cache-semantics <=4.2.0: [GHSA-ch52-4w7c-c8xp](https://github.com/advisories/GHSA-ch52-4w7c-c8xp).

Registry latest remained braces 3.0.3/http-cache-semantics 4.2.0. npm's suggested
old-major commit-analyzer 6.3.3 downgrade is not an accepted production fix.
Its functional fixtures stay useful, but production selection remains blocked.

## git-cliff alternative

Verified release binary 2.14.2, upstream x86_64 Linux GNU archive SHA256
24f397c733add5390fdceee3a2088588ab0d5f944ce00d34cb7029b888cf2db4.
Actual equal-time, docs/breaking-docs, semantic types, repeated calculation and
old-checkout/descendant-tag fixtures passed using documented --use-branch-tags,
--offline and --no-exec. The default without --use-branch-tags failed the old
checkout case; no private ancestry or bump algorithm was introduced.

Candidate policy preserves pre-1.0 breaking changes as minor; the first official
SDK tag and breaking policy still require acceptance. No initial/project version
is stored in config. [Upstream bump options](https://git-cliff.org/docs/configuration/bump/).

Cargo-audit 0.22.2 against the upstream v2.14.2 Cargo.lock and RustSec database
83278802b2c75399c153d84a6de246503b62966a found seven advisories:

| Package/version | Advisory | Patched range |
| --- | --- | --- |
| bytes 1.11.0 | RUSTSEC-2026-0007 | >=1.11.1 |
| crossbeam-epoch 0.9.18 | RUSTSEC-2026-0204 | >=0.9.20 |
| quick-xml 0.26.0 | RUSTSEC-2026-0195, RUSTSEC-2026-0194 | >=0.41.0 |
| quinn-proto 0.11.13 | RUSTSEC-2026-0185 | >=0.11.15 |
| quinn-proto 0.11.13 | RUSTSEC-2026-0037 | >=0.11.14 |
| rustls 0.23.31 | RUSTSEC-2026-0285 | >=0.23.45 |

The graph is not accepted as clean merely because functional tests pass.

## Existing-tag publisher and tag Action

softprops/action-gh-release 3.0.3 exact efb35369e0ad2afab669f228072c1b0d510eae64:
actual localhost-only API fixtures confirm failed upload leaves a draft, retry
fills missing assets, existing assets are retained with overwrite_files=false,
publication follows uploads, and repeated invocation creates no second release.
Its upstream lockfile has two high vulnerable-package findings: brace-expansion
(GHSA-q2hr-2g5m-vwhr / GHSA-qhr7-859c-m2p7 / GHSA-6j4f-fj2g-mc7p) and undici
(GHSA-3wwx-pv8p-q78v / GHSA-r53p-7pc4-xj5r / GHSA-rfgv-xxqx-mfg5). Supported
upstream fixes/replacement qualification remain required. Existing-asset name
matching is not a byte-identity check; verify hashes before publication.

rickstaa/action-create-tag exact a1c7777fcb2fee4f19b0f283ba888afa11678b72:
entrypoint-only fixtures, with independent runner configs and owned bare remotes,
confirm one non-forced tag winner for conflicting source races, explicit source
binding after main advances, and refusal of an existing local tag. No Docker
image execution/audit or production tool selection is claimed.

## Independent build/consumer work

Build staging refuses dirty, wrong/missing-tag or mismatched-SHA source and own
version fields (including workspace and root lockfile entries); the version is
injected in a new build directory from its existing tag. Owned Rust 1.98.1
fixtures compile on host/ARMv7/AArch64; a host executable reports the injected
version. These are synthetic compiler/package checks, not full SDK/native CI.

Actual Cargo fixtures resolve an immutable SDK Git tag to the expected locked
commit despite branch advancement. A temporary local patch changes the built
answer, is rejected by official dependency identity verification, and reverts to
the Git source when removed. No Buddy/Manager files changed.
