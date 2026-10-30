# Logical cancellation during a native callback

October 23, 2026 UTC. One fixed RM2 diagnostic canceled its logical registration
while its native worker callback waited, retained the native receivers, then
observed six expected passive reads and one matching late completion. The
registration remained Quarantined. Stock xochitl was restored, all twenty
document files stayed byte-identical, and the inactive temporary root was removed
after complete archival.

This is positive evidence for this cancellation diagnostic. The canceled request
did not become normal success, cancel native execution or release its owners.
It does not establish production cancellation recovery or safe unload.

## Frozen inputs and controls

Source is `031959d6c3ebfd3839dfb62a84b6f945702cbc6f`; attempt is
`6289c2759b8c4b3f8d67d35a95d07499`. The exact firmware 3.28.0.172 executable
matches SHA-256 `071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df`.
The [design and owned checks](../../openspec/changes/establish-native-platform-contract/worker-cancellation-witness.md)
retain the distinction between the fixed parent diagnostic and its canceled token.

| Artifact | SHA-256 |
| --- | --- |
| Payload | `cd7c59147da5969ad0999538b3b3ad5998486da6b68ce8dd92bf2ab1cfe55b8f` |
| Candidate | `768b04618620025ef6a606bf9d8b89dc1a673a7fc55ba939bd4c45032da28a66` |
| Preparation | `c0289e8d96149ebfad9ada78776e433047e1b46716dccadf84ad5cd1f7ec52e6` |
| Controller | `d78ac75ccf084fd55fed3096435381e83878afe96e4a59eb29db6856551d0ccd` |
| Sequence | `ef273c031b192c59a226e86d24db5c35a233383437715f9db433d5aef3a050ea` |
| Independent package review | `981491458be0a72f2860e6b06c0c8b429e90f0486c342245b6c0a65ad4d1715e` |
| Independent saved-result review | `ecc726750a94c24b82d02987baef17baafc91411b41be8a83163b4e076c1f1ec` |

Review independently rebuilt the ARM payload byte-identically, reconstructed all
twenty packet files and the arm script, checked thirteen Git inputs with three
CRLF-only working-copy differences, exercised 494 negative wire vectors and ten
sequence scenarios, and resolved 254 required global symbol/version imports
against eleven exact providers. Owned controls passed 318 quarantine checks on
host, ARM/QEMU and host ASAN/UBSAN, plus 31 Entry modes on host and ARM/QEMU and
both compile-only startup variants. None substitutes for the native result.

## Actual observation

Actor PID 26097/start 125914896 admitted the attempt once. Candidate
26799/start 125917599 used the ordinary fixed fixture bootstrap. The fresh GUI
join identified the configured center page at index 5 of 9, ready/coherent with
the expected creation-controller relationship, status 2/page status 8 and no
loading, error, tile or work blockers.

The callback entered once off-GUI and announced its plain waiting marker. After
the GUI source Sample and registration Frame unwound, the diagnostic canceled
the token while the callback had not returned. It observed Quarantined phase 7
with retained owners, then granted continuation of the already-authorized six
passive reads. Those calls separately matched the configured IDs and inverse
indexes 4, 5 and 6. Configured neighbor expectations came from the full preserved
fixture, rather than fresh individual GUI neighbor observations.

Returned job ID was `25`. The final facts were:

| Fact | Observed |
| --- | --- |
| Callback entered / returning / off-GUI | 1 / 1 / 1 |
| Normal completion / matching late completion | 0 / 1 |
| Duplicate / skipped / registration depth | 0 / 0 / 0 |
| Phase / refusal | Quarantined 7 / `diagnostic-cancel-while-running` |
| Native submission accepted / unknown | true / false |
| Planned cancellation / while waiting / work observed cancel | true / true / true |
| Work wait expired / request released | false / false |
| Owners retained / closed / holder destroyed / settled | true / false / false / false |

The parent published `worker-cancel-observed`, with empty parent refusal and all
native/render/durable authority flags false. Process and window/engine remained
the same. A matching late completion did not reactivate the token or reclaim its
native references. Process-resident quarantine retained them until process exit.

Plan timestamps span 33739..44502 ms, or 10763 ms, including bootstrap and the
fixed five-second loading opportunity. Source Sample/preparation spans
42097..44494 ms, or 2397 ms; cancellation is at 44495 ms. These are GUI diagnostic
timestamps, not measured worker wait/accessor duration or continuous UI pause.
The strict two-second hold remained unexpired. The original 90-second setup,
30-second plan, 360-second actor and 185-second recovery reserve were unchanged.

## Restoration, preservation and cleanup

There was one original stage/arm/request/stop sequence, no retry; sample and stop
returned zero, and the actor's primary exit was zero. Fresh stock
27915/start 125924093 was verified on the same boot. All three monitored services
were active/running with zero restarts and no jobs; no diagnostic mappings or
temporary service shadows remained. All twelve provider/unit entries, unit bytes
and service policy matched the baseline.

The complete twenty-file backup contains 737280 bytes, SHA-256
`a957667be0f30359870fdf998f29d350f1e73f3b22c499b2c229e05cb6807354`.
Before/archive/after inventories agree. Every file matches the pretrial backup,
including all nine full page objects/order, PDF, ink, thumbnails, content,
metadata and unknown values. No files were added or removed. This is sampled
preservation evidence, not SDK durable authority.

The runtime archive has 475136 bytes, SHA-256
`8c46fa546e121af9e8b3863b77596bdf79f961fe807b818076b78547eaf453d3`.
Before cleanup, all 72 current regular files matched both the final complete
archive and the preserved primary archive. Repeated guards verified the literal
owned root, permissions, nonce, inactive actor, absent old processes, exact stock
incarnation, same boot and absent service shadows. Only that inactive RAM root
was removed; cleanup made no service changes. Private streams, archives and
failed/superseded local harness runs remain preserved.

## Integration boundary

The [normal worker result](worker-local-order-result.md) established one normal
release path. This test separately establishes retained-owner behavior after a
planned logical cancellation. Neither proves native cancellation, physical
queue drain, general reclamation of quarantined work, arbitrary repeated resident
requests or code unload. A retained slot cannot be called released because a
late completion arrived. The experimental process restart is forensic recovery,
not a per-conversation production strategy.

The [execution-time order boundary](../research/execution-time-page-order-boundary.md)
still prevents promoting these six independently locked reads into atomic
creation authority. Native create-after, target render/capture/write and the
full consumer acceptance matrix retain their separate requirements. Shipping
capability remains unsupported; the earlier shutdown rendering fault is not
claimed repaired by this clean restoration.

Source basis: exact frozen source/artifacts and independent package review,
fresh native wire and original control records, complete bracketed post-trial
readback and guarded archival/cleanup receipts. Private page IDs, firmware and
raw archives remain outside the repository.

Independent saved-result review verified all 48 raw controller records and eight
bracketed readbacks, the exact 1709-byte wire and frozen validator, original
once-only control/deadlines, full preservation, restoration and cleanup. Six
empty result-collection polls carried local 90/unknown diagnostic records; they
were not unknown native submissions or repeated requests.
