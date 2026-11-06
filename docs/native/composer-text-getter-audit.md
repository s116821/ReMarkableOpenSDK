# Text-copy getter audit

The read is opt-in with COMPOSER_TEXT_PRIVATE_QT and matching private Qt headers.
Production text copying is compiled only for Qt6.10.3; the accepted native Profile
still pins loaded providers. Ordinary builds compile and return Unsupported for
a requested copy. Fixture builds allow their own matching host Qt version.
Use the same macro/header configuration across translation units.

The first compile exposed inaccessible fields inside QTextDocumentPrivate.
The final source uses its exposed get/isInEditBlock/blockMap/fragmentMap/
formatCollection accessors; no access-control macro, copied private layout,
new binary offset or patched provider is used.

## Executed route

| Access | Audited behavior and bound |
| --- | --- |
| document thread/type/revision/count | Exact live QTextDocument, owning thread, finite native revision. No subclass override is admitted. Revision/count getters read private scalar/map data. |
| document children / RTTI frame cast | QObject::children returns existing child storage; C++ dynamic_cast does not invoke QObject meta-object overrides. Every child, including unrelated prefix objects, consumes one visit per pass. QTextObject construction parents frame objects to their document. This relies on coherent live Qt document structure under the existing borrow/transaction premise. |
| frame parent/first/last position | Read existing frame state and map positions. Never call rootFrame, frameAt, objectForIndex or objectForFormat, which can materialize objects. Every non-root frame intersecting the range or either endpoint refuses, including nested frames/tables. No child-tree recursion. |
| private block map | findNode seeks the start; next visits one physical block. Each visited block, including an inspected boundary entry, consumes one visit per pass. Map search/successor/position use Qt's tree links, not callbacks or whole-document scans. |
| private fragment map | Access one physical fragment per counted visit. Avoid public block iterator fragment/operator++ and fragment.length: those can traverse hidden same-format fragment runs. No prefix scan is necessary when a map seek reaches the selected fragment directly. |
| block/character format | Existing format collection and scalar object-index/type checks. Qt checks QVariant's stored integer type before conversion; these getters do not run custom conversion callbacks. Lists/object formats refuse without looking up or creating their objects. |
| existing block layout | Access the block data's existing layout pointer; null needs no materialization. preeditAreaText reads stored engine state as an implicitly shared QString. No block.layout, documentLayout, shaping, glyph, resource or virtual layout call occurs. |
| characterAt | Bounded map lookup and direct stored QChar access. Validate bounds first; at most4096 output units plus four neighboring units for surrogate boundary checks. No selectedText/toPlainText conversion. |

The inspection counter is shared across both complete passes. It counts each
root-inspection entry, QObject child, block and physical fragment inspected,
including any extra boundary entry. At8192 it refuses before inspecting another
entry. Tree metadata operations are Qt lookups, not additional full structure
passes; the bound is an entry/work-policy limit, not a hard real-time guarantee.
Deadline/cancellation are checked between visits and output units.

Both revision reads surround the two inspections and copy. Attachment then
rejoins the native root/document and checks revision/count again before publishing.
The existing live-borrow and writer-serialization premise remains necessary;
equal counters cannot prove lifetime, absence of native reentry or a global lease.
No new native owner acquisition is introduced.

## Controls and source correspondence

Owned controls install a layout with counting virtual methods, a resource provider,
a document event filter and a contentsChanged counter. The diagnostic reads leave
all counters zero and preserve document revision, cursor position/anchor and
Undo/Redo availability. Fresh-document controls check that neither child objects
nor a block layout are materialized. These are executed owned-fixture observations,
not a proof about arbitrary native runtime connections.

The helper uses only Qt headers supplied by its matching toolchain. Exact fetched
Qt6.10.3 primary-source hashes are retained with the local R8 receipts. Relevant
definitions:
- [qtextdocument.cpp](https://raw.githubusercontent.com/qt/qtbase/v6.10.3/src/gui/text/qtextdocument.cpp): characterAt, revision, characterCount; lazy rootFrame route.
- [qtextdocument_p.h](https://raw.githubusercontent.com/qt/qtbase/v6.10.3/src/gui/text/qtextdocument_p.h): exposed accessors, block/fragment maps and private data boundaries.
- [qtextdocument_p.cpp](https://raw.githubusercontent.com/qt/qtbase/v6.10.3/src/gui/text/qtextdocument_p.cpp): frame construction/scanning and object materialization.
- [qtextobject.cpp](https://raw.githubusercontent.com/qt/qtbase/v6.10.3/src/gui/text/qtextobject.cpp): document parenting, frame position getters, lazy block layout and hidden public fragment scans.
- [qfragmentmap_p.h](https://raw.githubusercontent.com/qt/qtbase/v6.10.3/src/gui/text/qfragmentmap_p.h): individual physical entries and tree lookup/successor operations.
- [qtextformat.cpp](https://raw.githubusercontent.com/qt/qtbase/v6.10.3/src/gui/text/qtextformat.cpp): integer property type checks and objectIndex.
- [qtextlayout.cpp](https://raw.githubusercontent.com/qt/qtbase/v6.10.3/src/gui/text/qtextlayout.cpp) and [qtextengine_p.h](https://raw.githubusercontent.com/qt/qtbase/v6.10.3/src/gui/text/qtextengine_p.h): stored preedit getters.
