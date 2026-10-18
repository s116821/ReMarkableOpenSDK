# Change: Consolidate SDK validation into compact ordered CI

## Why
SDK main still expands seven workflows into about nineteen PR jobs. Four workflows classify paths independently; candidate resolution and result wrappers add jobs, and SDK unit suites run again as artifact integration. The user explicitly requests the compact stage model already delivered for Buddy/Manager, with genuine SDK platform coverage retained.

## What Changes
Replace the seven workflows with one ordered core and a reusable metadata-only title workflow. Give each existing validation one stage/platform owner; retain real Windows staging as one extra Checks job. Build exact source/host/ARMv7/AArch64 candidates and a retained consumer smoke executable, then verify/execute those bytes without repeating the SDK unit suite. Preserve all metadata, semantic, docs-only, feature isolation, package, integrity and RM1 gates. Reconcile older open SDK workflow candidates with explicit source accounting and their owners.

## Impact
Application PRs have five applicable jobs: PR Title, Checks/Linux, Checks/Windows, Build and Integration. Current nonpublishing main has four applicable jobs; Windows qualification explains the extra job over the three core stages. A qualified official Release remains a separate later consumer of the same predecessor-bound artifacts. No dummy release job or unfinished publisher import; independently useful CI consolidation can ship now. No SDK runtime/API/Cargo, tablet, production tag/release or repository setting changes.
