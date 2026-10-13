# Capture-order producer provenance amendment

Status: source implemented; focused verification and review in progress; no native qualification.

## Proposal and design

The correspondence producer must describe facts already established
by its implementation so a consumer does not infer a native render generation,
page-plane transform, common clock, or live admission from a saved PNG.
The current producer emits completion version 4, adding one exact
`evidence_semantics` object. Frozen version-3 receipts keep their original meaning
and shape; consumers must explicitly select version 4 rather than accepting
unknown fields or silently defaulting this descriptor. The existing discovery
scope and request purpose describe the same guarded acquisition mode.

- `version`: 1, the semantics descriptor version.
- `origin`: `DevelopmentUnqualifiedCaptureOrder` for every output.
- `acquisition`: `QQuickWindowGrab` or `InjectedImageCallback`, selected from the
  actual acquisition path. Neither value asserts a qualified firmware/device.
- `ownership`: `DeepCopiedPixelsEncodedOwnedPng`; the entry copies returned pixels
  before post-read getters and encodes that owned image into the bound PNG.
- `clock`: `QElapsedTimerEntryRelativeMilliseconds` or
  `InjectedEntryRelativeMilliseconds`, selected from the actual entry clock.
- `clock_scope`: `ThisEntryAttemptOnly`; no globally comparable clock identity.
- `render_binding`: `UnverifiedWindowToSource`; no native generation is supplied.
- `coordinate_space`: `FullWindowImagePixels`; width/height/DPR are not page-plane
  geometry, nor permission to invent a normalized affine or valid page region.
- `epoch_scope`: `LocalObservedInvalidationOnly`; equality is not a content revision
  or proof of complete input/change coverage.

Keep legacy v1/v2 and historical v3 schemas and the v11 Rust consumer unchanged. No new Rust constructor,
native origin variant, profile, session/visit ID, live guard or product surface.
All authority flags remain false. Native QElapsedTimer use must report monotonic
support before this mode starts; an injected clock remains explicitly fixture
evidence. This does not impose a hard execution bound on native calls.

## Tasks

- [x] Implement producer descriptor and monotonic-clock admission for version 4 only.
- [x] Check injected and actual QElapsedTimer cases, original PNG ownership and legacy controls.
- [ ] Review the exact source and available CI; preserve native qualification gaps.
- [ ] Coordinate consumer interpretation only when a real consumer requires it.

## Requirement delta

### Requirement: Explicit development producer semantics

The version-4 producer SHALL describe its actual acquisition and clock mechanisms,
owned-image custody, limited local epoch, and unverified render/page-plane scope
without supplying missing qualified native facts.

#### Scenario: owned injected fixture
- WHEN fixture callbacks supply the image and clock
- THEN completion explicitly identifies both injections and keeps all authority false.

#### Scenario: actual entry clock with fixture pixels
- WHEN QElapsedTimer times an owned fixture image acquisition
- THEN the clock identifies QElapsedTimer while acquisition still identifies the injected image.

#### Scenario: production clock unavailable
- WHEN QElapsedTimer cannot provide monotonic timing for the selected mode
- THEN startup refuses the configuration rather than describing a monotonic bounded attempt.

## Consumer coordination

Main confirmed the current selected Reader publication needs no additional SDK
DTO or constructor. Existing d01 mapping and synthetic-only HistoricalCapture
policy remain unchanged and reject this development schema. Any future qualified
producer requires separately reviewed native origin/profile, actual scope and
render/page-plane evidence, and a Main-owned sealed SourceAdmission bound to the
same private backend at each effect boundary. This descriptor grants none of it.
The internal coordination receipt has no public link.

## Focused implementation evidence

Existing vendor SDK 5.8.203 / ARM QEMU image `416c7a7be003`, C++17 with
warnings-as-errors: five selected cases passed (`order-good`, `order-real-clock`,
`order-owned-pixels`, `completion-diagnostic-success`, `source-facts-good`).
The strengthened owned-pixel assertion was rerun alone and passed: a later getter
changed the original external buffer to zero while the saved PNG retained white.

A sixth distinct case `order-window-grab` passed with the existing Qt software
scenegraph and offscreen backend. It uses actual QElapsedTimer and actual
`QQuickWindow::grabWindow()` on an owned window, no injected image callback,
checks the known `#123456` background in the saved PNG, exact provenance fields,
24 mapping reads and unchanged original timing bounds. Its printed `grabs=0`
counts fixture callback invocations only, not Qt's acquisition. This is an owned
ARM-emulated Qt acquisition result, not a tablet/xochitl render qualification.
No image/runtime package was installed. The cases join the existing entry script;
`CAPTURE_ORDER_CASES` permits focused execution without repeating the prior matrix.

The earlier 20-case result remains attributed only to `29bcfaa`; it was not rerun
or promoted to native evidence here. Strict OpenSpec validation passed. The
non-monotonic QElapsedTimer refusal is source-reviewed only: the available runtime
reports monotonic support, so no false unavailable-clock execution is claimed.
