# Rust 1.88 minimum-compiler verification

October 8, 2026. `Cargo.toml` declares Rust 1.88; previous verification used 1.98.1. This check tests that existing compatibility claim independently of unresolved native input/shutdown work. It changes no Rust API, dependencies or native diagnostic source.

## Scope and provenance

Repository checkpoint: `bfd42eee2847314d566be728862382afc8d07a41`. Tracked `src/`, `tests/`, `tools/`, `Cargo.toml` and `Cargo.lock` have no difference from reviewed capability-query revision `6f46ef4cee39827cba652b62ef7f8761902ebb6c`. Lockfile SHA256: `6a50c7a2e0b4d1fc1119b9819fa2556b627ff668ead9ee819f41738a95315ec7`.

Host compiler: `rustc 1.88.0 (6b00bc388 2025-06-23)`, full compiler commit `6b00bc3880198600130e1cf62b8f8a93494488cc`, host `x86_64-pc-windows-msvc`, LLVM 20.1.5. Installed with `rustup toolchain install 1.88.0 --profile minimal --no-self-update`; the default toolchain was not changed. Build output was isolated in ignored `target/msrv-1.88`.

## Commands and actual results

```text
cargo +1.88.0 test --locked --all-features --target-dir target/msrv-1.88
cargo +1.88.0 test --locked --no-default-features --target-dir target/msrv-1.88
```

| Configuration | Unit/integration tests | Compile-fail doctests | Ignored |
| --- | --- | --- | --- |
| All features | 36 passed | 5 passed | 1 private historical-capture test |
| No default features | 21 passed | 5 passed | 0 |

Both commands exited zero. Counts across rows overlap and must not be added as unique coverage. Unit tests intentionally compile the existing mock module under `cfg(test)` even without the optional mock feature; the no-default integration build separately exercises the feature-disabled public surface. No private fixture directory was supplied or consumed. Existing tests were reused; no new fixture was written to restate the implementation.

This establishes that this exact source/lockfile builds and passes these existing suites on the declared minimum compiler for this Windows host. It does not qualify another host, ARM target, firmware, adapter, runtime ABI, native navigation or artifact reproducibility. No new minimum-version Clippy claim is made. The broader target-artifact, independent review, license/publication and native gates stay open.

Source basis: current repository manifest/lockfile and direct local compiler/test outputs. This was an ordinary owned host Rust build; it did not compile/load a native payload, attach to a process, change device services, or retry any rejected diagnostic/control operation.
