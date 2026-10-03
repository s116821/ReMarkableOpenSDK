# RM1 USB development baseline

## Why
REM-49 needs reproducible facts from SCRAPPY-DOO's reset RM1 without borrowing RM2 qualification. Manual observations establish firmware and platform identity but are not a reusable compatibility gate.

## What Changes
Add a development-only, Linux-host Python collector using an existing authenticated SSH alias. Collect only fixed model, architecture, firmware, runtime/library hashes, storage and framebuffer facts. Reject missing, malformed, changing or unexpected identities. An observation never enables a native operation. SDK Rust and native observers remain unchanged.

## Capabilities
### New Capabilities
- `rm1-baseline`: bounded read-only RM1 identity/storage observations with explicit unqualified native status.
### Modified Capabilities
None. This does not promote any RM2 adapter or shipping support.

## Impact
SDK tools and owning SDK specification only; no Buddy/Manager API change or consumer PR required. Requires Linux, Python 3.11+, OpenSSH and already-authorized USB key access. No target installation, credentials/configuration/document reads, native injection, service restart, firmware change or account pairing. REM-25 remains BYORBSAX-owned.
