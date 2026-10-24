# Stock navigator ownership after boot

October 22, 2026: a second read-only ownership census reached the normal stock
UI. It completed over 454 visited visual items/boundaries and eight typed
owners in 3878 ms. The linked LegacyMainWindow/MainView composition shared one
native navigator and layout, whose active window was `legacydevice/window/main`.
This identifies a concrete candidate for normal document navigation; it does
not establish a selected document, page or insertion authority.

| Input / observation | Value |
| --- | --- |
| SDK source | `77101e1f6229d692905d1cdfb659ea2197d2c33a` |
| Mode | `READER_SELECTED_NAVIGATION_OWNERSHIP` |
| Attempt | `0174efa1c51747f0934f8a733bf460af`, consumed |
| Payload SHA-256 | `ddda720370062380afec182627adf7d8004a33a3b13f3f2e84317d54d4c4e2fe` |
| Candidate PID / start | `11422` / `123734701` |
| Wire / census validation | Both passed, bound process/nonce/schema/time |
| Stage / refusal | `ownership-observed` / empty |
| Complete / visited / owner rows | true / 454 / 8 |
| Entry begin / end | `33805` / `37683` ms, 3878 ms whole call |
| Selected identity / readiness / authority | Unobserved; all authority flags false |

The native source matched the [earlier boot census](navigation-ownership-census-result.md).
This independent attempt added one fixed thirty-second host delay after the
ready marker. The original ninety-second setup, twenty-second operation cap,
prepublication margin and independent restoration remained in force. Waiting
was an opportunity for startup to finish; the returned facts established which
UI actually existed.

## Observed composition

| Owner | Navigator / layout group | Layout ready / active window | Composition |
| --- | --- | --- | --- |
| GesturesWindow | 1 / 1 | true / `legacydevice/window/main` | no lexical pair |
| ShortcutsWindow | 2 / 2 | true / `legacydevice/window/main` | no lexical pair |
| LegacyMainWindow | 3 / 3 | true / `legacydevice/window/main` | linked root |
| MainView | 3 / 3 | true / `legacydevice/window/main` | linked mainView |
| Navigator | 3 / absent | no layout | lexical pair, no full composition |
| SearchAction | 4 / absent | no layout | inherited pair, no full composition |
| LibraryActions | 4 / absent | no layout | inherited pair, no full composition |
| CalendarActions | 4 / absent | no layout | inherited pair, no full composition |

The root and MainView had the exact creation-context URL
`qrc:/qt/qml/xofm/modules/legacydevice/qml/LegacyMainWindow.qml`.
Their scoped `root`/`mainView` IDs agreed, the main view was the root's content
item and direct visual child, and both properties referred to the same native
`WindowNavigatorWrapper` and `WindowLayout`. Both owner rows described that
same composition. Class/context strings were not truncated.

Groups describe QObject identity within this census. Distinct auxiliary and
action wrappers explain why requiring one navigator across the entire window
is too broad. This census does not establish equivalence of their backing
routers. A receiver selector should instead qualify the linked MainView,
complete its bounded search to reject duplicate compositions, and reread the
linked IDs, properties and layout immediately before any action.

The visible Navigator's packaged MainView binding requires an unloaded
document. Together with the census, this supports the inference that no Reader
document was loaded at the sampling instant. It is not a selected-page
observation. The inspected MainView startup code also supplies no guarantee
that waiting alone reopens the fixture. A subsequent test therefore needs a
normal existing-document open, followed by a fresh Reader/source observation.

## Preservation and recovery

The actor recorded `primary_exit=0`; stock xochitl restored as PID `12368`,
start `123740767`, matching the original executable. Fresh checks found all
three services active/running, zero restart counts, no jobs, runtime shadows or
trial mappings. All eighteen fixture files and all twelve provider/service
inventory entries were unchanged. Before/archive/after fixture bytes agree;
the fixture archive SHA-256 remains
`3f22dd7853c44fd54dc11ac9224d3e27639069860193b4345bfa9aaf7ec255cf`.

The 390144-byte runtime archive SHA-256 is
`dae8161037ece5d3d7550a77b06e3f08e138d2db6cc713adcc2a9bb1c6a7ec61`.
The exact owned RAM directory was removed only after archive, restoration,
process/unit, packet-hash and flat-content checks. No navigation, document
lookup, creation, account/firmware action or retry occurred.

Source basis: exact committed source and retained packaged-resource analysis;
the private process-bound census, actor archive, fixed-delay sequence receipt,
99 independent offline profile checks, fresh stock/fixture readback and
cleanup receipt. The unloaded-document conclusion is an inference from the
observed visible item and its source binding. No worker lease, insertion
completion, lifecycle repair or safe unload is established.
