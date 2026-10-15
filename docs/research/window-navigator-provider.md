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


## Subsequent static constructor join

The next bounded analysis closed the previously unresolved receiver assignment
for one owner-construction path. It constructs the concrete Qt navigator, stores
its shared object pointer, reloads that same pointer and passes it to the wrapper
constructor, which installs it as the receiver used by open. The navigator's own
incoming receiver is likewise traced to a constructed WindowLayoutStack. The
supported static chain is therefore wrapper to Qt navigator to layout stack,
not the earlier unproved alternative suggested by a similarly named strict
navigator type.

Private constructor-join receipt SHA256
`3351b96c3892d3c0c6053cda95e1d320b8a889fed4cdd4d8d26a7224e5ba2d33`
binds the exact instruction locations and twelve contributing artifact hashes.
This is author/Astra static analysis of one construction path. It does not prove
the runtime instance supplying the retained Reader, execution of the selected
route, the other wrapper-construction callsite, DocumentView creation or a
readiness condition. Earlier candidate-only findings remain the historical
starting point; the new assignment evidence narrows that specific uncertainty.

The layout stack's selected method delegates through another member. Its
constructor initialization is the next grounded source question; no component
factory or document-ready signal has yet been established. Static offsets are
private analysis locators, not SDK API or permission for native reads/invocation.
Direct native page opening remains outside this investigation.


## Loader, database and exact route decomposition

Further constructor arguments on the same static path bind the layout stack to
WindowLoader, and that loader to WindowDatabase. The loader first looks up a
route description. Its selected downstream path uses QQuickWindow::contentItem
and parent/context packaging before delegating to a provider held by that
description. The provider's concrete component-creation implementation remains
unresolved; these calls do not establish Reader creation or pixel readiness.

The wrapper's route parser compares the exact delimiter `/window/`, producing
the prefix and suffix used by the database. The observed route therefore maps
to module key `legacydevice` and window key `main`. Exact retained strings then
identify module `xofm.modules.legacydevice` and its QRC module path. This is a
source-backed module selector, not conversion of an arbitrary slash-separated
route into a guessed filename. The prior raw full-route miss remains accurate.

Private loader/parser receipt SHA256
`aea5dc0c78b1d29cf23f78328eebdef149d3209be4c62c841f35f58fdfe3f200`
binds this author/Astra static analysis. The observed module-name metadata lies
outside the previously retained registration descriptors 1 through 64; the subsequent targeted continuation found its registration at ordinal 66.
Only two additional registration descriptors and the selected ten-node metadata
tree were needed; this did not scan all QML bodies. See the creation-source
follow-up below. No native operation follows from the module name.


## Reader parent source located

The exact legacydevice module manifest exports LegacyMainWindow. Its bounded
source acquisition establishes contentItem MainView from device.view.main and
forwards navigator/document-related dependencies and the opened arguments. The
exact imported module export then identifies MainView, whose source supplies the
previously missing documentView declaration and DocumentView creation edge.
The route key main to LegacyMainWindow registration and the sampled capture
window to this component instance remain separate unresolved joins.

MainView declares a Loader with id documentView and objectName DocumentView,
initially invisible and with asynchronous false. A neighboring Component wraps
DocumentView. Navigator's Component.onCompleted handler assigns that component
to the Loader's sourceComponent. The open helper defers its entry and callback
when the Loader is not Ready; onLoaded replays the helper. Document-loaded state
subsequently controls visibility and remains separate from instance creation.
The unqualified documentView reference in retained Reader source now has a
concrete enclosing source declaration, without proving any live instance.

This changes the timing assessment: this Loader is configured for synchronous
DocumentView instantiation when that source assignment executes; successful
construction is still a condition, not a runtime observation. This source does not establish an asynchronous
Loader.Loading interval spanning a frame. Capture before creation remains a
conditional possibility only if the producer's admitted frame/request precedes
this creation path or trigger. If a witnessed frame were established to follow successful execution of that
exact source assignment and component construction, the creation explanation
would need reevaluation; a MainView completion name or frame alone does not
establish that ordering. Actual 7a4 lacks that component/window and ordering
join, so neither a premature-capture diagnosis nor a repair is established.

Loader loaded/Ready is now an application-source-backed creation discriminator
for this exact Loader, subject to separately qualifying its runtime identity,
ownership and observation mechanism. It does not establish document identity,
page loading, current pixels or freshness. No native property getter, callback,
probe or operation is selected here. A vendor comment cites a Qt issue, but the
public issue could not be retrieved; it is not evidence of this capture's cause.

MainView output SHA256
`2bc2866c59245e74af769e277e3aae801ad76bc7f816f24ccf1fc7ac4f5e3cfc`
and acquisition receipt
`cfd90ffeb24c9a2a55ed6a11d6548344fcada87adfc7201079f31f1c4b45eebd`
bind the 23823-byte private source. LegacyMainWindow's 2780-byte source is bound
by receipt `d88b73a233a1d659429730d39f1b72c66aa285e623410a4ef86234787f727307`.
The SDK author directly read the relevant creation/defer/open source blocks;
Astra traced the bounded module/export acquisitions. These are source findings,
not native execution or independent proof of the original loading-window role.

Final private source-chain receipt SHA256 `fe0cbfdb6dd678a3bccef8b88d0e715b0eec374036b735185970561e96fa0465` binds sixteen acquisition/source artifacts plus the retained Reader source hash. The document-view module manifest explicitly maps DocumentView to the previously retained node 92; this closes the source export chain, not runtime correspondence.
