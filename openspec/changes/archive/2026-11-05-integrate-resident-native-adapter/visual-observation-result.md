# Visual-only capture source checkpoint, October 28, 2026

The resident SDK now has a visual-only Qt capture path for inspecting setup
screens without inventing Reader facts. It is implemented and verified with
owned objects and authenticated host sockets. The Buddy setup consumer and an
actual native image remain unfinished; task 3.0c stays open.

## Implemented behavior

Live revision 4 adds bodyless `CaptureVisual` and `VisualCaptured` carrying a
distinct immutable `HistoricalVisual`. The image validator checks the static PNG
container, dimensions, DPR and digest and performs a bounded full decode. It is
also used internally by the existing source-image type without changing its
before/after source requirements. Neither type grants capture or write permission.

The native backend retains the admitted lease and original deadline. The visual
wrapper pins the GUI application, window, root and engine through private PNG
encoding, extraction, hashing and final admission. Refusal is sticky. Source and
visual calls share a reentrancy gate; nested calls poison outer publication.
Visual capture performs no Reader join, input, selection or mutation.

Rust and Qt isolate the visual request/reply tags and procedure from source
capture. Only image replies may exceed the ordinary control-frame bound. Original
revision 1/2/3 journal bytes and digests remain intact, with old tag grammars and
v3 large-capture support; old revisions are rejected live. Retained original v3
fixtures and their provenance are in `tests/fixtures/resident-history/README.md`.

## Verification

- Final guarded source/visual producer: 357 checks on host ASAN/UBSAN and
  ARM/QEMU. Cases include deletion of actual pinned window/root/engine at every
  measured permit boundary, late expiry, exceptions, hidden windows, quotas,
  cancellation, and nested source/visual calls through final publication.
- Rust/Qt wire: 23 vectors and 22,056 checks on host and ARM/QEMU. Owned endpoint:
  1,847 checks on each. Existing queued-image expiry uses the same unchanged
  transport path and original deadline.
- Authenticated Rust/Qt producer test receives and checks every decoded pixel
  of an actual Qt window grab in source and visual modes. Visual mode deliberately
  removes usable Reader page facts and performs zero source joins. Source mode
  still refuses a changed final order. All three explicit socket tests passed;
  the producer/socket test was repeated after the added owner-deletion controls.
- Linux current-toolchain and Rust 1.88 all-feature suites, Windows all-feature
  suites, and Linux/Windows strict Clippy passed. Final added ambiguity controls
  cover both insertion/selection, local close and explicit Indeterminate replies,
  attempted visual use and repeated close without changing the original loss.
- Five package-containment regressions and all 22 strict OpenSpec items passed.
  Independent source review accepted the production code and the final controls.

The first producer run had 327 checks. The final 357-check runs add actual owner
destruction; the earlier evidence is preserved separately. These are owned
fixture and transport results, not native Reader or product qualification.

## Build-only artifact and next work

The ARM artifact was built with the existing pinned Qt 6.10.3 and Dobby inputs:

- Library SHA256: `c0121f577e19a484c962ad98ad8f60121e14d0695a3f9198e95978ad07895380`.
- Profile SHA256: `c0d8cbbb9b4618a870de348fedaedbf242b8009bf4615d67bd83d4f3c03a538d`.
- Input-manifest SHA256: `f04b30946c75188102bf55642353e613d743f00cb1f6dfdf5edfa40ea93f3aa1`.

The profile declares wire revision 4 with mutation disabled and unqualified
source integration. No tablet transfer, execution, input or service change was
performed for this checkpoint. The older spent native candidates are unchanged.

Next, update the pinned Buddy consumer and its setup barrier/image handoff under
the original setup clocks and independent recovery. Verify that the image is
bound to the admitted runtime and intended stage before a fresh native attempt.
Native source capture, creation/reuse, installed Reader/Writer/blank-start flows,
write/reopen, simulator and coordinated final delivery remain required.

Source basis: current source, original retained protocol fixtures, executed
host/ARM tests, authenticated Qt capture, build manifests and independent review.
