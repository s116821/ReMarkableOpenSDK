# Fixed two-request resident Reader diagnostic

This implements the smallest native checkpoint in
[request retirement](resident-request-retirement.md), within the existing
research proposal. It is a fixed diagnostic, not a generic production IPC service.

One activation nonce authorizes a frozen plan containing two distinct request
nonces, the same expected document/page/index/count and a single normal fixture
open. The host publishes one existing identity-bound atomic request for the
whole plan. The adapter bootstraps that fixture once, waits the existing bounded
loading opportunity, and creates two separate read-only request tokens in turn.
Neither request navigates, creates a page, submits worker work or changes services.

Each request constructs a new synchronous selected Reader/creation-controller
Sample. A process-resident registration retains only weak engine/window identity
and owned scalar results between requests; it retains no document/controller,
native ID argument, borrowed permission or Sample. Request B starts on a later
GUI turn only after A's tracked Sample stack, cleanup and retirement finish.
Both samples must report the frozen expected identity, coherent map and readiness
with the same live engine/window and unchanged process incarnation.

The new retirement fence spans every request callback/getter and all associated
local destruction. The per-request cleanup closes its owned duplicate directory
descriptor before retirement. Terminal, retired and published remain separate.
The result records both fresh nonces/generations, both observed identities and
timings, each retirement/cleanup, one bootstrap and shared-process/window facts.
All native/render/durable authority flags remain false.

The existing process-bound files and original activation directory/owner/request
checks guard the plan. Its closed bounded result is published once, with unknown
post-link outcomes retained. One fixed 30-second plan budget is clipped to the
original 90-second setup window; each Sample is capped by the remaining plan time.
No request, retirement or timer renews the original independently supervised
360-second actor deadline or 185-second recovery reserve. One final stock
restoration occurs after the whole experiment; no restart separates A and B.

WHEN A is not retired or cleanup fails, THEN B performs no getters and no second
request is admitted. WHEN the original request/root changes, restoration begins,
the setup/plan expires or the pinned window/engine changes, THEN later phases
refuse without reopening, retrying or renewing any clock. WHEN A is retired,
THEN a later fresh B may observe under its own identity and permission.

- [x] Implement/review fixed pair registration and startup, keeping existing
  single-request artifacts unchanged.
- [x] Verify owned valid/failure/lifetime/window/deadline/publication paths and
  same-window fresh Sample scopes on host and ARM.
- [ ] Freeze/review a new exact native source/payload/package and fresh baseline.
- [ ] If selected, record actual A retirement then B join in one process, complete
  preservation, original final restoration and exact cleanup.

Source/control checkpoint: the retirement core passes 743 checks on host Qt 6.4.2
and ARM Qt 6.10.3/GCC 13.4 under QEMU. The fixed pair passes 19 modes and five
invalid configurations on both, invoking the actual owned selected-view helper
rather than mocking its observations. Checks cover distinct nonces/generations,
original A retirement/cleanup before B, fresh getter calls, request descriptor
counts, source readiness, cancellation/deadline/window replacement, request inode
replacement, nested event delivery, duplicate output and post-link uncertainty.
The separate startup shared object compiles on both and was never loaded by these
checks. Earlier fixture expectation and local shell line-ending failures remain
preserved; they are not native outcomes. No existing Entry/startup source changed.

No mutation, native drain, unload, historical shutdown repair or shipping
always-on transport follows from this checkpoint. The finite retirement nonce
history is a research limit; production duplicate/journal policy remains separate.

Source basis: current one-shot Entry's file/clock contract, selected-view Sample,
positive native creation/worker results and the request-retirement proposal.
The fixed pair and wire are engineering design, not native evidence.
