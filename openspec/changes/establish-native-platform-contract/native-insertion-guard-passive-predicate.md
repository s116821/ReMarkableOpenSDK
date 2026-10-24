# Fixed passive predicate for the negative native witness

This is the reviewed read contract for the proposed negative-only witness.
It does not select native code, an artifact or execution. Implementation and
exact profile/provider/native qualification remain separate.

## Entry authority and immutable inputs

The read contract applies only to the recognized fresh operation at the exact
original insertion callsite, with its matched outer scope, Worker incarnation,
native owners and calling thread. Original Worker+176 final lock ownership must
come from qualified control flow, not mutex probing or pointer equality alone.
Any unqualified recognized scope follows the proposal's fail-stop policy; it
does not execute a helper, forward, or throw through uninspected frames.

Freeze before submission, on the GUI path:

- A nonnegative real-source index `s`, known wrong-source position `w != s`,
  middle-insertion index `p = s + 1`, original GUI count `f`, and right-neighbor
  index `s + 1`, all checked without signed overflow.
- Distinct real-source/wrong-source UUID values, the immediate right-neighbor
  UUID, requested target UUID and exact operation/binding facts. The wrong UUID
  is known in this same fixture at `w`; it may equal the right-neighbor UUID if
  `w == s + 1`. This is not an unrelated-document or corrupt-input test.
- Actual public-Qt QString objects for these UUIDs: 36 UTF-16 code units,
  canonical lowercase hex with hyphens at 8/13/18/23, no braces/NUL, validated
  before freezing. Keep those actual value objects alive and immutable
  throughout the worker read and exception path; any public-Qt construction,
  copying or detachment must happen before this stage. Do not construct native/private QString
  representation by copying its words or touch its refcount under the lock.
- Fixed `MaxCurrentRecords = 16`, with `2 <= f <= 16` and all frozen positions
  in range. For this deliberately fixed fixture, require actual count equal to
  `f` as a separate conservative positive-witness condition; the actual native
  cap below is still mandatory and cannot be replaced by this GUI value.

Operation recognition precedes validating bindings, and the negative-only
zero-forward barrier is independent of every read/predicate/cancel outcome.
There is no accepted insertion state for this diagnostic.

## Exact-build opaque receivers, not native C++ classes

Retained ARM establishes map `M = Worker + 200` at the original call. It also
establishes search receiver `V = Worker + 280 = M + 80`, current ordered begin
word at `Worker + 284 = M + 84`, and end word at `Worker + 288 = M + 88`.
The ordered search's observed record stride is eight bytes. These are pinned
machine offsets for this executable, not a declared std::vector/shared_ptr/Page
layout or a general PageMap API.

Before any native lookup:

1. Check the frozen Worker/map relationship and every receiver/address addition
   for uintptr32 overflow, nonzero addresses and required four-byte alignment.
   Read only the two current pointer-width begin/end words from this qualified
   native object while its final lock is held. No native count/getter call.
2. Treat those words as unsigned addresses `B` and `E`; do checked integer
   arithmetic, not C++ subtraction of invented array pointers. Require both
   nonzero, four-byte aligned, `E >= B`, and `0 < E - B <= 128`.
3. Require `(E - B) % 8 == 0`; let `n = (E - B) / 8`. Require `2 <= n <= 16`,
   `n == f`, `s < n`, `s + 1 < n`, `w < n` and `p == s + 1`. Recheck the
   effective insertion argument equals `p`. The helper's signed differences
   are then small positive values, rather than unchecked wraparound values.
4. Compute frozen expected record addresses `B + 8*s`, `B + 8*(s+1)` and
   `B + 8*w` with checked multiplication/addition, each strictly before `E`.
   These identify record positions; no Page or refcounted owner is loaded by
   adapter code and no private record object is synthesized.

Eight-byte **stride/span congruence** is the established requirement; it does
not establish an eight-byte native alignment type. The shown ARM pointer loads
need four-byte alignment. An experiment may additionally reject a non-eight-
aligned `B` as a stricter policy, but must label that as policy, not recovered
ABI. Neither alignment nor a mapped-range test validates arbitrary heap data.

This contract assumes normal live, internally consistent storage protected by
the original mutex. A span cap cannot validate every page/QString pointer the
native search will dereference. Strong owners and matched call provenance
supply normal lifetime assumptions; corruption/noncooperating writers remain
outside the proof. Do not add IO or a lock-probing API to pretend otherwise.

