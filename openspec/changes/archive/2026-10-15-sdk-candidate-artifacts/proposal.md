# Change: Build and verify actual SDK candidate artifacts

## Why
Main now contains independently accepted staging, byte verification and maintained semantic tooling. Actual current SDK source must produce retained multi-target outputs and tested host executables before later release authority can consume them.

## What Changes
Extract exact selected candidate_artifacts.py and four transferred-output refusal controls from72136ed. Add a separate read-only candidate → frozen three-target build → downloaded-output integration workflow using maintained artifact Actions. No publisher, official tag or SDK runtime changes.

## Impact
Host/ARMv7/AArch64 compilation does not qualify RM1/RM2/PaperPro native operation. Source crate is the primary SDK distribution; rlibs require matching compiler/dependency graph. Candidate metadata remains explicitly unofficial and requires later verified remote tag authority.
