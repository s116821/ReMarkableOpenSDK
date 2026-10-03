# One read-only development facts entry

Status: concrete source/access proposal, **not ready for a native build, packet,
transfer or device trial**. Main selects one development startup/preload followed
by fresh Main-manual selection of the existing six-page fixture. Preserving the
pre-restart process or selection is not required for this read-only experiment.
Main is the sole tablet operator. The accepted facts source stays at
`6875514bc498d25457a835accfc7e96e4b28447f`; this proposal changes no implementation.

## Entry constraint and smallest candidate

The existing `qt_qml_access_probe.cpp` uses Q_COREAPP_STARTUP_FUNCTION and queued
application-thread work. Its existing-engine access is to the newly started
xochitl process's own engine; it is not a reviewed hot-attach route into today's
stock process. Startup into Myfiles does not provide a document owner. Current
process_vm_readv helpers cannot execute Qt getters/mapping calls or install these
observers. XOVI remains a candidate, not qualified hot attachment. Do not introduce
remote calls, another loader, a daemon, a cgroup supervisor or a new effect framework.

Propose a separate narrow facts-only startup translation unit including
`qt_page_facts.h`, independent of Probe's controller/creation/open configuration.
Reuse only the demonstrated public Qt startup/queued-entry pattern, bounded current
window-engine discovery, exclusive private output and the development operator's
single-attempt launcher plus independently armed stock-restoration transaction.
No native source implementation or artifact is selected by this document.

The entry has these states, each used at most once:

1. Bootstrap on the application GUI thread. Check the frozen profile/nonce/session
   directory and current attempt identity. Observe a unique GUI-affine existing
   engine through bounded window metadata: at most 16 windows and 8 queued
   readiness-event admissions within the original setup deadline. No document,
   page or mapping read happens here. Ambiguity, cancellation or a retained-engine
   replacement refuses; never reset to a new engine. No component import or
   controller lookup is needed to decide that a GUI engine exists.
2. Retain the engine weakly and publish one `facts-waiting` marker containing the
   nonce, attempted PID/start identity and setup clock information. This means
   only that the finite facts request can be admitted. It does not mean a native
   document is ready or facts have been observed. Install one directory watcher
   before publishing; there is no periodic owner/mapping scan.
3. Main verifies this stage belongs to the live attempted process, manually opens
   the expected fixture, then publishes exactly one `facts-request` token. The
   token is a fixed nonce/PID/start/read-facts message, at most 128 ASCII bytes.
   The publisher is a fixed one-file operation, not a UI command executor or a
   setup-effect supervisor. Its exact source and owned-file admission tests need
   review before inclusion in a packet. It must not import PageOpenArmGate,
   open-arm/openOnce, or the held native_page_setup_guard implementation.
4. Queue token processing on the GUI thread. Check directory/file ownership, mode,
   regular-file/no-follow status, complete exact token and current attempt/engine
   identity, closure/restoration flags and the absolute setup deadline. A stale,
   preexisting, malformed, late or duplicate token refuses. Consume the one request
   before any document getter. A token lost during teardown cannot be replayed.
5. Begin exactly one retained, parentless PageFactsSession using the frozen private
   expected document ID and complete six-page order, pageCap=6 and budgetMs=5000.
   Its existing connections/baseline, mapping, alias, epoch, weak lifetime, thread,
   cancellation and final-delivery checks apply unchanged. Progress also refuses
   session closure, restoration or attempt-generation loss. Myfiles/no owner or
   the wrong document is a final refusal, never a reason to wait or read again.
6. Serialize one bounded private result and a separate public callback status.
   Retain the reader and entry through queued completion and outer getter unwind;
   do not delete either from an engine/lifetime signal or nested event loop.
   Native getters can hang despite logical checks; the external restoration duty
   remains armed throughout. Always restore stock, including after successful facts.

Bootstrap readiness admissions above discover only the process's Qt engine.
They do not qualify or repeatedly sample document facts. Selection and ownership
are accepted only by the one post-request reader. No read begins automatically
at Myfiles, on an incidental focus event, or on the first convenient matching page.

## Fixed bounds and timing eligibility

| Bound | Clock origin and meaning |
| --- | --- |
| 20 seconds | Absolute startup-entry setup admission budget; fresh selection and token acceptance must finish before expiry. No renewal from a waiting marker or UI action. |
| 5 seconds | One facts reader's monotonic logical budget starting at the accepted request. Includes discovery, getters and queued delivery; it does not preempt a hung getter. |
| 35 seconds | Existing host live-callback observation admission, from its reviewed operator clock origin. Late collection is historical and cannot retrofit live proof. |
| 45 seconds | Existing independently armed systemd rollback timer's initiation, from timer arming before activation. This is not a 45-second completed-restoration guarantee. |

Preserve the accepted operator's restoration unit bounds and verification procedure
separately, including its 240-second TimeoutStartSec. Do not relabel a transport
timeout, timer expiry or active service as proof of completed stock restoration.
No changes to any of these bounds are proposed.

