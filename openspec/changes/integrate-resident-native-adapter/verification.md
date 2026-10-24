# Protocol foundation verification

October 24, 2026. Initial protocol checkpoint; the later owned endpoint checkpoint
below adds socket interoperability. No native executor is connected.
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

## Owned endpoint checkpoint

The Rust exporter creates 14 canonical frames and SHA-256 fingerprints in a
fresh output directory. The Qt codec checks exact bytes/digests, every split and
truncation, coalesced input and malformed frames: 14,781 assertions pass on host
Qt 6.4.2 and ARM Qt 6.10.3 under QEMU. The owned endpoint suite passes 1,760
assertions on both toolchains, including a separate process with pinned peer
credentials, 200 requests, reconnect nonces, stale sequences/requests, expiry,
late completion, backend exceptions, nested-loop replacement and inode cleanup.

The actual Rust `Session` also communicates with the Qt endpoint over Unix
sockets: 200 fragmented observations, one intentionally expired plain insertion
request, sticky mutation ambiguity, fresh server nonce and rejection of the old
request. Explicit EOF and the backend's dispatch count distinguish rejection
from an accidentally admitted request that merely times out. This test passes
with Rust 1.98.1 and host Qt. The fixture performs no native insertion.

Independent source review identified and accepted fixes for original-socket
identity across backend reentry, pinned-inode permission/cleanup identity and
deadline recheck after reply encoding. Reviewed endpoint SHA-256:
`1fbfa9ce38650c5282b4d28a6dfcf20f967568a64235c29067f3fde6c2785370`.
Final review found no blocking endpoint issue. Native physical completion,
reclamation, profile verification and consumer acceptance remain unimplemented.

The new checks are wired into Linux CI. Initial hosted runs found newly reported
vulnerabilities in existing release dependencies. The hash-locked Python recipe
updates certifi to 2026.10.19, filelock to 4.2.1 and GitPython to 3.2.1; a clean
install and audit report zero vulnerabilities. The uploader lock updates undici
6.29.0 to 6.29.1 with a zero-vulnerability audit. Its unmodified pinned upstream
source rebuilds to the same bundle SHA-256 on Windows Node 24.11.0 and Linux Node
24.21.0: `fc75d84d235209050f8bfb8d062004ed13625993a48be085688aebbe040b034c`.
Hosted uploader qualification and release-loopback fixtures must validate the
updated exact branch head; these rebuilds alone do not establish full CI success.

A local uploader-fixture invocation without its required provenance file was
refused before execution. A first Linux Cargo run encountered a registry CA
error; using the existing verified registry cache offline allowed the socket
test to pass. Local Windows Clippy passes; the Linux test image lacks Clippy,
so Linux lint remains the configured hosted CI gate. These are retained setup
results, not passing checks or changes to audit policy.

The complete hosted pipeline subsequently passed for transport commit
`9dbdd7ad6f8754ad2a3032e72b5a4be7c43f4251`, including Linux checks, Windows
staging, build, integration, uploader qualification and loopback fixtures:
[run 40126510550](https://github.com/s116821/ReMarkableOpenSDK/actions/runs/40126510550).
That run does not validate the later native-source increment below.

## Native observation source checkpoint

The exact handoff's conditional predicate and native/QML witnesses are imported
with original blob hashes. The full-order adaptation passes 308 owned assertions
on host Qt with ASan/UBSan and ARM Qt 6.10.3 under QEMU. Coverage includes 1–17
pages, malformed identity, duplicate/reordered IDs, inverse-map faults, guard
loss at every getter, nested reentry, wrapper deletion and foreign engines.
The production read-only backend compiles on both toolchains; the actual private
native profile compiles for ARM against the retained reviewed Dobby headers.
Neither compile check loads the adapter or calls the native profile.

The imported insertion condition passes 105 owned scenarios / 547 assertions on
host sanitizers and ARM/QEMU. The predicate fixture passes 203 assertions on the
host, with zero native calls. The initial new order-fixture compile failure was
fixed and retained separately; only the subsequent successful runs support this
checkpoint. The insertion predicate's production mechanism is unchanged.

Independent source review found no blocking issue in the new order observer or
read-only backend. The separate lifetime design review found that general GUI
last-owner release remains unqualified, and requires generation-bound Core APIs
and exact submitted-Task retirement before reuse. No reusable mutation is enabled.
The host/ARM fixtures are source/build evidence; actual native dispatch,
reclamation, consumer integration and the product acceptance matrix remain open.
