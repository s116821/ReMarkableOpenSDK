# Existing item-guard operand reporting

Status: reporting-only precode, October 13, 2026. Main owns the exact host
implementation/review/freeze and any separately selected observation. This lane
performs no tablet operation. The [FocusScope admission](passive-qml-focus-admission.md)
and [scheduled diagnostic](passive-object-diagnostic.md) remain the admission,
read and output contracts. Held 8112/equivalent and spent 0404 remain excluded.

## Concrete missing information

The saved selected-qml-focus-v3-r1 observation was read and hash-checked locally:
SHA-256 `da4ab5f938237ce9c14dcca8c02ac3c51d3927118da9e57b2339c79068cb8c9e`.
It records focus-current / visual-chain, OPEN and 32 READs completed, no CLOSE
and unknown remote cleanup. Main reports host completion/refusal in 9.422s and
original cause `item window/visibility/enabled`. The receipt does not contain
the failed operands. Neither the prior candidate equality nor this later refusal
establishes a live class/layout, lifetime, physical visibility or source witness.

The exact selected graph, SHA-256
`e0f8126640651dd570d5d931cf7ff68c091391528a9b1576488ffad1dc264eac`,
already obtains owner from item_window and flags from item_flags_byte, then uses:

```text
if owner != window or flags & 160 != 160:
    raise Refusal("item window/visibility/enabled")
```

Add only a fixed diagnostic at that existing refusal branch. Keep the condition,
cause, exception and terminal failure unchanged. Do not diagnose the historical
receipt retroactively or alter either bit mask to make traversal succeed.

## Existing values and actual provenance

After the existing owner and flags acquisitions both complete successfully,
the refused branch may derive exactly three booleans from those local values:

- window_matches: owner equals the same existing traversal window value;
- effective_visible: bit 32 is present in the same flags byte;
- effective_enabled: bit 128 is present in that same byte.

These operands reproduce the original condition: its refusal corresponds to
at least one false boolean. No active-focus flag, raw flags, pointer, document
value or target string is reported. An all-true diagnostic is inconsistent
with this branch and must not be emitted. Use native booleans, not truthy
strings/numbers; independently validate their domains at the reporting boundary.

Derive callsite from the existing chain's focus boolean: focus-current or
viewport-current, with fixed role visual-chain. Do not guess it from read count,
candidate label, pointer or the last unrelated progress checkpoint. Parent
validation does not execute this condition, so parent labels are invalid here.
No depth or leaf identity is claimed.

Before each of the two unchanged acquisitions, check whether its exact existing
(field address, width) key is already in Graph.spans. Record owner_source and
flags_source independently as read or span-cache. Then execute the original
number/field/read calls in the original order with identical arguments. Reuse
their returned values; never bypass caching, force rereads, merge spans or add
a memory/metadata request. A span-cache operand describes the earlier sampled
value used by this check. An object-cache hit is a separate matter: it must not
be described as fresh type/header/backlink validation, and does not prevent this
operand diagnostic if the existing condition is reached. No new class-validation
claim is added to the payload.

## Single bounded diagnostic variant

Use the existing object_diagnostic result slot for this mutually exclusive
variant. Do not emit a second nested diagnostic or also attach an unsupported-
type classification. The terminal graph cannot proceed from one refused branch
to another. Reject stale/multiple diagnostic state rather than combine evidence.

| Field | Exact domain |
| --- | --- |
| kind | item-window-flags-operands-v1 |
| callsite | focus-current or viewport-current |
| role | visual-chain |
| window_matches | boolean |
| effective_visible | boolean |
| effective_enabled | boolean |
| owner_source, flags_source | read or span-cache, independently |
| binding | pre-read-only |
| object_validated, native_authority | false |

object_validated false means this diagnostic confers no object/lifetime
qualification; it does not erase the earlier checks that actually completed.
The three booleans describe sampled equality/bits only. They do not prove
physical visibility, opacity, clipping, focus ownership, source identity or
whether a transition, stale cached value or invalid lifetime caused refusal.
No candidate sidecar or classifier is used to compute this variant.

Construct the bounded payload inside the existing failure path. Keep it within
the same 512-byte nested UTF-8 ceiling and 8 KiB final result including newlines.
No raw pointers/addresses, flags byte, target strings, arbitrary exception text
or additional output/file channel is allowed. Preserve original cause digest,
scheduled fields, phase, actual OPEN/READ/CLOSE counters and cleanup status.
Progress retains only its existing fixed scheduled callsite/role; do not put
operand booleans or classifier results there or add a checkpoint for this change.

If either acquisition is short, errors, expires or never completes, no operand
diagnostic exists. If optional construction/domain validation fails, retain the
original refusal without guessed operands. If deadline, retention or output
limits prevent a completed result, diagnostic absence stays unknown; it is not
all true or all false. Reporting must not mask the original exception, report
success, renew a clock or continue to active-focus/parent/child/graph reads.

## Exact delta and focused verification

All fifteen admissions and field/layout definitions stay unchanged. Retain the
accepted manifest 14,539 bytes / SHA-256
`7732c113e83d2fcb4ea3bdf822858ce9816eccfef03c4201279ed4fc7a0e3dbc`
and sidecar 3,439 bytes / SHA-256
`9bed08426200b40399d898589a9fa8f694477b829f9431fe0dc25d7736e762dd`.
No new static candidate, source extraction, provider, pin or manifest version
is needed. Keep the original access method, requested/reserved 32 KiB budget,
2s before-OPEN memory clock, 10s whole invocation, graph/metadata/output/progress
caps, complete span rereads and post-process/maps continuity. No guard relaxation,
UI/native action, additional invocation, retry or cleanup behavior follows.

Existing Sol reviews this precise contract against the selected graph. Main
then freezes the smallest reporting successor. Focused local controls should
exercise the seven refused combinations and all-true pass with unchanged read
trace/cause; independent read/span-cache operand provenance and object-cache
reuse; both actual chain callsites; failure before the second value and optional
reporter failure; all-true/malformed output rejection; single diagnostic slot,
no progress operand leakage, and existing deadline/nested/final-output failures.
Do not repeat vendor/layout research or a broad native/job/candidate matrix.
Implementation acceptance and any observation remain separate steps. Historical
operand values, remote cleanup and native qualification remain unknown.

Source basis: exact saved graph and observation checked locally, Main's actual
operation report, existing pinned flag definitions and current coordinator scope.
No new reverse engineering or target execution is performed here; the proposal
is unimplemented and unselected.
