# Change: Admit the actual main commit before SDK candidate calculation

## Why
A maintained history calculator can return a candidate due to an older untagged application commit even when the current actual main message is documentation or malformed. Accepted candidate artifacts do not by themselves establish current-commit eligibility.

## What Changes
Extract the selected72136ed current-message guard and refusal fixtures, reconciling its obsolete required-scope rule with accepted optional Conventional Commit scopes. Qualify current-message/source identity and gate application push candidate calculations with that read-only guard and the selected single-first-parent push observation.

## Impact
No version engine, publisher, tag, source-version field or runtime change. This is current-message admission; successful predecessor status, actual merged-PR/squash proof, protected remote identity and official first-release/publication authority remain separate.
