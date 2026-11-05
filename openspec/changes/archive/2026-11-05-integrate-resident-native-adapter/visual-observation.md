# Visual-only resident setup observation

The copied-reader setup reached SDK readiness but its external allocation scan
refused two matching allocations. Readiness does not identify the screen. Add a
separate visual-only Qt capture request so a developer can inspect the owned
window before completing setup. Never guess an allocation or fabricate Reader
facts to make the existing source-capture request accept a Library screen.

## Protocol and evidence

Advance the live protocol to revision 4. Command 6, `CaptureVisual`, has no body.
Reply 8, `VisualCaptured`, contains width/height (u32), DPR bits (u64), procedure
byte 2 (`rm2-window-visual-png-v1`), SHA256 (32 bytes), PNG length (u32), and PNG.
It carries no ReaderFacts. Use the same 1 MiB PNG, 1872-per-axis, 2628288-pixel,
10513152-byte decode allocation and 1114112-byte payload limits as source capture.
All ordinary control frames remain bounded to 65536 bytes. Rust exposes a distinct
immutable `HistoricalVisual` with full bounded PNG validation and no conversion
to source evidence, PageObservation, creation receipt or capture/write authority.

Historical revisions 1, 2 and 3 retain their original grammar, bytes, digests and
response correlation. Only revision 4 is accepted live; visual tags cannot appear
under an older revision. No automatic reconnect/replay or deadline renewal is
introduced. Visual delivery cannot settle prior uncertain effects.

## Producer and lifetime

The admitted read-only backend uses the existing private bounded Qt PNG sink and
one `QQuickWindow::grabWindow` on the GUI thread. Pin the original app, focused
visible active window, root and engine for the complete call and publication.
Retain the original request lease and absolute deadline; refusal is sticky even
if caller code restores a lost condition. Share the source-capture reentrancy gate
so nested source/visual requests refuse both outer and inner publication. Check
again after encoding, byte extraction and hashing. Exceptions refuse without
exposing partial bytes. No Reader join, native mutation or input is performed.
Synchronous Qt calls cannot be preempted by the deadline; late publication refuses.

## Verification and remaining integration

Verify malformed/truncated/corrupt/oversized images, original v1/v2/v3 history,
cross-kind reply rejection, fragmentation, cancellation/deadline, sticky effect
loss, nested capture and owner/context loss. Exercise the guarded producer with
owned Qt objects and actual Rust/Qt socket transport on host and ARM/QEMU where
applicable. Keep those results separate from native screen validation.

The Buddy setup consumer must be updated and separately reviewed before a fresh
native candidate uses this request. Its original setup deadline, actor/observer
barrier and independent recovery remain mandatory. Actual native source capture,
creation/reuse, installed product flows and final acceptance remain unfinished.
