# Passive visual association through existing SFTP

Status: precode method proposal; static manifest prepared; host observer, current
access, operational selection and runtime evidence remain unimplemented/unverified.
Main owns the host observer and is the sole tablet operator. Preparation does not
authorize a tablet read or reuse held 8112/equivalent or spent 0404 operations.

## Problem and intended observation

The completed R1 application QObject tree did not reach the relevant native
objects. This proposal follows different, concretely identified storage: QtGui's
focused window and window list, QtQuick's content root and primary delivery-agent
focus item, visual ancestry, and a typed SceneView/viewport/manager/worker chain.
It addresses the narrower association question in the
[scene-tile research](../../../docs/research/scene-tile-render-geometry.md).

A successful sample would report that the observed focused visual path contains
exactly one allowlisted SceneView and that its viewport, tile manager and worker
references agree before and after bounded reads. A mismatch can falsify that
sampled association. Neither outcome proves rendered content, a stable visit,
request completion, current document/page identity or native qualification.
No new capture receipt, Rust DTO, live admission or consumer pin is proposed.

## Access method and ownership

Use host-owned SFTP random-offset READ requests to one read-only handle for
`/proc/<unique-current-xochitl>/mem`, through the already installed, independently
permitted OpenSSH subsystem and trusted USB SSH identity. No executable or script
is transferred, no code is injected or called in xochitl, and no attach, stop,
input, policy or service change is selected. Starting an existing SFTP session
still uses server-side resources; it is not zero-impact observation.

