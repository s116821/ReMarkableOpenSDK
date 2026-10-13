# Scheduled object refusal diagnostic

Status: source-only precode, October 13, 2026; no host implementation or tablet
operation by this lane. Main owns implementation/review/freeze and any separately
selected observation. The accepted [one-window admission](passive-qml-window-admission.md)
at `c18997b7abe97af277ff34daabfd2af0e882f92f` and its 14-entry manifest stay
unchanged. Held 8112/equivalent and spent 0404 remain excluded.

## Question and scope

Main's selected-qml-window-v2-r1 receipt records 26 completed READs followed by
the original unsupported-object-type refusal, no first-window diagnostic,
OPEN completed, no CLOSE and unknown remote cleanup. Root reports the exact
447-byte receipt SHA-256
`1a1781536385d9db3edeaf1aeb73a5cea6a77ff77de9b2ab79da0f0d7409f9cc`.
The supplied read-order reconstruction is conditional on cache/alias behavior;
it does not identify the actual rejected callsite or class. Do not recompute a
callsite from read counts or retrospectively label this receipt.

The successor reports the actual scheduled object-validation callsite and role
set at the existing `len(matches) != 1` branch, using its already obtained vptr
and exact original match count. It replaces the first-window-only diagnostic
with one bounded object diagnostic, never emits both, and preserves the original
unsupported-object-type refusal. No admission, object traversal, memory read,
provider, fallback, metadata request, RPC, clock or cap is added or enlarged.

## Actual callsite attribution

Pass a fixed host enum explicitly at each existing `object` call, without changing
its address, role set, order or cache behavior. `chain` uses its existing focus
boolean to choose the corresponding fixed current/parent callsite:

| Callsite | Exact existing role set |
| --- | --- |
| `window` | window |
| `content-root` | content-root |
| `delivery-agent` | delivery-agent |
| `focus-current` | visual-item, content-root, scene, viewport |
| `focus-parent` | visual-item, content-root, scene, viewport |
| `viewport-primary` | viewport |
| `viewport-current` | visual-item, content-root, scene, viewport |
| `viewport-parent` | visual-item, content-root, scene, viewport |
| `manager` | manager |
| `worker` | worker |

The output's fixed role label is the sole role name above, or `visual-chain` for
the four-role set. Validate callsite/role correspondence locally; never derive
it from addresses, class guesses, unordered set serialization or exception text.
These enums describe scheduled checks, not the actual type of the object.
No parent depth is claimed. In particular a parent validation occurs before the
next current-node iteration; do not report both as the focus leaf.

An object-cache hit executes no admission matcher and cannot produce this
diagnostic. At an uncached object's existing vptr acquisition, retain whether
the exact span was already in the span cache; output `vptr_source=span-cache`
or `read` accordingly. Do not force a fresh read or bypass either cache to gain
diagnostics. Short, failed or expired reads do not produce a vptr classification.
If a valid cached span is used, equality describes that earlier sampled value
at this actual scheduled check, not a newly acquired target value.

## Exact candidate file

Preserve the accepted v2 admission manifest (14,328 bytes, SHA-256
`3e1e83652fba03dbd9b1e8e897fd7e3fc29445a20d1a180645f4ac00bed58f03`)
and its prior diagnostic sidecar unchanged. The proposed successor instead pins
`passive-object-diagnostic-v1-candidates.json`, 3,439 bytes, SHA-256
`c9964df2baf8b19c3bc0a09ccbb04b40be54557fa0479e8931f4fb848bdca352`.
Compared with the old v2 diagnostic sidecar, only purpose changes to
`scheduled-object-vptr-diagnostic-only` and two entries append. The base manifest
digest, provider hash, schema and two window entries remain identical.

All four fixed labels bind to exact primary-vptr/typeinfo pairs in the already
pinned QtQuick image SHA-256
`b20f9adaefe8cebeca445906e891370f75f0b0af878ac1b35e0c91159778628f`:

| Fixed label | Static candidate |
| --- | --- |
| `qml-qquickwindow` | QQmlPrivate::QQmlElement<QQuickWindow> |
| `qml-qquickwindowqmlimpl` | QQmlPrivate::QQmlElement<QQuickWindowQmlImpl> |
| `qml-qquickfocusscope` | QQmlPrivate::QQmlElement<QQuickFocusScope> |
| `qml-qquickrectangle` | QQmlPrivate::QQmlElement<QQuickRectangle> |

