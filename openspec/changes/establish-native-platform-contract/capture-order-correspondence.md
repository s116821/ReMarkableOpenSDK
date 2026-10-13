# Capture and native order correspondence amendment

Status: source implemented, owned ARM-emulated fixtures verified, and independent source review accepted; native qualification and coordinated merge remain open.

## Proposal

Add default-off six-page `developmentCaptureOrder` selection to the existing
v11 receiver capture entry. Existing capture-only and source-only purposes stay
distinct. This produces unqualified historical correspondence, not a Rust native
adapter, page observation, render acknowledgement or effect permit.

## Design

Require v11 configuration and six-page cap/order, excluding source-only mode.
Use separate `capture-order-observation-request` and temporary token names with
purpose `capture-order-observation`. Reject legacy capture/source tokens in this
mode and the new token in existing capture/source modes.

Reuse the QML forward/reverse mapping reader and bounded parser. Read actual
native order on the retained capture owner at existing identity checkpoints:
before the single grab, after owning its pixels, at queued completion, and at
final output binding. Match the expected order and initial observed tuple/order.
Existing sticky epoch/lifetime/input guards apply. Incomplete mapping, disagreement,
invalidation or elapsed original deadline refuses completion. No separate facts
session, new observer platform, deadline reset, retry or extra grab is introduced.

Keep the original five-second capture/120-second setup ceilings and image/JSON
caps. Guard checks are admission/delivery checks, not hard execution bounds on
native calls. Scope deferral must hold teardown through reentrant getter stacks.

Publish completion version 3 with discovery scope
`receiver-capture-order-unqualified-v1`, actual `order`, `page_count`, and
`observed_order=true`. Native/render/UI/atomic authority remains false. Preserve
existing output files and capture lifetime; v11 consumers must reject version 3.
Do not combine it with a later facts-request. No native startup configuration,
transfer, activation or consumer import is provided by this amendment.

## Verification tasks

- [x] Implement opt-in purpose and shared reader/parser; preserve default schemas.
- [x] Verify six-page success with exactly one grab and actual mapping reads.
- [x] Verify reverse mismatch, post-grab drift, reentrant invalidation and deadline refusal.
- [x] Verify purpose separation and legacy capture/source-only controls.
- [x] Obtain independent source review and verify available exact-commit CI.
- [ ] Complete coordinated merge, then canonical specification sync in the parent delivery; do not archive unfinished native gates.
- [ ] Native lifecycle/qualification remains open; fixtures cannot satisfy it.

## Requirement delta

### Requirement: guarded capture-order correspondence

The selected development mode SHALL publish order only from complete native
forward/reverse reads within the original guarded capture attempt and SHALL
retain all existing unqualified authority flags.

#### Scenario: successful owned fixture
- WHEN all six native mappings agree at every capture checkpoint
- THEN version 3 completion contains their actual order and the single image digest.

#### Scenario: incomplete or changed evidence
- WHEN a mapping fails, changes, invalidates scope, or reaches the original deadline
- THEN no usable successful capture completion is published.

#### Scenario: purpose isolation
- WHEN a legacy or source-only request is presented to the selected mode, or the new request is presented to an old mode
- THEN the request is refused without acquiring an image.

## Source verification (October 12, 2026)

`tools/qt_capture_order_test.sh` compiled with C++17, warnings-as-errors and the
existing vendor SDK 5.8.203 in local image `416c7a7be003`, then passed all 20 cases
under its ARM QEMU with offscreen Qt. The success fixture performed 24 forward
mapping reads (six at each of four boundaries) and one image grab. Reverse-map
failure, pre/post/queued/final-binding drift or invalidation, cancellation during
mapping with nested event processing, stale/wrong-purpose tokens, and original
capture deadline expiry refused usable completion. Legacy capture, v11 capture,
and source-only success/refusal controls passed. The new cases also join the
existing entry test script for CI. OpenSpec strict validation passed (1 change).

Desktop Qt images `7d49afd13851` and `b4098f9c8b6f` compiled the initial candidate
but could not run because QtQuick.Window/QtQml.WorkerScript modules were absent.
No packages were installed; no desktop runtime pass is claimed. The final source
was tested using the existing vendor SDK and owned fixtures only. No tablet,
proprietary application lifecycle, hard native-call bound, or render generation
was verified. Full SDK/native/release gates remain open.

## Independent review and CI receipt

The existing Sol reviewer accepted exact base `2093f82e995d2bf51eff59bbdca1a82e74eeccb8`
to source `29bcfaaf25f21e5c8f30d295f8e943c305f30f19` with no concrete correctness or
contract regression. Review covered purpose/config isolation, getter-stack scope,
shared mapping/parser, original deadline and epoch, four capture boundaries,
held file/token/hash validation, completion withdrawal and default-mode controls.
The reviewer inspected source/assertions and did not independently rerun the
reported 20 ARM cases. The internal review receipt has no public link.

Both available GitHub checks passed on that exact source:
[owned-fixtures](https://github.com/s116821/ReMarkableOpenSDK/actions/runs/38339297722/job/115106456152)
and [owned-metadata-fixtures](https://github.com/s116821/ReMarkableOpenSDK/actions/runs/38339297739/job/115106456102).
These RM1 checks do not replace the targeted ARM/Qt execution or qualify native
lifecycle. Consumer integration, full native/profile/release gates and the
parent OpenSpec delivery remain open.
