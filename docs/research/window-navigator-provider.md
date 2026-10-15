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


## Static main-key association

The subsequent bounded setup analysis finds explicit data flow pairing the key
main with the exact component path qml/LegacyMainWindow.qml in a description
builder. Finalization passes a non-null description to a captured receiver.
The capture's assignment from an incoming aggregate is also established.
The aggregate's concrete module/database identity remains unresolved, so this
is stronger than a filename association but is not yet proof of the complete
legacydevice/window/main registration or its runtime execution.

Private association receipt SHA256
`1788cb0bc8fdb13ec8a7e795a13cf13141ca05145fb61c69d803362a7e56b800`
and captured-receiver receipt
`b1f0b1c8e5031c74b811e16f255bda24653254ad2852a50994b625f73ac85ed8`
bind this author/Astra static derivation. The next exact question is the caller
supplying that aggregate; no new resource body or native invocation follows.


## Module wrapper and registration interface join

The indirect setup-table investigation connects the exact setup callback to a
constructed legacydevice module wrapper carrying the observed QRC module root.
The wrapper stores and later invokes that callback. Its call argument copies
the registration receiver from the field bound under the explicit name
windowAdder. This joins the module context and named registration interface to
the already-established main/LegacyMainWindow description-building path.
The concrete provider behind windowAdder and its database insertion remain
unresolved; no live module initialization or registration execution is claimed.

Private receipt SHA256
`2aa6153c8db15735d756bc1584719e390b44c05a9198962a027042183c8343f4`
binds the module/callback join. The subsequent argument/binding receipt
`acc1140b4bd0bca5406a8061448d57b6a895995ba04468bcf7f2d01cef4d8feb`
binds the callback invocation and windowAdder field derivation. These are
bounded author/Astra static findings. The next source question is that
specific interface's provider, not another guessed component filename or a
native invocation. The captured loading-window correspondence remains open.

The selected binder explicitly expects xofm::gui::WindowAdder and fills that field from dependency lookup. Provider-boundary receipt `303c7142040b25bec5165350a87b61bc480a00fc9c8c587d768c10381157abc1` distinguishes this expected interface from the still-unresolved concrete provider. An adjacent unrelated type-name string is not used as type evidence.


## Concrete WindowAdder factory construction

A subsequent exact-type investigation identifies a concrete Qt WindowAdder and
its constructor. The factory callback constructs that class, and the factory
holder identifies the expected WindowAdder interface. Explicit assignments trace
the factory's captured database to the same owner's previously identified
WindowDatabase, then into the constructed WindowAdder. The factory is retained
and copied into a produced aggregate.

Private receipt SHA256
`1bcef18ad5690f7ad46addb43095b8ea429b750e0fb196d79b6663145f9c1151`
binds this author/Astra static construction evidence. Publication and resolution
of that exact factory under windowAdder, and the factory callback's module/context
argument, remain unresolved. Thus construction and database capture are now
supported without claiming that the legacydevice wrapper actually receives this
provider or that the complete route was registered at runtime.


## Named factory publication path

The traced factory result reaches the qtgui wrapper's publication method, which
captures that exact factory in a callback and supplies the explicit dependency
key windowAdder and expected WindowAdder type. The selected helper compares the
key bytes and transfers callback state into the selected record on its successful
branch. This closes the static named-publication path; it is not an observation
that publication or dependency resolution succeeded in the running process.

The value callback preserves its incoming context argument and forwards it into
the factory, where it becomes the constructed WindowAdder's context field.
Whether this argument is the particular legacydevice wrapper remains the next
assignment join. Private receipt SHA256
`1b4708e5614a24cbca0c28c02b36dfa57765c8163f26ac1084ba4fddd351308b`
binds this author/Astra static evidence. No native invocation or captured-window
identity follows from the publication code path.


## Exact module context on the binding path

The inspected new-module registration branch constructs an injector with the
same module pointer whose binding method it subsequently invokes. That injector
is retained in the module record and passed to the binding call. Combined with
the already-traced binder and factory forwarding, this closes the context
assignment for the legacydevice wrapper on that static path: the constructed
WindowAdder receives that module wrapper as its context.

Private receipt SHA256
`c3e537a32d5d1e47734e4753d881f72777a9435fc8a84d43a5cbf78cd84c239d`
binds this author/Astra instruction-level argument. This specializes the normal
new-record branch; it does not prove that registration, binding or dependency
resolution succeeded in the captured process, or qualify alternative branches.
The selected database insertion method is a separate final static check; no
native readiness or capture correspondence is inferred from the context join.


## Static registration chain closed on the inspected path

The final bounded insertion analysis verifies that the description builder holds
the actual database pair supplied by WindowAdder. The selected database method
uses the description's module and window keys, copies the description into a
new entry, links that entry into the corresponding tree and increments its count
on the successful missing-key branch. The missing-module branch likewise links
its new outer entry. The existing-key branch reports an error; successful
registration is not unconditional.

Together with the preceding assignments, the supported static chain is:
legacydevice module wrapper to its scoped WindowAdder, to the legacydevice/main
description naming qml/LegacyMainWindow.qml, to insertion in the same
WindowDatabase used by WindowLoader. The explicit component export chain then
reaches MainView's documentView Loader and retained DocumentView source.
This closes the source registration/creation mapping for the inspected path.

Private final registration receipt SHA256
`27ac9695d496a12b40d99d34f8c724b6028ab1522e5cb44067fd759e4f34303b`
binds the exact insertion instructions and contributing artifacts. The source
path is author/Astra static evidence, not proof that module initialization,
dependency resolution, registration, loading or construction succeeded in the
captured process. No alternate path or firmware profile is qualified by it.

The remaining capture question is therefore runtime correspondence and ordering:
was the exact captured window/root this registered component instance, and had
its successful source assignment and Reader construction occurred before the
admitted frame and grab? Actual 7a4 supplies neither join. The exact Loader's
loaded/Ready state is a source-backed creation discriminator, while documentLoaded,
identity, current-page pixels and freshness remain separate. No native getter,
callback instrumentation, capture-gate change or new experiment is selected by
this completed source investigation.

Coordinator review read both final context/registration receipts, verified all 22 referenced artifact hashes and inspected selected injector, binding, builder, accessor, description-copy and tree-link instructions. It found no actionable inconsistency; this was bounded evidence/selected-instruction review, not an exhaustive re-derivation or vendor execution. Actual 7a4 remains a spent loading-only observation; held and spent experiment exclusions are unchanged.