Only the two named item wrappers were newly inspected; there was no broad type
enumeration. Exact mangled-name locations lead to single-inheritance RTTI with
the corresponding defined base typeinfo relocation (`R_ARM_ABS32`, zero addend).
Each primary header has offset-to-top zero and `R_ARM_RELATIVE` to that wrapper's
RTTI. The retained script uses file-backed PT_LOAD address translation. Full
provider and sidecar hashes bind the bytes and selected interpretation; addresses
and extracted material remain private. This establishes diagnostic locators,
not inherited item-layout admission or the rejected object's class.

As in the accepted diagnostic, reject wrong/mixed pins, duplicate/extra entries
or labels locally before connection. Before SFTP launch/OPEN, use only the same
invocation's existing fresh map/provider binding to derive all four expected
values with checked ARM32 arithmetic. Require distinct values and matching
readable/non-writable QtQuick coverage for each header, first vtable word and
typeinfo word. No additional target bytes or metadata are requested. Failure is
terminal, without falling back to another sidecar/provider. The old sidecar
remains historical evidence, not a second active classifier.

## Bounded refusal and failure semantics

At the original unsupported branch, first retain the core callsite, fixed role,
vptr source and exact match count over **all 14 admission entries**. Candidate
comparison is optional diagnostic work and cannot erase that core. Attach one
`object_diagnostic` to the ordinary refused result, with these exact fields:

| Field | Domain |
| --- | --- |
| `kind` | `scheduled-object-vptr-equality-v1` |
| `callsite`, `role` | Fixed corresponding labels above |
| `vptr_source` | `read` or `span-cache` |
| `admission_matches` | Integer 0 through 14, excluding 1 at this branch |
| `candidate_matches` | Integer 0 through 4, or null if classification failed |
| `candidate` | `none`, the sole fixed matching label, `ambiguous` for multiple, or `unavailable` if classification failed |
| `candidate_manifest_sha256` | Exact new sidecar digest above |
| `binding` | `pre-read-only` |
| `object_validated`, `native_authority` | false |

Four-way multiplicity is defensive only; distinct properly bound addresses
cannot simultaneously equal one word. Never pick a preferred label. On an
optional classifier exception, use null/unavailable while retaining core fields
and original refusal. Do not swallow the entire diagnostic merely because that
optional work failed. If the core/result itself cannot be retained or the
deadline/output bound prevents completion, preserve the existing bounded failure
path and report the diagnostic as unavailable by its absence; never invent
counts. Absence is not a zero-match result.

The entire nested object must fit the existing 512-byte serialized UTF-8 ceiling
(including a separate newline if used), within the same 8 KiB final result plus
newline. Use compact fixed output; no addresses, raw vptr, arbitrary class/error
strings, lists of objects or second classifier output channel. Preserve original
cause digest, phase, actual OPEN/READ/CLOSE counts and cleanup status.

For existing host progress only, retain the fixed scheduled callsite and role
before the associated object validation awaits, within the existing 1 KiB
committed/1 KiB temporary record and original retention/deadline policy. Do not
put match counts, candidate labels, addresses or classifier results there.
Progress records the last scheduled check, not its completion or refusal;
timeouts and missing/malformed/stale records remain unknown. No new file,
retention cap or write-after-timeout is allowed. This supplies bounded scheduling
context if no final result survives; it cannot turn a timeout into a diagnosis.

Candidate equality remains pre-read-only and unvalidated: no live header, type
layout, backlink, lifetime, post-continuity, visual/document/page/render or native
authority follows. A unique hint supports separate source review only. Zero
hints leaves the class unknown, but the actual scheduled failing check is now
identified when the final diagnostic is retained. No automatic retry or type
addition follows. Preserve 32 KiB charged/reserved reads, 2s memory phase, 10s
whole invocation, all metadata/graph/output bounds and cleanup uncertainty.

## Focused handoff

The exact precode and private candidate artifact require existing Sol review.
Main implements/freezes the reporting delta only after that review. Focused
owned controls should verify explicit current/parent callsites and exact role
mapping (including cache reuse), unchanged read trace and original refusal,
zero/unique/defensive multiple matches, classifier-failure core retention,
no diagnostic on object-cache hits or failed/short reads, wrong pins/coverage
before contact, and nested/final/progress size and deadline failure behavior.
Do not repeat completed layout/native research or a broad test matrix. No
successful graph, new admission or device operation is part of this handoff.

Source basis: Main/Root's attributed actual receipt and conditional source-order
assessment, the exact selected graph read locally, retained pinned window
evidence, and focused saved-image RTTI inspection for the two item wrappers.
Private artifacts have no public download link. Implementation, selection and
native qualification remain open.
