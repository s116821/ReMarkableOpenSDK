# Fixed pen observer stream

The measured [read-only native profile](../../../docs/experiments/pen-device-readonly-native-result.md)
supports exact RM2 axes and the fixed path. Add a plain record-state validator
for the later standalone writer parent's observer, separately from evdev IO.
Reuse the fixed policy's Event carrier; it is not a serialized native ABI.

Start only from an explicitly supplied released snapshot with valid native
coordinates/distance and all pen/eraser/touch/stylus keys up. Count at most8192
records. Validate bounded X/Y updates within the fixed path rectangle, pressure
0 or2630, distance0 or100, pen/touch Boolean changes and complete SYN_REPORT
frames. The child never emits eraser/stylus/tilt changes; reject those and every
unknown/dropped event. Frame boundaries require coherent pen/touch/pressure,
with at most one contact interval. A partial record stream, held contact,
malformed transition or quota failure remains sticky refusal. Never reset or
discard failed observations to obtain a new release proof.

Keep raw contact possibility, complete contact frames, final released state and
endpoint observations separate. The kernel may filter unchanged values, so do
not require178 observed records. Empty or release-only observations can support
a released-state check but cannot prove a stroke. The eventual helper must
separately bind device/FD/process identity, real raw read sizes, original clocks,
observer queue/quiet interval and a fresh matching kernel snapshot before
positive release. This parser alone cannot supply any of them or prove sole
input ownership, current document routing, visible ink or persistence.

Use synthetic original traces with filtered/redundant values, split batches,
partial/dropped/unknown events, repeated contact, quota, wrong initial state,
conflicting snapshots and full fixed path. Host/ARM/sanitizer controls label all
kernel facts as modeled. Native writer/recovery and full writing/reopen packet
remain under16.8a/b; this is a focused implementation component within that work.
