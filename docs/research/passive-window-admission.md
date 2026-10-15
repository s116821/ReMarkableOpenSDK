# Passive window admission and pointer bases

Status: October 13, 2026 static findings following a refused observation. This
does not revise the [frozen passive method](../../openspec/changes/establish-native-platform-contract/passive-visual-sftp-method.md),
its manifest, provider set or authority. No device access was performed for this
investigation, and no actual runtime window class was identified.

## What the saved refusal establishes

The retained `selected-visual-r3-reporting` receipt records a completed memory
OPEN and six completed READs, phase `visual-sample`, with original cause
`Refusal:unsupported object type`. No memory CLOSE or successful visual result
was recorded; remote server cleanup remains unknown. These are saved operator
artifacts, not a fresh observation by this research lane.

In the exact selected source, four reads obtain the two global values, one reads
window-list membership, and the next reads the selected window's vptr. The
matcher raises that reason unless precisely one entry across the whole frozen
allowlist has that runtime vptr. Thus the receipt and source order localize the
failure to the first window admission, before its vtable header, private data,
content item or focus ancestry are read. This is a source-based reconstruction;
the receipt does not contain the observed vptr, class or match count.

The saved preparation yields 13 distinct candidate runtime addresses. However,
the observer obtains a fresh light snapshot and binds its maps before sampling;
the preparation alone must not be substituted for that invocation's actual
binding to turn the generic refusal into a recorded zero-match diagnosis.

## A concrete completeness gap

The frozen window entries admit only exact `QQuickWindow` and
`QQuickWindowQmlImpl` primary vtables. The hash-pinned Qt Quick image also contains
distinct primary vtables for `QQmlPrivate::QQmlElement<QQuickWindow>` and
`QQmlPrivate::QQmlElement<QQuickWindowQmlImpl>`. Relocated RTTI identifies each
wrapper's single public nonvirtual base, and each primary vtable header has
offset-to-top zero. These concrete wrappers are missing from the allowlist.

This is ordinary QML construction machinery: upstream Qt 6.10.3 declares
`QQmlElement<T>` as a subclass of `T`, and its `createInto` constructs that wrapper.
See [QQmlPrivate construction](https://github.com/qt/qtdeclarative/blob/v6.10.3/src/qml/qml/qqmlprivate.h).
An exact-vptr matcher cannot admit the wrapper merely because it inherits the
allowed base. This proves incomplete coverage of available types, not that
either wrapper caused the observed refusal.

The saved maps also name Qt Quick Templates 2. Its local vendor image contains
`QQuickApplicationWindow`, a corresponding QML wrapper, and separate copies of
the two window wrappers above. That provider is outside the frozen image-hash
set. Matching a class name across providers would lose the required image and
address binding; presence in saved maps is not fresh mapped-file hash evidence.
`QQuickView` is another available Qt Quick subclass outside the window list.
Neither these candidates nor a name-filtered scan of application RTTI establish
an exhaustive list of application-created windows or identify the focused one.

## QWindow is the relevant pointer base

Upstream [QGuiApplication](https://github.com/qt/qtbase/blob/v6.10.3/src/gui/kernel/qguiapplication.cpp)
stores `focus_window` as `QWindow*`; its focus-event path assigns the focused
window pointer. There is no general conversion from a secondary parser-status
or surface interface at that storage boundary.

Relocated RTTI in the inspected vendor images gives these public nonvirtual
base relationships:

| Concrete class | Base | Byte offset |
| --- | --- | ---: |
| QWindow | QObject | 0 |
| QWindow | QSurface | 8 |
| QQuickWindow | QWindow | 0 |
| QQuickWindowQmlImpl | QQuickWindow | 0 |
| QQuickWindowQmlImpl | QQmlParserStatus | 20 |
| QQuickView | QQuickWindow | 0 |
| QQuickApplicationWindow | QQuickWindowQmlImpl | 0 |

The identified QML wrappers use single-inheritance RTTI with their wrapped type
at offset zero. Consequently these inspected candidates have a zero-offset
QWindow base. Subtracting the QSurface or QQmlParserStatus offset from a stored
QWindow pointer would be incorrect for them. This is not permission to assume
the same layout for an arbitrary unknown subclass or to adjust an unrecognized
pointer until it matches.

## Evidence and remaining work

Qt Quick SHA-256:
`b20f9adaefe8cebeca445906e891370f75f0b0af878ac1b35e0c91159778628f`.
Qt Gui SHA-256:
`93fe582cc61673342ca49e12306d7f689860016582fa46ce135ffa280a972839`.
Local Qt Quick Templates 2 SHA-256:
`f9308c791a7dbbd1349c3bbc17ecaf878b64d3e3189470ed47e798c2b3c3b42c`.

Private source records `window-provider-rtti-20261013.json` (SHA-256
`c9a9017ae8cfeca2e3a8e786557b172088867e3d97df859863eca35ed955fe5a`)
and `window-base-offsets-20261013.json` (SHA-256
`05b39da16fbaa180bc849d5e32a7eafd81078b57f2f1a42087d0845ef23826c4`)
retain address and relocation evidence. They have no public download links;
proprietary bytes and raw disassembly are not included here. The name-based
wrapper scan is supplemented by exported base-class RTTI inspection; it is not
a complete binary type census.

Any proposed admission revision needs separate precode review of its exact
types, provider bindings and existing read/cap accounting. Recording the already
read vptr and match count could distinguish failure causes without an extra
target read, but even that reporting change is a proposal, not implemented or
selected by this research. No retry, new reader, heap scan, arbitrary subclass
admission or successful source witness follows from these findings.
