## Why

Applications currently mix product behavior with device-specific paths, geometry, input and native-runtime assumptions. Safe automatic page creation needs a reusable, independently qualified hardware boundary; the existence of a Qt method or working simulator cannot establish that boundary.

The October 1 architecture decision commits ReMarkableOpenSDK as an independent project with its own specifications. The September 29 decision excludes XOVI from production. The first delivery must expose only the capabilities required by native page creation and the current Reader path, with honest unsupported results while research continues.

## What Changes

- Establish SDK-owned OpenSpec and a narrow, versioned semantic contract for device capabilities, page identity, guarded capture/navigation and creation receipts.
- Keep persistent page identity distinct from current runtime/visit ownership and capture evidence.
- Require explicit unsupported, stale, canceled and indeterminate outcomes; never equate command acceptance with durable native creation.
- Investigate out-of-process native IPC/services first and qualify the accepted mechanism on RM2. Record rejected alternatives without shipping XOVI or silently falling back to metadata mutation.
- Keep one mockable contract across RM2/ARMv7 and future Paper Pro/AArch64 adapters; expose per-capability qualification rather than universal support flags.
- Integrate only necessary Buddy seams through a build-time dependency. Consumer conversation binding and its operation journal stay in Buddy.

## Capabilities

### New Capabilities

- `native-platform-contract`: capability discovery, identity/evidence, guarded native operations, qualified creation receipts, adapter and release behavior.

### Modified Capabilities

None. This empty repository has no implemented canonical baseline.

## Impact

SDK repository gains its own contract and eventual implementation/tests/release artifacts. ReMarkableBuddiesDocs owns the coordinated product delta; ReMarkableBuddies adapts its DeviceBackend boundary. Manager continues installing compatible Buddy artifacts, not a second SDK runtime. No cloud sync, firmware update, personal pairing, production injection, public Buddy admin API or all-at-once platform extraction is included.

Native page creation remains an unpassed research gate. A product-level manual blank-successor fallback is permitted only after the exhaustive investigation required by REM-25; SDK `Unsupported` alone does not satisfy that product gate.
