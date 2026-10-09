# Successful creation versus early shutdown failure

October 8, 2026. This source-only comparison asks whether the executed lifecycle paths in successful experiment 8287 and crashing experiment 0404 establish an SDK ownership or teardown regression. **No such defect was established.** These were different entrypoints at different completion states, not a controlled before/after test of one shutdown path. The rejected 3876 matching build remains blocked; no build, fixture or device action was performed.

## Provenance checked

The full private 8287 operator receipt identifies SDK `97726f3f9cd77eb2f12d1a583feb3e8a31b0b308`, with armed, callback-verified, restored and cleanup-verified true. Its source-build receipt names `qt_qml_access_probe.cpp` and `qt_qml_creation_bridge.cpp`, plus bridge MOC. All four production source/header hashes match that Git revision after accounting for LF/CRLF; the saved SO hashes to `84378ef83dfa133f903b4a97a6169b983d228367b544d75a7964bb0552234169`, matching the operator and build receipts. This verifies saved provenance, not a reproducible rebuild or general lifecycle qualification. Three documentation hashes in the historical build receipt do not match that revision; no claim of complete documentation-snapshot equality is made. The production source match is unaffected.

The 0404 operator receipt identifies SDK `71d9dbfa33ba828540fa0a9f22ec4db8aca862d6`, armed true, callback-verified/restored/cleanup-verified false. Its saved SDK ZIP comment identifies the same revision, and archived `qt_page_facts_startup.cpp`/`qt_page_facts_entry.h` match Git after newline normalization. Its selected configuration enables receiver-source-facts and disables input observation. Preserved host failure and crash evidence remain as recorded in [the original investigation](shutdown-0404-prior-art.md).

Private receipt SHA256 values for reproducibility:

| Evidence | SHA256 |
| --- | --- |
| 8287 build receipt | `03202cf0d08354aacbc776febec4f85ac729e45b2cd2dd67fe4fe0b7f733e631` |
| 8287 operator receipt | `6b8a2428485b120e27e579925867ecc116c097ad2d451c5cf78ea6e77974f645` |
| 0404 operator receipt | `7fd19a27bda41000b12918b9a902a8442afdb1df87c296ad1e763c24df6562f4` |

## Executed entrypoints and reachable lifecycle differences

| Boundary | 8287 creation Probe | 0404 FactsEntry |
| --- | --- | --- |
| Startup | One queued application-context callback constructs app-parented `Probe` | One queued application-context callback constructs parentless `FactsEntry`; completion requests `deleteLater()` through shared QPointer holder |
| Setup | Precise timer, application event filter, aboutToQuit cancellation; engine destruction queues refusal | Precise timer, directory watcher, application event filter, aboutToQuit cancellation; engine destruction calls cancellation |
| Before operation | Engine/root readiness leads to an owned QML component/helper and creation bridge | Window-to-engine lookup, context check, waiting-file publication; request dispatch checks for the purpose-specific token before owner discovery |
| Cleanup | Disarm bridge, stop timer/remove filter, delete owned helper/component, schedule Probe deletion | Mark closed/done, stop timer, remove waiting file, queue completion outside tracked call depth; completion disconnects focus observers, resets reader and requests entry deletion |
| Borrowed native objects | Engine/window observations are weak pointers | Engine/window/owner observations are weak pointers; inspected pre-token cleanup does not delete them |

Source anchors: [creation startup](https://github.com/s116821/ReMarkableOpenSDK/blob/97726f3f9cd77eb2f12d1a583feb3e8a31b0b308/tools/qt_qml_access_probe.cpp), [Probe constructor/destructor and cancel/finish](https://github.com/s116821/ReMarkableOpenSDK/blob/97726f3f9cd77eb2f12d1a583feb3e8a31b0b308/tools/qt_qml_access_probe_core.h#L692), [FactsEntry startup](https://github.com/s116821/ReMarkableOpenSDK/blob/71d9dbfa33ba828540fa0a9f22ec4db8aca862d6/tools/qt_page_facts_startup.cpp), [FactsEntry setup](https://github.com/s116821/ReMarkableOpenSDK/blob/71d9dbfa33ba828540fa0a9f22ec4db8aca862d6/tools/qt_page_facts_entry.h#L72), [request admission](https://github.com/s116821/ReMarkableOpenSDK/blob/71d9dbfa33ba828540fa0a9f22ec4db8aca862d6/tools/qt_page_facts_entry.h#L561), [completion](https://github.com/s116821/ReMarkableOpenSDK/blob/71d9dbfa33ba828540fa0a9f22ec4db8aca862d6/tools/qt_page_facts_entry.h#L955).

The inspected FactsEntry pre-token path does not call `grabWindow`, render/update, QML helper creation, native insertion or page getters. Its capture/focus flags start false, reader/retained-owner storage starts empty, and request admission precedes their activation. Engine-destruction cancellation is synchronous here, unlike Probe's queued refusal, but before request admission it only changes entry state, stops its timer, performs owned-file bookkeeping and queues completion. No wrong-order release of a native graphics resource follows from these calls. Directory-watcher/event scheduling can alter timing; that possibility is not a demonstrated defect or exoneration of preload involvement.

## New discriminating fact and limit

8287 diagnostics record `cleanup_enter_ms=cleanup_return_ms=12816`, terminal `resolved`, and one creation callback. In Probe source, `finish()` records those markers around `cancel()`, which deletes its owned helper/component before publishing the terminal receipt. Thus the successful run reached that cleanup-return boundary before the host's successful callback observation. This does **not** prove the later deferred Probe destructor ran, or that native asynchronous display work was drained.

0404 instead underwent early restoration without a verified callback; successful bootstrap/request admission is not established. Comparing its crash with completed 8287 therefore does not isolate parent ownership, watcher presence, synchronous cancellation or any individual new feature. The saved invalid rendering read cannot identify one of those differences as causal. Changes to the old creation helper are not evidence about 0404's different startup entrypoint merely because both appear in the repository diff.

No source-backed fix or new host fixture follows. Remaining independent SDK work includes completing the existing capability/profile API and deterministic model requirements (tasks 3.1/3.2), but those already have partial implementation and require a separately scoped completeness review; this finding does not commission duplicate framework work. Native adapter, active-owner and mutation/recovery qualification remain genuine open gates, not source-only tasks that this comparison can close. Existing diagnostic source review remains closed and was not repeated.

Source basis: current project handoff v33, three latest REM-25 comments, fully parsed private receipts/build manifests and saved diagnostics, exact Git sources and archived-source comparison. Raw native data remains private. Lifecycle interpretation and causal limits above are explicitly inference from those observations.
