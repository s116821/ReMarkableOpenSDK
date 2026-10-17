# Change: Qualify read-only SDK release observation

## Why
Actual current-message admission now runs on main, but a conventional direct push can still have an eligible message. Exact merged-main PR observation and retained-release metadata are separate identity boundaries needed before later official publication.

## What Changes
Extract exact selected72136ed read/download-only Octokit adapter and six original tests, add focused wrong-source/base/unmerged/API-failure refusals, qualify in the existing semantic job, and require only its merged-PR observation before application main candidate calculation through maintained github-script.

## Impact
No release write API, tagger, publisher, runtime or source version change. Release download/metadata helpers are fixture-qualified only; actual live activation is restricted to GET-only merged-PR observation. Squash-method/protection/predecessor/first-release/final-publication authority remains separate.
