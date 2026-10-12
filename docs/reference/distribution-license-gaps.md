# Distribution, API and license decision record

October 12, 2026. Engineering inventory for task 1.4, not a license grant,
legal clearance, release selection or native qualification.

## Current source and ownership

The inspected SDK research revision is `e408a55f447bfba893842d8bf4527b4fc2a0ac51`.
Its manifest remains `publish = false`, has no `license`/`license-file`, and has
no package-content allowlist. Live SDK main was
[`984ebf5`](https://github.com/s116821/ReMarkableOpenSDK/tree/984ebf57755c95f6d3f2735e1a8f0818c3c6e704):
its tree contains neither a Cargo manifest nor LICENSE/NOTICE/COPYING file.
The inspected [REM-50 PR 3](https://github.com/s116821/ReMarkableOpenSDK/pull/3),
head `7fb824e8a218b7beabb138f3aa38df2c5bcdc930`, also adds no SDK license.
Its [composition contract](https://github.com/s116821/ReMarkableOpenSDK/blob/7fb824e8a218b7beabb138f3aa38df2c5bcdc930/release/composition.md)
explicitly requires accepted shipping source/license/API from the source owner.
SCRAPPY retains release/CI/packaging ownership; its unresolved advisory and
publication gates are unchanged.

No approved SDK license choice was found in those sources, task 1.4, the current
REM-50 issue and latest five comments, or Main's current handoff. This is a bounded
reconciliation, not a claim to have searched every historical decision.

## Reuse and provenance inventory

This reuses the [existing prior-art ledger](../research/native-platform-ledger.md)
and [mechanism comparison](../research/runtime-mechanism-comparison.md), plus a
tracked source/header search and the concrete gesture comparison. A missing
copyright marker is not proof of original authorship.

| Material | Relationship to current SDK | Distribution action still required |
| --- | --- | --- |
| Buddy gesture calculation at [`ded1cadd`, `xochitl_integration.rs`](https://github.com/s116821/ReMarkableBuddies/blob/ded1caddccd30d6452fc2613a017f0118b0c575b/src/workflow/xochitl_integration.rs) | SDK [`5c0bc303`, `gesture.rs`](https://github.com/s116821/ReMarkableOpenSDK/blob/5c0bc303bb9b22e8e01bce9b2bd2a7fe47c8010e/src/navigation/gesture.rs) preserves endpoint, timing and per-step floating-point/truncation calculation. Treat as an adaptation/provenance item, not merely unrelated inspiration. | Resolve ownership and applicable grant before selecting a different SDK license. Buddy supplies [GPL version 3 text](https://github.com/s116821/ReMarkableBuddies/blob/ded1caddccd30d6452fc2613a017f0118b0c575b/LICENSE); this report does not decide copyrightability or relicense it. |
| `image`, `sha2`, optional `serde`/`serde_json` and transitive crates | Actual Cargo dependencies, not vendored implementation in SDK source. Exact package versions, published VCS metadata and notice-file hashes are in the [inventory](dependency-license-inventory.json). | Choose applicable license alternatives and ship notices for the actual source/binary distribution and feature graph. Metadata inventory is not a finished notice bundle. |
| [libremarkable `d9125f1`](https://github.com/canselcik/libremarkable/blob/d9125f136ed34926c2528c34722aa3494611d644/LICENSE.md) | MIT; existing ledger explicitly records source review only, no copied source/dependency. | No adoption is claimed. If later incorporated, retain its copyright/permission notice and assess the selected dependencies separately. |
| [Inkling `089efb2`](https://github.com/nathanmarlor/inkling/blob/089efb2c9f24ce64127c6fc4d7fd30f93ae5ad94/LICENSE) / [XOVI `2b99649`](https://github.com/asivery/xovi/blob/2b99649f5e4fd6288be7792a8570bd16418adb70/LICENSE) | MIT / LGPL-3.0 license text. Existing ledger records dispatch/visual-tree inspiration and explicitly no adopted loader/code. Neither is a Cargo dependency here. | Any selected bundled loader/extension needs its own precise source, notices and applicable LGPL/source/relinking review. Eligibility is not adoption or clearance. |
| [rm-librarian `7d0fe08`](https://github.com/rmitchellscott/rm-librarian/blob/7d0fe08678203c129b9d848419da2a7e96a2abe5/LICENSE) / [creation QMD `67d39e9`](https://github.com/rmitchellscott/xovi-qmd-extensions/blob/67d39e943d8fda30936ad7f51a30f59f0292a9f8/3.28/createPagesRM2Size.qmd) / [navigation QMD `0817207`](https://github.com/alefaraci/xovi-qmd-extensions/blob/0817207c6345f198d5cafeca5cd278deba2cae0b/3.28/linkFromSelection.qmd) | GPLv3 prior art; creation QMD declares GPL-3.0-only. Existing ledger says mechanisms/citations rather than copied implementation. Current Qt bridge uses an independently structured fixed helper/guard path; this inspection does not certify all expression independently authored. | Retain attribution links; resolve any actual copied/adapted expression before granting broader distribution terms. Do not assume a mechanism reference licenses unrelated proprietary QML. |
| Qt/vendor runtime, GNU diagnostic tools, private firmware evidence | Public SDK tools use Qt headers/APIs; no Qt library is in the Cargo graph. Private/untracked historical GNU modifications are outside tracked SDK delivery. Proprietary providers and disassembly remain excluded. | A future native binary requires exact provider/toolchain/source-license and notice review. This Rust dependency inventory cannot clear that binary or authorize proprietary redistribution. |

## Dependency notices

`cargo metadata --locked --offline --all-features` resolves **38 registry package
instances**; two are different `miniz_oxide` versions. All have declared license
metadata and at least one license/notice file in the cached published crate:
79 files are indexed by path and SHA-256. The inventory is tied to SDK revision
and Cargo.lock hash and contains no local cache paths or private material.

Most offer MIT/Apache alternatives; preserve the actual expression rather than
calling the entire graph MIT. Notable distinctions are `moxcms`/`pxfm`
(`BSD-3-Clause OR Apache-2.0`), `zlib-rs` (`Zlib`), and `unicode-ident`
(`(MIT OR Apache-2.0) AND Unicode-3.0`). The last requires the additional Unicode
terms even when choosing MIT. Version-check's legacy `MIT/Apache-2.0` spelling
is retained verbatim, not silently normalized into an approved selection.

Notice work depends on the selected alternative: [MIT](https://spdx.org/licenses/MIT.html)
retains copyright/permission; [Apache section 4](https://www.apache.org/licenses/LICENSE-2.0)
includes the license, change/attribution obligations and any applicable NOTICE;
[BSD-3-Clause](https://spdx.org/licenses/BSD-3-Clause.html) requires source/binary
notice retention and prohibits implied endorsement; [Zlib](https://spdx.org/licenses/Zlib.html)
preserves source notices and identifies alterations; [Unicode-3.0](https://spdx.org/licenses/Unicode-3.0.html)
has its own notice conditions. The actual per-package texts linked in the inventory
remain authoritative. This is not an exhaustive per-file provenance audit or a
claim that all 38 packages are linked into every target artifact.

## Concrete distribution gap

A list-only `cargo package --list --allow-dirty --offline` inspection on October
12, 2026, in the pre-fix `e408a55`-based dirty working tree (including draft inventory
and local untracked files, not the immutable commit tree alone) returned
155 prospective files, including 71 under `tools/` and four pre-existing,
untracked `tools/rem25_disposable_server` files. No archive, build or publication
was performed. Thus `publish = false` does not keep private development files out
of local package selection. A reviewed Rust-crate content allowlist and a
package-list exclusion check are needed before any source package is selected.
Do not run a broad directory archive or infer package safety from Git tracking.
The fixture workspace is excluded by Cargo's nested-package handling, which is
also distinct from its feature checks.

The subsequent [containment increment](../../openspec/changes/establish-native-platform-contract/package-content-boundary.md)
selects exactly the eight traced Rust files, Cargo manifest/lock and README, plus
Cargo-generated metadata. Its actual Windows list has 13 members, with zero tools.
Two focused list-only regressions pass: actual checkout contents and an isolated
Git-index fixture whose harmless untracked decoys are excluded with the allowlist
but included when the old unrestricted manifest is restored. All four historical
file hashes remain unchanged. No package archive or native build was made. This
fix does not settle missing license/notices, package documentation completeness,
REM-50 owner integration, or distribution approval. The initial failure above is
retained as evidence of the defect, not a description of the corrected manifest.
Final distribution MUST include the explicitly selected license/notice files and
review their actual packaged contents; this current allowlist intentionally lacks
that clearance and must not be treated as a final release file set.

The existing Sol reviewer independently passed both package-list regressions and
accepted the source closure. The [REM-50 owner confirmed this containment scope](https://github.com/s116821/ReMarkableOpenSDK/pull/1#issuecomment-6127144709).
Its [current staging verifier expects the entire tracked tree](https://github.com/s116821/ReMarkableOpenSDK/pull/1#issuecomment-6127131882),
so adapting that verifier to distinguish build sources from declared crate contents
remains REM-50 work. Do not widen the allowlist to satisfy that mismatch.

## Decisions for the maintainer and release owner

1. **Choose the SDK's intended grant and confirm ownership.** GPLv3 is the concrete
   project precedent: [Buddy](https://github.com/s116821/ReMarkableBuddies/blob/ff8ad75bec45fca403d55a7b6d93eb83ab732e3d/LICENSE)
   and [Docs](https://github.com/s116821/ReMarkableBuddiesDocs/blob/7686a67c2303cc675d544e2edf635c4f580b6a4c/LICENSE)
   carry that license text. Confirm `GPL-3.0-only` versus `GPL-3.0-or-later` explicitly;
   the standard license's example boilerplate alone does not settle that choice.
   If permissive licensing for unrelated consumers is desired instead, first
   resolve the Buddy adaptation and contributor rights. No option is selected here.
2. **Select first distribution scope.** An experimental Rust source crate using
   the existing synthetic/unsupported API can be described truthfully, but cannot
   be advertised as a supported native SDK. Keep `publish = false` until owner
   decisions, approved contents/notices and REM-50 release gates are complete.
3. **Freeze the intended API/support promise.** Existing UUID/opaque observation
   equality/page/order/origin is sufficient for Main's historical Store projection.
   A native current observation/guard, adapter and integrated effect admission
   remain missing qualification; saved evidence and local tests cannot supply it.
   Do not add an API framework just to make task 1.4 appear complete.

Source basis: current checked-in code and published Git trees, current project
issue/comments and Main coordination, cached exact Cargo package metadata/texts,
existing attributed prior-art research, and primary license texts linked above.
Adaptation treatment and recommended release gates are engineering judgments.
No maintainer license choice, legal approval or new native access is inferred.

Independent factual review by the existing Sol reviewer verified all 38 package
instances and 79 notice-file hashes against cached published crates, Cargo.lock,
raw license declarations/VCS metadata, and the REM-50/primary-license summaries.
The review did not independently retrieve the restricted main-tree web view or
a sample docs.rs notice page; main-tree absence remains the author's GitHub API
snapshot, and docs.rs links identify published paths rather than verified browser
accessibility. The review is not a full copy/proprietary-code/legal audit. Internal
review coordination has no public transcript link. Package-fix source review is
recorded separately in its owning plan.