The spent 604d OPEN experiment reached the setup deadline without an accepted token
or native open. It restored stock; this is negative timing evidence, not proof that
another human/tool sequence fits. This facts-only setup needs one document opening,
not the earlier blank-target/source-page round trip. It accepts whichever page is
freshly active in that exact document; there is no required source/target page pair.
That reduces proposed steps but does not measure their duration.

Before selecting a native build/packet, Main must supply a concrete timed procedure
supported by existing or separately authorized stock-only evidence: how the live
waiting stage is observed, one fresh fixture opening is completed, and the fixed
request is published within the same 20-second admission. Include transport and
tool delays and margin, with monotonic clock origins. No chat round-trip, screenshot
latency or intended speed is assumed to fit. If this cannot be supported, preserve
the accepted source and mark this candidate not ready; do not try blind injection,
extend a timer, automate effects, or make hot attachment a gesture-MVP prerequisite.

## Packet, private output and recovery protection

A later packet requires a new nonce and exact source/config/artifact/operator hashes;
604d and the earlier creation packet remain spent. Freeze all literal paths before
staging. Proposed owned names under `/run/rmb-qt-probe-<new nonce>` are:
payload.so, launch.sh, restore.sh, native-probe.conf, owner, dropin.sha256,
parent.signature, admission.lock, attempt.claim, attempt.identity, entry.closed,
restore.claim, restored, restore.failure, verification.first-refusal,
facts-waiting, facts-request and its one temporary publisher file, callback.json,
and diagnostics.json. The final packet must enumerate any added file; unknown
names or a mismatched owned parent prevent cleanup success.

The sole success stage is `facts-observed-no-change-during-read`; resolved, waiting,
creation/open success, deadline or malformed values never qualify this trial.
The public callback contains only frozen nonce, exact facts success/refusal stage
and typed application/engine context booleans. Controller availability is neither
required nor implied. Private diagnostics (maximum 8192 bytes for this six-page
trial) contain explicit fact kind, expected/observed document/current ID/index/order,
local instance/epoch/read interval, required metadata/connection result and authority
flags fixed false. No raw QObject addresses or restored operational observations.
Publish private diagnostics first, then callback, using exclusive no-follow mode-0600
bounded writes in the root-owned mode-0700 directory. Partial output is invalid and
never overwritten/retried. A well-formed refusal is distinct from absent/unknown output.

Main's final operator must preflight fresh model/firmware/Qt/executable and effective
stock-unit hashes, live PID/start, unchanged baseline policy, absence of conflicting
session/preload state, exact no-clobber file publication and separate recovery access.
The reuse reference is the unchanged launcher/restore/operator at Buddy
`53e94cc630d7ba06fcf407a227b7326135e0f15f`, inspected at current Buddy `197619f`.
Reuse restoration mechanics only, not OPEN mode, validators, spent identities or
its payload. Preserve xochitl.service identity and normal direct restart policy.

Arm the existing singleton transient restoration service/timer and verify it before
publishing the owned ExecStart override or issuing the one activation. Main and timer
request the same restoration service; no competing restore-script actor. Launcher
consumes its fixed attempt before preload, and every later invocation execs stock.
Success, refusal, expiration and unknown outcomes all keep mandatory stock rollback.
Retain the live attempted PID/start/executable/payload and empty-job proof and the
private result inside the 35-second admission. Final files cannot establish those
live flags after the attempted process has been replaced.

Require actual stock restoration receipt, original effective unit/restart policy,
stock process/cgroup without payload, attempted generation gone, empty xochitl job
and original services active. Collect evidence before exact-name cleanup. Uncertain
restoration retains stage and rollback duty; no retry or stage removal assertion.
Prior successful development restoration does not qualify production failure,
simultaneous actor loss, hard syscall bounds or an automatic Supervisor lifecycle.

Freeze a fresh private six-page baseline before the later trial. Preserve every full
page object/order, original PDF/ink and all non-navigation metadata/file hashes.
Main must explicitly name any permitted lastOpened/navigation timestamp differences;
do not silently broaden preservation exceptions. Compare post-restoration content,
metadata and original-file hashes, and retain visual evidence where needed. Reading
facts must not insert, move, remove, open a native page, draw, pair, sync, upgrade or
write through an SDK authority object. Manual selection is the separately selected
setup action; no atomic snapshot, input continuity, render/write authority, durable
revision or Rust PageObservation follows from a facts record.

## Review sequence and missing evidence

Review this access/trigger/output/recovery proposal before implementing its narrow
entry and owned protocol fixtures. Required fixtures include startup at Myfiles with
no request, one request at no/wrong owner followed by later favorable UI (no reread),
correct one-shot request, stale/wrong/duplicate token, replacement/lifetime/thread
loss, deadline/late delivery, restoration closure and malformed private output.
Source plus those fixtures precede an independently reviewed native artifact and
literal operator packet. No full guard framework or unrelated OPEN tests are required.

Current missing fields: supported manual timing procedure; independent review and
reproduction of the source checkpoint below; fresh private baseline/config; nonce;
native ELF/dependency/source hashes and independent rebuild; exact refreshed baseline
identity; complete literal operator/effect/recovery packet. Their absence means
**not ready**, not permission to substitute historical hashes or execute placeholders.
Native creation correlation, gesture/input/render authority and production runtime
qualification remain separate work.

