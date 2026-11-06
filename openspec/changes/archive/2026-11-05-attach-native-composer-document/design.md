# Design

Prepare a non-forgeable-by-construction profile only after exact ARM/Qt build and loaded executable, QtCore, QtGui and libstdc++ file/mapping checks. This is a trusted-process diagnostic boundary, not a defense against same-process malicious code. Use the validated profile to prepare a single attachment state with a deadline fixed at preparation.

The exact typed callback is std::function<bool(Scene*)>. During its one admitted invocation, inspect Scene+0x26c participant and +0x190/+0x194 root/control-block; require participant+8 back-reference, matching native focus/root IDs, exact primary/secondary SceneTextItem vptrs, no native operation/IME scope or nesting, and no compose range/virtual extension. Attach only to the embedded QTextDocument at +0x1ec on its current Qt owning thread. There is no native resolver, dynamic cast, shared_ptr copy/release, worker dispatch or arbitrary user callout.

These dereferences require a real, live Scene borrowed by the exact native callback; a caller-provided raw pointer is not authenticated by this adapter. Full provider checks and correct native callback delivery remain prerequisites. The source can reject visible mismatches but cannot make an arbitrary pointer safe.

Copy only diagnostic IDs/scalars into the result. The callback always returns false, so it requests no damage flush. Connections retain only Qt recorder state. A copied callback retains attachment state, not the Scene/item/document owner. Cancellation marks state before disconnect. The native callback only try-locks the attachment mutex; caller-side stop waits for an admitted attachment and recorder publication to settle. A completed stop freezes the record prefix. Retirement snapshots distinguish a cancellation request from settled retirement. Logical retirement is not callback destruction or code-unload proof.

At most one native invocation and one recorder exist. Duplicate/late invocations refuse and retire collection. Callback-owned state can outlive the handle without accessing retired raw scene pointers. Polling removes idle expired connections. The Qt observer accepts the original absolute deadline, without refreshing the budget after dispatch. Native IME/Qt depth fields are one-time diagnostics, not a coherent draft lease or authority that remains current.

Owned layout/Qt fixtures must remain explicitly synthetic. Exact ARM compilation and emulated layout tests do not qualify a loaded Xochitl invocation. Root owns runtime scheduling and independent recovery.
