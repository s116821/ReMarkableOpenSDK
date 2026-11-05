# Design

Base actual accepted main32b86e4fef04844cbf2ec31129bf029cc1d9899a. Reuse selected72136ed check_commit.py and three existing test methods. The maintained pinned PSR10.7 parser remains the only message/bump parser. Existing shipping semantic config controls allowed types and breaking behavior; parse_squash_commits remains false.

The old selected strict_scope=True and mandatory nonempty scope contradict accepted SDK18 optional scopes. Change only that rule: allow absent scopes, reject explicitly empty/whitespace scopes, keep docs/nonconventional/unsupported headers ineligible. Preserve existing borrow-from-older-history and known-guide/unknown-source path fixtures. Add exact SHA/checkout mismatch and unchanged source/ref assertions. Fixture tags exist only in disposable local bare repos.

Add the focused fixture suite to the existing read-only semantic qualification job. In the candidate workflow, application main pushes must observe a nonzero before SHA and one first-parent step, then run the exact current-message guard using the hash-installed/audited environment before candidate calculation. PR synthetic merge messages are not release messages and do not use this main-push guard. Maintained path admission still handles docs-only skips; all required global checks and later final release authority remain separate.

Do not mint a production tag, publish, authorize first SDK release, edit runtime/package manifests, import the cumulative old workflow or claim merged-PR/successful-squash authority from a local parser. One focused new admission suite is needed; do not repeat unchanged local SDK builds.