## Fixed searches and bounded comparison work

Use only the already-retained exact native `0x7b1f60..0x7b2140` search and its
observed machine inputs: opaque `V` address and an address of one frozen actual
QString; returned existing-record/end address. Pin the executable code range,
main mapping and exact Qt comparator providers before enabling this route.
Calling convention evidence is machine-level; no original private declaration
is inferred. No function pointer is handed out by this document.

At most three lookups, always for the preselected real-source, right-neighbor
and wrong-source strings, stopping at the first failed check. Do not search the
target tree, enumerate IDs, follow returned IDs/indexes or discover new input
positions. A returned address must be in `[B,E]`; `E` means not found. A found
address must be strictly before `E` and `(returned - B) % 8 == 0`. Require it to
equal the corresponding expected record address in this very same span.

The normal native search compares at most `n` records per call. At cap 16,
three calls inspect at most 48 records, with at most 48 content equality calls.
The native length checks compare content only when its size equals the frozen
36-character expected size. Thus each equality's content work is bounded by
36 UTF-16 code units under the qualified comparator contract; a record with a
different length is a mismatch without a long string scan. This bounds normal
comparison work, not wall-clock time, OS scheduling or malformed-memory faults.

These searches prove, under the stated valid-map assumptions, real source at
`s`, immediate right neighbor at `s+1` and deliberately wrong source at `w != s`.
They do not need to materialize/copy the page's actual QString. The actual-source
scalar can be reported as matching the frozen real source only after its exact
record-position check succeeds; it cannot be inferred just from wrong-source
search failure or a GUI snapshot. A mismatch is refusal/uncertainty, never a
new search plan or permission to use a newly found position.

The effective target QString at the inner argument can be compared to the
frozen requested target through existing passive `0x144d9c..0x144dec` equality,
using live actual QString addresses. Its length check bounds normal comparison
by the same frozen 36 characters. A mismatch, including generated collision
fallback, remains a tracked refusal; it cannot bypass outer eligibility.
The inner template argument is not a source-ID input and is not dereferenced
as one. No source-ID fact is derived from it.
Including this target comparison, at most 49 normal content equality calls are
possible: 48 from the three capped searches and one for the effective target.

Finally reread the same begin/end words and require `B/E` unchanged. This is a
consistency check, not an ABA/version/all-writer proof. There is no GUI read,
event pump, IO, allocation, ownership/refcount operation or arbitrary callback
in the passive read stage. Qualified native value-comparison imports are its
only downstream calls. Unexpected throwing/interposed comparators violate its
noexcept/cantunwind assumptions; a catch does not make them recoverable.

## Negative result, exception boundary and omitted tree question

For the positive refusal witness, all binding/span/position/argument checks
must hold, actual source must match the frozen real source and differ from the
deliberately wrong expected source, and the helper-forward counter must be zero.
The private rejection is emitted only from the qualified wrapper/callsite path.
It still must traverse original cleanup into the outer private catch, followed
by independently attributed native job/Task completion and preservation checks.
An unexpected match, missing record, changed span, cancel/deadline, invalid
binding or fail-stop is not this positive predicate-mismatch result.

The scalar passive read stage is distinct from C++ exception transport.
Throwing may invoke exception-runtime allocation/bookkeeping while the original
mutex is held; the entire wrapper must not be advertised as allocation-free.
Qualification must cover that actual unwind path. Allocation failure or an
unrelated exception is unsuccessful/unknown with independent recovery, not a
fabricated passing refusal. Retained records, QStrings and native owners must
not be released by an implicit static destructor or worker cleanup.

Full ID-tree absence is deliberately **not** established here. The active-order
search and the tree at `M+96` have distinct semantics and bounds. Because the
tracked negative operation never forwards, tree absence is unnecessary to prove
that this invocation did not insert; target absence is instead checked by the
complete baseline/post-readback. An accepted creation proposal would need a
separately justified tree/collision contract and cannot inherit authorization
from this refusal result.

Source basis: retained exact ARM for 0x6d5bf0/0x6d5c98/0x7b1f60/0x144d9c,
canonical final-lock findings and ABI/LSDA review pinned by the prior receipt,
and the revised negative-only OpenSpec. Cap 16, exactly three frozen searches,
count equality and rejection interpretation are proposed diagnostic policy.
No native implementation, all-writer/GUI visit lease or shipping capability is
qualified by this draft.
