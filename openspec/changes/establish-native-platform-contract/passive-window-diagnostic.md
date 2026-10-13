# First-window vptr refusal diagnostic

Status: source-only precode amendment, October 13, 2026. Main owns any host
implementation, focused local verification and separately selected observation.
This lane implements no observer and performs no tablet operation. The
[baseline passive method](passive-visual-sftp-method.md) at SDK
`91f0973e4eb07dda1757fb2328872cb3f6b2cd6b` remains the admission/access contract.
Held 8112/equivalent and spent 0404 operations remain excluded; neither this
amendment nor a diagnostic result authorizes replay or another access route.

## Problem and exact scope

The [reviewed window finding](../../../docs/research/passive-window-admission.md)
at `ae7076d77fb16557c00ed7c5d02d6205801b38bb` identifies two missing concrete QML
wrapper vtables but does not identify the class in the failed observation.
The original first-window matcher uses the same generic refusal for zero and
multiple admission matches, without retaining either count. A future refusal
can distinguish these cases using the already-read four-byte vptr and existing
invocation bindings, without reading any additional target bytes.

This amendment adds only a bounded diagnostic on the **first selected window's
original `len(matches) != 1` refusal branch**. It does not intercept other object
checks, change the 13-entry admission table, suppress the original refusal, or
continue the graph after a candidate match. A unique original admission match
follows the unchanged baseline validation path; later header/role/backlink
failures do not acquire this diagnostic. No extra reread, header/RTTI/string
read, metadata request, RPC, provider hash or window enumeration is added.

## Exact static candidate binding

The original private manifest remains byte-identical: 14,107 bytes, SHA-256
`8bc489413a6cd6b1f301667717e87fed906928211a22c6876e8cfd6a9b850147`.
The separate private `passive-window-diagnostic-v1-candidates.json` is 1,925
bytes, SHA-256
`0b41f62847b9075d04e0f8e057882d9ff16197a09853d3557140526d6f10cd50`.
It is diagnostic data, never appended to the admission manifest. Main must pin
both exact byte lengths/digests in the reviewed successor before selection;
missing, modified, duplicate or extra candidate entries refuse locally.

Both candidates belong exclusively to the already pinned ARM32 little-endian
QtQuick 6.10.3 image, SHA-256
`b20f9adaefe8cebeca445906e891370f75f0b0af878ac1b35e0c91159778628f`:

| Fixed output label | Static candidate |
| --- | --- |
| `qml-qquickwindow` | QQmlPrivate::QQmlElement<QQuickWindow> |
| `qml-qquickwindowqmlimpl` | QQmlPrivate::QQmlElement<QQuickWindowQmlImpl> |

The saved-image inspection maps each virtual address through its containing
file-backed PT_LOAD segment, using `file_offset + (VA - segment_vaddr)`; it does
not assume VA equals file offset. The primary header's signed offset-to-top
word is zero. Its following typeinfo word has `R_ARM_RELATIVE` (23), with the
file word as the image-relative addend, pointing to the recorded wrapper RTTI.
That RTTI uses `__si_class_type_info`; its base slot has `R_ARM_ABS32` (2), zero
addend, and the defined QQuickWindow or QQuickWindowQmlImpl typeinfo symbol.
The sidecar retains the locators and relocation records. Full-image SHA-256
binds those file bytes; the sidecar SHA-256 binds their selected interpretation.
Neither relocations nor class names are discovered from the live process.

Before memory OPEN, use the **same invocation's fresh, already required**
QtQuick provider identity, complete mapped-segment validation and unique load
bias. Resolve both expected primary-vptr values with checked ARM32 arithmetic;
require distinct values and matching readable, non-writable provider coverage
for each eight-byte header, first vtable word and recorded typeinfo word, using
the already obtained maps only. A failed binding refuses before memory OPEN;
it does not trigger repair or an extra metadata request. Do not use preparation
maps in place of that invocation's fresh binding. The existing mapped-file
identity/hash requirements are unchanged.

Only equality with `QtQuick_load_bias + recorded_primary_vptr_VA` is evaluated
against the observed word. No pointer-base subtraction, nearest-symbol match,
address-range heuristic, inherited-type inference or alternate provider is
allowed. Templates 2, QQuickView and other possible types are outside this
two-entry diagnostic. No public result contains addresses, offsets, raw words
or names obtained from target memory.

