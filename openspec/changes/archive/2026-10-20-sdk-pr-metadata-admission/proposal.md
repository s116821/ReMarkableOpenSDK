## Why

SDK PR body edits currently start the complete source pipeline. Removing edited
events would lose recovery after correcting an initially invalid title. Metadata
omission must preserve earlier failed application checks rather than replacing
their normal contexts with skipped results.

## What Changes

Reuse the reviewed Buddy exact body-only classifier and maintained title Action.
Expose body_only through the existing reusable SDK title gate. Omit SDK core
work only for exactly changes.body.from with a string previous value; base,
title, mixed, malformed and unknown changes keep full validation. Distinguish
all five omitted flat SDK core contexts without changing normal executed names,
existing stage counts, explicit main/skipped-title guards or publication custody.

SDK owns this change in its own OpenSpec. The central Buddy/Manager change stays
with its existing author. No runtime, Cargo, dependencies, source versions,
artifacts, firmware, account settings or native operations change.