## Source implementation checkpoint

Main and Astra accepted the proposal at `2f6218b` before source work. The separate
`qt_page_facts_entry.h/.cpp` implements pinned directory/attempt identity, finite
engine bootstrap, one waiting/request admission, a single retained reader and
private/public output. It enforces acceptedRequestAt plus the five-second budget
through queued dispatch, reader progress, delivery and serialization; queued delay
cannot restart that clock. The startup translation unit requires a separately
reviewed private native config and fixed 20-second/six-page/five-second settings.
Only owned-config syntax checking was performed; no native SO was built.

The caller keeps entry and reader parentless through native getter/outer stack
unwind. Closed/restoring state, generation, directory inode, weak engine, affinity
and deadline loss refuse. Private results bind nonce and attempted PID/start plus
the exact facts tuple and metadata/connection validation; authority remains false.
Exclusive bounded output does not qualify a hard syscall limit. A late/unknown
success write is withdrawn as an unusable final callback, never retried; the later
live operator must independently apply its generation/time admission and retain
unknown I/O outcomes rather than infer a completed transaction.

The fixed publisher/validator are Buddy source
`1b9e1930cf03b254a9d9302a932969101846f04a`, under tools/native_page_facts_probe.
The publisher uses one nonblocking descriptor lock and no-follow pinned files,
live process/ELF/payload/proc evidence, exclusive temporary creation and no-clobber
link publication. It spawns no child and performs no arbitrary command/UI/service
effect. Token publication itself is not native facts success. The validator checks
fixed public/private shape, fresh nonce/process/document/order/interval/epoch and
false authority flags; live generation and stock restoration remain separate.

Author checks: 22 owned Qt entry/protocol cases plus one original-QML cross-repo
publisher-to-entry integration pass; startup source passes owned-config syntax
checking. Buddy's 19 publisher mechanics cases and 86 strict PowerShell proof
checks pass; publisher CLI passes syntax checking. Qt checks use pinned image
416c7a7be0038156797b0892f031f352b841d1921fae83f712d0a272e4724618,
network disabled, both sources read-only, vendor SDK ARM compiler with warnings
as errors and qemu/offscreen fixtures. Publisher/integration data named payload.so
is non-ELF and mapped read-only; ignored-loader warnings are expected, not proof
of a loaded extension. No native preload extension/helper artifact, literal device
packet or device action was selected. Accepted reader source `6875514` is unchanged.
Independent source review/reproduction remains pending. Manual timing feasibility
remains unproved, so this checkpoint does not make a native candidate ready.

## Accepted source and operator timing handoff

Main independently passed the 22 entry cases, cross-repo integration, 19 publisher
cases, 86 proof checks, startup syntax and strict OpenSpec validation. Astra's full
cross-repository review accepts SDK `127b332fdea561692908f1c9bf6207981d50a89e`
and Buddy `1b9e1930cf03b254a9d9302a932969101846f04a` as the source/owned-fixture
checkpoint with no blocking findings. This supersedes the pending review status
above, not the unproved timing/native readiness status.

Main accepted a source-independent timing assessment and handed actor choice to
the parent. Physical user availability beside tablet/local console is unconfirmed.
Before any activation, prebrief one intended fixture opening, any active page,
local cues and the exact prepared stage-check/publisher invocations. During a later
selected attempt: one positive waiting/current-generation check, one fresh manual
fixture opening, then one fixed publisher invocation. An absent/stale/late check
ends setup and restores; no polling, second check, chat acknowledgement, page round
trip, blind coordinates or deadline extension. Physical verification can avoid
image transfer only if that physical actor/cue procedure is explicitly selected.

For a separately selected stock-only rehearsal, start a monotonic host setup clock
before the activation/setup action, not after its transport returns. Record stock
startup (or label warm-start-only), one stock generation exchange, local cue/manual
opening/render confirmation, all human/image interpretation gaps and one read-only
transport exchange standing in for publication. This is a measured stock surrogate,
not a positive facts stage or exact new publisher/watcher timing. No native build,
injection, request root or packet is needed for that rehearsal. Native cold-start,
publication/admission processing and uncertainty require supported allowance.

Eligibility remains a conservative total below 20 seconds: startup/stage proof,
manual opening/render/review, transport/publication/admission allowance and explicit
margin. Unknown intervals or a failed bound mean not ready. Main's tool route must
include screenshot retrieval, image review and dispatch latency; installed helper
hashes are observed but source-unqualified, never rebranded as reviewed. Parent must
choose an available physical setup or a supported measured Main-tool procedure.
No rehearsal/device action or timing pass was performed by this worker.

Source basis: Main's current selection of fresh manual setup and unchanged bounds;
accepted facts source and Astra full/repair review; current repository startup and
Buddy launcher/operator/restore source; current Project's recorded spent-604d negative
outcome. Astra's bounded access assessment is current connected-chat output, not a
hardware transcript. Private evidence has no public link here. Proposed trigger,
timing simplification and integration are design inferences; no new native access,
timing, device or recovery result is claimed.
