# Design

## Status and integration
Precode proposal for integration-owner review; no implementation or executed
text-copy controls are claimed. Parent typed delivery is archived in PR38.

Add a small composer_text diagnostic helper and an optional immutable ReadRange
to Attachment preparation. Delivery forwards that optional request to its
existing Attachment. Existing callers without a range retain their behavior.
The request is copied before native enqueue; one attachment can produce at most
one text result under its original deadline. Reuse its shared State, claim flag,
cancellation gate and publication mutex. Do not add another worker, owner token,
queue, observer connection family or lifetime manager.

The public production route remains the accepted Profile plus typed Scene
callback. An internal helper may take a live document reference only from that
admitted join; a fixture-only seam supplies owned Qt documents. This does not
introduce a public arbitrary-pointer or borrowed-document authority constructor.

## Input, output and bounds
ReadRange contains signed 64-bit start and length, measured in QTextDocument
UTF-16 code-unit positions. The interval is half-open [start, start + length).
Before integer narrowing or addition, require start >= 0, length >= 0,
length <= 4096, and a representable non-overflowing end. During the borrow require
characterCount >= 1, start <= characterCount - 1 and
length <= characterCount - 1 - start. The final Qt document sentinel is excluded.
Zero length is a successful empty read at an otherwise admissible boundary,
including the end of an empty document; it does not bypass structural checks.

Use a fixed array of 4096 uint16_t elements and an explicit used length (maximum
8192 payload bytes), with requested range and optional native revision-before/
revision-after scalars. No NUL-terminated interpretation, UTF-8 conversion,
normalization, trimming or implicit full-document inference. Result states are
NotRequested, Pending, Copied and Refused, with typed refusal reasons. Refused
results expose zero text units, never an accepted partial prefix. Native scalar
facts remain diagnostics and contain no dereferenceable retained object.

Preserve code units exactly: embedded U+0000, U+00A0, U+2028, U+2029, combining
marks, surrogate pairs and ZWJ sequences. U+2029 denotes the document's paragraph
separator; it does not identify the original keyboard newline spelling.
Reject a start or end between a high and low surrogate. Otherwise raw isolated
surrogate units, if present in the document, are preserved without replacement.
This is lossless UTF-16 storage, not a promise of a valid Unicode scalar string
or whole-grapheme selection. A future transport needs its own lossless encoding.

## Admission and bounded structural inspection
Run only during the trusted native Scene borrow on the actual document owner
thread. Reuse exact provider/profile and focused-root joins; require the exact
QTextDocument type, matching root/document identity, participant operation scope,
IME scope and nesting all zero, native compose start -1, compose length zero,
virtual extension zero and Qt edit depth zero.

Admit ordinary root-frame paragraphs only. Refuse a selected or boundary block
inside a table/child frame, list blocks, inline object formats or U+FFFC. Ordinary
font/style formatting may remain, but this result contains text only and grants
no formatting or object preservation claim. Check every touched block and the
blocks containing both interval endpoints; an endpoint at a new paragraph
therefore also checks that paragraph, even for an empty interval.

Use root-frame/block/fragment iterators and positional interval checks to prove
root membership and inspect the actual selected character formats. Avoid
QTextCursor positioning: Qt6.10.3 setPosition calls setX and can enter layout.
Do not use cursor charFormat as a selected-character test; it normally refers
to the preceding character. Do not flatten all document text or copy entire
fragments to obtain a short slice.

A single structural-visit budget of 8192 counts every frame-entry, block and
fragment inspection, including skipped prefix entries needed to reach the
range. Exceeding it refuses with InspectionLimit, without widening or retrying.
Thus a late range in a highly fragmented document can deliberately refuse.
Skip disjoint child-frame subtrees without recursion; refuse intersecting or
endpoint-containing child frames. No unbounded whole-document scan or temporary
whole-document string is permitted. Use characterAt only for the bounded output
and at most four adjacent code units for surrogate endpoint checks. All copied
positions must have passed the structural checks; NUL is data only after bounds
have been verified, since an out-of-range Qt read can also return NUL.

Check preedit on each touched/boundary block's QTextLayout and refuse any nonempty
preedit. Do not create a document layout, request shaping, fetch a resource or
call a virtual layout provider. Audit the chosen block/layout getters against
the pinned Qt source before implementation acceptance. These local checks make
no claim of absence of unrelated preedit elsewhere in the document.

