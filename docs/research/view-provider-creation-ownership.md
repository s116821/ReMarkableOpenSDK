# View provider creation and retained ownership

2026-10-15. Selected static analysis of the saved, hash-verified executable;
no vendor execution or native operation. This follows the
[Loader witness feasibility decision](loader-generation-witness-feasibility.md)
without repeating the completed registration mapping.

## Finding and decision

The selected provider does retain the outer view it creates. Its concrete type
is `xofm::bindings::qt::View`; the created item is held in a shared ownership
representation with its creation context and component. This resolves the
previously unjoined provider/result on the inspected successful path.

It does not supply an established application-facing source-to-instance
observation. The found cache/accessor is an internal C++ mechanism, not a
qualified QObject property, public notification or supported external reference.
The proposed external public-observer diagnostic remains unselected. An internal
reference exists; permission and a supported way to observe it do not follow.

## Selected source chain

The concrete WindowAdder implementation constructs this View provider and
returns it to the description builder. The builder transfers the provider
pointer into the description and clears the moved-from pointer. WindowLoader
retrieves that same provider for its virtual creation dispatch. This is provider
ownership transfer, not proof of QObject parenting or a live registration.

The provider dispatch first obtains either a cached item or a new one. On the
inspected cache-miss path it constructs QQmlContext and QQmlComponent objects,
checks component readiness, obtains an object through the component's virtual
creation stage, rejects a null result, sets C++ ownership and an object name,
applies initial properties, then completes the component creation stage and
checks for errors. The begin/complete semantic labels are supported by the
known component receiver, call sequence and expected Qt ABI; this remains
selected static evidence, not native execution or independent ABI qualification.
The object-name setter is a real public-facing name exposure in this path, but
its live uniqueness and source/capture correspondence are not established. It
must not be promoted into an exact-instance anchor merely because it is named.

Successful output is retained as the provider's item shared pair, context shared
pair and component pointer. The return holder's RTTI identifies
`std::shared_ptr<QQuickItem>`. The attachment path checks parent state and uses
QQuickItem::setParentItem with the packaged visual parent derived from the
supplied window contentItem.

The returned success holder is temporary: WindowLoader releases it while
returning success/error status. Its manager releases the held control block
and frees the holder; this does not imply unconditional immediate deletion of
the created item. The provider's separate cache remains on the successful path.
A cache hit therefore cannot itself establish a fresh creation event.

## Lifetime and correspondence limits

The provider destructor sets the cached item's `deleteNow` property to true,
calls a further lifecycle helper and cleans up retained state. This is concrete
evidence of application lifecycle behavior around the cached object, not a
permission to read or modify that property. Complete deleter effects, callback
reentrancy, all replacement paths and live lifetime stability are unqualified.
Neither copying a shared pointer nor retaining a QPointer alone proves the
required uninterrupted source/window/generation relationship.

On the mapped main route, the provider creates the outer LegacyMainWindow view.
Its retained QML definition contains MainView, which contains the DocumentView
Loader. Observing an outer cached item would still require a qualified join to
that source definition and the nested MainView/Loader generation. This analysis
does not retroactively identify the actual 7a4 window/root, prove construction
before its frame, or establish document readiness or pixels.

## Next decision

There is now a precise internal producer/result boundary for any separately
scoped cooperation or interception assessment: the known component creation
result and the provider's retained item/context/component association. No
public-facing exposure was established in the selected dispatch, return,
accessor and notification paths. The QString-only WindowNavigator notification
does not carry that association.

Do not implement a listener or getter based on these private offsets. Reopening
this candidate requires a concrete supported exposure of that association, or
an explicitly assessed different observation mechanism with purpose, effects,
identity/lifetime and independent recovery boundaries. It must also preserve
the distinction between an already-cached outer view and successful creation
of its nested Reader. This is a bounded decision for the examined path, not an
exhaustive claim that no interface can exist. Other SDK work remains open.

## Evidence and validation scope

Private receipt SHA256
`a752572879954823d4190db5f85a97d6e40b8467bf2fd60245e1e200b6394938`
binds 26 artifact hashes and selected instruction-level findings. Input integrity
was reverified against executable SHA256
`071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df`.
New bounded disassembly/data reads used the existing immutable ARM tool image,
network disabled and input mounted read-only. Private source bytes, addresses
and instructions are not published. Artifact filenames are analysis labels,
not independent type or method evidence.

Source basis: directly inspected saved/new bounded static evidence, retained
source mapping and author/Astra analysis. Private evidence has no public
transcript link. No implementation, owned runtime test, build, profile, nonce,
getter execution, callback attachment or device action occurred. Main remains
sole operator; actual 7a4 is spent, all held/spent exclusions and the existing
independent recovery requirements remain unchanged.

Coordinator review verified all 26 receipt artifact hashes and inspected the
selected type/vtable, factory/description transfer, cache hit/miss, component
creation stages, retained state, visual attachment, temporary holder release
and destructor instructions. It found no actionable inconsistency within the
stated static scope. This was bounded independent source review, not live
Reader correspondence, complete lifetime qualification or native authority.
