# Intended-target native write and separate readback

Status: one RM2 development profile; actual input and before/ink capture observed.
The combined sequence expired before its last reopened capture. A fresh capture-only
readback is pending. No supported public capability or whole-sequence success follows.

## Frozen trial

- Device: RM2, firmware3.28.0.172, unchanged qualified providers.
- Source:0418baebe3a19e9b3fbfc93ff7c4d025aec5cfb6.
- Nonce:49251774b42a4e5aa8ba89db11d598aa, spent; never repeat its stroke.
- Payload:6ef5d44a1bacb1efff6c46bfa52e5961df6b9167f620625d0b4f379258df0400.
- Candidate12243/start136306762; holder12382/start136306996;
  separate writer15089/start136312502.
- Existing target: index6, page74ddda38-0a48-4100-931d-80196312d27d,
  document e7f661f1-db6f-4dfc-854a-b38aff7f75de, unchanged12-page order.
- Original90-second candidate,360-second actor and185-second recovery reserve.

The [startup-cost investigation](fixed-pen-spawn-owned-result.md) establishes
why the old one-second inspection and premature host cutoff were insufficient.
The qualified source uses the existing20-second observation maximum without
removing ownership, lifetime, notification or cancellation checks.

## Actual input and image evidence

The original holder result accepts178 written records in56 frames. The actual
kernel-observed stream contains169 records/55 frames/53 contact frames, exactly
one contact interval, both fixed endpoints and final release. Preserve accepted
writes and observed records as distinct facts. Independent recovery reports
armed/attempted/written/observed; final snapshots are released. The child exited0
and is gone, holder exited0, all device/IPC descriptors and trace closed.

Original pen JSON:
3968713109373d7b3a276f1aa9bda68f3221353b6b977cf477beaa5e573b8678.
Original2704-byte trace:
db9b2982575c4fb6e1e641e072f9afd832f1999ce264464e018496b1be5d2c21.
The current production stream policy independently replays those untouched
records on host, ARM-QEMU and ASAN/UBSAN. Receipt:
a49655756a9bc4fac196c63cd30b1184b422b43428fbe87927481362bc86d863.
That offline replay does not recreate external live process/device admission.

The before and ink pairs both identify the exact intended document/page/index/count
before and after acquisition. Their original scalar hashes match the sequence
report and their PNG lengths/digests decode correctly. The ink image visibly
contains the new short black stroke. PNG digests:

- Before:055cf96da4c1b5c4174004eea7069c1a321aab8cfd866aa49b583f128e8fab5e.
- Ink:86bfad35e8178d26772b2b8daf298e9bde2821fd35d3a77fd32a9eeb1799c668.

The15-check pair/preservation review is
a98164f13f61c2b970ea5b861490d09966425d5371fe1ded7af28d4cd08a0e3b.
Its249 changed pixels include toolbar state as well as canvas changes; the
whole-image difference count alone is not an ink detector. Original images,
JSON, input records and private fixture exports remain local.

## Stock close, incomplete final capture and preservation

Stock ordinary close dispatched and the closed composition was observed on its
first poll. Reopen also dispatched. The90-second setup expired before the final
reopened pair was produced. The original report remains observed=false,
refusal=setup-expired, reconciliation_required=true; there is no complete
18-file frame or manufactured successful sequence.

Full runtime archive:
d8a9163faddce4790bece72de0a1c6017ae0cedc60f3294e0da0c6053a1a7644.
Stock16528/start136317735 and initially active Buddy were restored automatically;
all three services are active. No recovery.failed marker was present.

The full26-file backup preserves all12 IDs/order and22 original file bytes.
Only intended target.rm, its thumbnail, and known content/metadata bookkeeping
changed. The content changes are pages/6/modifed and sizeInBytes; metadata changes
are lastModified and lastOpened. Unknown metadata, all other original ink/PDF
bytes and the local file are preserved. Backup749056 bytes:
bb6cdbffdc51aa04be6767327c500547b384de382016689414c16c7b28f15262.
This is stable stored data after stock restoration, not physical-power-loss proof.

## Distinct next question: existing-ink readback

Use the existing capture-only startup, Entry::NavigateIdentityCapture and
identity/pixel pair reader in a fresh fixed window. Reopen and observe the
existing ink without publishing any pen GO, drawing another stroke or inserting
a page. Compare the readback's observed identity/image and original ink file
with this preserved result. Do not treat a late capture as completion of the
spent sequence.

The capture startup now refuses invalid/empty deadline bytes before slicing,
using the exact admission functions already exercised by the16-case startup
regression. The standalone reader resets/unblocks a12-second alarm and uses
nonblocking output with repeated original admission checks. Actual owned output
fixtures cover partial writes, full-pipe revocation, closed peer and inherited
blocked/ignored alarm; ARM-QEMU skips host-kernel signal proof. Existing104-scenario
own-process/pair/control tests remain separate from native authority.

The current output/process test receipt is
2a7d791f3007e779c712ab6030c88acd48bb602f9c7ea27e18dc1f9c86882187:
host/sanitizer4 output cases, ARM-QEMU3 output cases, plus104 process/control
scenarios per environment. The first container lacked the CHOWN capability for
the existing foreign-owner negative fixture; its failure remains preserved.
The successor grants only that capability inside the isolated owned test.
Source review33 checks binds the exact admission functions, removes only new
bounded-output/alarm operations to recover all prior reader tokens, and binds
the complete15-input ARM capture link and4 fixture/11 selector fences. Receipts:
c5d2085090933d351ecce25ac2c06fffd6fddf01ef0ac463eea9fb09693c39d6 and
604114f78f0561d82eefced4d794598ed3521559f6c728067125fd21894c048b.
These source changes have not yet been selected for a fresh native readback.

Source basis: exact source/artifact/provider bindings, original native pen/image/
sequence streams, independent offline replay, visual inspection and full stable
storage comparison. Native final readback and broader failure/lifecycle/product
qualification remain open.