## Coherence, reentry and lifetime
Inside the admitted callback, use this sequence:
1. Validate the native join, document affinity/type and quiescent fields.
2. Record revisionBefore, validate range/structures/preedit and copy raw units.
3. Revalidate the join and the same document/root scalar correspondence plus all
   native transaction/composition guards, touched-block preedit and document
   character count; record revisionAfter.
4. Require matching revisions and unchanged correspondence. Recheck the existing
   cancellation/duplicate state and original deadline before publishing.

No edit block, text mutation, nested event pump, IPC, queue wait or application
callback is allowed between revision reads. No arbitrary caller hook runs under
the Attachment mutex. Temporary block/layout/frame references remain inside the
borrow. Do not retain Scene, item, document, cursor or frame pointers in the
published value. The native callback continues to return false.

The native borrow must keep the document alive and serialize relevant writers
through the complete synchronous read. Equal finite revisions and identity
scalars do not prove that premise, defeat ABA/full counter wrap, or make a lease.
A getter path that can invoke user/virtual code, or cannot be shown to fit this
premise, is a source blocker rather than permission to rely on a second revision
read as lifetime protection. This proposal adds no native ownership acquisition.

Attachment callback claim/try-lock rules reject duplicate and concurrent entry.
Cancellation or expiry before publication discards the local result; exception,
revision change, composition or join drift produces a typed refusal without
text. Existing stop waits for admitted callback settlement and closes the
observer. Stop remains forbidden as reentrant same-thread join work, as in the
accepted parent contract; do not add callbacks that could create that deadlock.
Test-only phase hooks can inject owned state changes or atomic cancellation,
but must not delete a document while the helper holds an admitted live borrow.

Publication and stop settlement use the existing mutex. A copied value published
before closure remains frozen historical data, like the observer prefix; closure
does not retroactively revoke copies already returned. Snapshot reports copy
status separately from attachment retirement. After stop returns, no new text
can publish. Late callback copies refuse before touching their Scene argument.
Logical cancellation/copy completion does not permit typed slot reuse, native
owner release or payload unload.

## Planned controls and acceptance
After proposal acceptance, exercise the actual helper and Attachment/Delivery
integration using owned Qt documents on host ASan/UBSan and the pinned ARM Qt
provider under emulation. Compile production headers without fixture macros.
Controls must assert exact units/count, explicit refusal and unchanged native
document content, revision, selection state and Undo/Redo availability.

Cover empty/end ranges; negatives, overflow and 4096/4097 limits; NUL/NBSP,
paragraph/line separators, combining marks, supplementary characters, ZWJ,
isolated surrogates and split-pair endpoints; multiline and styled root text;
tables, nested frames, lists, images/custom objects and boundary-only structure;
preedit in selected/endpoint blocks; inspection-budget exhaustion in fragmented
prefixes; wrong thread, native/Qt transaction guards and before/after revision or
identity drift. Include duplicates, original deadline, cancellation before/
during publication, stop settlement and late callbacks after handle destruction.
Use deterministic owned fixture hooks, never timing-based race sleeps or deliberate
dangling-pointer document access. Check that no layout/resource callback is
entered by the read path. Native field refusal remains exercised through the
existing fixture join seam; fake guards are not real Xochitl execution.

Run only these meaningful focused controls once PR-ready, and repeat changed
controls only for a concrete failure/change. CI owns the ordinary broad checks.
No test runs are part of this precode publication. Source/fixture acceptance does
not qualify actual device selection, delivery, serialization or draft authority.

## Source basis and remaining boundary
Current accepted Attachment/Observer/Delivery source supplies dispatch and
settlement. Retained R2 findings (SHA256
4228949cc270dd9db2dae0e6be696f532dd3661f5785047008c73b579dc1faee)
supply native field, finite revision and UTF-16/paragraph evidence; they are
offline evidence, not runtime observation.

Qt6.10.3 primary source confirms raw characterAt storage access and the cursor
layout/format pitfalls:
- https://raw.githubusercontent.com/qt/qtbase/v6.10.3/src/gui/text/qtextdocument.cpp
- https://raw.githubusercontent.com/qt/qtbase/v6.10.3/src/gui/text/qtextcursor.cpp

Caps, optional API shape, structural refusal policy and publication semantics
above are proposed design decisions. Actual owned range/page/visit provenance,
native writer serialization, folio/OSK semantic Submit, genuine edit lease,
conditional native clear, focus acknowledgement and typed physical retirement
remain separate unfinished work. Consumers must retain their current refusal
when genuine content-revision authority is absent.
