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

The complete hosted pipeline passed for native observation source commit
`3ead77bb1fe5824cd2317bc5af4d05b62771723f`:
[run 40128080628](https://github.com/s116821/ReMarkableOpenSDK/actions/runs/40128080628).
The resident/Qt test step executed successfully. This precedes the bootstrap
increment below and does not validate that later source.

## Bootstrap source/build checkpoint

Owned admission checks pass 261 assertions on host ASan/UBSan and ARM/QEMU,
including expired/oversized launch windows, wrong identity/hash/schema, unsafe
file/directory shape, symlinks, repeated consumption, existing socket/claim and
directory replacement between admission and endpoint startup. The endpoint suite
still passes 1,760 host assertions with the new optional directory pin.
Loaded-artifact checks pass 16 host sanitizer assertions using an owned no-op
shared library; they verify mapping/inode correspondence, mode refusal and
same-byte pathname replacement. This fixture does not load the native adapter.

The Rust admission encoder passes its canonical-layout and boundary test on
Rust 1.88.0 and 1.98.1. All-feature tests, default tests, all-target/all-feature
Clippy and five package/notice/decoy checks pass locally. Native source remains
outside the 16-member Rust package; lockfiles and default dependencies are unchanged.

The complete native adapter links against ARM Qt 6.10.3 and the exact retained
repaired Dobby archive. ELF inspection reports ARM32 little-endian hard-float,
NOW, NODELETE and STATIC_TLS. The build records source/header/generated-moc hashes,
profile/artifact hashes and compiler identity. No actual native pre-main entry,
Qt startup, profile installation or Supervisor recovery was executed.

Retained failed setup attempts: the first full native link lacked generated moc
files (fixed in the build script); a loaded-artifact fixture's QFile copy failed
on the Docker bind mount (replaced with exclusive creation from retained bytes);
one endpoint invocation used the wrong vector directory and failed before socket
tests. None is counted as a passing native/runtime check. Independent source
review found no blocking bootstrap defect and explicitly retained the admission-
window and hook-installation/refusal claim limits.

The complete hosted pipeline passed for bootstrap source commit
`e57f4ce2d2d1b7ea86417ff42bd5b55e02a8f85c`:
[run 40129429678](https://github.com/s116821/ReMarkableOpenSDK/actions/runs/40129429678).
It does not validate later changes.

## Non-last release primitive checkpoint

The owned primitive fixture passes 394 checks on host ASan/UBSan and ARM/QEMU.
It covers invalid/last/large counts, unchanged weak and vptr sentinels, bounded
spurious/contention failures and 256 immediate competing-destruction iterations.
ARM code inspection shows LDREX/STREX and barriers, eight attempts, no undefined
helper symbols and no call instructions. The first assembly-check invocation
failed because its grep missed conditional STREXEQ; runtime checks had passed.
The corrected check passed in a fresh retained output directory.

The exact-profile wrapper builds in the full ARM Qt6.10.3 adapter. Independent
review identified that a public raw-pointer release API could consume a reference
twice; it is now private to the not-yet-implemented typed resident slot. No native
owner is released and mutation remains disabled. Paired ownership, partial release,
physical retirement and actual native availability remain untested/open.

## Retained-owner slot checkpoint

The preceding non-last primitive commit
`5ecdb26a497aa3e1070f3b09ff99ba119a4e46ee` passed the complete hosted pipeline:
[run 40130707032](https://github.com/s116821/ReMarkableOpenSDK/actions/runs/40130707032).
That run precedes this slot increment.

The typed slot passes 10,195 owned checks on both host ASan/UBSan and ARM/QEMU,
including 1,000 sequential acquisition/reclamation cycles with stale handles,
partial-output exceptions, empty/mismatched owners, permit reentry, cancellation
during submission, unknown job results and per-owner retained-release retries.
The mock ordinary-release function aborts if called. Production-path headers
compile separately without the test seam. These cycles use a private fixture
retirement gate; they do not prove real Task quiescence or native capacity.

Independent source review found no blocking ownership/reentrancy defect and
required the partial-release, partial-exception and stale-token cases covered
above. Initial host/ARM fixture compilations failed under misleading-indentation
warnings in three mock functions; fresh runs passed after separating statements.
The output directories and failed attempts remain retained. Mutation is still
disabled; the production retirement gate and native validation remain open.

## Core request identity checkpoint

The preceding retained-slot commit `9e513bd60702cde8d89c946d9be852af56095327`
passed the complete hosted pipeline:
[run 40131281539](https://github.com/s116821/ReMarkableOpenSDK/actions/runs/40131281539).
That run precedes the Core changes below.

The adapted original ARM hook suite passes 101 ordinary scenarios / 9,393
assertions, including a new 1,000-cycle registration/reclamation case with 8,006
assertions, plus the original expected-exit fail-stop scenarios. Stale and foreign
tokens cannot cancel, observe or retire a replacement; attempted retirement from
an active outer frame is rejected. Original conditional-forward, private-rejection,
unwind, duplicate, interference and inflight behavior remains covered. The final
run also compiles the production Coordinator header without test seams.

A portable host ASan/UBSan fixture passes 1,800 assertions across 200 concurrent
normal/unwind cycles. It pauses an owned outer frame while cancel, snapshot and
retirement race from the other thread, then joins the owned caller before using
the private test retirement gate. This test installs no native hook.

The full native ARM adapter links after the Core change. Independent source
review found no blocking issue after a declaration-order fix ensured rejected
foreign-token carriers also destruct outside the registry lock. Only the inner
hook implementation is mutex-free; cancel/snapshot use the registry mutex.
These fixtures do not establish that a real native Task has no future invocation,
and production retirement, owner-slot connection and mutation remain disabled.

### GUI retirement and scope import

The two fence/scope headers and their fixtures are unchanged original Git blobs
from the pinned handoff; all six original hashes, including runners before SDK
adaptation, are recorded in `upstream-source.json`. Runners use caller-selected
fresh output directories and preserve artifacts. The owned retirement fixture
passes 748 checks on host ASan/UBSan and ARM Qt under QEMU. The request-scope
fixture passes 20 scenarios / 143 checks in both environments, including actual
Qt/JS bridge disarm. These establish adapter GUI lifetime behavior only; no
native Task, native owner release or production mutation executor is exercised.

Hosted CI also completed successfully for the preceding Core-token commit
`7be87e60c74d395ccf162e9cd24bd518ff44761e`
([run 40131829352](https://github.com/s116821/ReMarkableOpenSDK/actions/runs/40131829352)).
That run predates this import and is not validation of its later source.

### Coordinator owner-slot connection

The original orchestration fixtures are imported with Git-blob provenance and
run against the changed Coordinator. Host ASan/UBSan and ARM/QEMU cover legacy
negative controls, conditional insertion orchestration, adjacent-target checks,
framed cleanup and permit revocation. These use owned models, real Qt witnesses
and a real owned worker thread; they do not dispatch the native implementation.

Six additional ARM cases exercise the actual Coordinator-to-ResidentSlot path
with owned ABI callouts: normal conditional completion, zero submission ID,
cancellation/deletion during submission, and facade deletion during both native
owner acquisitions. They pass 95 checks. The fixture's QString-backed ID has the
pinned 12-byte / 4-aligned shape on ARM only, so host runs retain the original
boundary models. Separate production-header compilation excludes fixture seams.
The full native read-only artifact also links successfully. Physical retirement,
profile attribution reuse and native mutation activation remain unimplemented.

The preceding GUI-fence commit `f9614c105b6c8b2cfa241c8f75d108e11b1972fb`
passed its own [hosted run 40132632510](https://github.com/s116821/ReMarkableOpenSDK/actions/runs/40132632510).
That is not CI evidence for the later Coordinator connection.
