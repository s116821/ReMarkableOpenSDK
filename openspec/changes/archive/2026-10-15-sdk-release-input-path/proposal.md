# Change: Align executable release inputs with maintained path filters

## Why
The license inventory is executable package-check input stored under docs. Current dorny can express its exception, but maintained PSR10.7 uses fnmatch and a separate broad docs ignore list. An actual upstream parser diagnostic ignores an inventory-only feature commit.

## What Changes
Move the unchanged inventory into tools, update its checker reference, restore simple docs exclusions and include the already selected named release README guidance exclusion. Adapt existing maintained-filter controls and verify the upstream parser now observes the executable tool input.

## Impact
No runtime/core/Cargo/lock/package member definitions/notices/consumer/workflow changes. No custom classifier, bump engine or release/tag action. SDK runtime author is separately informed through PR16 comment6196570915.
