## Integration

1. In `native/resident/wire.h`, reserve request 11 and reply 13 under
   `RSDK_DEVELOPMENT_READER_FOCUS_V1`. Define the wire values explicitly and map
   the accepted sampler enum to them. Check exact message shapes and preserve
   the ordinary frame limit. In `Endpoint::finish` add a command/reply check
   alongside `validPreEnterReply`, before digest and scope stamping.
2. In `native/rm2_3_28_0_172/read_only_backend.h`, reuse the existing base
   admission and window/engine permit. `MutationBackend::submit` already
   forwards other command kinds to `readOnly_.submit`. Branch on request 11
   before the ordinary creation/order join. Run the accepted
   `focus_diagnostic::observeJoinedLeaf` with the same permit and
   `min(MaxObservationMs, floor_ms(lease.deadline - now))`. The millisecond cap
   bounds local work. The original absolute lease deadline still governs
   every permit call, completion and output. A remainder below 1 ms skips the
   sample and leaves retirement to the existing deadline. Recheck the same
   permit after the sample and finish only through the existing endpoint.
   When the original lease can still accept a reply, use
   `Rejected(UnknownIdentity)` for unavailable or drifted GUI admission.
   A valid original permit with a failed selected join yields `Refused`.
3. In `Cargo.toml` and `src/native_adapter.rs`, follow the existing
   development-only codec pattern. Gate typed variants with
   `development-native-reader-focus`. The opt-in types serve the private
   diagnostic client. Reuse the existing `Client`, `Session`, authentication,
   progress, digest and terminal-loss logic. Apply the exact reply whitelist
   before the generic rejected-response arm.

Cancellation uses existing transport closure and lease retirement. The client
rechecks cancellation, deadline and peer identity before accepting a decoded
reply. If cancellation races with server publication, buffered bytes cannot
complete the canceled client operation. There is no new cancel frame.

The existing pre-Enter and focus flags must remain independent. Focus sampling
adds no hook, startup marker, timer, capture slot, producer, listener or second
owner. The existing bootstrap and authenticated endpoint supply the GUI owner.
Build the opt-in artifact separately with the ordinary read-only baseline
admission and without pre-Enter or effect flags. Opt-in builds of the two codecs
must still be byte-compatible for ordinary and existing diagnostic messages.

## Runtime prerequisites

Source route verified at `bootstrap.cpp` startup, `MutationBackend::submit`,
`ReadOnlyBackend::submit`, `Endpoint::acceptFrame/finish` and the Rust transport
client. A later run needs a reviewed exact adapter/profile/executable hash set
for the same supported RM2 profile, an already admitted read-only GUI owner with
that exact adapter loaded, the existing activation directory and listener, and
the authenticated supervisor peer on its original session. Admission includes
the expected peer PID, UID and process start, current runtime identity,
boot and activation identity, and current socket/directory identity. Main
verifies a disposable Reader fixture and the live route before use.

The protected candidate has no focus request. The source review establishes no
currently admitted owner carrying the opt-in artifact. If the exact owner or
route is missing, report that runtime blocker. This proposal supplies no
installation, activation or authorization path and has no dependency on a
positive composer focus or Submit finding.
