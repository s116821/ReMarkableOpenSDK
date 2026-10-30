# First stock navigator ownership census

October 22, 2026: the read-only census completed on RM2, reporting three typed
owners over fourteen visited visual items/boundaries in 173 ms. All three
layouts reported `bootsplash/window/main`; none had the document-view lexical
composition. This is positive metadata evidence of the early boot UI, without
navigation or selected-document/page observation.

| Input / observation | Value |
| --- | --- |
| SDK source | `77101e1f6229d692905d1cdfb659ea2197d2c33a` |
| Mode | `READER_SELECTED_NAVIGATION_OWNERSHIP` |
| Attempt | `d71732d3266543e1b357e47e78bee60d`, consumed |
| Payload SHA-256 | `a552be4588a4f6dbae482499afe5d2278a9d49b87ffa04fb121a797392527a6e` |
| Candidate PID / start | `9198` / `123679691` |
| Wire / census validation | Both passed, exact process/nonce/file/time/schema |
| Stage / refusal | `ownership-observed` / empty |
| Complete / visited / owner rows | true / 14 / 3 |
| Entry begin / end | `3795` / `3968` ms, 173 ms whole call |
| Selected identity / readiness / authority | Unobserved; all authority flags false |

## Observed owners

| Stock view type | Navigator / layout group | Layout ready | Layout active window | Lexical composition |
| --- | --- | --- | --- | --- |
| GesturesWindow | 1 / 1 | true | `bootsplash/window/main` | no pair |
| ShortcutsWindow | 2 / 2 | true | `bootsplash/window/main` | no pair |
| BootSplashProgressWindow | 3 / 3 | true | `bootsplash/window/main` | no pair |

Class/context text was not truncated. The exact guarded wrappers and layouts
matched their expected native ancestry. Gestures and keyboard windows had
content items parented to their owners; the splash window's content item was
null. All three owners were direct visual children of the window root.
Numeric groups describe distinct QObject identities within this census, not
backing router relationships. The full eligible typed-owner census contained
no linked `root`/`mainView` composition.

`WindowLayout.ready` was true even during boot splash. It therefore cannot be
used alone as normal document-UI readiness. The entry was queried about four
seconds after its own startup clock began. Sampling before normal document
views appear is a concrete timing issue for this attempt. It is a plausible
explanation for earlier early-startup refusals, but those attempts did not
record these ownership facts and cannot be retrospectively assigned this state.

The next independent attempt can use the same census source after one fixed
host-side settling delay, keeping the original ninety-second setup window,
prepublication margin and twenty-second operation cap. Waiting is not proof
of readiness; the returned ownership/layout facts must still establish the
actual UI. No candidate is reopened or resampled, and the spent attempt is not
reused. A normal post-boot census is still needed before choosing a receiver.

## Preservation and recovery

The actor recorded `primary_exit=0`; stock xochitl restored as PID `10045`,
start `123682337`, matching the original executable. Fresh checks found all
three services active/running, zero restart counts, no jobs, runtime shadows or
trial mappings. All eighteen fixture files and all twelve provider/service
inventory entries were unchanged. The saved before/archive/after fixture bytes
agree; its archive SHA-256 remains
`3f22dd7853c44fd54dc11ac9224d3e27639069860193b4345bfa9aaf7ec255cf`.

The 386560-byte runtime archive SHA-256 is
`fc814e8839f0c01b57b42cc2dbdb8701367634891034695b27980233164d3e97`.
Only the exact owned RAM directory was removed after archive, restoration,
process/unit, packet-hash and flat-content checks. No navigation, creation,
document lookup, account/firmware action or automatic retry occurred.

Source basis: exact committed source and four owned Qt suites on host/ARM,
independent byte-identical payload/packet reconstruction and 93 offline
operator checks, the private bound native receipt, actor/runtime archive and
fresh stock/fixture readback. Initial consumer validation defects and corrected
vectors remain preserved; these do not qualify a native action lease or fix
the separate historical shutdown problem.
