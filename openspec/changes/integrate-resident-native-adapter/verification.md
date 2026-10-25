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

### Attribution lifetime and quarantine

Updated owned slot tests pass 10,259 checks on host ASan/UBSan and ARM/QEMU.
They cover attribution-before-owner cleanup, one-time cleanup across partial
owner-release retries, stale tokens, and quarantine after failed, canceled or
reentrant attribution. Independent review found and then verified the fix for
an unknown-state admission gap: failed attribution now blocks submission, and
an uncertain submission blocks attribution without calling its factory.
These use controlled allocation/release callouts, not native quiescence.
The Coordinator suite also passes again on host ASan/UBSan and ARM/QEMU after
the quarantine fix, including all 95 actual slot-connection checks on ARM.

The full exact-profile ARM adapter links with the new private context cleanup
(artifact SHA256 `26643a3aef8f5bb4478833a30e6bd02e0a23a3326ac04b90868ec7f7da37a3e5`).
The profile's actual attribution allocation/destruction has compile/source
coverage only. The production physical-retirement caller remains open.
The previous Coordinator connection at
`3c8e7b698042dc59ed0b37eb0b41a4366d08a5bd` passed
[hosted run 40133254430](https://github.com/s116821/ReMarkableOpenSDK/actions/runs/40133254430);
that run predates the attribution changes.

### Resident executor lifecycle

The executor source now connects logical terminal state, framed Coordinator
cleanup, separate Fence notification, original Core retirement and slot
attribution/owner retirement. Independent review caught stale success publication
after rejected correlation or late Core evidence; the corrected implementation
preserves late evidence, clears success and requires reconciliation. Final source
review found no further blocker.

The focused lifecycle model passes 5,194 checks on host ASan/UBSan and ARM/QEMU.
It injects settled operation evidence and owner records while executing the real
Fence, executor publication and slot finalizer. It covers 300 successive requests,
stale tokens, nested event loops, completion-capture destruction before reuse,
terminal correlation failures, late interference/cancellation/job mismatch,
uncertain owners and bounded last-owner retention. It does not exercise the full
Coordinator-to-executor pipeline or prove native Task quiescence.

The existing Coordinator regression suite also passes on host ASan/UBSan and
ARM/QEMU after cancellation was connected to the original slot token, including
the 95 actual Coordinator/slot ABI checks on ARM.

The production ARM adapter links successfully with the executor and real MOC
objects, excluding all fixture seams. Artifact SHA256:
`9b93031f86b30d41c954eb9d10643e88e54238321b46ec0d243330cf81cadf68`.
Bootstrap remains read-only and the profile remains unqualified; no activation,
tablet action, endpoint mutation dispatch or native repeated-operation test ran.
Preserved failed builds include the initial private-QObject conversion compile
error and the first fixture warning failure; neither is credited as a pass.

The preceding attribution commit
`faadbd253a3d6debae74fcbdf9788e3cc5e4ca16` passed
[hosted run 40133916422](https://github.com/s116821/ReMarkableOpenSDK/actions/runs/40133916422).
That run is not evidence for this later executor implementation.

### Coordinator and executor together

The complete owned pipeline passes 195 checks on host ASan/UBSan and ARM/QEMU.
It runs actual Coordinator start, Qt worker and JS delivery, post-order reads,
Fence cleanup, attribution destruction and owner-slot consumption. Core outcomes
and native owner operations remain explicit models. ARM uses the actual typed
Coordinator acquisition branch; host substitutes a 12-byte owned ID because its
QString-backed fixture identity has a different ABI shape.

Normal and nested-delivery scenarios reuse the same executor for three requests,
including middle/end insertion. Child-producer and child-destruction loops must
unwind before physical retirement. Old JS callbacks are delivered after cleanup
and during a replacement request. Cancellation and late Core interference retain
the occupied slot. A known null-window preflight refusal has no owners,
registration, probe or mutation attempt; capacity recovers only after framed
cleanup and notification, and two subsequent requests complete normally.
Independent review accepted the fixture and the narrow no-effect reuse proof.

The production ARM artifact links again. Native activation, original Task
quiescence, repeated native capacity, endpoint mutation and consumer acceptance
remain unqualified. Earlier executor/model evidence is retained at its own
checkpoint rather than credited to this later change.

### Authenticated mutation dispatch

The real Unix endpoint plus mutation dispatcher passes 133 owned checks on host
ASan/UBSan and ARM/QEMU. The executor boundary is explicitly modeled here; the
separate pipeline suite covers its actual Coordinator/Qt/Slot integration.
Checks cover exact operation nonce and immutable lease deadline, original facts,
exact after-order conversion, capacity and before-dispatch refusal, uncertainty,
deadline expiry, socket disconnect and backend-facade deletion. Late completions
cannot publish after scope loss. Source mutation dispatch does not establish
native Task qualification or durable/target-selection authority.

Bootstrap links with the dispatcher but still uses `installBeforeMain(false)`;
its immutable profile lacks conditional admission, so the executor factory
refuses mutation support. The profile manifest still has mutation disabled and
unqualified status. Production ARM artifact SHA256:
`af55efd6783455c0f489896c09855bceeeb63c6f75b9689ffabaf09d46b6e314`.
No activation or tablet action ran.

Independent source review found no blocker in dispatch, conservative refusal,
immutable deadline propagation or bootstrap's disabled-mutation gate.

The earlier executor checkpoint `4768bb3e649685d7f105bb8ba6e26c5242c772af`
passed [hosted run 40135448615](https://github.com/s116821/ReMarkableOpenSDK/actions/runs/40135448615).
That run does not validate the later pipeline or dispatcher changes.

### Authenticated Rust client

The Linux client adds eight owned-socket integration tests and two focused unit
checks. They cover real kernel peer PID/UID and boot/start authentication, unsafe
permissions/symlinks, retained-directory rename, socket replacement during
journaling, known journal/expiry refusal before mutation bytes, 200 successive
observations plus insertion correspondence, EOF/wrong reply/oversized/partial
response ambiguity, slow fragments and an actual partial write blocked at its
absolute deadline. No native Reader is involved.

All-feature suites pass on Linux Rust 1.88.0 and 1.98.1 and Windows Rust 1.88.0;
Linux/Windows Rust 1.98.1 Clippy passes with warnings denied. Rust 1.88.0 also
checks the `native-adapter` library for `armv7-unknown-linux-gnueabihf`; this is a
cross-compile check, not ARM execution. Separate default tests, ordinary consumer
checks on both toolchains, five package/license checks and 22 OpenSpec items pass.
The Linux-only optional Nix 0.31.3 graph adds three registry packages; full original
license notices and the exact lock inventory are retained. Default consumers
still have no SDK feature/dependency leakage.

The rebuilt Qt endpoint passes 1760 owned checks. Both its original Session test
and the new authenticated Client test pass against that binary, including native
endpoint deadline retirement retaining the original client mutation identity.
Fixture bootstrap assertions are not artifact attestation. The Supervisor must
independently verify artifacts and implement activation/recovery; the consumer
journal must implement durable operation deduplication. Independent source review
found no blocker after clarifying those obligations. Native activation, target
acquisition, durable consumer integration and product acceptance remain open.

Earlier hosted runs for pipeline `a36f46f6b0b4aa7555d3150870fff1e1b6f1b299`
([40135912446](https://github.com/s116821/ReMarkableOpenSDK/actions/runs/40135912446))
and dispatcher `bc951c1033bd55c2ff2e7c4b8fce78c2e7ffd734`
([40136477280](https://github.com/s116821/ReMarkableOpenSDK/actions/runs/40136477280))
passed. Those runs do not validate this later client/lockfile change.


## Separate selection protocol and guarded inner call

Revision 2 introduces `SelectAdjacent`, `ReconcileSelection`, `TargetObserved`
and `SelectionReconciled`. The expected target must already be current or the
immediate successor. Session success requires the original document, exact target
and unchanged complete order. Selection loss is effectful and retains the original
operation/digest; read-only reconciliation cannot settle that effect. Both codecs
explicitly reject revisions 1 and 3. Buddy remains pinned to revision 1 SDK
`095f6c73777195b2bee56321a81857346a76ebde` until a reviewed journal upgrade
preserves old canonical bytes/digests without rewriting or discarding attempts.

Owned coverage includes 17 Rust protocol tests, 9 real Linux socket tests and
both Rust/Qt interoperability tests. Socket refusal, EOF, wrong/partial replies,
explicit unknown and slow-progress fixed-deadline checks exercise both insertion
and selection. The selection peer independently verifies journal publication
before receiving request bytes. The Qt fixture constructs owned selection replies;
it does not run a native Reader. The expanded corpus contains 18 vectors, with
19869 Qt checks including revision 1 rejection. Host and ARM-emulated endpoint
suites each pass 1760 checks. Windows Rust 1.98.1 all-feature tests and strict
all-target/all-feature Clippy pass. Linux Rust 1.98.1 all-feature/transport and
explicit Qt interoperability pass. Explicit Rust 1.88.0 verification is recorded
below after completion; the first container named selection-msrv-r1 used its
1.98.1 default and must not be credited as MSRV evidence.

`reader_adjacent_selection.h` integrates the measured method contract without
fabricating an InsertAfter request for an existing page. Its synchronous inner
step accepts only exact supplied fresh order under the 17-page observation bound,
returns already-selected with zero calls, or consumes one claim before a single
qualified public `moveForward()` invocation. It copies borrowed inputs before
permission callbacks and never inspects the old Sample guard or objects after
invocation starts. It does not perform the required fresh post-transition join.

Host Qt 6.4.2 and ARM Qt 6.10.3/QEMU owned fixtures each pass 67 checks, including
current/adjacent/refusal, consumed claims, reentry, foreign engine, deletion during
permission/invocation, invocation exception, QVariant return, 17/18-page boundary
and destruction of the owner of all borrowed inputs during permission. The runner
refuses an existing output path so evidence cannot be overwritten by rerunning a
selection. Source review accepted the helper subject to that runner fix; the
completed regression additions and final runner fix were independently accepted.

No production selection callsite, shared native effect gate, native selection executor or
consumer selection journal is enabled by this increment. Fresh post-transition
joins, native settlement/qualification, Supervisor recovery and product acceptance
remain open. The bootstrap still disables effects. No tablet contact occurred.

Source basis: current source diffs, owned local test outputs and independent source
review. These are source/build/fixture results, not native qualification.

The explicit `cargo +1.88.0` Linux all-feature suite, both Qt interoperability
tests and `armv7-unknown-linux-gnueabihf` native-adapter library check subsequently
passed (`selection-msrv-r2`). This is the MSRV evidence; ARM execution remains the
separate owned Qt/QEMU fixtures above.


The read-only backend now routes ReconcileSelection through its existing guarded
actual Reader/order observation and a separately tested reply mapper. It retains
the original request shape and document, computes presence from the actual current
order, and keeps insertion/selection reply kinds distinct. It never settles an
effect or dispatches navigation. Host and ARM full-order/mapper fixtures each pass
316 checks; independent source review accepted this read-only integration.
Separate default tests, the exact 17-member package check and ordinary consumer
boundaries on Rust 1.88.0 and 1.98.1 also pass.


## Shared resident effect reservation

`ResidentEffectGate` serializes effect admission across process-resident executor
instances. Mutation reserves its private token before Coordinator start; only
its existing known pre-acquisition no-effect or physically reclaimed path releases
after publication, completion-capture destruction and active-request cleanup.
Unknown/last-owner/failed retirement keeps the reservation. Dropping all caller
tokens or an executor facade does not reopen capacity. Read-only work stays outside
this gate. Future selection integration remains required.

Independent review accepted the final code after removal of an unused fixture
friendship seam. Final owned host/ARM lifecycle tests pass **5537 checks**, including
300 reuse cycles and two executors sharing one gate, stale/foreign/wrong-thread
release, dropped tokens, uncertainty after facade destruction, and shared exclusion
during completion-capture destruction and nested pumping. The full Coordinator
pipeline passes **195 checks** on both hosts. ARM dispatcher passes **133 checks**.
These are explicit owned models, not native Task/owner qualification.

The final ARM library links with SHA256
`c16c622de8fde432108b284e09bb20af5977600880e82cff2493bab1bb3314d1`, unchanged profile
`4800f33c19510a2e2db58ea3cef3d647a381daf1e52a0c785c21874171960dc9` and pinned Dobby
inputs. No loading, device contact or native activation occurred. Initial gate
runs (5533 lifecycle checks before the extra capture-deleter assertion) remain
separate retained evidence. Hosted CI for the preceding selection increment
`afb3b9c3c16fd12f09529e33bed2fe2da6d34022` passed
[40141519244](https://github.com/s116821/ReMarkableOpenSDK/actions/runs/40141519244);
it does not validate this later gate change.

## Resident selection executor source checkpoint

`ResidentSelection` now owns the bounded zero/one-transition request lifecycle,
same-Reader/document fresh postjoin, original deadline and shared effect token.
Final publication and capture destruction occur after sample unwind. Attempted
uncertainty retains the process reservation. The production factory remains
hard-disabled; the authenticated dispatcher returns Unsupported for selection.

Independent source review accepted the implementation after two concrete fixes:
reentrant failure cannot be overwritten by a suspended callback, and the final
owned join result can revoke a callback's pending success after sample/helper
unwind. Tests exercise truly nested getter/invocation duplicates and late join
refusal, preserving attempted-effect classification.

Final owned host Qt 6.4.2 and ARM Qt 6.10.3/QEMU selection suites each pass **2933
checks**. Both authenticated dispatcher suites pass **144 checks**, including the
disabled selection route and exact response correlation. Tests cover zero/one
invocation, fresh postjoin, readiness limits, source/order/object changes,
deletion, cancellation, deadline, callback/capture lifetime, cross-executor
exclusion and uncertainty after executor destruction. These are owned models;
positive authenticated selection dispatch and actual native settlement remain
unqualified.

The ARM library builds with SHA256
`73040102155b125d26da9c7ebc5a3da58c71a3f4e956162654faafe5207129f4`,
unchanged profile and pinned Dobby inputs. Final directories are
`selection-executor-host-r4`, `selection-executor-arm-r1`,
`selection-backend-host-r1`, `selection-backend-arm-r1` and
`selection-executor-library-r1`. Initial compile failure and earlier pre-review
results remain preserved, not credited as final evidence. OpenSpec strict
validation passes 22 items. No device contact, loading or activation occurred.

Source basis: current source, local compiler/test outputs and independent source
review. Consumer journal compatibility/selection composition and native acceptance
remain required before this change can complete.

## Historical wire inspection

The separate `HistoricalFrame` API retains canonical full-prefix v1/v2 bytes,
original SHA256 and immutable decoded fields. It does not add legacy live
transport admission or a replay API. The private common codec enforces each
revision's grammar; selection tags under v1 refuse. Original request and response
vectors are retained from the pre-v2 corpus, independently hash-checked by review.

Four historical regression tests and 17 current protocol tests pass. Windows
Rust 1.98.1 all-feature/default suites and strict Clippy pass; explicit Linux
Rust 1.88.0 all-feature tests (including nine owned transport cases) and ARM
native-adapter library check pass. Ordinary consumer checks pass on 1.88.0 and
1.98.1; the exact 17-member package and five package regressions pass.
Strict OpenSpec validation passes 22 items. Independent source review accepts
the SDK API/codec. Original-byte response correlation and reservation survival
still require consumer integration before changing its SDK pin.
Both explicit Rust 1.88.0/Qt socket interoperability tests also pass against the
fresh owned endpoint (1760 checks) and retained v2 corpus.

On October 25, exact SDK source `9880766f5bb0bdb91445716132703b02587d372e`
passed all required hosted checks, build and integration in
[CI 40143522893](https://github.com/s116821/ReMarkableOpenSDK/actions/runs/40143522893).
The selection executor predecessor `edec80a1b3eb508be025a28b57f34dec06262207`
also passed [CI 40143225169](https://github.com/s116821/ReMarkableOpenSDK/actions/runs/40143225169).
Buddy `5fcfad9bd3a315dabde996f005038dda95956474` now consumes the historical
decoder with original-byte digest and same-revision response checks; its
[CI 40144048485](https://github.com/s116821/ReMarkableBuddies/actions/runs/40144048485)
passed. Fifteen resident journal/socket tests pass within its 546-pass/3-ignored
Linux library run, including retained-v1 restoration, append preservation and
read-only v2 reconciliation without replay. Full consumer evidence and product
requirements remain in the ecosystem Docs change. Selection composition and
native qualification remain open.

## Native manifest protocol correction — October 25

Runtime integration inventory found `profile.json` still advertised wire revision
1 although the Rust protocol and Qt encoder/decoder already require revision 2.
The manifest now declares revision 2. Its observation-v1 profile label is unchanged;
`mutation_enabled=false` and `unqualified-source-integration` remain unchanged.
This corrects delivery metadata, not protocol behavior or native qualification.
Independent source review accepted the correction and required the fresh build
because the manifest digest is compiled into the bootstrap.

Before the change, all 38 recorded native source inputs of retained
`selection-executor-library-r1` matched the current source; that earlier artifact
and its original manifest remain preserved. The corrected source rebuilt with
GCC 13.4.0, Qt 6.10.3 and the pinned Dobby archive under the existing strict build
script into a fresh `runtime-profile-v2-library-r1` directory:

- Adapter: 1,333,464 bytes, SHA256 `b012e6c68567d82aeb642a902dfba3ff1a70ddefe2e829f147795d79488117a4`.
- Profile: SHA256 `0ab8a72e32d978cbb0be14a01c6b0145240f83f13c64cdee6fef5431c411bdff`.
- Dobby archive: SHA256 `622b17dc65d0dcad717d83264f783ad12044565ed8ea86f9e5088d9abf5328ef`.

All 38 new source-input hashes match the current files, and the new manifest
digest is present in the compiled adapter. ELF inspection confirms ARM32
hard-float, BIND_NOW, STATIC_TLS and NODELETE. Manifest parsing and direct codec
inspection confirm revision 2 correspondence. This is cross-build/file evidence;
the artifact was not installed, loaded or executed on the tablet. Actual resident
startup, reusable native capacity and product qualification remain open.
