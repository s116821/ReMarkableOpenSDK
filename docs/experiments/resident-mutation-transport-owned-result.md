# Resident mutation cleanup and fresh readback: owned Entry result

October 24, 2026 UTC. The default-off file diagnostic can now compose one
conditional insertion/target acquisition A, retire its request resources, and
admit a fresh read-only B in the same registration. This is an owned Qt result.
Native acquisition/Core, bootstrap and identity joins are explicitly modeled;
no tablet mutation or native lifecycle qualification follows from these tests.

This continues the [request scope result](mutation-request-scope-owned-result.md)
and implements the selected [retirement design](../../openspec/changes/establish-native-platform-contract/mutation-request-retirement.md).
The always-resident native owners and code are retained. Request retirement is
not the end of a Buddy conversation or a requirement to restart xochitl between
conversations.

## Actual composition and resources

The fixture drives the real file-backed `reader_insertion_transport::Entry`,
Coordinator, owned Qt/worker/JavaScript delivery and full-order observer. Only
the documented native and Sample identity boundaries are replaced. This is
stronger than assigning a synthetic final JSON object, but it remains distinct
from executing the adapter inside stock xochitl.

The selected mode requires conditional forwarding, target acquisition and three
distinct canonical activation/A/B nonces. Construction, start and full deletion
of the Coordinator run under registration-owned request scopes. A owns one
duplicated directory descriptor and an inventoried QObject subtree. The normal
owned case destroys all nine inventoried objects and closes the descriptor
before B. The registration root, heartbeat, result publisher, Fence and retained
native owners are explicitly outside A's inventory. Native resources are not
claimed reclaimed merely because the facade disappeared.

The existing heartbeat admits B only after A is Retired, depth zero, and the
entire retired notification has unwound. A nested event loop in that notification
cannot advance the handoff. B obtains its own descriptor, fresh token and
callback-local identity join, then calls the actual full-order observer. No
document pointer, actualId or Sample permission from A is reused.

After B also retires and its notification unwinds, a fresh C mutation is refused
because A's unresolved-effect latch remains set. B's successful observation does
not acknowledge reconciliation automatically. The native Resident, Prepared,
Core, closure code and hook remain resident; no native Close, drain or unload is
introduced.

## Failure and wire semantics

The original 90-second setup and 30-second operation limits include construction,
A, cleanup, B, C refusal and final publication. B does not renew either clock.
Actual provider/packet/recovery admission remains a separate prerequisite for a
future native trial.

Schema4 freezes A's result, finish time and process/window/engine facts before
cleanup. The combined result records final registration context separately. An
observed inner Coordinator can coexist with an incomplete outer A if its finish
boundary loses permission. A successful A also remains recorded if B later
fails. Neither incomplete outcome means that no insertion happened or permits
replaying the effect.

Descriptor-close uncertainty is sticky and is never retried. Cleanup failure,
unexpected destruction, cancellation, deadline/window loss or application quit
prevents subsequent work or a committed positive result. Readback exceptions
become incomplete outcomes and still follow request retirement. Weak-owner
checks bracket callback boundaries. Late pre-link quit/deletion leaves pending
bytes only; late post-link quit/deletion preserves pending plus a two-link result
as forensic evidence. Parsing positive bytes does not establish that this file
commit or live operator admission succeeded.

Only schema4 has an 8192-byte cap. Existing schemas retain 4096. The strict private
consumer validates the mutation projection through the unchanged schema3
consumer and separately validates fresh B/order, retirement, identities and
original clocks. Original schema4 bytes remain the consumed receipt.

## Verification

| Evidence | Result | Scope |
| --- | --- | --- |
| Actual Entry R2, host Qt | 30 scenarios / 1270 checks | Owned runtime |
| Actual Entry R2, ARM/QEMU | 30 / 1270 | Vendor ABI plus owned runtime |
| Actual Entry R2, host ASAN/UBSAN | 30 / 1270 | Owned lifetime controls |
| Retirement handoff getter, host and ARM | 748 checks each | No admission during nested retired notification |
| Existing transport host/ARM/sanitizer regression | 946 plus 150 target serializer checks each | Preserved default modes; synthetic target serialization |
| Strict consumer focused controls | 515 checks | Pure Python; includes all 45 original schema3 serializer inputs |
| Independent original-byte compatibility | 30 facts / 23 wires per platform | Host, ARM and sanitizer outputs; forensic links distinguished |

The 30 cases cover complete A/cleanup/B/C, nested finished/child/retired listeners,
late JS delivery, uncertain A/B close, cleanup throw/deletion/cancellation/expiry,
pre-B and outer-A clock/window loss, failed/wrong/missing B observations, readback
throw/deletion, unexpected facade destruction, bootstrap refusal and pre/post-link
quit/deletion. The N16 case uses a real 17-ID post-order serializer; largest R2
wire sizes are 5799 host, 5806 ARM and 5800 sanitizer bytes, below 8192 and above
the old 4096 cap. These are measured run sizes, not an exhaustive byte maximum
for arbitrary diagnostic text.

The first compile failed on a Qt6.4 QPointer conversion in the inventory. It is
preserved separately and repaired with an explicit QObject pointer conversion.
R1's 28-case result is retained; R2 adds the two outer-A finish controls. No
earlier result is credited to a later source snapshot.

Independent source/consumer review binds all R2 input and output digests. Its
receipt SHA-256 is
`d0f23e40c2b4a46f9229bf80962afb80219fee41f0ae5f049eac7f9281d9f209`.
It also confirms the repaired post-guard weak-owner check in the readback action;
no separate guard-deletion regression is claimed. A later review-worker turn was
interrupted by a provider content flag without identifying a triggering
operation; no successful edit or result is credited to that failed turn. The
interruption is recorded in the canonical issue, separately from these results.

The selected default-off startup also compiles and links with the pinned vendor
ARM SDK, including the added Fence MOC dependency. This artifact is not loaded
by the compiler or fixtures. Its provider, initializer and execution packet
qualification remains separate from a successful link.

## Remaining native question

Run this exact composed request in one freshly admitted idle-tablet candidate:
does one actual insertion/selection retire the request facade/resources, then
allow the fresh read-only identity/full-order observation in the same process
under the original deadlines, with another mutation still refused? Independently
verify restoration and full source/PDF/ink/order preservation. This is separate
from intended-target writing/save/reopen, other-writer exclusion, native-owner
reclamation, shutdown fault repair and product SDK/Buddy integration. Supported
native, render and durable authority remain false.
