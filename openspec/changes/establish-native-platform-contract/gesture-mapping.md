# Ordinary logical gesture mapping

## Proposal and design

Task 3.6 separates reusable SDK gesture geometry from Reader event emission and
native completion authority. The first mapping preserves Reader's existing RM2
portrait virtual coordinate path: Next starts (700,512), Previous (100,512),
and each moves to the opposite endpoint in 15 steps after a 50 ms hold, with
10 ms between moves. Coordinates use the existing 768 by 1024 virtual space;
Reader retains its tablet input conversion and always-attempted release.

The caller supplies an explicitly admitted tablet and orientation. RM2 portrait
is the only implemented pair. Unknown model, unknown orientation, RM1, Paper Pro
and other orientations return typed unsupported results before any event. No
default model or inferred orientation is permitted. The SDK returns immutable
geometry/timing only, with no observer, dispatch counter, native capability,
receipt or current-page claim. Existing NavigationRequest and Platform semantics
remain unchanged. Reader owns admission, one dispatch, release and uncertainty.

## Verification and delivery

Focused tests check exact legacy per-step truncation in both directions, endpoints,
timings and unsupported inputs. Main integrates the real Reader emitter using an
exact SDK revision and explicit caller admission. Independent review and consumer
verification remain required; no native owner/capture gate is closed by mapping.
Canonical sync and archive remain deferred with the unfinished owning delivery.

Source basis: current SDK contract and Reader xochitl_integration.rs/touch.rs at
ded1caddccd30d6452fc2613a017f0118b0c575b, plus Root's current-chat instruction.

## Source verification checkpoint

SDK source freeze: `5c0bc303bb9b22e8e01bce9b2bd2a7fe47c8010e`.
Rust 1.88.0 all-feature tests passed (38 unit/integration tests and five
compile-fail doctests; one private historical fixture ignored). The two focused
mapping tests also passed without default features. Formatting, strict Clippy
and strict OpenSpec validation passed. Both current CI checks passed:
[owned fixtures](https://github.com/s116821/ReMarkableOpenSDK/actions/runs/37877737739)
and [owned metadata fixtures](https://github.com/s116821/ReMarkableOpenSDK/actions/runs/37877737587).

The existing independent Sol reviewer accepted this exact source with no
actionable finding, compared the local historical Reader trajectory/timing and
independently checked all 40 model/orientation/direction combinations (two plans,
38 typed refusals). Its three focused checks passed. This review result was
reported through the current project coordination tools; the internal report has
no public link. Consumer integration/review and native qualification remain open.
