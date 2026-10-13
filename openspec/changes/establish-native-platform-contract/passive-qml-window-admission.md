# One exact QML window admission

Status: source-only precode successor, October 13, 2026. Main owns the host
successor, its review/freeze and any later separately selected observation.
This proposal performs no tablet action. Held 8112 transfer/activation/equivalent
replay and spent 0404 remain excluded. The [baseline method](passive-visual-sftp-method.md)
and [diagnostic amendment](passive-window-diagnostic.md) retain all requirements
except the exact manifest bindings and admission/count delta specified here.

## Evidence and justified scope

Main's `selected-window-progress-r2/observation.json`, 747 bytes, SHA-256
`8e690628bb65029aba12d1ef4ed1fe9f0758d679a600170e9ba0e2be725b54a4`,
records memory OPEN and six READs completed, zero original admission matches
and one diagnostic equality with fixed label `qml-qquickwindowqmlimpl`.
The original unsupported-object-type refusal remained; no CLOSE was attempted
or completed and remote cleanup is unknown. The diagnostic explicitly reports
pre-read-only binding, object_validated false and native_authority false.
This receipt was read and hash-checked locally; execution belongs to Main.
Root's current REM-25 comment `74e98b08-19bd-4e26-9e6a-1b73e1ba30ed` corroborates
the same receipt and reports host 9.172s/10s, exit 1 and Active0. Neither report
explains the previous timeout or establishes a live object's class/lifetime.

The sole proposed new entry is the already identified, exact pinned QtQuick
`QQmlPrivate::QQmlElement<QQuickWindowQmlImpl>` primary vptr/typeinfo pair, role
`window`, offset-to-top zero. No other wrapper, subclass or provider is added.
In particular QQmlElement<QQuickWindow>, QQuickView and Templates 2 types remain
outside admission. A diagnostic label is never used as an admission predicate.

The [retained static research](../../../docs/research/passive-window-admission.md)
binds this pair to the exact QtQuick image, its primary zero-top header, relocated
wrapper RTTI and single public nonvirtual QQuickWindowQmlImpl base. That base
already has an accepted entry in the old method. Its QQuickWindow, QWindow and
QObject primary-base chain has zero displacement. The secondary parser-status
interface is not the stored focus-window pointer and is not traversed.

