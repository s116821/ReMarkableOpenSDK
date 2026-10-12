# Ordinary build-time consumer check

`tests/fixtures/consumer-boundary` is a standalone Cargo workspace with exactly
one ordinary dependency: this SDK checkout, with default features disabled. It
has no dev dependencies. Unlike SDK unit tests, it cannot see SDK `cfg(test)`.
This tests the existing Rust API, not a native adapter or an installed Reader.

From the SDK root, with Python 3.11+ and an installed Rust toolchain:

```text
cargo +1.88.0 fetch --locked --manifest-path tests/fixtures/consumer-boundary/Cargo.toml
python tools/check_consumer_boundary.py --toolchain 1.88.0
python tools/check_consumer_boundary.py --toolchain stable
```

The checker itself is offline and locked. It verifies the local SDK resolution,
an empty SDK feature set, and fixture dependency pins matching the SDK lockfile.
It ordinarily builds/runs the `consumer` binary against public UUID, capability
and unsupported-observation APIs, then separately checks two intentionally invalid
binary targets. Each must fail with E0432 for its exact unavailable SDK module:
`mock` or `development_capture`. Unrelated errors and successful imports fail the
gate. Do not build every fixture binary together: two intentionally cannot build.

Both source manifests omit their own version. Cargo's generated `0.0.0` metadata
and lock entries are development resolution defaults, not SDK release authority.
Its relative path dependency is intentional local test input, not an official
consumer pin. No developer absolute path or alternate package source is committed.

## October 12 local evidence and integration boundary

On Windows x86_64 MSVC, the positive ordinary executable and both expected-error
checks passed with Rust/Cargo 1.88.0 and 1.98.1. The current stable formatter was
used because Rust 1.88's optional rustfmt component was not installed. This is
feature isolation and host compatibility evidence, not a supported firmware,
native operation, artifact ABI or release qualification claim.

The existing independent Sol reviewer inspected this source/document scope and
reran the full checker on Rust 1.88.0 with PASS and no blocking finding. The author
also passed focused Clippy, formatting and strict OpenSpec checks. Compiler commits
were `6b00bc3880198600130e1cf62b8f8a93494488cc` (1.88.0) and
`48a229ceaefd4985c50990b14116b6d856af0985` (1.98.1). Review coordination is internal
chat evidence, with no public transcript link.

REM-50 / SCRAPPY retains [SDK PR 3](https://github.com/s116821/ReMarkableOpenSDK/pull/3)
and CI/release ownership. At reviewed head `7fb824e8a218b7beabb138f3aa38df2c5bcdc930`,
its Cargo fixtures test tag/lock/patch identity using an owned minimal SDK. This
check covers the actual SDK module boundary instead. The owner can add the command
above to its existing checks; no new workflow or hosted outcome is claimed here.
Its unresolved release/advisory gates are neither changed nor waived.

Subsequent [REM-50 coordination](https://github.com/s116821/ReMarkableOpenSDK/pull/1#issuecomment-6127131882)
reports both hosted compiler jobs passing for the exact `e408a55` research snapshot
at PR3 `b1966b2d69c6721a582136aef09354a09f653d54`. That owner-reported result does
not cover later source changes. Before required incoming-PR or shipping coverage,
the owner must change the job to verify/run the actual candidate checkout;
historical pinned-snapshot qualification is not a substitute.

Main's existing Buddy SDK pins are unchanged. Newer SDK capability APIs tested
here must not be attributed to an older consumer revision. The native adapter,
full production build-time integration and qualified support matrix remain open.
See the [owning incremental plan](../../openspec/changes/establish-native-platform-contract/consumer-build-boundary.md).
