# Protocol foundation verification

October 24, 2026. Host-only checkpoint; no endpoint or native executor is connected.
The implementation remains feature-gated and `UnqualifiedPlatform` is unchanged.

## Executed checks

- Rust 1.88.0 `cargo test --locked --offline --all-features`: pass, including
  13 resident protocol integration tests and a sequence-exhaustion unit test.
  The existing explicitly private historical capture fixture remains ignored.
- Current installed Rust 1.98.1 default-feature tests and all-target/all-feature
  Clippy with warnings denied: pass.
- Ordinary standalone consumer checks on 1.88.0 and 1.98.1: pass. The consumer
  cannot import mock, development capture or native adapter without their features.
- Default normal dependency tree contains only the SDK itself. The optional
  native protocol uses the already pinned SHA-256 dependency; lockfiles unchanged.
- Exact 16-member source package and all five package/notice/decoy checks: pass.
- Strict OpenSpec validation: all 22 current items pass. This does not verify
  unimplemented endpoint, native lifetime or product acceptance tasks.

## Fault coverage and review

Tests exercise every split/truncation boundary of an insertion frame, bytewise
and coalesced delivery, oversized/zero lengths, permanent decoder poisoning,
unknown schema, trailing bytes, nil IDs, duplicate/missing order IDs, each runtime
identity field, duplicate/stale/foreign replies, changed request digest and
operation, wrong target/order, a lost request before any write, explicit unknown
mutation replies, repeated cleanup, capacity exhaustion, fresh server nonces,
sequence overflow and 10,000 sequential modeled requests.

Independent source review identified insertion at the maximum representable page
count and two loss paths that could erase mutation uncertainty. Regressions first
reproduced the defects; fixes reject unrepresentable insertions before dispatch
and retain unresolved operation/digest through repeated closure and explicit
indeterminate replies. Final review reported no remaining blocking finding in
this protocol increment. Review did not execute the tests or inspect a tablet.

The reconnect regression establishes client rejection of an old response.
Server-side stale-request rejection, OS nonce generation, peer credentials,
actual socket deadlines and callback reclamation still require the endpoint
implementation and tests. Session correlation is not durable deduplication.
Insertion observation is not a committed receipt or permission to write.

Source basis: executed local command results, current source/test files and an
independent source review of this increment. No native runtime result is claimed.
