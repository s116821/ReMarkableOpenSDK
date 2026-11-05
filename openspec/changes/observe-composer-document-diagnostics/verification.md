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
bundle inputs were untouched. Independent review and CI remain pending; the
OpenSpec change stays active until that review accepts this diagnostic component.

See [diagnostic contract and limitations](../../../docs/native/composer-document-diagnostics.md).

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
