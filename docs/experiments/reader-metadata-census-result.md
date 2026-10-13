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

## Evidence and remaining delivery

Source basis: frozen SDK source and its
[public implementation/review receipt](https://github.com/s116821/ReMarkableOpenSDK/pull/1#issuecomment-6151055548);
SDK's direct result/collector inspection; Main's private operator, preservation,
image and restoration records; and Sol's independent source/evidence review.
Those private records and internal review messages have no public transcript
link. Sol inspected all 240 captured streams and 20 preserved file hashes;
SDK does not claim to have repeated that full inspection or any device execution.

This result preserves the already demonstrated
[insertion, native ink and reopen milestones](caller-selected-insertion-result.md).
Current-owner/page/render/lifetime and complete recovery qualification remain
open. The observed nonce is spent; future preparation requires fresh exact
source/artifact/provider/context and independent recovery review. Held source5
activation/equivalent replay and spent 0404 replay remain excluded. This document
does not select another operation, archive the SDK change or gate Main's
independent permitted preparation.
