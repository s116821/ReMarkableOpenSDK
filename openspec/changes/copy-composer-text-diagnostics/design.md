# Design

## Status and integration
Root accepted precode2d1cb60 before implementation. This change now implements the
bounded read and its focused owned controls; independent review/current CI and
same-PR archive remain pending. Parent typed delivery belongs to PR38.

A small composer_text helper and optional immutable ReadRange extend existing
Attachment preparation. Delivery forwards the range and shares its existing
cancellation atomic with Attachment. Existing callers without a range retain
their behavior. Reuse shared State, one-use claim, original deadline, publication
mutex and Observer. No worker, queue, registry or lifetime framework is added.

The production route remains the accepted Profile plus typed Scene callback.
The internal helper requires a live owner-thread document borrow, with relevant
writers serialized for the entire read. Public diagnostics do not construct
that authority. No new native strong/weak ownership acquisition is introduced.

## Range and result
Signed64-bit start/length form [start,start+length) in UTF-16 code-unit positions.
Check nonnegative values, length<=4096 and addition overflow before preparation;
check against characterCount-1 before narrowing during the borrow. Exclude the
terminal Qt sentinel. Empty/end ranges pass the same admission checks.

Result contains a fixed4096-element uint16_t array, explicit count/range,
NotRequested/Pending/Copied/Refused status, typed failure and optional native
revision-before/after diagnostics. Refusal zeroes public units/count; no accepted
partial prefix. NUL/NBSP/U2028/U2029 and raw isolated surrogates are preserved.
Reject boundaries splitting a valid surrogate pair. No normalization, UTF-8
conversion or grapheme/original-keystroke reconstruction is attempted.

## Concrete getter route
COMPOSER_TEXT_PRIVATE_QT opts into matching private Qt headers. Production copy
requires Qt6.10.3; fixture builds use their own matching host provider. Ordinary
builds compile and refuse requested copying as Unsupported. Macro/header choices
must be consistent across translation units. The existing Profile remains the
loaded-provider gate; this does not select runtime bootstrap.

The audited implementation avoids public cursor/root-frame/object/fragment
getters with layout, lazy creation or hidden traversal. It uses:
- Existing document QObject children and RTTI to find materialized non-root frames.
- Exposed private block/fragment maps to inspect one physical entry per visit.
- Existing format scalar properties and existing block-layout preedit storage.
- Bounded raw characterAt reads after range validation.

Every intersecting frame/table and selected/boundary list/object format refuses.
All touched and endpoint blocks receive composition checks; ordinary text styling
is permitted but not returned. U+FFFC and frame markers in the copied text refuse.
The live coherent Qt document premise includes completed native/Qt transactions;
object discovery does not repair or validate arbitrary corrupt Qt memory.

One8192-visit counter covers both full structural passes: root inspection entry,
every document child, every inspected block and every physical fragment,
including extra boundary entries. Private map lookup avoids unnecessary prefix
block/fragment scans; all actual QObject-prefix visits are counted. No recursion,
whole-document string, hidden same-format scan or unbounded repeat is used.
Qt tree metadata lookups are not a hard real-time bound. Cancellation/deadline
checks occur between structural visits and output units.

See the [getter audit](../../../docs/native/composer-text-getter-audit.md) for
specific Qt sources, accessor behavior and the executed no-callout controls.
Matching headers supply the data access; no copied private layout, access-control
macro, new binary offset or provider patch is involved.

## Coherence and publication
The accepted native join checks loaded identity, focused root and actual document
thread, native operation/IME/nesting/composition/virtual-extension fields and Qt
edit depth. Observer attaches under its existing contract. The text helper also
requires exact QTextDocument type and no Qt edit block, reads revision/count,
performs structural/composition inspection, copies units, repeats inspection and
reads revision/count again. Attachment then rejoins the same native root/document
and rechecks revision/count before final publication admission.

No event pump, IPC, queue wait, edit or application/virtual layout callback occurs
in the copy. All object references stay in the synchronous borrow. Equal finite
revisions and identity scalars cannot establish lifetime, global ABA protection,
an active visit or a genuine edit lease.

The existing mutex orders admitted publication and terminal settlement. Stop's
atomic request and the shared slot cancellation gate prevent further admission;
cancellation observed before final publication admission discards local data.
An already-admitted value can publish before terminal closure, matching the
observer's frozen-prefix model. Stop waits for settlement; no new text publishes
after it returns. Previously published copies remain historical data. Snapshot
reports copy status separately from attachment retirement.

Duplicate callbacks use the existing claim and never rejoin. Late callbacks
refuse before examining Scene. Handle methods remain serialized; stop must not
be reentered from inside native join. Fixture phase hooks can change owned state
or request cancellation, but do not delete an admitted borrowed document.
Native callbacks still return false, and typed slots remain pinned after request.

## Verification and remaining boundary
See [verification](verification.md) for executed exact source/fixture evidence.
Six focused text groups and changed Attachment/Delivery regressions passed on host
sanitizers and ARM Qt emulation; ordinary and private-Qt production headers compile.
Initial compile failures are preserved, and generated outputs expire after review.

There is no actual Xochitl delivery or device qualification. Owned range/page/visit,
writer serialization, folio/OSK semantic Submit, genuine edit lease, conditional
native clear, focus acknowledgement and typed physical retirement remain open.
Consumers requiring genuine content authority continue refusing.
