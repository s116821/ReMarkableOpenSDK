# Native pen events: release and ABI boundary

October 23, 2026. Source review and a device-free vendor-compiler/ARM-emulator
probe. No new pen event, tablet write, document close or reopen was performed.
The [historical one-stroke/save/reopen result](../experiments/caller-selected-insertion-result.md)
remains a separate fixed-page observation, not a general writer contract.

## Source findings for a fixed-stroke successor

The retained helper source SHA-256 is
`39454bd204f789c95e672a9e3e88fea5102d46224b3c9d3c0bda1e1364b02ec9`.
Its `strokes` CLI validates virtual coordinates before opening the writer, but
then also sleeps and takes an unrelated legacy screenshot. It does not use the
separate guarded backend's complete owned-pen observation window. Do not inherit
that backend's source lease or release evidence by proximity.

The reviewed RM2 path uses virtual 768x1024 coordinates, swaps/inverts axes and
interpolates in native units. The fixed historical path `(360,480)->(372,484)`
derives to native `(11138,7371)->(11056,7616)` with float32 conversion and
truncation toward zero. Its full sequence derives to 56 SYN_REPORT frames and
178 records, including initial release, hover, contact, interpolation and final
release. These are source-derived counts, not captured native kernel events.

The pinned evdev 0.13.2 write loop retries interrupted/short writes without a
deadline and treats a zero-byte write as success even if bytes remain. The Pen
has no Drop contact-release operation; the dependency's device Drop ungrabs.
Closing a descriptor, killing a writer or restarting xochitl is therefore not
independent evidence of contact-up. The existing normal-path best-effort release
cannot establish recovery after its process dies mid-stroke.

An explicit fixed release consists of pressure zero, distance100, BTN_TOUCH0,
BTN_TOOL_PEN0 and SYN_REPORT. It does not move X/Y or clear BTN_TOOL_RUBBER;
require the eraser already inactive rather than silently introducing a different
event policy. Before a new writing diagnostic, a proposed dedicated stroke-only
child should have a separately scheduled recovery holder with a checked,
already-open descriptor to the same device, bounded/classified write outcomes,
and independent release/frame observation. This design is not implemented or
native-qualified. Recovery is best effort if the holder, device or kernel fails.
Never redraw after possible dispatch; preserve/reconcile unknown effects first.

## Actual vendor input-event ABI probe

The original [device-free probe source](../../tools/reader_pen_event_abi_fixture.cpp)
was compiled in the existing pinned SDK image
`sha256:416c7a7be0038156797b0892f031f352b841d1921fae83f712d0a272e4724618`
using the vendor RM2 environment
`/opt/codex/rm2/5.8.203/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi`,
C++17, Wall/Wextra/Werror and O2. It executed under qemu-arm with the matching
sysroot, not on the tablet. ELF is ARM32 little endian, EABI5/hard float.

Observed output:

```text
input_event=16 align=4 offsets=8/10/12 timeval=16 bytes=000000000000000001004a0100000000 no_device_open=true
```

Compile-time checks cover pointer/endian/record layout and event/axis/key
constants. The event was value-initialized through vendor `linux/input.h`;
the bytes above represent EV_KEY/BTN_TOUCH/value0. There is no open/ioctl/read/
write to an input device in this program.

Userspace `timeval` is16 bytes in this SDK, while this kernel event record has
eight bytes before type/code/value. An earlier proposed `sizeof(timeval)==8`
gate was incorrect and is withdrawn. Construct vendor `input_event` records
directly; do not serialize a guessed userspace timeval or inherit the host/Rust
layout. This is one compiler/header/emulator result, not native driver delivery,
complete frames, released contact or writer qualification.

For reproduction, source the exact environment above, compile the linked file
with `$CXX -std=c++17 -Wall -Wextra -Werror -O2`, then run the output with
`qemu-arm -L "$SDKTARGETSYSROOT"`. Preserve the compiler/header/image provenance
and actual output. The original receipt SHA-256 is
`8bd94c29839db479fd63e699bdfc4a8337d91340030fb7acf726fd70b7024a43`;
the subsequently published source is byte-identical to that probe input.

Source basis: retained exact helper/dependency source and historical report;
original vendor compile, ELF and emulator output, independently rehashed/reviewed.
Recovery-holder requirements are proposed engineering work, not tested behavior.
