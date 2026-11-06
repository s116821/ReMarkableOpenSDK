# Verification

All checks below use owned fixtures, not a running native Xochitl callback.

| Check | Result | Boundary |
| --- | --- | --- |
| Host Qt6.4.2 ASan/UBSan attachment | Six lifecycle groups passed; 11.6s including compile | Synthetic join seam, no native layout |
| ARM Qt6.10.3 attachment under qemu | Same six groups plus exact-offset synthetic layout/refusal matrix passed; 10.7s | Real Qt document in owned byte storage, not native owner provenance |
| Observer with new absolute deadline API | Seven existing scenarios passed on sanitized host and ARM emulation; 10.4s / 9.6s | Required because the child changes recorder construction |
| Production headers | Compiled on both architectures without test macros | No profile activation |
| OpenSpec | Strict child validation passed | Review/archive pending |

The final controls pin the observer deadline to the exact preparation deadline;
exercise late/duplicate callback copies, exception, destruction, and cancellation
during native join and after recorder creation. Deterministic phases use promises
and observed mutex contention, not scheduling sleeps. The ARM matrix exercises
all join refusal fields, real Qt edit-depth refusal, and different Qt affinity.

The first ARM compile caught an incorrect draft size assertion: QTextDocument is
8 bytes on the retained SDK, while std::function<bool(Scene*)> is16. The guard was
corrected and the failure receipt retained. Final host/ARM checks cover the later
retirement-settlement source too. Fontconfig cache/XDG warnings reflect the
read-only fixture sandboxes; all final commands exit0.

Parent PR35 correction9b3f2ac is included by fast-forward; its independent review
accepted cancellation semantics. This child adds a separate scene join and
absolute-deadline overload; it still needs independent review and CI acceptance.
The canonical child spec is synchronized; archive follows acceptance in this PR.

No tablet operation, payload transfer, loaded-profile acceptance, real native
callback delivery, Submit, clear or runtime selection occurred.

## Acceptance and archive

Independent review accepted head 9851572ef38f1ac44ece2bd6c07d35bac961aa4e
on 2026-11-05 within the documented trusted-process/native-borrow preconditions.
[Exact-head CI](https://github.com/s116821/ReMarkableOpenSDK/actions/runs/42131610396)
passed title, Linux, Windows staging, Build and Integration. The canonical spec
was already synchronized. This archive does not change implementation or repeat
passed tests; final-head checks and merge remain with Root. Actual native
delivery and tablet qualification remain separate unfinished REM-61 work.
