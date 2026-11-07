## ADDED Requirements

### Requirement: Development joined Reader focus receipt

The diagnostic SHALL use the existing authenticated revision-4 resident
connection and the original pending lease. It SHALL accept request kind 11 with
an empty body and return reply kind 13 with exactly one unsigned enum byte.
These kinds SHALL be available only in their respective opt-in native and
client builds. Existing Hello and Ready messages SHALL remain unchanged.
The operator SHALL establish exact opt-in artifact identity before dispatch.
An unrecognized kind SHALL fail closed without fallback or downgrade.

| Value | Receipt |
| --- | --- |
| 0 | Refused |
| 1 | NoLeaf |
| 2 | ExactScene |
| 3 | InsideScene |
| 4 | ScopeOnly |
| 5 | OutsideScope |

The classification SHALL refer to the active focus leaf of the one joined Reader
window at sampling time. ExactScene is the joined selected SceneView itself.
InsideScene is a descendant of that scene. ScopeOnly is within the joined device
scope but outside the selected scene subtree, including the scope itself.
OutsideScope is in the admitted window outside the device scope. NoLeaf is
reserved for a valid join with no active item. The existing focused-window
permit excludes the standalone sampler's NoFocusedWindow and OtherWindow
outcomes. They SHALL never be emitted as wire values.

Both messages SHALL use the ordinary 65,536-byte payload limit. The exact
payload lengths, excluding the existing four-byte prefix, SHALL be 204 bytes
for the request and 237 bytes for the enum reply. The original runtime,
connection nonces, sequence, operation and SHA-256 of the full length-prefixed
request SHALL govern correlation. The only alternative replies SHALL be the
existing Rejected(Unsupported) for unavailable base admission and
Rejected(UnknownIdentity) for unavailable or changed GUI admission, subject to
the original lease. A focus receipt SHALL never complete an ordinary or
pre-Enter request, and their replies SHALL never complete a focus request.

The existing selected Reader join SHALL establish the exact current scene,
device, page relationship and readiness before sampling. It SHALL use the same
GUI permit, join epoch, reentry scope and original absolute request deadline
through sample validation. Focus or relationship drift, including observed
away-and-back changes or reentry, SHALL yield Refused while the original GUI
permit and lease still allow publication. If the deadline has expired, the peer
is stale, or the lease has retired, the server SHALL discard the result.
The client SHALL reject a decoded reply if cancellation, deadline or peer
checks fail before completion. A late or mismatched reply SHALL never satisfy
a successor request.
Lost requests SHALL have no automatic retry and no pending mutation.

The receipt SHALL carry only the enum classification. It SHALL NOT create a
focus lease, page or visit identity, text receipt, edit or Submit authority,
capability advertisement, or mutation effect. The diagnostic SHALL NOT use
creation or source callbacks, activate a route, or consume or clear a mutation
reservation. Existing ordinary reply bytes, semantics and limits SHALL remain
unchanged with either feature configuration.

#### Scenario: Joined focus remains stable

- **WHEN** the same authenticated read-only owner completes a ready selected
  Reader join and its active focus leaf is a descendant of the selected scene
- **THEN** it SHALL return InsideScene against the original pending request

#### Scenario: Focus or the selected Reader changes during observation

- **WHEN** the sampler observes focus, page or ownership drift or reentry and
  the original GUI permit and request lease still allow publication
- **THEN** it SHALL return Refused without a classification

#### Scenario: Retired request cannot publish

- **WHEN** the server observes deadline expiry, peer change or retirement
  before publication, or the client observes cancellation before completion
- **THEN** the server SHALL discard the result or the client SHALL decline
  completion, respectively, and the result SHALL never complete a successor

#### Scenario: Exact shapes and explicit opt-in

- **WHEN** a disabled or older codec receives a focus kind, a frame has extra
  or missing bytes or an unknown enum value, or a reply mismatches the
  pending command or request identity
- **THEN** it SHALL fail decoding or correlation as appropriate without
  interpreting the frame as any other request or reply
