## ADDED Requirements

### Requirement: Bounded pre-Enter historical text command

The opt-in development diagnostic SHALL reuse the authenticated revision-4
Endpoint, connection incarnation, sequence, operation and request digest. It
SHALL use request kind 10 and response kind 12 in their respective tag
namespaces. The digest SHALL cover the complete canonical length-prefixed
request, including the range. A text reply SHALL match only the same pending
text command with the existing full identity and digest checks. The only other
permitted replies to that command SHALL be existing Rejected(Unsupported) for
unavailable admission and Rejected(Capacity) for an occupied lifetime slot.
A copied length different from the request SHALL NOT complete the text command.

The following fields follow the kind byte. All multibyte integers and code
units SHALL be unsigned and big-endian.

| Kind | Body | Constraints |
| --- | --- | --- |
| Request 10 | `offset:u32 count:u16` | Half-open UTF-16 range. Count 0 through 4096. Checked offset + count at most 2147483646. |
| Reply 12, copied | `status:u8 = 0 count:u16 units:u16[count]` | Count 0 through 4096, equal to the pending request count at correlation. |
| Reply 12, refused | `status:u8 = 1` | No count, units, revision or other data. |

The ordinary 65536-byte payload bound SHALL apply to both messages, excluding
the four-byte length prefix. The codecs SHALL reject unknown status, excess
count, overflow, truncated units and trailing bytes. The document sentinel
SHALL stay outside the copied range. The native copy SHALL refuse endpoints
that split an actual surrogate pair. Codecs SHALL preserve isolated surrogate
units and all other admitted units, including NUL and paragraph separators.
They SHALL NOT require Unicode scalar validity, transcode to UTF-8 or
normalize text. An admitted empty copy SHALL remain distinct from refusal.

Only an admitted bounded snapshot with a completed, normally returned original
call and successfully copied text SHALL produce status 0. Terminal capture
refusal while the lease still admits a response SHALL produce status 1 with no
text, including after original exception or lost result publication. The reply
SHALL remain historical and SHALL NOT provide Submit, currentness, draft,
conditional clear, focus, page/visit or mutation authority.

#### Scenario: Decoding and correlation are bounded
- **WHEN** a frame has an unknown status, malformed size, an invalid range or extra bytes, or a copied reply disagrees with its original command or count
- **THEN** it SHALL fail decoding or correlation as appropriate and no text completes the request

#### Scenario: New command against an ordinary or older build
- **WHEN** a well-formed text command reaches an updated native endpoint without installed, qualified private capture admission
- **THEN** it SHALL return existing Rejected(Unsupported) with no capture arming
- **AND** an older revision-4 implementation SHALL retain its existing unknown-kind refusal or connection close, with no fallback, downgrade or replay

#### Scenario: Existing wire compatibility
- **WHEN** codecs are built with or without the opt-in
- **THEN** revisions 1 through 3 SHALL reject the new kinds and retain their exact historical encoding, decoding, digests and correlation
- **AND** existing revision-4 kinds, including development close kinds, SHALL retain their exact bytes and semantics
- **AND** Rust builds without the development opt-in SHALL neither expose the new typed command nor accept the new tags

### Requirement: Resident text request lease and lifetime

The endpoint SHALL supply the original absolute RequestLease deadline and a
shared one-way retirement signal to the native capture guard. Preparation,
dispatch, snapshots and publication SHALL use that same deadline. No stage
SHALL convert the time remaining into a renewed relative budget. The guard
SHALL observe retirement and expiration before native copy admission and before
copy publication. The endpoint SHALL additionally verify the original live
pending lease, peer and deadline before reply publication. A canceled, expired,
lost or retired request SHALL NOT publish text through a successor request.

The existing dispatcher SHALL retain at most one Capture handle for the
pending lease and cancel or release it at terminal cleanup. An armed route
SHALL retain its consumed slot after cancellation, expiry, loss or cleanup.
No reconnect or duplicate request SHALL rearm that slot or replay the lost
diagnostic. The diagnostic SHALL NOT use or clear the mutation reservation.

#### Scenario: The original lease retires during an armed capture
- **WHEN** the original lease is canceled, expires or loses its connection before admitted text publication
- **THEN** the result SHALL be discarded for transport and the process-lifetime slot SHALL remain consumed
- **AND** an ordinary stock Enter SHALL continue exactly once
