# Design

Use the existing immutable copied page, ResidentSlot token, typed request and
one-shot Attachment. Add an immutable mode selector with Observe as the legacy
default, including optional ReadRange, and Scalars as an exclusive mode that
rejects any range. The exact mapped QtGui provider already required for revision
and the embedded private layout must also resolve the four scalar document
getters. Use the existing pinned QtCore ABI for QObject signalsBlocked.

Keep root, control, participant, document and document-private correspondence
in the callback-local join. Compare all five identities and focused ID across
both joins. Both joins require the stock type and participant back-reference,
document-thread affinity, zero native transaction and IME state, and zero Qt
edit depth. Read control identity as an opaque value.

Sample isUndoRedoEnabled, isUndoAvailable, isRedoAvailable, maximumBlockCount,
revision and signalsBlocked. Independently read the ARM QtGui private bytes at
0x40 and 0x41 as cachedWasUndoAvailable and cachedWasRedoAvailable. Require
literal 0/1 for both. These values are comparison caches in Qt 6.10.3. Qt can
update them after an activation returns. They provide no receiver-delivery
evidence. Compare the entire tuple across both joins, then check cancellation
and the same original deadline under the attachment lock before publication.

Report NotRequested, Pending, Observed or Refused independently of snapshot
availability, attachment and text status. Only Observed carries scalar data.
After logical retirement a completed sample remains historical. Valid blocked
signals, redo and cache mismatches are reportable states. The branch returns
false without observer creation, copying text, traversing receivers, another
enqueue or damage flush. A completed sample and balanced callbacks do not
retire the typed request's native owner pins.
