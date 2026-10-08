# Unqualified receiver-subtree capture prototype

Proposal against SDK397357b. Main reports58 v4 had a complete8-item focus chain,
receiver1, scene0, engine rejections0 and class rejections8. The narrow hypothesis
is a correlated SceneView off that chain. This is not an active-owner premise.

## Explicit requirement delta

WHEN developmentReceiverSubtreeCapture is selected upfront, THEN require
development capture, setup120000/facts5000 and main-dev-facts-120s, and refuse
simultaneous developmentFocusAncestry. No refusal selects this as a fallback.
WHEN a complete stable focus ancestry identifies exactly one structural receiver,
THEN walk that receiver's subtree once, completely, capped at256 items,8 edges
and8 SceneView metadata candidates. Refuse incompleteness, caps or ambiguous
valid correlations. Never accept the first match from a partial walk.

Retain weak window/root/actual-focus-anchor and sticky invalidation, including
subtree children/parent/window/destruction changes. Progress checks enclose walks
and native getters under the original5s capture clock and existing host rollback.
Use the existing scene classifier and a separate capture predicate preserving
context/window/engine/visibility/enabled/receiver ancestry/drawingAreaFocused/
document QObject identity guards, excluding only SceneView.hasActiveFocus.
Document.id, currentPage, currentPageId and scene.pageId must agree with the
explicit expected fixture identity. No ordinary activeOwner change is permitted.

WHEN capture succeeds, THEN publish the original private PNG and identity
provenance as capture completion v2, exactly37 fields (old36 plus discovery_scope
receiver-subtree-capture-unqualified-v1). Native/render/UI/atomic authority stays
false. Frame correctness still requires Main's complete-image visual review.
WHEN this mode is selected, THEN issue no retained-owner ticket, accept no facts
request and perform no facts read. Existing facts publisher rejects v2 completion.

Scoped failure uses v5 exact34 fields (existing v2's33 plus the same discovery_scope)
through the existing owner-refusal publisher, never an active-owner success label.
Existing v1/v2/v3/v4 formats retain their prior semantics and consumer rules.

## Design, tasks and limits

The new default-off entry config chooses a separate bounded discovery/capture
predicate. Existing focus and window discovery, ordinary activeOwner and facts
contracts remain untouched. Buddy owns explicit config/collector selection.

- [x] Implement retained bounded subtree discovery and capture-only entry routing.
- [x] Implement explicit consumer config/scoped completion and refusal decoding.
- [x] Check unique off-chain capture, two valid scenes, cap and getter-mutation
  invalidation with owned Qt fixtures; compile exact production ARM source.
- [ ] Obtain bounded independent review and freeze a fresh packet for Main.
- [ ] Main alone performs the next real capture and full PNG visual inspection.

No nonce/device operation follows from this proposal. Broad source/native/release
qualification, canonical sync and archive remain open; no broad matrix is a
prerequisite to the specifically authorized bounded experiment.

## Source verification checkpoint (2026-10-08)

Implemented SDK 3e2e2b9c0a30538095b6c76255959df3273b6e7d and Buddy
4d388b7d83c0db87d2c8da1ba4acd9e2126c7b0e. Exact frozen SDK execution passed
7 receiver cases, 8 funnel checks and 4 privacy checks (19 total). Selected
production ARM compile passed with -Werror and -z defs, without fixture macros.
Artifact SHA256: 3c68f13b4237cbe63492625e9c3d7c6f2764aa624558657cc0b7cf34a8842efa.
Buddy scoped checks passed31; existing focus131, v4 28, completion156,
topology304 and owner292 checks passed. Getter mutation covers membership
addition and sticky invalidation, not object deletion or complete lifetime proof.

Private review receipt: rem25-device-private/sol-receiver-subtree-r1/review-receipt.json,
SHA256 14ea59e81934595e5f22d81a43afa6e930c2f9ed72a33526cc4761e6f6bc2018.
Astra source review reported no actionable defect; frozen receipt closeout and
Main consumer review remain pending. No fresh nonce or native run is selected.
All native qualification, full-image review and lifecycle gates remain open.

## Accepted completion-refusal diagnostic increment (2026-10-08)

Main accepted a separate, default-off `developmentCaptureCompletionRefusalDiagnostics`
selector requiring the complete existing v11 selection. This source-only increment
addresses an otherwise aggregate final queued-callback refusal. The prior actual
failure does not establish a particular predicate or a deadline cause.

The selected callback runs the original six checks exactly once in their original
short-circuit order: allowed, identity, epoch, token, facts request, temporary facts
request. It records only the first existing failure. `allowed-deadline` requires the
existing allowed guard's cached predicate to equal `deadline`; other allowed
failures remain `allowed-refused`. Remaining fixed reasons are `identity-refused`,
`epoch-changed`, `token-refused`, `facts-request-present`, and
`facts-request-tmp-present`. No native getter, traversal, second check, deadline,
acceptance rule or authority changes. The accepted/baseline/post-read times are
retained existing scalars; failure samples the existing clock once for metadata.
Effective deadline remains min(setup budget, accepted + 5000).

Only selected final-callback refusal adds `completion_refusal` to the existing
callback. Its exact eight fields are kind (`development-capture-completion-refusal`),
version (1), reason, capture_accepted_ms, baseline_ms, post_read_ms, failure_ms and
effective_deadline_ms. Successful/default-off callbacks retain exactly four
fields. Successful completion v2 remains exactly 37 fields and scope v11; the
SDK development transport/parser and owner-refusal wire remain unchanged. Buddy
uses a selected-only 1024-byte callback cap, strict duplicate/unknown/type/reason
validation and retained diagnostic data; these cannot qualify facts or success.

WHEN multiple final checks would fail, THEN only the first evaluated failure is
recorded and later checks remain short-circuited. WHEN the selector is off or the
capture succeeds, THEN no nested diagnostic field appears. WHEN the selector is
requested without v11, THEN configuration refuses. WHEN selected callback data
has duplicate/unknown fields, an unknown reason or a noninteger timing, THEN the
consumer retains no decoded diagnostic and grants no authority.

- [x] Implement the minimal SDK and Buddy diagnostic paths.
- [x] Exact production ARM compile with -Werror/-z defs and no fixture macros.
- [x] Three focused ARM/QEMU owned-fixture checks: selected success shape,
  first allowed failure preceding a later request failure, default-off shape.
- [x] Twenty focused consumer checks and three PowerShell syntax parses.
- [ ] Independent source review and Main acceptance of frozen revisions.
- [ ] Main may separately select a fresh packet and device observation; none is
  selected or authorized by this source increment.

Broad/native qualification, canonical sync and archive remain unfinished.
