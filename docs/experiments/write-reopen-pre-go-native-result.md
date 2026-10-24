# Pre-GO native write/reopen refusal and startup repair

The first composed write/reopen trial failed before holder readiness or a stroke
request. Stock was restored; a separately checked restoration returned the
initially active Buddy service. All 26 original files and the full 12-page order
remained unchanged. No target ink, close/reopen result or input qualification
follows from this attempt.

Later result: the [fresh-image writer](fixed-pen-spawn-owned-result.md) has
passed host/owned regressions and a separate native one-thread profile plus
owned-file process suite. Those checks do not change this failed trial or
establish native writing and persistence.

## Exact trial and observed order

The spent nonce is 20e93dbdabd64527a25d0e491b68535b. Source was committed
471688040920403884b74f863cdfa7580084e2c1; the ARM payload was
d4b757b009b5308582f395d114783eb9a76f7e1c664779de73d1e5b0ce1655fb.
The existing intended target was index 6 of 12; this packet did not insert a page.
The profile remained RM2, firmware 3.28.0.172, Qt 6.10.3 and systemd 255 with both
systemd-v1 and unified-v2 cgroups.

1. The packet's 26 staged files matched their checksums. The original independent
   actor paused Buddy and started the guarded research UI 6509/start 135046563.
2. The dedicated sibling pen unit ran the exact holder 6654. It exited 2 with
   “fixed pen holder admission failed.” No holder claim, ready record, GO,
   selected request or event trace was present in the original archive.
3. The actor observed the failed holder and began restoration. The candidate then
   aborted with SIGABRT and the vendor Qt assertion
   “n <= d.size - pos”, qbytearray.h594.
4. Guarded recovery restored stock 8194/start 135057962. The generic no-ready pen
   fence withheld Buddy rather than inventing released-input evidence.
5. A separate restoration first checked no request/GO/claim/trace, no remaining
   original processes and empty/absent pen/actor/Buddy groups in both hierarchies.
   The holder source enters device preparation only after its missing claim.
   The unchanged original Buddy definition and initially active state were
   verified before one start request; Buddy 8747 became active. No stroke was
   dispatched as part of that reconciliation.
6. A full stable post-restoration backup matched the pretrial archive byte for
   byte. Later stop requests targeted only the already empty failed test units.

The retained pen-unit start/exit times are 1350468119642 and 1350469655861
microseconds on the boot clock. The candidate assertion appears at 1350471.411755
seconds in the journal, after the holder exit. The restore-claim mtime was
06:34:29.768438494 UTC; the assertion's wall timestamp was06:34:29.877 UTC.
These measurements support holder failure → restoration admission revoked →
secondary startup assertion. They do not provide a native stack identifying the
exact failed admission branch.

## Reproduced source defect and repair

In the old startup, an earlier failed owner/claim/cancel check short-circuited
reading the deadline. The next expression nevertheless ran
deadline.chopped(1) on the empty array. Controller startup similarly trimmed an
unchecked self-identity result. Either path could abort instead of refusing.

The repair returns false before slicing a missing/invalid deadline and requires
a nonempty newline-terminated self identity. It does not weaken admission,
renew clocks, resume a spent request or change normal mode selection.

The exact-source owned fixture extracts the actual readPrivate/actorAdmission
functions and Controller identity fragment. Files are real owned files; the
self-identity fault seam is explicitly modeled. Host, ARM/QEMU with Qt 6.10.3 and
ASAN/UBSAN each exercise 16 cases. The old source produced 11 expected aborts;
the ARM run reproduced the same qbytearray.h594 assertion. Fixed source produced
no aborts and empty stderr. This establishes the defect and repair independently
of the still unproved exact native crash branch.

Fixed startup source SHA:
6425c2a5a76f6e6d26f21674c57eacccbfcb1e12843890b63efd2d94cfee3004.
The separate ARM link/fence check passes with synthetic bindings and no dynamic
exports; link artifact 572d2c4cf9426ae54043482e256cfa818717c6a90f69632fd60130e02842d641
was not executed on the tablet.

