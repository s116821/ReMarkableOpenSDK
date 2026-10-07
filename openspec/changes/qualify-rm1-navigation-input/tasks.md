# Tasks

- [x] Read exact shared logical-navigation plan and current Reader prior art.
- [x] Observe RM1 input names/capabilities and framebuffer mode read-only.
- [x] Freeze proposal/design/scenarios before helper implementation.
- [x] Implement metadata-only probe; strict host compile, 15 sanitizer-backed metadata boundaries and quiet host refusal pass.
- [x] Compile ARMv7 and run 15 metadata fixtures under qemu-arm.
- [x] Obtain independent metadata-only source review at 8f51dd2; no blocking source finding, current-value prose correction requested.
- [ ] Accept documentation correction and the metadata-only lifecycle scope split.
- [x] Run authorized bounded read-only probe on RM1 with exact source/artifact verification; record ranges and verify owned cleanup.
- [x] Verify hosted metadata fixture/ARM compilation workflow at 8f51dd2: runs 37128133518 and 37128133569 pass; emulator evidence remains separate from hardware.
- [ ] Sync implemented metadata requirements to the canonical SDK specification.
- [ ] Final independent delta review and archive this metadata scope after required CI.

Gesture/profile/shared-native qualification remains unchecked in the active
`qualify-rm1-logical-navigation` change; no navigation completion is claimed.
