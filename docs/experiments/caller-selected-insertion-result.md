# Caller-selected insertion: one RM2 result

October 8, 2026 (America/Chicago). One explicit disposable-fixture trial on RM2
firmware 3.28.0.172 honored the supplied sixth page-UUID argument. The new identity
persisted at index 1 after stock restart and ordinary UI reopening. This establishes
one allocation result, not a production insertion capability or shutdown repair.

## Reviewed inputs

The [discriminator](../research/client-selected-insertion-discriminator.md) used
SDK `1331b577e2aae3d66cfc6d42a8e185f1afcc7d82` and consumer recovery
`b74493782df714117e4553afec7fc17281a98b51`. Main was the sole tablet operator.
The original creation entry, creation bridge and inherited dormant page-open
linkage were built with vendor Qt/MOC; no page-open configuration or shutdown
diagnostic entry was selected. Six focused ARM/QEMU creation checks and 19
independently repeated Linux recovery/coordinator checks passed before selection.

The ARM32 hard-float payload was 237984 bytes, SHA-256
`abcb1ddc90efddb2c06d560e419acf9fb0cf30d2fa565d7b65d197a4615d2e4c`.
Independent review matched all 41 exported source files to SDK Git objects,
configuration/build hashes, compiled identities, packet manifests, exact rendered
recovery templates and all 24 retained fresh baseline records. The partition was
ten runtime providers, twelve immutable document files and only content/metadata
declared mutable. Main's provider audit reported all 228 required symbols and
versions available. Private configuration, document identities and raw artifacts
remain outside this repository.

## Observed allocation and preservation

The saved creation receipt records an accepted mutation claim, boolean true return,
one callback, no duplicate callback and completed helper cleanup. Its own
`durable_success` remains false and `effect_requires_reconciliation` true; those
fields were not promoted into an SDK native receipt.

Independent before/after archive comparison established:

- Six pages became seven, with exactly the requested target at index 1.
- Removing that target leaves all six original page objects exactly equal and in
  their original relative order.
- All twelve immutable files remained byte-identical, including PDF and existing
  ink. Only the new target's page file and thumbnail were added.
- Content differences were limited to pages, last-opened bookkeeping, page count
  and size. Intermediate metadata changed the current-page index from 0 to 1;
  after returning to the source it was 0 again. Final metadata differences were
  only last-modified/last-opened bookkeeping; unknown fields were preserved.

After restart, ordinary UI reopening displayed the target as blank Page 2 of 7.
One ordinary SDK Previous gesture restored the annotated original Page 1 of 7.
The author independently inspected both final captures. This was development UI
operation, not qualification of an SDK native-open or active-page authority API.

## Recovery and unresolved lifecycle behavior

The wrapper exited 1 in `observe-insertion-stop-request`: Main's manual stop
request arrived after restoration had begun and was refused before writing.
No request or insertion was retried. The independently armed actor's existing
deadline recovery restored stock; candidate stop reported `Result=success`,
`ExecMainCode=2`, `ExecMainStatus=15`. No SIGSEGV was observed in this trial.

Nevertheless, the first restored UI was Library with an unexpected-close/Reopen
banner, independently visible in the capture. Its cause is unproven. Successful
service restoration and one SIGTERM outcome do not establish clean document UI
restoration, explain the historical 0404 fault or qualify shutdown safety.

Main initially tapped a transient banner and opened a different existing document,
then closed it without ink changes and used the persistent fixture tile. This
operator recovery is retained as a limitation, not described as a clean automatic
round trip. Main's final receipt reports stock services active, original policies,
no owned overrides/jobs, the transient actor absent, task-owned files removed and
the preexisting fixed screenshot restored byte-for-byte with its original mode.

## Evidence and interpretation

Retained private archive SHA-256 values:

| Evidence | SHA-256 |
| --- | --- |
| Actor receipts | `3c5e908f748dd20a4ca042a606b05240b78a64e1b835069ee28f32877f66daa7` |
| Complete final evidence | `bd58117ee17dbb71dc753c1aba479ed18e65835d76596bd2185c63de0603a95f` |
| Final fixture archive | `aca7f80f445221ef766abbd684e4f54360ceafd756de1e2aa9d64db072ce68c1` |

The result supports caller-selected identity for this exact firmware, fixture and
one-call path. Retry idempotence, atomic source/order comparison, durable consumer
reconciliation, notebook coverage, crash durability and production lifecycle
qualification remain open. The six-page configuration is now spent; the preserved
fixture has seven pages. No new trial is selected by this result.

Source basis: actual Main receipts and closeout, independently inspected local
archive bytes/page records/immutable files and screenshots, exact Git/configuration/
artifact/packet checks, and focused test outputs. Final ordinary UI actions and
device cleanup were performed by Main; the author inspected retained captures and
cleanup output rather than operating the device. Private evidence has no public
download link. Hashes identify retained evidence without publishing its contents.
