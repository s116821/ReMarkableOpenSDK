# Native map mutex reuse after rejection on RM2

October 23, 2026 UTC. The requested-mode successor completed the negative native
insertion path, then successfully acquired and released the original final map
mutex once after the original stack unwound. Full fixture preservation and stock
restoration passed. No insertion was forwarded or page added.

This answers the specific mutex-reuse question left by the
[earlier negative witness](insertion-guard-native-negative-result.md).
The Core remains unconditionally negative-only; conditional permitted forwarding
and accepted insertion are still unimplemented.

## Candidate and observed result

Source `096b211ebef5d4d141f1c9111563b0653e2434da`, spent nonce
`e2c2cc7e879b4fc599ef62b62d9aa79b`, RM2 firmware 3.28.0.172. Do not replay it.
The actual candidate used the separately reviewed nineteen-input ARM build with
`READER_INSERTION_POST_REJECT_PROBE=1`.

| Evidence | SHA-256 |
| --- | --- |
| Actual payload | `2f14dd023f6fe25cb59aeda8cc67886e8004109a98d5a129ad60823ba9df016f` |
| Frozen candidate | `d409f366882a3df9bc4e6dd23ccc8160b703ea074043a22d918ef903ff786606` |
| Original 1,255-byte wire result | `e6d1d0b8e19642c774419d7175a9f4782ff645e7ddf4304f611e303e00e17c76` |
| Independent artifact/packet review | `207dc8ed1713257032c5f18ed290bd4219ae876652d6b96f4fe6bc15b5aedadb` |
| Independent controller/readback-source review | `1f1441c9f8318344096a8433e0c73d8f143dce93243da55ad262a3ff21970a5d` |
| Combined source/artifact admission | `4c7bcf35eb62202d398cf6d449a238e3e268316f94efb0f14f6f364eba5fd8b5` |
| Independent saved-result/readback review | `2405ef9e66270e1eb72aa8f11522d663425bab4af4109465877678480b9bab80` |
| Independent runtime/source review | `563b37c7206e42e6de1ac825995a796414249c4eb631890c004fcba49bc3af15` |
| Exact inactive-root cleanup execution | `9b0f3df5348dcd27dad42706c7c08ec3883a8f3a0d8ddbf2a66b409f5bb6fb60` |
| Independent cleanup execution review | `b70e79f78a053526cc19ad8aad839521b7d78ea67eb94bde34fa6172ca86fb41` |

Static inspection verified the actual initializers, eager binding, unchanged
provider identities, 305 required imports with no missing names/versions and
byte-identical reconstruction of all twenty packet directory files and arm
script. It separately checked the requested factory path and actual compiled
trylock/unlock sequence. This is distinct from the
[owned controls and default-mode build](post-rejection-map-lock-owned-result.md).

Native startup reported exact profile qualification and application absence at
installation, with two observed tasks. Automatic navigation and source joins
matched in one process/window/engine. Probe job 25 and operation job 26 were
distinct; the active-job snapshot matched 26. One outer and one inner call
produced the deliberate source-mismatch witness and matching private catch,
zero tracked forwards, one ordinary callback and matching completion, and
pageAdded=0. There was no cancellation, interference, refusal or unknown result.

The additional seven-field result reports required/attempted/acquired/released
true, try_result=0, unlock_result=0 and native_authority=false. The strict consumer
requires these facts as well as the entire original negative conjunction; an old
receipt cannot pass. The plan ran from 33,615 to 45,505 ms on the original setup
clock, including the fixed navigation opportunity. This is elapsed asynchronous
work, not one continuous GUI pause. No clocks were renewed.

## Restoration and preservation

One stop was acknowledged, and the original independent actor completed primary
restoration. A new stock process was verified. The runtime archive is 8,443,392
bytes, SHA-256
`b592f7f7419adfb12204283cfb842dc61b4f259fb476e33d418dec79f673a753`.
Its 72 regular files and 52 raw transport records were independently checked.
Seven temporary guarded result-read polls exited 90 with empty streams before
the final result. Their transport unknown flags remain preserved; the later
valid process-bound wire establishes the native result without replaying the
publisher or operation. Not every transport is claimed known-successful.

Fresh full before/archive/after readback found all twenty fixture files unchanged,
all nine original page objects/order preserved and the proposed target absent.
Twelve provider/unit records plus pthread, unit contents and service policy
match. Stock, Buddy and sync are active/running with zero restarts/jobs; no trial
mappings or runtime shadows remain. The complete fixture archive retains SHA-256
`a957667be0f30359870fdf998f29d350f1e73f3b22c499b2c229e05cb6807354`.

After independent cleanup-source review, the final full archive and manifest
matched the preserved runtime files. Exact nonce/owner/root, restored process,
inactive actor, absent old processes and runtime-shadow guards passed before
removing only the inactive trial RAM directory. Its final archive was byte-identical
to the original runtime archive. Local source, logs and failure evidence remain.

## Scope and next question

The factory admitted the original nonrecursive mutex at worker+176, with kind
zero, complete writable/nonexecutable storage and retained native owners. The
probe made one trylock and immediately unlocked on success. The bounded native
result establishes one reacquisition/release after this particular unwind. It
does not prove every cleanup, all writers' exclusion, physical owner release,
safe unload or a generally qualified adapter. A busy result would have been
inconclusive about the mutex owner; it was not observed here.

The mismatch was deliberately constructed in the frozen negative predicate;
ordinary creation retained the real source. This does not show that the Reader
actually became stale. Native/render/durable authority flags remain false and
native owners remained resident until process termination.

Next is a separate correct-source conditional gate, including full-tree target
absence and effective requested-ID equality before forwarding. Native void return,
an ordinary callback or pageAdded alone cannot establish that the requested page
was inserted or selected. Supported insertion and integration remain open.

Source basis: current exact source/build and independent static reviews, original
process-bound result, preserved runtime archive and full fixture/provider/service
readback. General cleanup and future capability conclusions remain limited as
stated; proprietary artifacts and document identities stay private.