Portable owned inputs and checks:

    python3 tools/reader_write_reopen_admission_prepare.py --out /tmp/admission-inputs
    sh tools/reader_write_reopen_admission_test.sh host fixed /tmp/admission-inputs /tmp/admission-host

The ARM mode needs the documented matching SDK/QEMU environment. The optional
--old-startup input permits an explicitly supplied historical source; it is not
needed to test current refusal behavior. No full startup constructor, UI module
or tablet is run by these tests.

## Separate native thread counterexample

A later read-only program compiled the original holder's exact pre-device
source prefix. It opened four new private controls and called Context.start;
it never claimed a holder, waited for GO, instantiated NativeIO, opened an input
device or changed the UI/services. Its new nonce was
5b87b67f9ea04077886cc1ae3f5c9962.

That actual Qt 6.10.3 process reported three threads and
singleThreaded() == false, while all four file controls, candidate identity,
original clocks and Context.start/context.live passed. The four injection
environment variables were absent. This is a concrete incompatibility with the
original single-threaded Qt-parent prerequisite, and the leading explanation
for a pre-claim refusal. The exited trial holder's thread count was not captured,
so the subsequent profile is not retrospective proof of that exact branch.
The cause/names of the additional threads have not been established.

The successor [holder design](../../openspec/changes/establish-native-platform-contract/fixed-pen-holder.md#native-parent-threads-and-a-separate-writer-image)
requires a pinned Qt-free writer image and mandatory native spawn backend,
preserving one GO, original budgets, child fencing and independently held
recovery. Merely removing the old thread guard is not the selected repair.
The existing fork fixtures remain owned evidence; a replacement and native
event delivery are still unqualified.

## Evidence and limits

Private originals remain local; no firmware, document bytes, core or credentials
are published.

| Evidence | SHA-256 |
| --- | --- |
| Initial forensic runtime archive, 2,996,224 bytes | 9c8a52682ffe208a4a5f6494b4af723eba42894b9c5ba3f374bb3ae36f77b1de |
| Stable before/after document archive, 748,544 bytes, 26 files | b52f4a1b1dd55579069f63389e18d29a525e7b259f4500b8a70fe34216b163f6 |
| Independent original archive/transport audit, 402 checks | 98dbe63b24d867fd72b4bde965c33158f7164d7051c883b5bf7f1aa108b10b6a |
| Independent old/fixed startup review, 43 checks | 663dc635dfd57bdebc8608d7845f4b0ad150c362771bbf84fd26f0645f56fe08 |
| Portable current owned checks, 16 per host/ARM/sanitizer | e65ffb60f80a39157d1832fb21a1c6d20d141aed0a4d9927412d0742b73804b4 |
| Fixed startup ARM link/fences, unexecuted | 8bc40c83f150cbce2f544c4b102e3e4a8103a42d9fb0ffe2efd97498abe77652 |
| Native read-only admission-profile executable | 05586b57a68c9a07838328085d4702c72bb17b6cf9f300d32d388a83c7b9d0c0 |

The failed trial did not reach intended-page selection, pen readiness/contact,
positive release, three captures or ordinary close/reopen. Full source/device/
firmware qualification, fault recovery, resident native ownership and supported
input capability remain open. The fixed startup and proposed writer replacement
must receive their own exact-source/artifact admission before another trial.

For simulator calibration, preserve independent holder failure, revoked
admission, secondary application failure, stock restoration, conservative Buddy
withholding and later no-GO reconciliation. Do not collapse this into either a
successful write or a single generic timeout. The SDK's focused regression now
covers the empty-array abort; integrated Buddy simulation has not been changed.

Source basis: original current-trial streams/archive and full backups; current
source and frozen exact-source owned outputs; subsequent native read-only profile.
The specific historical abort branch and original holder thread explanation are
explicit inferences, not direct native stack observations.

