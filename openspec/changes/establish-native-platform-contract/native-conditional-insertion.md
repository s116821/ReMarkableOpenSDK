# Separate conditional insertion from the negative witness

The fixed negative Core and its deliberately different frozen source remain the
default. The native negative and post-rejection mutex results do not grant
insertion. Add a separate correct-source condition and a separately selected
one-forward policy, with owned controls before any changed native packet.

## Passive correct-source condition

Use a new original header/namespace `reader_insertion_condition`, not a new
interpretation of `observedMismatch`. Freeze source index, current page count,
source ID, requested target ID and optional immediate-right ID before worker
dispatch. Canonical IDs are distinct; a right ID is required exactly for a middle
source and absent exactly for the last source. Permit a single-page document.

At the qualified original inner call, require the effective insertion index to
be source+1 and effective target to equal the requested target. Read the active
ordered span, require its count equals the frozen count and search the source at
its frozen index. For a middle source, search only the immediate right ID and
require source+1. For the last source, require insertion index==count without an
out-of-range right lookup. Retain the active-record cap of sixteen.

Target absence must use the separate full ID-tree find. Active `indexFor(-1)`
conflates absent IDs and existing IDs outside the active vector. The original
outer can replace a collided requested UUID with a generated UUID; the inner can
return void on collision while the outer still emits its success-shaped tail.
Neither outcome establishes the requested insertion. Gate both effective-ID
equality and full-tree absence before forwarding.

For the exact ARM profile, tree=map+96, end sentinel=map+100, root=map+104 and
node count=map+116. A separate checked header read must bound the actual tree
count (not the active count) before calling the qualified passive find. Require
nonzero aligned root distinct from the end sentinel, tree count at least active
count and at most sixty-four. An owned Tree{root,count} result carrier is not the
native header layout: the injected readTree receives the tree address and fills
these words from the separate +8 and +20 offsets after checking through +24.
Find must return exactly the end sentinel. Re-read the active span and tree
header after passive calls; changed values refuse. No allocation, QString
construction/detach, GUI calls, I/O, locks or arbitrary reporting in this helper.
Return facts and refusal causes, not a caller-supplied permission boolean.

This normal-data bound assumes a valid finite native tree under the original
final mutex; it does not validate every node, repair cycles, prove all-writer
exclusion or impose a preemptible wall-clock bound. The native profile must
qualify the actual find code and compareStrings provider/GOT binding separately
from the existing equalStrings binding. No new native accessor is exposed until
those actual loaded bindings and lifetime/thread/callsite contracts are checked.

## One forward and cancellation boundary

Registration gains an explicit policy, default NegativeOnly. Admit exactly the
matching frozen form. Preserve one registration, one claimed outer operation,
resident owners and untracked-call behavior. Before the first tracked insertion forward,
the new policy uses atomic Decision{Pending,Canceled,Rejected,ForwardCommitted}.
Cancellation competes with the final pending-to-committed compare/exchange;
deadline, cancellation and interference are checked before that transition.
Earlier original handler loading/status effects remain possible. Clock checks are
sampled; crossing a deadline after the last check cannot be made atomic with the
commit compare/exchange and requires the post-commit reconciliation path.
If cancellation wins, reject through the original private-unwind path. If commit
wins, invoke originalInner exactly once with unchanged arguments; later
cancellation cannot revoke it or throw the private rejection after an effect.

Keep this transition in a separate original owned helper
`reader_insertion_forward_decision::Decision`, with State{Pending,Canceled,
Rejected,ForwardCommitted}. forward()/cancel()/reject() each attempt one strong
pending-state compare/exchange and return Transition{changed,state}; current()
reads the state. No clock, callback, allocation or I/O belongs to this primitive.
It does not dispatch an insertion, imply original return or record all later
cancellation requests. The coordinator/Core must retain those separate facts.
Test deterministic winner cases and real owned competing threads before wiring
the helper into Core. A committed state means the tracked forward was admitted,
not that an insertion happened.

Record forward attempted, inner returned and outer returned separately. Change
normal outer-return handling only for this new policy; the negative policy keeps
its existing MissingInner interpretation. Any original inner/outer-tail exception
after commitment is an unknown possible effect requiring read-only reconciliation,
not automatic retry or document rollback. Late cancellation/deadline/interference
likewise cannot be reported as clean pre-effect cancellation. Preserve unrelated
exception propagation and direct fail-stop behavior for uncertain ownership.

## Coordinator, result and acceptance

The new coordinator deliberately supports middle and last source cases while
retaining old negative-mode validation. Keep early/foreign/duplicate signal
checks, exact active-job attribution, ordinary callback identity and retained
native owners. A separate closed result consumer must distinguish rejection,
forwarded-but-incomplete and verified observation; no old negative receipt passes.

Ordinary method acceptance, native void return, a callback, a matching completion
or pageAdded alone does not prove insertion. Require the committed forward,
matching job/completion/pageAdded at the expected insertion index and callback,
then fresh page identity/order showing count N+1 and the complete expected order:
all original expectedPages with the target inserted exactly once immediately
after the original source and before the frozen right neighbor where present.
Preserve the full expected order in the coordinator, outside the locked helper.
Adjacency alone cannot exclude collateral page loss or reorder. Selection,
writing, reopen/persistence and full source/ink/PDF metadata preservation remain
separate native/product acceptance checks. Ambiguous outcomes reconcile without
retry or duplicate creation.

Owned controls cover middle/end/single-page sources, wrong source/right/count/
span/tree header, active-only false absence, effective-ID substitution/collision,
both cancellation race outcomes, exact forwarded arguments/once, inner and
outer-tail exceptions after forwarding, success-shaped tails without insertion,
and early/foreign/duplicate signals. Native activation requires a new reviewed
source/artifact/provider/packet and recovery plan. No supported capability, safe
unload or owner release follows from this design.

Source basis: existing source contracts, native negative/mutex observations and
retained exact-binary tree/collision/comparator findings (private receipt SHA-256
`6ef3886b1b08ac3ad26faf99e3f062671da8a42a190fbcc7ce7c618c6a8d2c2c`).
APIs and controls above
are a proposed successor; conditional implementation and native evidence remain
open.
