# Change: Verify the current SDK consumer and package boundary

## Why
The identity and gesture crate is now on main, but release composition requires a real ordinary consumer and package-containment checks. The current Rust workflow compiles documentation-only changes.

## What Changes
Reuse the selected consumer checker and containment controls, adapting only their API/package assumptions to current main 4cbd3c2. Add source/package/CI path admission using maintained dorny/paths-filter. Keep a required lightweight result and structural validation on documentation-only changes. Repair only two inherited canonical section wrappers if required by strict validation.

## Impact
No SDK runtime/API, dependency, source version, release/tag, native, device or Buddy pin changes. This is a nonpublishing REM-50 prerequisite, not the full release pipeline.
