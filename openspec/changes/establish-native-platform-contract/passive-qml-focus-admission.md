# One exact QML focus-scope admission

Status: source-only precode, October 13, 2026. Main owns the host successor,
focused implementation review/freeze and any later separate operational selection.
No tablet action is part of this work. Held 8112/equivalent and spent 0404 remain
excluded. The [window admission](passive-qml-window-admission.md) and
[scheduled-object diagnostic](passive-object-diagnostic.md) retain all their
requirements except the exact appended entry and local pin/count changes below.

## Evidence and single supported candidate

Main's selected-object-diagnostic-r2 observation was read and hash-checked locally:
SHA-256 `0d51d4b3eb04b2aead7dda1dbf3c656bfdf45fc570c99e99dbcc8352f5dc121c`.
It explicitly records focus-current / visual-chain, vptr_source read, zero
admission matches and one diagnostic qml-qquickfocusscope equality. OPEN and
26 READs completed, but the operation still refused with the original cause;
CLOSE was not completed and remote cleanup is unknown. Execution belongs to
Main. This is actual scheduled-callsite evidence, not a read-count inference,
but remains pre-read-only, object_validated false and native_authority false.

Propose only the exact pinned QtQuick primary pair for
`QQmlPrivate::QQmlElement<QQuickFocusScope>`, role `visual-item`, offset-to-top
zero. Its private locators are copied from the already reviewed diagnostic
entry. Exact saved-image checks reconfirm the primary header and relocated
typeinfo, wrapper single-inheritance RTTI to QQuickFocusScope, and that base's
single-inheritance RTTI to QQuickItem. The QtQuick provider SHA-256 remains
`b20f9adaefe8cebeca445906e891370f75f0b0af878ac1b35e0c91159778628f`.
Header typeinfo relocation is ARM RELATIVE; wrapper/base typeinfo references
use ARM ABS32. No actual observed pointer is embedded into the manifest.

