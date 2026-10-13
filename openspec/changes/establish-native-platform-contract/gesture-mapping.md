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

## Committed consumer source handoff

The source integration is now committed in [Reader PR40](https://github.com/s116821/ReMarkableBuddies/pull/40)
at `67c73e9c4a0a4a6ba5d4fb05eea8ef3d383629a6`, consuming the exact SDK
`5c0bc303bb9b22e8e01bce9b2bd2a7fe47c8010e`. Its paired
[Docs contract](https://github.com/s116821/ReMarkableBuddiesDocs/blob/0c1138e2fa133e0db0d268c83783097afc931357/openspec/changes/durable-conversation-context/sdk-gesture-integration.md)
records the isolated consumer scope and retained native limits.

SDK independently compared the committed emitter, Touch, native-page parser,
navigation completion and backend against the previously reviewed working
sources: all five match after line-ending normalization. The
[earlier source review and six focused host checks](https://github.com/s116821/ReMarkableOpenSDK/pull/1#issuecomment-6150350493)
therefore remain relevant to those exact implementation bodies; those checks
were run on the working sources, not rerun on this commit. Hosted
[build/lint and tests](https://github.com/s116821/ReMarkableBuddies/actions/runs/38434469227)
passed for this committed Reader head. The separate diagnostic orientation
arguments do not adopt the protected pen-stroke/tool work.

This closes task3.6b's source handoff only. The observed document orientation is
configuration evidence, not proof of the active native page or view. Current-page
authority, native gesture/completion, process-crash release and lifecycle/recovery
qualification remain open, as do other device/orientation mappings. No device
operation, merge, canonical sync or archive follows from this checkpoint.

Source basis: current Git blob comparisons, GitHub API head/check outputs and the
linked source/host review receipt; Main's independent successor checks remain
attributed to Main rather than represented as SDK execution.
