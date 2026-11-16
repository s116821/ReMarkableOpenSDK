# In-progress verification

This change is active. The implemented primitives below do not yet expose a native request issuer, owned-range edit admission, Submit policy, verified native mutation or durable restart authority. No tablet/device/provider execution occurred.

## Native action ownership

Pinned xochitl `071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df` supports the private factory ABI `b69f84(out-owner-pair, item, start, length, kind)`. The nontrivial return uses a hidden result pointer. The ordinary whole-text removal caller at `4b78a8` passes kind zero, submits through Scene+0x260's ActionStack at `aac1f0`, then releases its local control at `9d620`. The factory allocates a 120-byte control/object region; object is control+0x10, control vptr is `134187c`, object vptr is `1341980`, with item/start/kind/length at object+4/+8/+0xc/+0x10. The direct `b68924` helper bypasses action history and is not selected.

`composer_native_action.h` implements only that private local ownership. It cannot admit a raw range. Construction, submission and release attempts remain distinct. Void submission return does not establish an edit; a throw after attempted submission is not a no-op or retry permission. Unestablished/partial owners are quarantined. Factory-local release is not retirement of the native history's independently retained reference.

Stack inspection shows history merge, copy and redo teardown occur after action apply. Pre-effect paths include the fixed 16-byte page-ID null scan `cc4240`, a global bypass flag, and the logging category's lazy initialization/debug branch. Removal initialization `b59e6c` succeeds solely from positive length/nonnegative position; its successful branch invokes no callback. Apply `b5d540` consults that logging category before the first CRDT call at `b5d5ac→caf2ac`. The forthcoming transaction must admit the supported existing quiet branch and compare after preparation; the stack call is not itself a writer CAS. History/notification/readback failures after attempted effect must remain Unknown.

## Bounded native anchors and plain preflight

`composer_anchor_walk.h` performs a complete bounded linked walk from root+0x80. It counts deleted nodes against its 1,024-node cap and caps live UTF16 at 4,096 units. It validates reciprocal traversal/closure, checked 48-bit counter intervals, exact author, live native LF, anchor order, duplicate/tombstoned/missing anchors and the entire copied Qt projection. It does not call unbounded stock ID-search helpers. Native LF maps only to Qt's actual paragraph separator for projection comparison. Counter-only zero is invalid even with a nonzero author; the independent r1 review found this edge and the r3 source/controls correct it.

These copied ID/position facts are not ownership. An issuer must mint anchors after its own verified provisioning and keep them joined to the existing Buddy binding and actual native incarnation. Root role ID `2:0` is not unique. A worker-owned `QPointer<QTextDocument>` can witness continuous lifetime of the embedded document across admitted callbacks, paired with the same selected visit/worker/root join; an old per-request Slot token cannot do this. Restart namespace remapping remains unsupported unless separately established; identical-looking text never reacquires missing IDs.

`composer_plain_edit.h` admits only an existing exact default Qt layout, same document/parent/thread, closed Qt edit state, actual and cached quiet undo/no-redo, unlimited maximum-block-count mode and unblocked signals. It uses the private inline existing-layout getter, never the lazy public documentLayout getter. It copies and inspects the entire bounded plain document, including material outside a potential edit interval. It grants no mutation authority by itself.

## Owned verification evidence

Evidence directory: `C:/Users/steve/Documents/Codex/2026-09-26/remarkable-manager-foundation/outputs/rem61-owned-composer-writer-20261116-r37`.

- `source-build-r3.json` pins the five current source/test files.
- `host-writer-r3.log` / `host-writer-r3-command.json`: owned Qt6.4.2 ASan/UBSan PASS.
- `arm-writer-r2.log` / `arm-writer-r2-command.json`: owned Qt6.10.3 ARM/QEMU PASS.
- Both include macro-free production-header compilation. Action failure/unknown/no-retry, quiet/layout/whole-document refusal, foreign-thread refusal, tombstones, exact LF IDs, projection, overflow, cancellation and traversal limits pass.
- Host scalar cache input is an explicit owned fixture provider, not a claim about Qt6.4 native layout. Production uses the previously pinned Qt6.10.3 scalar cache reader.
- `host-writer-r2.log` preserves a compile failure from fixture macro wiring and attempting C++ access to private Qt cache fields; production now reuses the accepted exact cache reader and the fixture uses its explicit provider. No failure assertion was removed.
- All fixture executable/object output was confined to the existing pinned images' disposable executable `/tmp` tmpfs. Logs, recipes and result hashes remain; no new persistent generated fixture tree was created and no denied cleanup was retried.

Assembly commands and slices in R37 are ordinary pinned-image objdump static evidence, not REA Evidence or runtime execution. Actual native instance/settlement, conditional compare/apply, provider integration and folio/OSK behavior remain unqualified.
