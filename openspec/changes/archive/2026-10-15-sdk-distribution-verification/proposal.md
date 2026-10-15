# Change: Verify SDK distribution identities without publishing

## Why
Main now contains accepted source staging. Its existing source-crate verifier and selected downloaded-byte verifier need independently mergeable package/lock/artifact controls before release composition.

## What Changes
Reuse exact selected verify_assets.py and existing generated-lock, package-inventory and retained-asset controls from72136ed. Reuse main's repaired source fixture, with fixture imports adjusted only. Run actual Cargo packaging of owned synthetic source, bounded retained-byte checks and maintained docs-aware admission. Do not add a downloader, tagger, bump engine or publisher.

## Impact
No SDK runtime/Cargo/consumer/package-inventory or synthetic-platform owner changes. Actual SDK distributions, ARM/native support, remote tag authority and official releases remain separate qualification.
