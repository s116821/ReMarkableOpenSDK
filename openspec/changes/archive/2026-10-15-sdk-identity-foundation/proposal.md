# Change: Deliver SDK identity foundation

## Why
Main has no Rust crate; independently usable completed identity primitives remain on the research branch.

## What Changes
Extract canonical nonnil UUID, page key and operation ID types into an unpublished dependency-free crate. Preserve main baseline tools.

## Impact
No native operations, observation authority or release automation. GPL-3.0-only; source version remains tag-derived.
