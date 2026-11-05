# SDK CI candidate source accounting

This is a fixed source inventory for the compact-CI review, compared with actual main `c7c101f9ea47c00c0e6f41f97e7212ba97e14749`. [REM-56](https://linear.app/magentumdragon/issue/REM-56/unify-ci-stages-and-publish-releases-only-after-successful-builds-and) owns progress, decisions and final closure status; this artifact is not a parallel task tracker.

All original branches/commits and historical outcomes remain recoverable. Superseded workflow-candidate closure prevents old definitions from reintroducing redundant jobs; it does not merge or accept every non-CI source/qualification in those cumulative candidates. Future eligible fragments must be delivered narrowly from current main, retaining their actual evidence/limits.

| PR | Exact preserved source | Branch retained | Nonworkflow entries: same / main differs / candidate only | Source accounting |
| --- | --- | --- | --- | --- |
| [3](https://github.com/s116821/ReMarkableOpenSDK/pull/3) | `a79355eec0c3963cb500d9c8623fdf7f5a1439f9` | `ci/rem-50-sdk-releases` | 6 / 6 / 27 | Original semantic/tag qualification; older cumulative source, tool policy and fixtures remain historical evidence. |
| [5](https://github.com/s116821/ReMarkableOpenSDK/pull/5) | `546d5e0eac156f6bd5b8b2b0e7ea8dac1f0f7beb` | `ci/rem-50-integrated-candidate` | 27 / 11 / 206 | Earlier integrated candidate staging/consumer work; main received bounded staging/consumer/distribution increments separately. |
| [6](https://github.com/s116821/ReMarkableOpenSDK/pull/6) | `331751d0b42caa42158b3c34c1a86750f59bc25b` | `ci/rem-50-publisher-source-candidate` | 27 / 11 / 214 | Source-built maintained uploader qualification; actual upstream tool/security evidence is retained, not activated by compact CI. |
| [7](https://github.com/s116821/ReMarkableOpenSDK/pull/7) | `53c2f740550331a24a150c624a3150dd90b9503e` | `ci/rem-50-clean-publisher-composition` | 27 / 11 / 220 | Maintained tag/build/publisher composition fixtures; unmerged official-release/protection/source policy is not silently accepted. |
| [8](https://github.com/s116821/ReMarkableOpenSDK/pull/8) | `72136edc94a95706fe1d1c7cb56a8bb6c5ffee4d` | `ci/rem-56-gated-release` | 53 / 12 / 244 | Accepted historical ordered pipeline candidate; exact helpers were incrementally extracted into main. Whole cumulative publisher workflow is not the compact-main implementation. |

`source-accounting.json` records the exact mode/type/blob for every candidate-only or different nonworkflow entry and a SHA256 of the complete sorted Git tree manifest. Same entries already exist byte-for-byte on the comparison main. Git objects at the preserved heads retain all file contents; no source is copied into this inventory or discarded. The differing categories are evidence to preserve, not a claim that old source should replace newer main.

## Disposition agreed for compact rollout

Root owns final coordinated closure after compact CI lands. Release/CI drafts3/5/6/7/8 are superseded workflow candidates; their retained non-CI work above is not a delivery prerequisite and is not implicitly approved for production. They must be closed/accounted rather than merged with obsolete definitions or rerun as a broad stale-source matrix.

Main recommended and Root accepted closing monolithic SDK PR1 at preserved `f17d12fcc1bec1784d9f42182bc7da8889626c03` after compact CI lands. Its `research/rem-25-native-platform-contract` branch, worktree and active native research remain intact. No workflow port or push to another owner's branch is required. Future source slices come from current main. Closure is not native qualification or REM25 completion.

SDK21 is normally merged at the comparison main and its actual read-only main observation is verified separately. SDK22 owns this compact-CI delivery. This inventory authorizes no tag/release/settings/native operation and records no deletion or branch rewrite.
