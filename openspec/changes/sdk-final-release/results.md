# Verification and activation hold

Precode f43ee3987619e55e0775b1e1e33558fcd6db56fa precedes implementation and is based on actual main d77c58bbd856ea5357d73beec6386feef3369b75. The selected final job comes from retained 72136edc94a95706fe1d1c7cb56a8bb6c5ffee4d, adapted to current trusted artifact outputs and compact graph.

## Local observations

- Fourteen maintained YAML/GitHub-expression controls pass. Existing compact-stage guards, exact artifact/receipt/provenance mapping and SDK consumer checks remain; new controls cover final main-only admission, all four direct predecessor failures/skips/cancellations, workflow cancellation, PR/docs refusal, exact retained inputs, no credentialed rebuild/probe replay, missing-only/original-ID finalization and native queue grouping.
- Two focused nonproduction controls pass in 1.188s. Actual pinned Octokit GraphQL and request Action bundles execute against an owned loopback API using rendered actual workflow inputs. They perform exact-tag absence lookup, explicit draft creation, original-ID lookup and finalization with the expected route/body. The actual replacement-ID shell admits55 and refuses56. No real provider release or credential is used.
- Initial loopback execution failed because the isolated fixture omitted required dummy GITHUB_ACTION metadata; the original output is retained in /tmp/rem50-sdk27-actions.log. Adding that metadata repairs the fixture; no upstream source, assertions or production input was weakened. Corrected output is /tmp/rem50-sdk27-actions-corrected.log.
- The Release job receives only the already-tested SDK and uploader artifact IDs, receipt and trusted provenance digests. It uses SDK verify mode, not integration/probe replay. The original maintained create-only uploader and read/download-only adapter/verifiers remain unchanged. Only the final Release job receives contents:write.
- Native queue:max serializes entire main workflows rather than only final tag mutation, with independent PR run groups. Official documentation bounds this to100 pending entries and waiting-time FIFO rather than dispatch order; candidate/tag conflicts refuse instead of retagging or rebuilding. No custom queue or new activation variable is added.
- Strict OpenSpec validation passes21/0 and diff checks pass. Full local actionlint1.7.12 DOES NOT PASS: it reports the known unsupported queue property, preserved in /tmp/rem50-sdk27-actionlint-final.log. No diagnostic is suppressed or validator patched. Actual hosted workflow schema acceptance/current checks remain required.

## Remaining source and activation gates

Current-head hosted PR workflow validation, independent implementation review and this change's canonical sync/archive remain pending. PR events must skip the real Release job. No unchanged local SDK/tool build matrix was repeated.

Merging this draft activates the first real publisher. Source-only acceptance or green PR checks do not clear that effect. Main/coordinator must explicitly clear the already-preserved first-official/protected-remote owner scope before normal merge. Read-only repository queries observed immutable releases disabled and no rulesets; this lane changed no settings and copied no admin/user token. The immutable-setting query itself needs Administration(read), so no CI credential workaround is proposed.

No production tag/release, experimental provider operation, source version field, Buddy pin, device action or native qualification is claimed. Keep the PR draft and unmerged until source/lifecycle checks and explicit activation clearance are complete. REM50 remains In Progress.
