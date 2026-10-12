# SDK licensing implementation

October 12, 2026. The maintainer authorized a standard open-source license after
withdrawing the proposed mandatory-upstream-merge condition. Root selected
GPL-3.0-only to follow Buddy's GPLv3 direction and preserve the adapted gesture
provenance; the maintainer then explicitly confirmed GPL-3.0-only. This is the
approved SDK grant; no further license preference is pending.

Use the unmodified GNU GPLv3 text from https://www.gnu.org/licenses/gpl-3.0.txt as
LICENSE. Cargo metadata and README explicitly grant GPL version 3 only; the license
text's illustrative later-version boilerplate does not change that grant.
Third-party dependencies keep their original terms. Retain the exact license and
notice files from all 38 published registry package instances in the existing
locked all-feature inventory, deduplicating only byte-identical texts and mapping
every original file/hash to its full text. Preserve the Buddy gesture source
revision/provenance and add its explicit GPL-3.0-only SPDX marker.
Original notice CRLF/trailing spaces are preserved; a file-specific whitespace
attribute avoids rewriting them, while focused SHA-256 checks enforce their bytes.

Add LICENSE and THIRD_PARTY_NOTICES.md to the exact reviewed package list. Keep
publish=false and experimental API/unsupported native statements. No forced merge,
third-party relicensing, package publication, native compilation or held-operation
continuation follows.

Focused checks verify canonical LICENSE bytes, Cargo license/publish metadata,
all notice hashes/text mappings and package membership. Existing Sol reviews the
increment independently; REM-50 receives the exact updated package contract.
Task 1.4's license/dependency/experimental API publication policy can close after
these checks; native adapter, release integration, source arrival on main and
full delivery remain their existing technical gates.

## Acceptance

- [x] Canonical license, explicit grant and all 79 original notice mappings/39 byte-distinct full texts verified; current lockfile package pins must match the notice inventory.
- [x] Three focused package/metadata/notice tests PASS locally and independently (latest independent run 0.987s); existing Rust 1.88 ordinary consumer, format and strict OpenSpec checks PASS. No package archive or native build was performed.
- [x] Exact 15-member contract prepared for Root/REM-50 relay: eight Rust sources; Cargo.toml/Cargo.lock/README/LICENSE/THIRD_PARTY_NOTICES; Cargo-generated Cargo.toml.orig/.cargo_vcs_info.json only.

Author retrieval from GNU succeeded: LICENSE is 35,149 bytes, SHA-256
`3972dc9744f6499f0f9b2dbf76696f2ae7ad8af9b23dde66d6af86c9dfb36986`.
The independent review's fresh GNU retrieval failed; it validated this recorded
baseline and exact staged/local byte equality instead. Notice bundle SHA-256 is
`f60789d8c5efd8b6a265e5ebd56528b19b8c50332c44b8d1470f454f0d77492f`.
These evidence limits do not leave the authorized license choice unresolved or
introduce an additional general legal-approval gate. Technical release integration,
qualified native capabilities and the overall SDK/OpenSpec delivery remain open.
