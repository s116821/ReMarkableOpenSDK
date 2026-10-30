# Identity-bracketed capture: owned helper checkpoint

October 23, 2026 UTC. A separate synchronous research helper now composes the
existing selected-page join with one bounded window grab and a distinct fresh
join after unwind. Host, ARM/QEMU and sanitizer controls pass. **No new tablet
capture, startup artifact, image-pair consumer or native qualification is claimed.**

The [actual acquisition result](conditional-target-acquisition-native-result.md)
established the selected-page relationship on one RM2 candidate. Earlier visual
captures returned loading images without it. This helper supplies a composition
to test that missing relationship; it does not diagnose the earlier pixels or
establish useful/blank document content.

## Implemented boundary

`reader_identity_capture::run` copies fixed expected identity and permission,
pins the GUI/window/root/engine and uses one original cooperative deadline of at
most20s. Its first actual observeCreationJoin supplies Reader/document/controller
and a borrowed Sample guard. The shared claim is consumed before captureObserved;
its observers require that exact window/root and never recursively start a Sample.

Capture permission includes the borrowed guard. The image context and guard are
checked inside SourceAction, and a scope guard destroys the returned context
before that callback returns on every path. Only owned scalars/IDs/hash escape.
After the first Sample fully unwinds, a separate fresh join requires the same
owners and expected ready identity. Retained owner lifetime/thread/engine are
checked again after both Sample returns. No old guard is revived, event epoch
filtered, deadline renewed or grab retried. A later refusal can leave forensic
PNG bytes; it cannot yield `captured=true`.

`attempted` records consumed capture admission, not proof that the physical grab
was reached. Failed attempts retain `unknown=true`. The returned before fact is
pre-action, while after is the distinct fresh join. Native/render/durable
authority remains false. Image coordinates are full-window; page transforms and
physical freshness are unestablished.

## Actual owned results

Pinned image: `416c7a7be0038156797b0892f031f352b841d1921fae83f712d0a272e4724618`.
Host Qt6.4.2, ARM Qt6.10.3/QEMU and host ASAN+UBSAN each passed97 scenarios /
784 checks plus separate production-header compilation. Leak checking was disabled;
these runs do not qualify native owner reclamation.

The real selected-page fixture performed two actual joins around known injected
pixels, with complete PNG decode, dimensions, marker pixels and SHA verification.
A separate real software/offscreen `grabWindow()` also passed the unchanged
Sample epoch and produced a decoded378-byte PNG in each execution. This is owned
Qt backend evidence, not a result from the tablet's vendor backend or panel.

Explicitly labeled modeled boundaries cover otherwise difficult callback/lifetime
timing. Controls exercise wrong/unready identities, missing/duplicate callbacks,
returned/action disagreement, shared claims, nested/pre-permission reentry,
sticky guard refusal/exception, deadline and owner changes during/after callbacks,
actual window/root/engine/Sink loss, epoch notifications, bad output FD, failed/
throwing/oversized grabs and failed fresh join with orphan bytes. Result scalars
survive destruction of all fixture owners without an escaped borrowed guard.

Independent source review found missing post-Sample owner checks. An exact
predecessor replay accepted an image after fresh-action owner destruction; the
corrected helper refused `fresh-join` with the same privately instrumented test.
This is a reproduced owned lifecycle bug and repair, not a native failure.

R1-R4 import failures are preserved: missing optional host QML modules were
supplied only by owned registrations in the fixture, with no installation or
production import change. R5's rare-lifetime setup hit the core's legitimate
multiple-engine refusal; its successor isolated that setup. R6 host/sanitizer
passed, while ARM reached the actual grab control but correctly exhausted a10ms
clock test before its asserted grab. R7 uses a labeled modeled sampler only for
that clock boundary. A reviewer also repaired an argument-evaluation-order flaw
in zero-grab assertions. No failed run is promoted to a passing run.

Final stderr is exactly the intentional libpng Write Error from the bad-FD
control; no sanitizer diagnostic appears. All six recursive input exports and
actual outputs are retained; existing selected-view/capture sources remain
unchanged. The fixture includes the existing owned model with its main renamed,
rather than replacing native behavior or duplicating that model.
Working raw hashes and compiled LF exports are distinct for the preexisting
capture core; normalization does not change the tested source semantics.

| Source/evidence | SHA-256 |
| --- | --- |
| Helper | `e040d59d7033c857e59dd7b8d67371347ab237bb810813aca84fef120270037a` |
| New fixture | `99132be1c64a330f1a993d1bccf1a954bea4805eab552fde1a800667f297ddad` |
| New runner | `62fc84cd84b0299206744532445b7c24944578ecdaae013dbd8278b8b519d14c` |
| Independent precode review | `c05ad9228b2944937b06a34f6fe32389fafc263d7e60a4952eec9a1def0bbb3a` |
| Independent helper source review | `d9d180839425d51200717aa485a96747cbf42b971db69354a57b8e43485e1fff` |
| Final owner-run output verification | `8aef7c885d88ac38759d06ed23e4607010d49de1bee43e806de93f953fc5d30a` |
| Actual predecessor/fixed unwind replay | `263c1b49da3c0dda40c7697c0631e9ebda33cfe5d981d9940f12d4537e000d17` |
| Independent final fixture/output review,104 checks | `11f3ab61f32fa20eb96c2290765bdd6890e4d5c8ba15dfa3445ee70a444bb8f3` |
| Independent complete artifact/retention supplement,58 checks | `49b9da4146e44f775f6335ca8df0eb68f515b2667f28d580c6291d88bc22accf` |
| Root saved-output/current-source audit,50 checks | `a41888652d5039d929b9aff40972fa847ff615de3b720f89aa5460b2eaa8e240` |

## Separate unfinished continuation

The [helper design](../../openspec/changes/establish-native-platform-contract/identity-bracketed-capture.md)
and proposed [Entry/image-pair design](../../openspec/changes/establish-native-platform-contract/identity-capture-entry.md)
keep native activation separate. The current Entry disarms before publication
and its generic publisher checks setup rather than capture-operation time; a
new mode needs its own limited publication admission after helper unwind. No
expired Sample closure may be used to publish an image.

The fixed entry/startup/provider profile, strict original-byte scalar/image
consumer and complete current-fixture recovery/preservation packet remain
unimplemented or unqualified. Useful/blank pixels, writing/reopen, resident
mutation/failure/reclamation and consumer delivery stay open. The current saved
fixture remains eleven pages/24 files; this work contacted no tablet and created
no page. The broader native-contract change remains unfinished.

Source basis: current original helper/observer/capture source, actual frozen
owned executions, retained failed attempts and predecessor replay, and independent
source review. Proposed native composition is not measured device behavior.

Later native result, October 23, 2026: [useful identity-bracketed capture and live collection](identity-capture-native-result.md) separately records one fixed idle-RM2 result and its recovery/preservation/cleanup audits. This owned checkpoint is not relabeled as native evidence; broader native/product capability remains open.