## Refusal result and unchanged control flow

At the specified branch, retain the original admission match count across all
13 original entries (not just entries with the window role). Compare that same
already-read vptr to the two prebound candidates. Attach one optional fixed
`window_diagnostic` object to the existing refused result:

| Field | Exact domain |
| --- | --- |
| `kind` | `first-window-vptr-equality-v1` |
| `admission_matches` | Integer 0 through 13; at this branch 1 is invalid |
| `candidate_matches` | Integer 0 through 2 |
| `candidate` | `none` for 0; matching fixed label for 1; `ambiguous` for 2 |
| `candidate_manifest_sha256` | The exact sidecar digest above |
| `binding` | `pre-read-only` |
| `object_validated` | `false` |
| `native_authority` | `false` |

Construct labels from fixed host constants, never exception text or target
strings. Keep the nested object at most 512 serialized UTF-8 bytes including
its own trailing newline if serialized separately, within the unchanged 8 KiB
whole-result limit including its final newline. Do not create a second output
channel or bypass the existing failure-output accounting. If no qualifying
four-byte read/branch completed, omit the object; absence is not a zero match.
If diagnostic construction/serialization fails, retain terminal refusal using
the existing bounded failure path, never a successful or guessed classification.

The classification is evidence only of vptr equality at one read. It does not
validate a live vtable header, object/private-data backlink, lifetime, displayed
class or visual association. `pre-read-only` explicitly means the early refusal
did not finish the baseline post-read identity/maps/field continuity checks.
Do not describe a matching static label as a verified runtime object class.
Preserve the original failure, OPEN/READ/CLOSE counters, phase, deadlines and
honest remote-cleanup status even when diagnostic data is available.

The original 2s clock from before memory OPEN, 10s complete invocation bound,
32 KiB charged/reserved memory budget, two maps snapshots, stat caps and all
window/ancestry/child limits are unchanged. Local classification consumes the
same clocks; it cannot renew them. Deadline/cap/transport/cleanup failures remain
terminal and authoritative. No automatic retry, further graph read, additional
remote cleanup request or new CLOSE behavior follows from this amendment.

## What a result decides

| Observation | Next source-only decision |
| --- | --- |
| Admission 0, candidate 1 | Evidence supporting review of that exact pinned wrapper as a possible future admission addition; any header/layout/backlink validation and admission change require separate precode and review. This run still refuses. |
| Admission 0, candidate 0 | The two candidate hypotheses did not match this read; retain unsupported type. No broader scan or automatic retry follows. |
| Admission greater than 1, or candidate greater than 1 | Inspect host table/binding consistency; do not choose a preferred entry or infer a class. |
| No diagnostic object | Read/branch not established or diagnostic unavailable; use the original failure evidence without filling in counts. |

These outcomes say nothing retrospective about the previous failed run. They
also do not close source/document, render, lifetime, recovery or native gates.

## Focused verification and delivery tasks

- [x] Freeze the two-entry static sidecar from the exact saved QtQuick image;
  preserve the original manifest and publish this precode contract.
- [ ] Independently review this amendment and exact private sidecar.
- [ ] Main implements and freezes the smallest reporting-only successor.
- [ ] Main/reviewer verify with owned synthetic inputs: zero/one/two candidate
  equalities and zero/multiple admission counts; fixed-label/no-address output;
  wrong hash/extra entry/overflow/wrong-provider coverage refusal; no diagnostic
  before a completed exact read or outside the first-window branch; identical
  transport/read trace and unchanged original refusal with a candidate match;
  output limit and deadline failures remain terminal. These are local controls,
  not a new tablet qualification matrix.
- [ ] Main separately assesses/selects any fresh permitted observation with
  current ownership, identity and advance notice. Source review is not a run.
- [ ] Attribute any receipt using the decision table; leave canonical sync,
  archive and unfinished native qualification open.

Source basis: current REM-25 issue/comments (latest returned October 13
11:12:49 UTC), selected observer source and saved refusal inspected locally,
reviewed SDK `ae7076d`, and exact saved provider/relocation inspection. Current
Root/Main coordination supplies this narrow requested scope. Proposed output
and tests are unimplemented requirements, not new runtime evidence. Private
sidecar and original receipts have no public download links.
