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
