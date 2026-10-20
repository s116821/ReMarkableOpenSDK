# Verification

The owned temporary witness at `82a6f5bd0742757edb5a03b878418fa3bcde3956` establishes real event behavior. Invalid-title run39377147522 fails title validation. Title-only correction on the same source passes title validation in run39377298105 and reaches the intentional early Linux Checks failure (job118478239212). Body-only run39377412636 executes only title admission and omits all five flat core jobs. Native commit check-runs retain that same normal `Checks (Linux)` failure ID and conclusion. No unchanged SDK runtime suite was replayed locally.

Actual skipped names in the job API are the literal conditional expressions from ci.yml, including `needs.title.outputs.body_only == 'true' && 'PR metadata (no Linux checks)' || 'Checks (Linux)'`; they are distinct from normal contexts. Friendly evaluated metadata labels were not observed for the skipped SDK jobs. The temporary witness is removed in a new commit; its history remains intact. Ordinary final source CI and independent source/spec review are pending.

Focused graph/expression/jq controls cover exact body-only admission, title/base/mixed/unknown edits, predecessor failure, push title skip and normal context preservation. Git tags remain the sole component version authority; this change has no source version or runtime/release-tool modification.
