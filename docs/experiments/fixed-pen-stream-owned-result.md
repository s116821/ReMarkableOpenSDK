# Fixed pen stream: owned record validation

The original fixed stroke now has a separate, sticky observer-state validator.
It accepts filtered unchanged values and arbitrary record-batch boundaries;
it does not require a native observer to echo all178 constructed records.
Snapshots and records here are owned models, not tablet input observations.

[The policy](../../scripts/reader_fixed_pen_stream.h) separates possible raw
contact, complete contact frames, endpoint observations and a successfully
finalized released-state match. Unknown, dropped, malformed, repeated-contact,
partial or over-quota inputs permanently refuse. Raw contact possibility is
retained even after refusal or quota exhaustion. Finalization is one attempt;
a failed state match cannot be retried into success. A matching empty or
release-only stream never becomes a stroke.

Frozen original host, vendor-ARM/QEMU and ASAN/UBSAN runs each passed257 scenarios
and51,413 checks, with identical stdout and empty stderr. Controls include all177
cuts of the original178-record carrier, filtered and redundant events, incoherent
frames, endpoint omissions, raw contact pulses before SYN, held/partial contact,
eleven conflicting snapshot fields and the8192-record limit. Source and fixture
remain separate from the [dispatch/recovery policy](fixed-stroke-release-owned-result.md).

Stream source SHA-256:
`eb79c6b4e6e3f74fd31f31022a1c0d66ac451707a37765a016e0785ce69158e7`.
Fixture:
`ac600b56ed24e60e432f934e913d65e26d92a2fc157a834f188e208ce077cbab`.
Runner:
`d857804ea9c9bdb427800b83e090e05e2f3dbfd208c7b683c6a60b6d4297fdfd`.
Original36-check audit receipt:
`98666c4d44b170f7aef4c7ee1a58174d0e294644f7554990d258338f0f94def0`.
Independent Root60-check source/original-byte audit:
`43a0beb6eb009b2f5f59958ccb2aa733c8536073dc79748d3d814bc7691661b3`.
Root's first verifier assumed the transport intent used an args key; its failed
lookup is separate from the passing execution matrix. The corrected verifier
checks the actual argv records. Private raw receipts have no public download link.

This component performs no input_event serialization, device open/read/write,
clock or queue observation. The caller must separately establish native profile,
framing, identity, original deadlines and quiet/fresh kernel state. A completed
endpoint/contact conjunction does not prove the exact52-step trajectory, sole
writer, Reader routing, visible ink or persistence. A copied report is a historical
fact; supplying more input later cannot retroactively revoke copies held elsewhere.
Standalone native holder, independent recovery and intended-target write/reopen
qualification remain separate work.

Source basis: original owned implementation/fixture, exact frozen inputs and
original three execution streams/binaries/receipts; independent byte/source audit.
