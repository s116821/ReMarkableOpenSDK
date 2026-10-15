# Verification and provenance

Actual main49709c78, published precode6f31c191503c10f59d4ba47f523c49c5cb86cf7f precedes code. release/stage_source.py is byte-identical selected72136ed, SHA256aa102242a17e4941a2caca18d618f9fea3519b044b9d966915063c25816f9c8a. release/test_portable_stage.py is also byte-identical, SHA25699cae545f8d00b095ec6b74baeb06b038f6e8e5d4f902734f4d646c40e47277d. No helper implementation changes.

Nine original source-admission test methods and two original candidate-admission methods retain exact Python AST bodies; the setup cleanup adaptation is recorded below. Test methods retain those bodies from selected test_stage_source.py/test_candidate_stage.py. Imports/file names are narrowed to this scope; the separate Cargo/cross/archive and maintained semantic-release composition classes are not extracted, disabled or represented as passed. They remain in the older candidate. Two portable tests retain exact original file bytes.

Local Linux: nine source-admission, two candidate-admission and two portable controls PASS (13 total, zero skips). Exact existing tag/source and staging-only version, dirty/mismatched/source/workspace/lock version refusals, destination preservation, local override/source drift, candidate official-admission refusal, LF/CRLF and POSIX keys/target exclusion are covered. Disposable local fixture tags are not production SDK versions. No source owner, Cargo SDK compilation, native, remote release/tag or publisher operation. Stage helper's retained distribution verifier is not newly qualified by these admission tests; its former exact-source qualification remains separately attributed.

Actionlint1.7.12 workflow parse PASS with external shellcheck disabled; strict OpenSpec7/7 PASS; diff checkPASS. Repaired current Linux/Windows staging run38760424322 passed all13controls on each OS. Independent source/spec review5535696217 accepteddd426ea3 with no findings; own canonical/archive synchronization is complete here. All runtime/Cargo/package/consumer/identity-workflow/capture and existing canonical/archive bytes preserved. No historical full-SDK substitute or selector weakening. User Git-tags-only version policy preserved; helpers do not calculate bumps or create tags.


## Preserved first hosted failure and narrow repair

Initial sourceba9a3a run38760085773 appears successful in job metadata, but actual Windows116508746356 logs contain nine source-admission cleanup errors: Python shutil.rmtree cannot delete read-only .git objects (WinError5). The default PowerShell multiline script then runs the next commands, and its final successful Git command masks earlier native-process failures. Linux116508746043 passed13 controls. Windows candidate/portable subcommands reported2+2 passes, but the aggregate is NOT accepted as a Windows pass. Original complete hosted logs remain available; no waived cleanup errors or metadata-only green claim.

Narrow repair: selected source test assertions remain unchanged; setup uses Python TemporaryDirectory cleanup, already used successfully by the original portable fixture, to handle Windows read-only Git objects. The workflow explicitly selects bash so GitHub runs the script with error propagation and stops on the first failed test command. Stage helper and portable test bytes remain exact selected source. Repaired current hosted logs were read and pass: Windows116509932606/Linux116509933495 each9+2+2OK under bash -e -o pipefail. Tested merge4655c50 includesdd426ea3+49709c78; this is not a manual rerun of unchanged code.


## Accepted source and own lifecycle

[Independent source/spec review5535696217](https://github.com/s116821/ReMarkableOpenSDK/pull/13#pullrequestreview-5535696217) accepts only source admission/portable identity atdd426ea3. Current source required SDK38760423753, RM1baseline38760423747 andmetadata38760423741 also succeeded; no SDK compilation/distribution/native acceptance follows from this staging scope. Initial failing Windows evidence stays retained above.

Exactly five own change files are archived; proposal/design/delta retain exact blobs, and only own tasks/results receive completion/evidence changes. Canonical two requirements/three scenarios copy the accepted delta unchanged with Purpose/Requirements headers. All unrelated mode/type/blob entries, including runtime/Cargo/capture/consumer/workflows and prior specs/archives, remain unchanged. No implementation tests rerun for this documentation-only lifecycle. The actual new-main composition after capturePR12 merges requires its own current checks and final preservation review before Root's normal merge.


## Actual main composition after capture delivery

Compose the accepted own lifecycle e2d05a51 with actual merged capture main2e0377f1a071d8a8d2e0b4cfb24f73bef486d4ce. The staging helper, all three control files, own workflow, canonical requirements and archived proposal/design/delta retain their exact accepted blobs. Every actual-main entry, including capture runtime/Cargo/optional dependency acquisition, package/consumer checks and canonical eleven-member inventory, is preserved byte-for-byte; only this own evidence text is added over the prior lifecycle. Strict combined specifications8/8 PASS. No local implementation matrix rerun; required current hosted composition checks and final preservation review remain before normal merge.

Diff against actual main is clean. A broad parent-to-merge diff reports an existing trailing blank line in the newly inherited capture archive delta; that actual-main blob is preserved rather than edited incidentally. This is an inherited formatting observation, not a waiver of any staging or runtime test.
