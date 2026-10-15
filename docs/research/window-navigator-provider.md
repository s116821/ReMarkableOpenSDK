# WindowNavigator provider investigation

October 15, 2026. This source-only investigation follows the [Reader creation gap](reader-creation-readiness.md). The retained firmware is SHA256 `071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df`. Vendor source, metadata addresses and disassembly remain private.

## Grounded provider evidence

The retained Reader uses a typed WindowNavigator property. An exact type-name search in the verified ELF found 28 ASCII occurrences and no UTF-16 occurrence. A bounded Qt string-table/metadata join identifies `xofm::bindings::qt::WindowNavigatorWrapper`, with QML element name WindowNavigator, creatable false and the declared reason Created from cpp. Its declared interface includes open with a route string and optional configuration map, close variants, and windowOpened/windowClosed signals; the inspected metadata declares no properties. This establishes the static declaration, not successful runtime registration, instance ownership or creation timing.

The exact static meta-call dispatch for the route/configuration overload reaches the wrapper implementation, converts the string and delegates through a receiver. RTTI and a primary vtable identify a candidate C++ navigator implementation, whose open method has direct and callback-state paths and forwards route/configuration to another receiver. Constructor assignments joining these receivers were not established. Consequently this is not a proved Reader creation path, and the branch condition is not a supported readiness predicate. No private offset, method call or signal attachment is selected for native use.

A separate exact type occurrence led to one 216-byte retained plaintext component. It imports the Qt bindings module, requires WindowNavigator and WindowLayout, and declares an opened(configuration) signal. It contains no Reader, DocumentView, Loader, route or documentId creation wiring. The acquired UTF-8 region excludes a preceding non-UTF-8 byte; neither its registration nor its instantiation callsite is established. The signal declaration alone cannot establish Reader-ready semantics.

Qt's [C++ type registration documentation](https://doc.qt.io/qt-6.10/qqmlintegration-h.html#QML_UNCREATABLE) distinguishes exposing a type from permitting QML construction. These semantics support investigating C++ instance supply; they do not identify the application's actual supplying factory.

## Earlier selectors and precise limits

The literal document-bearing route from retained source had zero exact ASCII/UTF-16 matches in the raw ELF. It remains present in previously decoded source, so the raw-string miss is not route absence. A source-backed lookup of the imported device.view.navigator module identified one qmldir resource. Its single 320-byte compressed frame decoded to a 972-byte manifest with 24 exports, including the already-retained Navigator component. It contains no WindowNavigator export or plugin/dependency forwarding. This rules out that manifest as a direct export record; separate C++ registration remains possible. The other queried routing-module path was absent only within the selected descriptor.

The next useful join is the concrete construction/assignment supplying the wrapper's receiver and the Reader's externally resolved documentView reference, or an exact route-registration/factory record. Resource directories, type-name strings, virtual-method candidates and signal names cannot substitute for that relationship. There is no established current-document identity, loading cause or new capture-admission repair.

## Evidence basis

Private wrapper metadata result SHA256 `d5de5009a25c35d5f15dd146f3c3f3395dcd0e0953d7aef1dc5ee20030294112`; single module acquisition receipt `203fd7a9b2648e44a257f8dfab9b0b9fc77fa00ea8a2b8a107f0b2c019651063`; bounded plaintext acquisition receipt `24cb2378b5116f1664b25a33563664bf3068f115815822251247ef9232e1c681`. Source basis is hash-verified local bytes, maintained ARM disassembly tooling, the existing Zstandard decoder and official Qt documentation. Private records have no public transcript link. Static receiver candidates and implications for investigation are explicitly inference. No firmware/QML execution, tablet operation, getter invocation or new native test occurred.

The SDK author and Astra analysis worker derived the static dispatch path from saved disassembly. Coordinator review checked the public prose, three cited JSON receipts and hashes, and official Qt semantics; it did not independently rederive that disassembly path or execute native code.