The [pinned OpenSSH 9.9p2 server source](https://github.com/openssh/openssh-portable/blob/V_9_9_P2/sftp-server.c)
maps a READ-only OPEN to `O_RDONLY`; READ uses its supplied offset with `lseek`,
then reads up to the supplied length and reports returned data/status. It does
not require the file to advertise a nonzero regular-file size. This establishes
protocol feasibility, not the installed subsystem's version, configuration,
permissions or behavior. Main must identify and review the actual existing
subsystem and client before selection. A standard whole-file download command
is not an implementation of this method.

The client must issue one exact outstanding READ at a time. Disable prefetch,
read-ahead, read-until-full, pipelining, reconnect and automatic retry. Do not let
a zero stat size suppress the requested offset read, and do not convert failed
seeks to sequential skipping. Charge requested bytes before sending each READ,
including failed and repeated reads. Reject short/oversized data, EOF, any error
status, unrecognized response, transport loss and deadline expiry. Do not repair
partial data with another request. The ordinary SSH transport may retransmit its
own packets; the prohibition concerns issuing additional SFTP READ operations.

Opening process memory remains subject to the kernel's
[ptrace access check](https://man7.org/linux/man-pages/man5/proc_pid_mem.5.html).
A denial is terminal. No other identity, subsystem, proc-memory tool, debugger
or `process_vm_readv` fallback is permitted. The existing SFTP session is the
selected candidate, not a route around an earlier denied operation.

## Exact profile and manifest

The private `passive-visual-sftp-v1-manifest.json` is generated from the saved
firmware 3.28.0.172 xochitl and exact vendor SDK 5.8.203 libraries. Its 14,107 bytes
have SHA-256 `8bc489413a6cd6b1f301667717e87fed906928211a22c6876e8cfd6a9b850147`.
Artifact addresses, type locators and raw
evidence stay private; this document defines their meaning without creating a
public fixed-address API. The manifest has no execution authorization.

Required SHA-256 values:

| Image | SHA-256 |
| --- | --- |
| xochitl | `071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df` |
| QtCore 6.10.3 | `43b0e210d64e59b534490d78c4c82cc1d2958999b0969aa0f77e11a082704e5d` |
| QtGui 6.10.3 | `93fe582cc61673342ca49e12306d7f689860016582fa46ce135ffa280a972839` |
| QtQuick 6.10.3 | `b20f9adaefe8cebeca445906e891370f75f0b0af878ac1b35e0c91159778628f` |

The manifest contains ELF class/endian/machine/type, all PT_LOAD and GNU_RELRO
ranges, QtGui global symbols and their GLOB_DAT slots, exact primary vptr/typeinfo
pairs, two vetted viewport primary/secondary pairs, getter-confirmed field widths
and offsets, and compile-only layout hashes. The generator validates vtable
offset-to-top and relocated typeinfo references from saved file bytes. It does
not validate live objects.

Resolve each image's unique load bias from its current matching device/inode
mapping at file offset zero and the first PT_LOAD virtual address. Verify the
remaining mappings against the complete load-segment/file-offset relationship,
including split permissions, RELRO and anonymous BSS tails. Do not use the
second segment's mapping start as a library base. Require xochitl's ET_EXEC bias
to be zero and checked ARM32 address arithmetic for every range. Match current
mapped provider files, not merely similarly named files on disk.

Read and corroborate the two QtGui GOT targets against that provider's expected
global storage. No xochitl COPY relocation exists for these globals in the
saved image. Unexpected relocation/interposition is refusal, not an invitation
to search alternate storage. Vtable/typeinfo headers must lie in matching
currently readable, non-writable image mappings. Heap object fields may be in
writable mappings, but are accessed read-only.

The deliberately narrow type allowlist is:

- QtQuick windows: QQuickWindow and QQuickWindowQmlImpl.
- QtQuick agent: QQuickDeliveryAgent.
- QtQuick visual items: QQuickItem, QQuickRootItem, QQuickFocusScope and
  QQuickRectangle; only QQuickRootItem is accepted as the content root.
- xochitl visual items: SceneView, QQmlElement<SceneView>, DeviceSceneViewport
  and QQmlElement<DeviceSceneViewport>.
- xochitl association objects: SceneTileManager and DocumentWorker.

Names describe prevalidated vptr entries; no target class strings, live RTTI
graph traversal, metaObject calls or shape-based subclass inference are allowed.
Unknown QML subclasses refuse. The actual focused-window and ancestor subtypes
have not been observed through this method; a refusal is a useful precise limit,
not permission to broaden the running attempt.

### Address bases and field widths

Every manifest `read_fields` entry names its base, byte offset, width and
interpretation. Resolve the named base before adding that field's offset; never
apply every offset to the public object or to its private data. `address_bases`
defines the binding of each named base to the selected object. The base of a
window, agent or visual-item private field is the pointer read from that exact
typed object's `qobject_dptr` field, with its `qobjectdata_qptr` backlink checked.
Native association fields below are in complete public objects instead.

| Field | Base to which its manifest offset is added | Bytes / interpretation |
| --- | --- | --- |
| `public_vptr` | Current complete typed public object | 4 / pointer |
| `qobject_dptr` | Current complete typed public object | 4 / pointer to its private data |
| `qobjectdata_qptr` | That object's dereferenced private data | 4 / backlink pointer |
| `window_content` | Selected window's private data | 4 / content-item pointer |
| `window_delivery_agent` | Selected window's private data | 4 / agent pointer |
| `window_visible` | Selected window's private data | 1 / visibility value |
| `agent_private_root` | Selected agent's private data | 4 / root-item pointer |
| `agent_private_focus` | Selected agent's private data | 4 / focus-item pointer |
| `item_parent` | Current visual item's private data | 4 / parent-item pointer |
| `item_children_descriptor` | Current visual item's private data | 12 / inline list descriptor |
| `item_window` | Current visual item's private data | 4 / window pointer |
| `item_flags_byte` | Current visual item's private data | 1 / masked flags |
| `scene_worker` | Selected complete public SceneView | 4 / worker pointer |
| `scene_manager` | Selected complete public SceneView | 4 / manager pointer |
| `scene_viewport_interface` | Selected complete public SceneView | 4 / interface-subobject pointer |
| `interface_vptr` | Referenced interface subobject itself | 4 / secondary vptr |
| `viewport_manager` | Complete public viewport after verified adjustment | 4 / manager pointer |
| `manager_worker` | Selected complete public manager | 4 / worker pointer |
| `list_allocation` | Validated inline/global list descriptor | 4 / opaque allocation pointer; never dereferenced |
| `list_pointer` | Same list descriptor | 4 / pointer-array address |
| `list_signed_count` | Same list descriptor | 4 / signed count |
| `vtable_offset_to_top` | Observed primary/secondary vptr value | 4 / signed displacement at its manifest negative offset |
| `vtable_typeinfo` | Same observed vptr value | 4 / typeinfo pointer at its manifest negative offset |

For each QtGui global, `got_read` explicitly uses the matched QtGui load bias
plus its slot offset and reads four bytes. After target corroboration,
`storage_read` uses that validated GOT pointer plus zero: twelve bytes for the
window-list descriptor or four bytes for the focused-window pointer. Window-list
and child-list pointer/count fields are relative to the descriptor, not its
allocation pointer. The viewport base adjustment is arithmetic defined by the
validated primary/secondary type pair, not another memory field or method call.
This typed contract supersedes the unreviewed flat-offset manifest; the route,
allowlist, actual offsets, widths and caps are unchanged.

## Minimal read sequence

1. Main establishes the trusted host/device identity, unique current xochitl
   PID/start time/executable, acceptable process state, matched provider hashes
   and mappings, permitted existing SFTP implementation and independent host
   cancellation. The initial mapping and process identity become immutable
   attempt bindings. Do not repeat R1, R2 or application-anchor discovery.
2. Read QtGui's corroborated window-list descriptor and at most 16 pointer
   entries, plus focused-window pointer. Require one occurrence of that nonnull
   pointer and an allowlisted window vptr/header. Check its QObject d-pointer
   and q_ptr backlink before reading visible, content-root and delivery-agent
   fields. A duplicate focus entry, invalid list, unknown type or invisible
   window refuses. Membership plus focus/visibility is not QWindow::isActive
   or modal-window qualification. Item visibility flags do not establish opacity,
   clipping, lack of occlusion or physical visibility.
3. Validate the agent's exact public type and QObject backlink. Its private
   rootItem must equal the window's typed content root. Read its nonnull focus
   item. This uses the primary delivery agent; unsupported subscene roots refuse.
   The vendor compile-only probe confirms delivery-agent, rootItem and focus-item
   offsets, and the library's actual getter confirms the pointer chain.
4. Follow at most 24 visual items, including the focus leaf and content root.
   Before accessing an item's private fields, validate its allowlisted primary
   vptr/typeinfo header, d-pointer, q_ptr backlink and aligned mapped extents.
   Require the same window, effective visible/enabled flags and an acyclic parent
   chain ending at exactly the content root. The root's visual parent must be
   null. The focus leaf must report active focus. Check each child occurs exactly
   once in its parent's bounded child pointer array; read at most 64 pointers per
   parent and never dereference siblings. Interpret list size as signed ARM32;
   negative/excess counts, nonempty null lists or duplicate selected edges refuse.
5. Require exactly one SceneView on the complete chain and its active-focus flag.
   A root-only/overlay-only focus chain or multiple SceneViews is inconclusive;
   do not pick the nearest view, enumerate other windows or fall back to BFS.
   Read the view's fixed manager, worker and ISceneViewport references only.
6. The interface must match one of the exact DeviceSceneViewport secondary-vptr,
   offset-to-top and typeinfo pairs. Derive the complete object by the verified
   base adjustment and require its corresponding primary vptr and QObject
   backlink. The saved accessor performs that adjustment, but must not be called.
   Validate the viewport's visual ancestry through the same SceneView to the same
   root, at most 24 items including viewport/root, with the same type/window/
   visibility/membership rules. Only the focus chain requires active-focus flags;
   the viewport need not itself own focus.
7. Require the view and viewport to reference the same nonnull typed manager,
   and the view and manager to reference the same nonnull typed worker. Validate
   the manager/worker primary headers and QObject backlinks. No queue, request,
   QString/page ID, controller, document, pixel, transform or callback body is
   read. DocumentWorker's QObject affinity is not worker-command serialization.
8. Reread the exact governing global/list/object/private-field/selected-array
   spans used in this attempt, with identical values required, then recheck
   process/start/executable identity and maps. No discovery expansion occurs in
   this second pass. Process or map change, inaccessible ranges, changed fields,
   ambiguity or unfinished verification prevents a matched sample.

All fixed-width reads require full mapped readable coverage before transmission.
Never read unrelated bytes merely to fill a buffer or merge separated fields.
Vtable/typeinfo headers and repeated shared ancestors may be cached during the
first pass only with an explicit list of spans to revalidate; cache reuse does
not create a lifetime guard. Reserve the entire recorded reread cost before
accepting each new span, including shared spans, so verification cannot silently
be omitted at the byte limit. Caps are ceilings, not a promise the largest graph
fits within both time and bytes.

## Budgets, output and recovery limits

| Resource | Fixed maximum |
| --- | --- |
| Window pointer list | 16 entries |
| Focus ancestry | 24 items including leaf/root |
| Viewport ancestry | 24 items including viewport/root |
| Child pointer array per checked parent | 64 pointers |
| All `/proc/PID/mem` READ lengths | 32 KiB total, including failures/rereads |
| Scalar result | 8 KiB, including refusal details |
| Cooperative observation | 2 seconds from immediately before memory-handle OPEN; never renewed |
| Independent host observer/session bound | 10 seconds for the complete selected invocation; never renewed |
| Maps/stat metadata | Two maps snapshots at most 128 KiB each; each stat snapshot at most 4 KiB |

Provider hashing/preflight is completed by Main before this invocation and then
bound to unchanged file/process/mapping identities. Metadata reads are separately
capped; they must not be hidden inside the memory budget or a whole-file proc
download. Their unknown-length EOF detection must use bounded requests whose
total requested lengths do not exceed the respective caps; exact cap without
proven termination is refusal. Observer output contains only bounded status,
identity/digest bindings, counts, phase, relationship results and evidence limits.
Addresses/type offsets, if necessary for private refusal diagnosis, remain private.
Do not emit arbitrary strings, raw memory, full object trees or document content.

The two-second clock is checked before and after each operation and before a
matched result. Main's independent host guard closes/terminates only the owned
observer/session by ten seconds. A cooperative deadline cannot preempt a blocked
SFTP operation. Closing transport or killing the client does not prove that a
server-side kernel read has returned or its handle is already closed. Main's
separate continuity/recovery assessment must report unknown server cleanup rather
than invent confirmation or signal/restart xochitl. No automatic retry follows.
These bounds limit issued work and host ownership; they do not establish a hard
execution bound on the kernel or absence of scheduling/page-fault overhead.

Results distinguish matched sampled association, observed relationship mismatch,
unsupported/ambiguous graph, access/read/transport failure, deadline/cap refusal
and process/map/field change. A mismatch is a sampled contradiction, possibly
during a transition, not automatically a native defect. Every result states
non-atomic observation, no lifetime retention, no accepted-request history,
no document/page identity, no pixels and no native/render/input authority.

R2's DocumentController is not this view's SceneController. No pointer equality
or historical-address join is allowed. A fresh same-process controller/document
wrapper relationship remains a separate missing source edge; this attempt does
not read or rerun R2 to manufacture it. Equal before/after values cannot exclude
ABA, an intervening away-and-back visit, cancellation or a freed/reused object.

## Tasks and verification before selection

- [x] Verify saved provider digests, ELF storage/relocations, exact type headers,
  getter edges and compile-only agent/root/focus offsets; freeze private manifest.
- [ ] Independently review this precode proposal and exact manifest.
- [ ] Main implements/reviews the smallest host observer and binds exact client/
  subsystem semantics, refusal output and independent host timeout/cleanup.
- [ ] Verify only the concrete host transport/parser hazards: exact offset/length,
  no prefetch/retry, short/error response, byte/reread reservation, unknown type,
  changed chain and blocked-child cancellation. Owned evidence is not tablet access.
- [ ] Main independently selects/announces a fresh permitted attempt after current
  preflight, or records the exact method blocker. No device execution here.
- [ ] Interpret any later sample without closing source/render/lifetime gates.

Source basis: exact saved ELF/provider inspection, vendor compile-only layout
measurements, pinned primary Qt/OpenSSH/Linux source, repository R1/R2 results
and Main's explicitly proposed existing-SFTP method. The read policy, budgets
and inference limits are an unimplemented design, not an observed native result.
