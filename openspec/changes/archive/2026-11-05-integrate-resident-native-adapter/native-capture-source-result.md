# Resident capture source integration, October 28, 2026

This checkpoint implements the guarded producer, revision-3 transport and Rust
historical image consumer in the existing integration change. Task 3.0b remains
open for actual native Reader capture qualification. Installed Session and Buddy
Reader/Writer/blank-start integration remain separate unfinished requirements.

## Implemented path

The producer imports the measured identity capture and visual encoder from
research revision `05af65656e07c939349c9a10419ba94cd8b7457e`; original blob hashes
are retained in `native/rm2_3_28_0_172/upstream-source.json`. It composes complete
page-order checks inside both borrowed source callbacks. Encoding uses bounded
private memory, and the outer lifetime/reentry guard survives through extraction,
digest verification and final publication. Source deletion during either callback
or final publication refuses without exposing intermediate bytes.

The native read-only backend uses the original lease/deadline and the shared
capture reply mapper. The immutable Rust `HistoricalCapture` owns the PNG,
before/after historical facts, dimensions, DPR, digest and named procedure. Its
constructor checks the static PNG container and performs a bounded full decode.
Neither receipt nor matching observations create a revision, input epoch,
operational `PageObservation`, physical freshness or writing permission.

Only capture replies receive the larger frame allowance. Ordinary controls stay
at 64 KiB; the PNG is capped at 1 MiB and the capture payload at 1 MiB plus 64 KiB,
excluding the prefix. The endpoint retains the original request deadline while
output is queued. Original v1/v2 fixture bytes and digests survive the upgrade;
live transport accepts v3 only. Capture does not clear uncertain effects.

## Owned verification

- Producer: 176 checks on host ASAN/UBSAN and ARM/QEMU. Cases include complete
  order mismatch, owner replacement/deletion, nested entry, post-encoding loss,
  cancellation, invalid budgets and the deliberate 17-page representation bound.
- Imported identity capture: 97 owned scenarios, 784 checks, including the real
  selected-page QML model and capture core. Modeled boundaries remain explicit.
- Rust/Qt wire: 21 vectors and 21,164 checks on host and ARM/QEMU, including large
  captures, chunked decoding, altered metadata, incompatible revisions and bounds.
- Qt endpoint: 1,847 checks on host and ARM/QEMU, including a stalled roughly
  900 KiB capture that is retired at the original request deadline.
- Three explicit Rust/Qt socket tests pass. The new producer test renders an
  owned QQuickPaintedItem, uses actual `grabWindow` and PNG encoding, checks order
  through modeled Reader joins, and sends the production mapped result through
  the authenticated Linux Client. Rust checks all 19,200 decoded pixels. A second
  run changes the final order and receives only UnknownIdentity after encoding.
- Linux current-toolchain and explicit Rust 1.88 all-feature suites pass;
  Windows all-feature suites pass. Linux/Windows strict Clippy passes. The normal
  suites intentionally leave explicit socket/corpus jobs ignored; those jobs were
  separately run as described above. Package containment has exactly 18 members
  and its five regression tests pass. Default features remain empty.

The received owned image is 57,908 bytes, 160 by 120 pixels, SHA256
`817fccd0baf991f5d4cb9c42cfa00a1b554c3843d1487f6ead13961a292808a9`.
It is an artificial test canvas, not tablet content. Original logs and inputs
remain retained under the `capture-integration-host-r2`,
`capture-integration-arm-r2`, `capture-rust-final-r1`, `capture-msrv-188-r1` and
`source-capture-*-r3` checkpoints. The earlier ARM signedness compile failure is
preserved; the corrected source passed the later ARM tests. The directory named
`capture-rust-msrv-r1` actually ran the image's default Rust 1.98.1 and is not MSRV
evidence; `capture-msrv-188-r1` explicitly ran Rust 1.88.0.

## Review and remaining acceptance

Independent source review found and drove fixes for publication after guard
retirement, callback owner deletion, fresh-output preservation, the profile's
wire revision and forbidden production fixture macros. Native artifacts remain
mutation-disabled and unqualified. The complete final ARM artifact and original
source manifest are retained separately from host/QEMU tests; building it does
not install or activate it.

Final `capture-native-library-r2` completed the 176 ARM producer checks, both
forbidden-fixture-macro checks and the full library link with GCC 13.4.0 / Qt
6.10.3. ELF inspection reports ARM32 little-endian hard-float. Library SHA256 is
`3e5dee321c68384bfd042e3697dc405f6f5d0de3891bfadb541bb55efac2b21d`;
profile SHA256 is
`cd1d38507aedadc7df1406b5a74843ba7ab307fb51c8213311d71cab6bb0d8ae`;
the source-input manifest SHA256 is
`fc2e912720de7b7a38dc2b5c56e18c424afad7c3d88b6a0aa60668d549486a22`.
The profile declares wire revision 3, mutation disabled and unqualified source
integration. The earlier artifact is retained separately; no frozen native
experiment was updated or replayed.

The previously frozen bootstrap candidate's UnknownIdentity result remains a
separate diagnostic outcome. No new tablet action occurred for this checkpoint.
Actual joined Reader capture, the 17-page extension on-device, repeated native
capacity, product input continuity, creation/reuse, persistence/recovery and
coordinated consumer acceptance still require fresh evidence. This source result
does not complete or archive the integration change.

Source basis: current source and independent review, original imported Git blob
provenance, local executed tests and retained build/image artifacts. Native
qualification is explicitly absent.