Pinned [Qt 6.10.3 focus-scope source](https://github.com/qt/qtdeclarative/blob/v6.10.3/src/quick/items/qquickfocusscope.cpp)
constructs QQuickItem and sets its focus-scope flag. The matching vendor header
adds no public data members; the previously reviewed QQmlElement wrapper also
adds none. A focused compile-only check with the existing vendor SDK 5.8.203
ARM compiler/headers finds equal wrapper, QQuickFocusScope and QQuickItem sizes,
zero conversions from wrapper to QQuickItem/QObject, and zero conversion from
QQuickItemPrivate to QObjectData. This supports reuse of the already admitted
QQuickFocusScope/QQuickItem layout, not generic inheritance-based admission.

## Every inherited field reached by the unchanged traversal

The focused compiler output checks the following against the unchanged typed
manifest. Addresses and numeric locators stay in private evidence.

| Existing field or check | Base and concrete evidence |
| --- | --- |
| Public vptr and its primary header | Complete public object; exact saved pair, zero signed offset-to-top and relocated typeinfo; live header reads still required |
| qobject_dptr | Same public QObject base; compiler confirms the existing four-byte pointer offset, with no wrapper adjustment |
| qobjectdata_qptr | Validated private QObjectData base; compiler confirms the existing four-byte backlink offset and zero private-base adjustment |
| item_window | QQuickItemPrivate; compiler confirms the existing four-byte window pointer offset |
| item_flags_byte | Same private base; generated ARM byte loads use the existing byte offset; activeFocus bit 0, effectiveVisible bit 5 and effectiveEnable bit 7 reproduce existing masks 1/32/128 |
| item_parent | Same private base; compiler confirms the existing four-byte parentItem offset |
| item_children_descriptor | Same private base when checked as a parent; compiler confirms childItems offset and twelve-byte QList descriptor |
| Descriptor allocation/pointer/count | Existing QArrayDataPointer layout: three ARM32 fields at the unchanged offsets; allocation remains opaque, pointer array and signed count retain original interpretation and caps |
| Selected child pointer array | Existing descriptor pointer, at most 64 four-byte pointers; check selected child exactly once, do not dereference siblings |

The generated flag accessors are inspected as data and never executed on a
target. They do not add flag reads or getter calls. Existing focus-leaf active
focus, effective visibility/enabled, matching window, parent cycle/depth and
selected-child membership predicates are unchanged. This role does not permit
reading SceneView-specific fields or treating the scope itself as the required
unique SceneView, viewport, manager or worker.

Evidence limits: private offsetof emits the retained conditionally-supported
non-standard-layout warnings. This is exact-image RTTI plus vendor-header
compile and pinned constructor semantics, not a new binary constructor trace,
live private-layout measurement or lifetime lease. That limitation is explicit;
all original live coverage/header/typeinfo/backlink and continuity checks remain
mandatory. No remaining source-layout gap was found for the fields listed above
under this exact profile; runtime validity and eventual graph completeness remain
unproven. Do not infer that other QML wrappers share an admissible layout.

## Frozen successor and minimal host delta

Preserve every earlier manifest, sidecar and selected receipt. Pin this new pair
together before connection:

| Private artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| passive-visual-sftp-v3-qml-focus-manifest.json | 14539 | `7732c113e83d2fcb4ea3bdf822858ce9816eccfef03c4201279ed4fc7a0e3dbc` |
| passive-object-diagnostic-v2-candidates.json | 3439 | `9bed08426200b40399d898589a9fa8f694477b829f9431fe0dc25d7736e762dd` |

Relative to v2 manifest `3e1e8365...`, only the new QtQuick entry, profile
`rm2-3.28.0.172-passive-visual-sftp-v3-qml-focus`, and base_sdk_commit
`ae5732262af33ab12bbd9296a03b04e62fd43b83` change. Schema version remains 1;
the base commit is preceding-contract provenance, not implementation identity.
All fourteen old entries remain byte-equivalent as JSON; there are now fifteen.
All provider identities, segment/relocation/global records, field definitions,
offsets, widths, interface pairs, flags, limits and evidence conditions remain
identical. Authorization remains false. QQmlElement<QQuickRectangle> and the
unadmitted window wrapper remain diagnostic candidates only.

Relative to scheduled diagnostic sidecar `c9964df2...`, only the baseline
manifest digest changes. The same four candidate entries and diagnostic-only
semantics remain. Update the host's exact pair/profile/preparation binding and
all-admission count/domain to 15 (1 still invalid on the unsupported branch),
and report the new sidecar digest. Preserve the repaired count/label consistency
predicate, optional-failure core retention, fixed callsites/roles, read/cache
provenance, 512-byte nested/8 KiB total result and 1 KiB progress limits. Reject
mixed pins, stale preparation, unknown entries or permissive multi-version loading
before target requests; do not rehash providers or issue new metadata requests.

## Permitted traversal and unchanged refusal

A new exact vptr match may proceed only through the existing readable-range and
provider permission checks, live zero-top/exact-typeinfo header, d-pointer and
q_ptr backlink validation. There is no pointer-base subtraction. Only then may
the existing window/flags/parent fields be read; child-list fields are read only
at the existing parent-validation callsite. A type/header/backlink mismatch or
wrong role refuses before those fields. All subsequent graph requirements and
unknown-type refusals remain, including the unique SceneView and viewport path.

This can permit later already-defined reads beyond the previous refusal, not
invent new reads or declare the historical object validated. Charge requested
bytes and reserve rereads before transmission under the same 32 KiB ceiling.
Keep 2s from before OPEN, 10s whole invocation, original metadata/graph caps,
single outstanding request, terminal short/error/cap/deadline behavior, complete
span rereads and post-process/maps continuity. Never renew clocks, use another
reader/provider/fallback, expand the family or select an extra invocation here.
No classification or sampled chain supplies document/page/render/pixel/accepted
request history, atomicity, lifetime, remote cleanup or native authority.

## Focused evidence and handoff

Private source receipt `focus-admission-v3-source-freeze-20261013.json`, SHA-256
`a63808f265299aff2b617137961735f2a971db52974dae55d2133dfadb8b36a7`,
records exact frozen pair/diff, static primary/base assertions, observation digest,
and compile input/assembly/script/warning hashes. Source checks assert equal
sizes, zero casts, inherited fields/list layout and the three flag accessors.
Private files have no public download links. This is focused source evidence;
no host observer or device execution was performed by the SDK lane.

Existing Sol reviews this exact precode and artifacts. Main then implements and
freezes only local pin/profile/count changes, checking the concrete new branch:
exact scope reaches inherited fields only after valid header/backlink; bad header,
typeinfo/backlink refuses first; wrong window/required flags or changed span
refuses; rectangle wrapper and mixed pins remain refused; byte/reread/deadline
guards still apply. No broad repeated research/runtime matrix is required.
Any later observation is Main's separate selection with fresh ownership/identity
and notice. Canonical sync/archive and native qualification remain open.

Source basis: exact saved observation checked locally, retained pinned-provider
RTTI, focused vendor-header compile and primary Qt source. Actual tablet execution
is Main-attributed; this proposal is unimplemented and unselected here.
