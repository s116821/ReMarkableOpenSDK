# Fixed stroke and independent release: owned policy result

October 23, 2026. The [fixed-stroke proposal](../../openspec/changes/establish-native-platform-contract/fixed-stroke-release.md)
now has a plain C++ policy and focused owned controls. Host, vendor ARM/QEMU and
host ASAN+UBSAN each passed **92 scenarios/2267 checks**, with identical stdout,
empty stderr and unchanged frozen inputs. No tablet or input-device operation
occurred in these controls.

## What the policy establishes

The [header](../../tools/reader_fixed_stroke_policy.h) constructs one fixed
56-frame/178-record path and a separate five-record release group without X/Y
movement or eraser events. Delays total92 modeled milliseconds. The original
stroke and recovery deadlines are reserved together. An explicit noncopyable
atomic claim is consumed before every IO callout, including no-effect refusal;
neither reentry, repetition nor a replacement budget can renew that claim.
The serialized recovery holder has its own one-attempt arm and one-use release.

A frame gets one write call. Error, EINTR, zero, short/nonrecord and oversized
results stop remaining stroke frames without byte retry or redraw. Possible
dispatch/contact facts survive refusal. Accepted complete bytes are not observed
input or ink. Recovery checks time after write and after its final identity
callout, against the same original deadline. Any armed recovery without positive
release observation leaves contact unresolved, including cases with no recovery
write attempted. Its `unknown` field describes release-dispatch ambiguity;
`unknown=false` alone never authorizes close/reopen.

The [fixture](../../tools/reader_fixed_stroke_policy_fixture.cpp) covers writes
at initial release, hover, contact, middle and final release; permission/clock
changes, exceptions, pause failures, duplicate/nested calls and sixteen actual
two-thread claim races. Actual owned pipe/fork/wait/descriptor cases demonstrate
a writer-child exit after its contact frame and one surviving parent-descriptor
release into the pipe. Device identity, clocks and release confirmation remain
explicit models. Pipe bytes do not qualify evdev filtering, kernel contact-up,
native scheduling or stock input routing. Recovery is one serialized holder;
concurrent access to that holder is outside this policy.

## Reproduced defects and fixes

The first historical controls reproduced recovery arming after reentrant
arm/release had already spent the holder, and a final identity callout exhausting
the deadline while release still reported observed. Both now refuse. The frame
builder also bounds its seven-record storage; the old oversized initializer's
undefined behavior was not executed.

Initial repaired controls passed83/1048 per platform. Independent Root review
then identified the missing whole-stroke claim. Actual counterexamples produced
**112 write calls** for both repeated and nested dispatch, representing two
strokes. A first reproduction compile failed because its container lacked a
writable temporary directory; the corrected reproduction and failure are both
preserved. The final92/2267 controls include the explicit claim and reject those
duplicates. The earlier pass is historical, not evidence for the final claim.

## Reproduction and evidence

Run [reader_fixed_stroke_policy_test.sh](../../tools/reader_fixed_stroke_policy_test.sh)
with a frozen folder containing the header, fixture and runner, plus a fresh
output directory. Select `READER_FIXED_STROKE_MODE=host`, `arm` or `asan`.
The pinned image is
`sha256:416c7a7be0038156797b0892f031f352b841d1921fae83f712d0a272e4724618`;
ARM uses the vendor RM2 environment and matching qemu-arm/sysroot. The distinct
[vendor event ABI probe](../research/pen-event-release-boundary.md) remains the
input_event layout evidence; the owned pipe carrier does not replace it.

Final header SHA-256
`63a368589bcca1ad8966f9386939ed9283e864e006114b602e4661e82811fd6b`,
fixture `38b957eab16f5a35ffb669124a693c6ace9a7d46523dc4a7ae7ebfa07095f6a9`,
runner `6657be8c6636345605975fe0c5e2d38b6309a438e1b3ccd14e18e391391d2102`.
Original owner verification receipt SHA-256
`792a594ead24e5fc4a77b7ac5edf6a3e8c94405da0376b61619793624c2e2ec1`.
Root independently inspected the source and rehashed original exports, three
run receipts/binaries/outputs and actual counterexamples:34 checks, receipt
`c8f3476bafefc9c51c532f0d1d44def6c9e95c9d8ccf7d414bd233e51b25b5ad`.
Root did not rerun the owner's matrix. Raw receipts remain private local evidence
without public download links.

Native input IO, same-device descriptors, real time/process admission, observer
stream/kernel release checks and independently scheduled recovery remain later
composition work. No native writer, intended-target ink, close/save/reopen or SDK
capability is qualified by this result.

Source basis: exact policy/fixture source, original frozen build/run outputs and
preserved counterexamples, plus independent source/evidence-binding review.
