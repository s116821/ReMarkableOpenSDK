# Reader metadata census: bounded refusal and recovery findings

## October 13, 2026 observation

Main performed one separately selected metadata-only observation on the authorized
RM2. The payload uses SDK source
[`e69344eff67d6f3b564d0c155fa44af5296fbd69`](https://github.com/s116821/ReMarkableOpenSDK/tree/e69344eff67d6f3b564d0c155fa44af5296fbd69)
and the [independent entry contract](../../openspec/changes/establish-native-platform-contract/reader-metadata-census-entry.md).
Candidate artifact SHA256 is
`ebebf9b55606bd8ca23a2f9a0de811185565d8b181654513f25a449feecaaf4a`.
Its provenance records both Git LF source hashes and actual compiled CRLF input
hashes, with exact normalization equality; it is not a byte-identical Git export.

The retained 303-byte result has SHA256
`d9edd1d9cf227382cee005480d82b62fdd5dea842f88c812533e3a5a09a83b61`.
SDK independently matched it to the collector's captured result bytes and
inspected the before/after ownership frames. Root identity, owner and process
generation were unchanged; final receipts were regular single-link files;
pending, cancel and restore markers were absent in those sampled frames.
These checks bind receipt collection, without establishing atomic cancellation
or object lifetime.

| Result | Interpretation from the frozen source |
| --- | --- |
| `application_thread=true` | The census passed its application GUI-thread admission |
| `current_window=true` | Focused visible/active QQuickWindow and associated engine admission passed |
| `visited=25`, both anchor counts `0` | The bounded deduplicated QObject/visual-child walk found neither exact named anchor in its sampled domain |
| `stage=anchor-uniqueness` | The walk reached its anchor-count refusal before owner-link and property-metadata checks |
| `owner_links=false`, `metadata=false`, `page_string_domain=false` | Initial values retained because those later checks were not reached; no measured negative owner/type finding |
| `native_authority=false` | No current document/page or action authority is emitted |

This demonstrates real entry and census execution with a bounded refusal.
It does not show that reader anchors are absent in an open document, another
window/root, another moment or the whole process. The exact named anchors are
`DocumentView.ToolSettings` and `SceneViewsRoot`; no application property values,
document/page IDs, render requests or input were read or issued by the payload.

Main's pre-activation paper image and post-restoration My Files image do not
bind the candidate interval. Main reports Library after restart; that is a
plausible explanation for the zero counts, rather than a conclusion supplied by
the result JSON. The observed window conditions admit both Library and document
contexts. The independent reviewer inspected both images and retained this limit.

## Failed automatic recovery and separate restoration

The original primary and fallback restoration actors failed. Their script used
`flock -w`, which the actual BusyBox utility did not support. Preserved failure
evidence includes `phase=stop-fence exit=1` and failed actor status. The original
operator receipt retains `recovery_unknown=PacketUnknown` and `cleanup=false`.
Its `restored=true` follows subsequent intervention and must not be interpreted
as successful original automatic recovery.

Main then completed a separate bounded restoration using supported nonblocking
`flock -n`, retained a stock-verified marker and manually removed the task-owned
units/root after preserving 20 files with hashes. Later stock records show the
original xochitl command/policy and no pending job. Sol independently inspected
the emergency source delta, preservation inventory and restoration/cleanup
records. These are Main's actual execution and subsequent recovery evidence;
the original actor failure remains unresolved by that successful intervention.

The reusable precondition is to check the exact target utility and demonstrate
owned nonblocking lock acquisition/release before activation. A proposed bounded
retry of supported `-n` calls belongs to Main's separately reviewed fresh actor
source. Utility compatibility and emergency restoration alone do not qualify
independent automatic recovery. No actor implementation is added here.

## Unchanged entry: request-time context and original deadline

The [frozen entry](https://github.com/s116821/ReMarkableOpenSDK/blob/e69344eff67d6f3b564d0c155fa44af5296fbd69/tools/reader_metadata_entry.h)
waits for one exact request. It does not latch a reader window during bootstrap;
the [census](https://github.com/s116821/ReMarkableOpenSDK/blob/e69344eff67d6f3b564d0c155fa44af5296fbd69/tools/reader_metadata_census.h)
samples the focused window, engine, content root and anchors when the queued
request check runs. A future distinct packet can therefore withhold the request
through Library bootstrap, use a separately permitted ordinary fixture-open
operation and fresh same-process context checks, then submit one request without
changing this entry.

The original 60-second clock starts at Entry construction. Ready publication,
fixture opening and request creation do not reset it. Startup, waiting, opening,
visual/fixture checks, transport, queued dispatch and the at-most-one-second
cooperative census must fit within that clock with an explicit positive margin.
The entry exports no remaining-time clock; a Main monotonic anchor before launch
is conservative, whereas a ready timestamp cannot establish a new 60 seconds.
Blocked GUI calls can delay timers and callbacks; external recovery is required.

Initial and coalesced watched-directory checks share the original cap of 32.
Keep unrelated evidence writes outside that directory. Missing requests wait;
malformed requests, expiry, cancellation, root/process binding loss, application
shutdown and terminal results do not authorize rearming or another request.
The same process generation must survive any ordinary fixture opening. Changes
in focused windows or engines can refuse admission, and sampled before/after
visuals do not establish continuous native page identity.

## Two later context refusals: no new census result

Main completed two fresh, separately reviewed follow-ups with the unchanged
entry behavior and new nonce bindings. Neither reached ordinary input or a
census request. The original deadline was retained, and both nonces are spent.

| Follow-up | Measured outcome | Recovery scope |
| --- | --- | --- |
| `c828` | Activated entry, then exhausted the original context margin before ordinary input/request; no census result. Original operator outcome remains `PacketUnknown`, elapsed 85.922 seconds | Configured primary explicitly started by the live host completed stock restoration and task-owned unit/root cleanup |
| `006` | Captured a blank frame 6.453 seconds after the before-launch anchor; Main refused the Library visual gate before ordinary input/request. No census result; original operator outcome remains `PacketUnknown`, elapsed 81.062 seconds | Live-host-started primary completed stock restoration, shared-image cleanup and task-owned unit/root cleanup |

SDK directly read both operator receipts. Their SHA256 values are
`08f4c74114603096fbf21f0ddfb7b017b5962e51fbc7fb3a09871f8c7ea8c98b`
and `6ba015b6d3e900c467b3fd28d1b43f4ea663bf1a385453167316ba061319d973`.
Both record activation, no request attempt and subsequent restoration/cleanup.
Main's context-guard refusal in the first case is attributed to Main's execution
report and supporting absence of input/request evidence; an independent captured
stderr from that guard invocation was not available to the reviewer.

For `006`, the captured image SHA256 is
`65922aade00cfdb39c846d7bca1c71f9f58b58ae3c9c2ee857cb3b563a1b8c06`.
Sol privately inspected the blank image and its same-process sample record.
The blank frame's cause is unknown. It is not evidence of painted Library,
a reader-anchor condition or an entry defect.

`census-ready` is published after entry/root binding, watcher installation and
timer setup. It checks neither painted UI nor Library/document presentation.
Window/engine/census checks occur only after the one request. A proposed bounded
nonblank-image filter can support the visual handoff, but Main must still inspect
and confirm Library/context: nonblank pixels alone do not identify the UI or
grant native page authority. Image filtering and human inspection must fit the
original clock; neither permits a reset, rearm or automatic repeated request.

The compatible primary's successful scripted recovery under a live host is
useful actual evidence. Neither follow-up exercised independent timer expiry,
host loss or the fallback recovery path. Those lifecycle cases remain open.
These successes do not alter the first `2ef` packet's failed automatic actors
and separate intervention. No readiness/source/lifecycle gate is closed from
these context refusals, and this record does not gate Main's next independently
permitted preparation.

## Evidence and remaining delivery

Source basis: frozen SDK source and its
[public implementation/review receipt](https://github.com/s116821/ReMarkableOpenSDK/pull/1#issuecomment-6151055548);
SDK's direct result/collector inspection; Main's private operator, preservation,
image and restoration records; and Sol's independent source/evidence review.
Those private records and internal review messages have no public transcript
link. Sol inspected all 240 captured streams and 20 preserved file hashes;
SDK does not claim to have repeated that full inspection or any device execution.
For the later follow-ups, Sol independently inspected 104 streams/52 transport
receipts for `c828`, and 110 streams/55 transport receipts plus 14 frozen source
hashes and the image for `006`. Root directly read the latter operator receipt
and final stock record. These are separately attributed inspections, with
operational details and original failure records retained privately.

This result preserves the already demonstrated
[insertion, native ink and reopen milestones](caller-selected-insertion-result.md).
Current-owner/page/render/lifetime and complete recovery qualification remain
open. The observed nonce is spent; future preparation requires fresh exact
source/artifact/provider/context and independent recovery review. Held source5
activation/equivalent replay and spent 0404 replay remain excluded. This document
does not select another operation, archive the SDK change or gate Main's
independent permitted preparation.
