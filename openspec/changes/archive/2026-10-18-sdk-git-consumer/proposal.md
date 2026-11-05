# Change: Qualify immutable SDK Git consumption

## Why
Current checks cover ordinary path consumption and package staging, but do not exercise the shipping dependency verifier against actual Cargo Git-tag metadata or temporary local overrides. Cargo's generated default package version is not the Git tag's version.

## What Changes
Extract the five selected owned Cargo consumer controls and committed-default-lock control from the retained release research. Add one current-SDK Git transport case using the existing ordinary consumer and a temporary local patch. Run these host-only controls once in the existing Linux Checks stage. Keep exact tag, locked commit and clean source verification in the existing helpers.

## Impact
Fixture Git tags and remotes exist only under owned temporary directories. Source manifests, SDK API/runtime, Buddy pins, publication and native capabilities stay outside this scope. Fixture build.rs identity demonstrates the generated build environment only; the current SDK has no runtime-version hook.
