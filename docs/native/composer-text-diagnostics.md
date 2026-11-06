# Bounded text diagnostics

The optional range on composer_scene::Attachment::prepare and Delivery::prepare
copies committed document code units during the already-admitted Scene callback.
It does not dispatch another request, move a cursor or edit the document.

Build this diagnostic with COMPOSER_TEXT_PRIVATE_QT and the exact Qt6.10.3 private
headers supplied by the pinned ARM SDK. The focused test script demonstrates the
include paths. Builds without that opt-in remain compatible and return Unsupported
for requested text copies. This source is not selected by runtime bootstrap.

ReadRange uses signed start/length in UTF-16 code units, capped at4096 units.
Invalid basic requests refuse preparation; invalid document bounds refuse the
copy. Successful empty ranges have Copied status and count0. Inspect status
explicitly: an empty result is not itself success. Results carry their own fixed
raw uint16_t buffer, explicit count and diagnostic revision pair. Never treat
that buffer as NUL-terminated or convert it lossily.

NUL, NBSP, native paragraph separators, combining marks and raw isolated surrogate
units remain intact. Split valid surrogate-pair endpoints refuse. The copy may
split a grapheme; it is not an original-keystroke or rendered-text reconstruction.
Tables, child frames, lists, object formats/markers and affected preedit refuse.
Two structural passes share an8192-entry budget; exhaustion refuses without
partial text. See [getter audit](composer-text-getter-audit.md) for exact limits.

Attachment retains the same one-use claim, original deadline and stop settlement.
Delivery now shares its existing slot cancellation gate with Attachment so
cancellation during an entered read also discards unpublished text. The final
publication admission is under Attachment's mutex; a value admitted before
terminal closure remains a frozen historical result. Stop waits for settlement,
and no further text can publish after it returns. Retained late callbacks never
dereference their Scene argument. Existing callback copies still pin native owners.

The native borrow must remain live and serialize writers. Profile, native join,
thread/type, transaction and composition guards plus before/after revision checks
are necessary diagnostics; they are not an owned-range/page/visit or edit lease.
Consumers requiring genuine authority must continue refusing. Submit, native
conditional clear, focus acknowledgement and typed physical retirement are absent.

Owned host/ARM controls and macro-free compilation are recorded in the active
OpenSpec verification. They do not qualify actual Xochitl delivery or tablet use.
