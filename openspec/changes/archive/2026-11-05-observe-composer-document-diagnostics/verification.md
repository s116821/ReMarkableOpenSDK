# Verification

The implementation and focused fixture were checked once after source preparation.

| Layer | Result | Limits |
| --- | --- | --- |
| Host Qt 6.4.2 | Six grouped real-Qt scenarios passed with ASan/UBSan; 7.3 seconds including compilation | Owned fixture only, not Xochitl |
| ARM hard-float Qt 6.10.3 | Same scenarios passed under qemu-arm; 6.1 seconds including compilation; callback/QString value-layout assertions passed | Emulation, not tablet execution |
| PR checks | Focused sanitized Qt check added to the existing Linux job | Remote CI pending |
| Native callback ABI | Exact retained metatype name, copy operation, request registers and synchronous borrow inspected | No native request invoked |

The ARM sandbox printed fontconfig cache warnings because its root filesystem was read-only; the fixture completed successfully. The host reported its temporary XDG runtime directory. Neither warning changed the tested source or native profile.

No tablet contact, transfer, activation, native clear or runtime selection occurred.
No broad local Cargo/lint suite was run. Existing drafts and the installed Reader
bundle inputs were untouched. The original handoff left independent review and CI pending; acceptance is
recorded below.

See [diagnostic contract and limitations](../../../../docs/native/composer-document-diagnostics.md).

## Cancellation/publication correction

Independent review found that the first revision could append after stop returned.
Terminal transitions now share the publication mutex. Closure requests stop new
admission; a previously admitted append precedes the serialized terminal transition.
The returned terminal snapshot is a frozen prefix. Emitter callbacks still only
try-lock; caller-side retirement can wait for the short publication section.

A deterministic control pauses after the last admission gate, observes actual
close-lock contention, releases publication, and checks the terminal prefix after
later signals. No sleep determines that interleaving. Test-only hooks are absent
from production compilation, which the script now checks separately.

Focused retest: seven grouped scenarios passed on host Qt6.4.2 with ASan/UBSan
(8.7 seconds including compilation) and ARM-emulated Qt6.10.3 (8.2 seconds).
Original R4 receipts remain preserved; the correction has separate R4b receipts.

## Acceptance and archival

Independent review accepted correction 9b3f2ac8a58c577a0ae6fb1505f2a215c8bdd449
on 2026-11-05. Its hosted SDK CI run 42130153549 passed title, Linux, Windows
staging, Build and Integration; Release was skipped for this pull request.
[Corrected-head CI](https://github.com/s116821/ReMarkableOpenSDK/actions/runs/42130153549).

The canonical diagnostic specification was already synchronized and is unchanged
by this archive. Source and tests are unchanged by archival; final-head CI remains
a separate merge check. Root owns final merge. Native scene attachment remains a
separate change/PR and is not qualified by this acceptance.

Reviewed R4/R4b generated binaries/objects have been removed after preserving
source pins and result receipts. No device artifact or held input was changed.
