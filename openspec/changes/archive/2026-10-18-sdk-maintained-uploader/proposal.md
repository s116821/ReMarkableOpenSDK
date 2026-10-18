# Change: Qualify the selected maintained SDK uploader

## Why
The current SDK pipeline verifies source and retained candidate artifacts, but the selected maintained create-only uploader recipe and its executable qualification remain only in retained research and Buddy. Deliver that existing capability before official-release composition.

## What Changes
Extract the exact selected source-build helper, small shared utility subset, two minimal build/runtime recipe files and four actual emitted-Action loopback controls from retained commit 72136edc94a95706fe1d1c7cb56a8bb6c5ffee4d. Build, freshly audit, verify provenance and execute those controls once in existing Linux Checks.

## Impact
No new job or platform matrix, release job, production credentials, tag, provider mutation, source version, Buddy pin or native operation. Preserve observed upstream missing-ID fallback and historical unselected dependency-audit failures as limitations; do not equate selected graph qualification with the full upstream test suite or production activation.
