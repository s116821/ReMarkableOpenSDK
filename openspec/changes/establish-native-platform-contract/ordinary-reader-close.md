# One ordinary document-close dispatch

## Source-backed question

The [ordinary close source](../../../docs/research/ordinary-reader-close-source-boundary.md)
identifies the declared DocumentView close function, separately from navigator
window close. Add a synchronous research helper using the existing selected
Reader/creation join's callback-local reader. It does not discover a different
receiver, invoke a native offset or add a product close API.

## Dispatch contract

Require a fresh ready join matching exact document/page/index/count, GUI thread,
same live window/root/engine and the existing source guard. Check one public,
zero-argument slot declared on that exact reader metaobject with QVariant
return type. Its invocation is the terminal action inside the guarded source
callback. Recheck the exact reader metaobject after the final source/permission
callout before using the selected method; a live pointer alone cannot establish
unchanged dynamic method metadata. Mark attempted before direct invoke, then do
not consult the old
source guard or reader pointers. The close is expected to invalidate them.

Consume an explicit shared one-use claim before any callout, including source
discovery. Nested, repeated, canceled, invalid and failed requests cannot renew
it. Retain the original elapsed-time budget across discovery and action. A
non-preemptible QML call can exceed it; report the overrun and retain possible
effects rather than retrying. The separate supervisor/recovery bound remains
necessary.

The result contains only copied pre-action facts and dispatch accounting. A
successful meta-call means dispatched; it does not mean UI closed or ink saved.
Any attempt requires fresh reconciliation. Do not add a signal wait using the
old callback-local owner, retain borrowed guards, or acknowledge a mutation as
durable based on documentClosed. Fresh closure, reopen/identity/capture and full
fixture-preservation observations belong to the later composed writing packet.

## Owned controls and limits

Exercise the actual helper with owned Qt/QML reader fixtures, modeled native
joins explicitly labeled. Include wrong identity/method/thread/context,
reentry/claim reuse, receiver deletion/notification during dispatch, thrown
callouts and deadline overruns. Native method availability and actual close/save
behavior remain separate, candidate-specific qualification.
