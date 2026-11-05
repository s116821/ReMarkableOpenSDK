# Change: Attach native scene document diagnostics

## Why
The Qt recorder requires a real live document. The retained scene callback supplies only a borrowed Scene; generic worker dispatch does not supply that argument.

## What Changes
- Add a build-pinned, one-use scene callback attachment to the exact focused root text item's embedded QTextDocument.
- Verify loaded provider identities before preparing the callback; validate native focus, root type, owner control block, transaction fields and Qt thread affinity during the borrow.
- Retire only observer-owned state/connections; never retain a Scene/item pointer or acquire/release a native owner.

## Impact
This remains a diagnostic adapter. It neither dispatches the native callback nor selects a runtime, page, owned range, draft lease or mutation. The caller must deliver it through the real typed native scene callback and keep its code resident.