Pinned [Qt QML construction](https://github.com/qt/qtdeclarative/blob/v6.10.3/src/qml/qml/qqmlprivate.h)
defines this wrapper with no added data members and constructs it through its
base. The [window implementation](https://github.com/qt/qtdeclarative/blob/v6.10.3/src/quick/items/qquickwindowmodule.cpp)
passes QQuickWindowQmlImplPrivate through the QQuickWindow private constructor.
A focused compile-only check using the existing vendor SDK 5.8.203 ARM headers
confirms equal wrapper/base size, zero-adjusting conversions to QWindow and
QObject, zero private-base adjustment, and the same inherited content-item,
delivery-agent and actual QWindow visibility offsets as the baseline manifest.
The QML implementation's separate visibility property is not substituted for
the existing actual-window field.

This combines exact-image RTTI with vendor-header layout and upstream semantics;
it is not a new binary-constructor trace or a live-object validation. The compiler
warns that offsetof on these non-standard-layout private classes is conditionally
supported. Keep the exact profile and all live range/header/backlink refusals;
do not generalize this evidence to other builds or unknown subtypes.

## Exact frozen successor and compatibility

The old manifest and diagnostic sidecar remain unchanged for historical runs.
Select the following pair together, by exact local byte length and SHA-256:

| Private artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| passive-visual-sftp-v2-qml-window-manifest.json | 14328 | `3e1e83652fba03dbd9b1e8e897fd7e3fc29445a20d1a180645f4ac00bed58f03` |
| passive-window-diagnostic-v2-candidates.json | 1925 | `b83007d5a1506dc895824672290a214a7f6a1365b568d27d899ce69dcee888cd` |

The manifest schema version stays 1. Its only JSON changes from the frozen
14,107-byte v1/`8bc489...` are the appended QtQuick entry (14 total admissions),
profile `rm2-3.28.0.172-passive-visual-sftp-v2-qml-window`, and base_sdk_commit
`55996ff39f14a12b1e99ba63506998bef4c07ae8` identifying the preceding contract.
That base field is provenance, not the successor implementation identity.
Provider hashes, PT_LOAD/RELRO descriptions, globals, all prior entries,
typed field bases/offsets/widths, interfaces, read policy, caps and evidence
limits are structurally identical. Authorization remains false.

The v2 diagnostic sidecar changes only base_manifest_sha256 to the new manifest
digest. Its two candidates, locators and provider are unchanged; it still
confers no admission. Its existing first-window refusal hook now counts all
14 admitted entries, so admission_matches has domain 0 through 14 (1 remains
invalid on that branch). Candidate count/label domains, fixed kind, 512-byte
nested/8 KiB total output, pre-read-only scope and failure behavior stay the
same. Report the selected v2 sidecar digest, not the old receipt's sidecar digest.
On a unique new admission the diagnostic refusal hook is not entered.

The loader, preparation binding and exact source freeze must agree on this
new pair before connection. Reject mixed old/new manifests, stale preparation,
wrong hashes and unreviewed entries; do not silently accept either manifest
under a shared permissive loader. No new remote hash/check is needed: retain
the existing fresh provider/process/map checks and unchanged access path.

## What admission permits the observer to attempt

For a fresh focused-window pointer occurring once in the bounded window list,
read the same four-byte public vptr. Compare it to the exact 14-entry table
using the current validated provider biases and checked ARM32 arithmetic.
Require exactly one match; unknown or ambiguous matches still refuse.

If the new pair matches, execute the **existing** window validation sequence:
matching readable/non-writable provider coverage; read signed offset-to-top
and relocated typeinfo and require the exact pair; check typeinfo coverage;
read the QObject d-pointer and require the private q_ptr backlink to equal the
same complete public address. No pointer adjustment or runtime RTTI walk is
introduced. Only after those checks may the existing actual-visible,
content-root and delivery-agent fields be read from the validated private base.
All agent/root/leaf/ancestor/window/selected-child membership checks and unknown
type, ambiguity, cycle, cap and changed-field refusals remain in force.

Thus this candidate could let traversal attempt the inherited window fields
and, if each subsequent check succeeds, the existing focus/SceneView/viewport/
manager/worker chain. It promises neither another admissible node nor a
successful chain. It adds no new field/read definition; a formerly refused
attempt may issue later reads from the already specified sequence, charged
before transmission with the same reread reservation and complete continuity
requirements. It does not merely declare the historical equality successful.

Keep 32 KiB total requested memory including failed/repeated reads, 2s from
before memory OPEN, 10s whole invocation, original metadata/graph/output caps,
single outstanding read and terminal short/error/deadline failures. No clock
renewal, second invocation, diagnostic reader, fallback, provider or native
action is added. Existing progress/cleanup reporting remains unchanged; host
termination does not prove remote cleanup. A graph reaching the end still needs
all original rereads and post-process/maps continuity before a sampled result.
Even success supplies no atomicity/lifetime lease, document/page identity,
render/request history, pixels or native authority.

## Focused source verification and handoff

Private `window-admission-v2-source-freeze-20261013.json`, SHA-256
`50cf6b4960ba6a19e971962dd518cf32d7163782fdc57d160bdb8cff069f8282`,
records the exact one-entry structural delta, provider identity, historical
receipt digest and compile input/output hashes. The separate sidecar rebind
check proves only its baseline digest changed. These are local source checks,
not a host-observer or tablet run; private artifacts have no public download link.

Before selection, the existing reviewer checks this exact amendment, frozen
pair and focused evidence. Main then implements/freezes only the necessary
loader/profile/count changes and supplies focused local controls: the new exact
pair reaches unchanged header/backlink checks; wrong header/backlink refuses;
the other diagnostic-only wrapper remains unsupported; mixed pins/preparation
refuse; changed spans and original byte/time limits remain terminal. Verify
the concrete delta without rerunning a broad research or runtime matrix.
Main alone assesses any later permitted operation with fresh identity, ownership
and advance notice. Canonical sync/archive and native qualification remain open.

Source basis: Main's saved receipt checked locally, current REM-25 comment,
reviewed SDK static evidence, pinned Qt source and the focused vendor-header
compile. Proposed admission/traversal is unimplemented and unselected here.
