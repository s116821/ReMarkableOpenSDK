# Composer document diagnostics

The opt-in C++ [observer](../../native/rm2_3_28_0_172/composer_document_observer.h) records an already-live `QTextDocument` through supported Qt signals. It has no loader, instruction hook, native enqueue, text mutation or Rust capability advertisement. The caller establishes the actual document and attaches on its owning thread.

## Use and lifetime

Call `remarkable::composer_diagnostic::Observer::attach(document)` while the document is live on its owning thread. Defaults are 256 records and a 120-second absolute deadline; both may be reduced. Invalid budget, capacity, thread or subclass returns no handle. The exact-type restriction avoids arbitrary subclass notification hooks. Serialize handle methods/destruction; direct signal callbacks may run concurrently.

The fixed snapshot holds actual Qt revisions, signal arguments, elapsed time, emitting thread and recorder-entry counters. Check `revisionPresent`: destruction and wrong-thread records never read the document. An address is correspondence, not an incarnation or page token. A busy snapshot is unavailable. Callback contention terminates collection instead of blocking the emitter.

`stop()` closes collection before disconnecting; handle destruction does the same. `poll()` handles idle expiry and disconnects terminal sessions. Every callback and snapshot checks the original deadline. An idle expired handle retains at most three dormant connections until polled, stopped or destroyed; there is no hidden timer or background thread.

Qt functors capture shared recorder state, never the observer handle. The observer does not own/delete the document or retain native strong/weak owners. Destruction callbacks use no document getters. An entered Qt functor can outlive logical disconnect: zero active callbacks or a canceled snapshot does not permit unloading the containing code. Runtime admission and independent recovery remain the consumer's responsibility.

## Measurement limits

This records actual revision notifications, emitting thread and overlap/reentry of its own recorder callbacks. It does not bracket the whole signal activation or native stack. A later listener can emit a nested signal after this recorder returns; both recorder depths are then one. The control fixture demonstrates that limitation.

Qt can coalesce or block signals. The fixture changes text with signals blocked to demonstrate that absence of records is not currentness. `contentsChange` also depends on layout state in the inspected Qt path. No complete edit count, closed native IME transaction, preedit absence, exact draft, owned range, Submit identity, active visit or conditional clear follows.

## Retained native callback ABI

Static target: RM2 firmware 3.28.0.172, ARM32 little-endian hard-float, xochitl SHA256 `071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df`. This is retained-artifact evidence, not current tablet identity.

- Metatype descriptor VA `0x1513188` explicitly names `std::function<bool(Scene*)>` through string VA `0x121a9d4`; size/alignment are 16/4. Copy constructor `0x6f1e3c` invokes function-manager copy operation 2.
- Request `0x6d6ce4` receives worker, page QString reference and callback reference in r0/r1/r2. The ARM return preserves both r0/r1 words of the job identifier. Const-reference spelling is an interoperability interpretation, not a recovered source declaration or executed ABI test.
- Core `0x6ee3dc` case `0x1b` copies the function, obtains Scene through `0x2028c0` and invokes synchronously with the local Scene pointer. The accessor checks the owning QThread and reads the scene from its scoped context. A true callback result requests damage flushing; it is not a mutation receipt.
- The dispatcher destroys its local callback copy and releases the scoped scene context after the case. A function value owns its captures, not the Scene argument supplied later. Never retain/dereference that borrowed Scene after return.
- The delivered bridge at `0x6d6b7c` uses `std::function<void()>`. Changing its address or casting its callback does not implement the scene route. This change does neither.
- Retained `SceneTextItem` constructor `0xb73f6c` embeds an exact `QTextDocument` at `+0x1ec`. The offset alone does not establish a current owner/focus or validate a remembered pointer.

Reproduction uses the exact retained file, metatype descriptor/name bytes, request/copy/accessor ARM ranges and exported Core evidence `ev_607722522c769a1424258116eb919622bde709ff101df224b91a8f7ed8e5f093`. Proprietary binaries and decompiled bodies are not distributed.

## Next native boundary

A selected runtime integration must supply the actual scene and focused text item's live document on its owner thread, with loaded-provider checks and lifetime through attachment. The native focused-root resolver can return a null control block and changes native reference counts; this observer does not invoke it or qualify a release path. Actual Qt affinity versus mutation thread must be measured; mismatch terminates this recorder instead of allowing cross-thread getters.

Native IME scope, page/visit, owned range and complete native callback nesting still need evidence. This component supplies no composer lease and does not close those integration gates.

## Validation and sources

`composer_document_observer_test.sh` uses real owned Qt documents. Controls cover Unicode/Undo/Redo, unchanged attachment, bounds, destruction, retirement during activation, wrong-thread notification, subclass refusal, blocked signals, nested notification and concurrent stop/destruction. Host or ARM-emulated Qt controls are not Xochitl/tablet evidence. CI runs the focused host check with address/undefined-behavior sanitizers.

The implementation is original. The lifetime interpretation is corroborated by Qt 6.10.3 [QObject activation and SlotObjectGuard](https://raw.githubusercontent.com/qt/qtbase/v6.10.3/src/corelib/kernel/qobject.cpp) and [document notification implementation](https://raw.githubusercontent.com/qt/qtbase/v6.10.3/src/gui/text/qtextdocument_p.cpp). No Qt implementation code or reconstructed proprietary source is incorporated.
