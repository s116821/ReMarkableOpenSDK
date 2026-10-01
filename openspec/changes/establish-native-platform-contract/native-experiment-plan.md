# Native insertion reachability and bounded experiment

Status: host research plan, October 1, 2026. No native adapter or mutation experiment is qualified. This supplements [research.md](research.md) and implements the comparison work in tasks 2.2–2.4 without completing those tasks. Main remains the sole tablet operator. All device stages below are proposed, not executed. Development observation is distinct from production acceptance Gates A–D.

## Candidate comparison

| Candidate | Evidence obtained | Missing requirement / disposition |
| --- | --- | --- |
| Existing xochitl IPC | Two live bus connections were inventoried; attempted root introspection was denied. The observed abstract socket was connected, not a demonstrated listening server. | No callable insertion or ownership protocol. Respect the permission boundary; do not bypass it. This is a narrow negative result, not exhaustive absence. |
| rm-sync execute | Server dispatch and request dataflow identify synchronization, archive/unarchive and logout roles; pending requests merge. | No insertion command or isolated operation receipt established. Do not test by calling generic execute. |
| Device web upload | Handler reaches library file import. Request metadata also identifies search/list/download/thumbnail roles. | Import does not establish insertion into an existing open document or cache/ownership coordination. Do not enable the service as an experiment. |
| Native Qt controller/task/worker | Concrete insertion path, caller-supplied UUID handling, queue dispatch and save callback traced. | Strongest semantic lead, but no acceptable externally reachable entry point or mutation-time source guard. Qt metadata is not an IPC transport. |
| Cooperative filesystem lock plus metadata edit | Entry locking reaches QLockFile-backed helpers. | Lock participation alone does not prove UI ownership, cache invalidation, worker exclusion or native reload. Exact path/lifetime and coordinated reload remain unproved. Offline editing would not satisfy current live insertion behavior. |
| Internal calendar page generation | Diagnostic provenance identifies an internal calendar integration path. | No independently callable insertion transport or qualified source guard demonstrated. Do not invoke account integrations to probe it. |
| Custom in-process bridge | Qt invocation is a possible research technique once a valid object and execution context exist. | No operable supported loading/object-access lifecycle established. A bridge reached through production injection or a fragile startup modification fails the accepted design. Renaming XOVI is not a solution. |

The current evidence does not identify a production-compatible mechanism that reaches insertion safely. It also does not establish that automatic insertion is impossible. This does not block the separately authorized development observation below or select manual fallback.

## Development stage R: temporary read-only runtime observation

The [REM-25 coordinator clarification](https://linear.app/magentumdragon/issue/REM-25#comment-b60d9e4a-a2e4-4d79-8c0f-9a425b852833) distinguishes existing development-only interoperability research authorization from production qualification. A supported plugin facility is preferred, not a prerequisite for private bounded observation. No shipping constraint changes.

### Concrete first access mechanism

Use a separately launched one-shot ARM helper calling Linux `process_vm_readv` against the verified xochitl PID. This is an external memory observation facility, not injection: no ptrace attach, thread suspension, code patch, function invocation, signal to xochitl, loader modification or persistent boot/service change. Permission is still governed by the kernel's ptrace access check. EPERM/ENOSYS ends the attempt; do not change access policy or silently fall back to a debugger.

Host evidence establishes a concrete root candidate. The matched Qt 6.10.3 header implements QCoreApplication::instance through its exported self pointer. The saved executable has a dynamic object symbol for that pointer and an ARM COPY relocation. Therefore resolve the effective executable storage from its verified ELF symbol/relocation and runtime mapping; reading only the Qt library's nominal symbol storage can be wrong. Keep artifact-local addresses in the private manifest, not a public hard-coded API.

An original compile-only layout probe against the vendor GCC13.4.0/Qt6.10.3 headers establishes ARM pointer width, QObject data-pointer offset, QObjectData q_ptr/parent/children offsets and the list descriptor's pointer/count layout. Compiler warnings explicitly identify non-standard-layout offsetof as conditionally supported: these measurements qualify only that toolchain/header baseline and must be corroborated by runtime invariants. They are not portable Qt promises. The actual mapped QtCore provider hash must match the inspected baseline before interpreting its data.

### R1: root and bounded object topology only

Main first reviews the exact helper/source and artifact imports, and records its checksum. Implementation and independent review remain with main's ordinary implementation lane. Test the reader against an owned host fixture for invalid pointers, partial reads, negative/oversized counts, cycles, PID identity mismatch and changing descriptors. Never execute xochitl on the host.

Before device execution, main gives advance notice and verifies USB SSH recovery independently of the UI, exact device/firmware, executable SHA, PID/start time, process state, and mapped QtCore identity. Use a temporary private directory with no auto-start registration; do not read document bodies, account fields, environment or whole process dumps. The existing 10-second outer operator deadline covers the command. The helper has a 2-second monotonic read budget and 64 KiB output cap; limit remote payload to 64 KiB, 64 visited objects, depth four and 64 children per node. Report truncation as an incomplete observation, never absence. Mapping input has its own 256 KiB cap; refuse overflow.

Resolve the self slot through the checked ELF/mapping, read its pointer, then only the fixed QObject header and QObjectData prefix needed for q_ptr/parent/list topology. Require aligned pointers and checked ranges contained in currently readable mappings; validate q_ptr backlink, list count and checked byte sizes before following children. Resolve class identity only from corroborated read-only image/vtable/RTTI metadata; do not call metaObject(), QObject getters, QML evaluation or constructors. Leave unrecognized classes unknown. Do not scan arbitrary heap regions to find missing objects.

Every remote transfer must return its full requested length; stop that branch on partial transfer, inaccessible range or inconsistent descriptor. Reread descriptors and root/process identity to flag change. These checks reduce misinterpretation but cannot establish atomicity, object lifetime or absence of ABA reuse. Report topology as sampled, untrusted research observations; it grants no page guard or mutation authority. A non-mutating read can still cause page faults or scheduling overhead, so record process continuity and observed effects.

Cleanup is termination of the one-shot helper, collection of its bounded private result and removal of only its exact temporary files. The helper never owns a suspended target or installed hook, so timeout does not require debugger detach. Main verifies the original xochitl PID/start/state and tablet responsiveness afterward. Unexpected target restart or responsiveness loss stops the experiment; recovery uses the preverified SSH channel under main's control, with no automatic service restart or retry.

### R2: explicit follow-up, not implicit scope expansion

If R1 locates relevant controller/document/worker objects, design a separate allowlisted read manifest for their pointer relationships and lock/queue descriptors using corroborated static evidence. Do not emit unrelated strings, content or raw memory. Root reachability may omit QML singletons, unparented workers or scene objects; missing children do not prove those objects absent. If R1 is insufficient, evaluate a named temporary debugger/Qt bridge with its exact stop/resume, thread-affinity, deadline and independent detach/recovery behavior before main executes it. R1 does not authorize that escalation or any native method call.

Research observations can test whether a proposed ownership/serialization seam exists. A later instrumented mutation experiment requires its own concrete executor guard and disposable-fixture plan; production acceptance still requires Gates A–D. Experimental access alone cannot qualify a shipping loader, current-page authority or durable creation.

## Gate A: specify reachability before building a mutator

For a proposed mechanism, record the exact endpoint or process-entry facility, provider and access policy; acquisition of the real document/controller object; executor thread and worker queue; startup, reconnect and teardown behavior; and package/lifecycle behavior under normal boot. Reproduce a bounded read-only observation through that same path on the qualified firmware. An unrelated successful probe does not qualify insertion reachability.

Reject the candidate if it needs denied-access bypass, XOVI as production, autonomous menu navigation, per-conversation restart or an unqualified custom injection scheme. A host-built ELF and ABI match establish build compatibility only. Do not transfer or execute a helper merely because it builds.

## Gate B: close the dispatch-to-execution race

Identify the precise point at which source document/page, runtime session, visit, input epoch and order revision are compared, and the serialization held from that comparison through native mutation. Include queued input and other native tasks. A GUI-thread check before an asynchronous worker command is insufficient. A filesystem lock or OS input grab is insufficient without evidence covering the rest of the execution path.

Before a success test, demonstrate deterministic rejection without mutation when each precondition changes between preparation and execution. Use a qualified controllable executor boundary, not timing sleeps. If the mechanism cannot expose or control that boundary safely, leave the gate open; an after-the-fact observation cannot repair a wrong-source mutation.

## Gate C: one bounded insertion and independent verification

After A and B pass, main prepares explicitly disposable fixtures: a multi-page notebook and an annotated PDF with known original bytes, ink and unknown metadata. Fixture preparation and any UI navigation belong to the operator's coordinated test procedure. They are not an SDK menu-automation fallback. Preserve fixture baselines privately.

1. Record exact firmware/runtime/provider and adapter fingerprints, device/instance/session scope, source page UUID, full before-order and relevant content digests. Persist the consumer's immutable Prepared request before dispatch, including operation ID, request fingerprint and a fresh intended target UUID. Check that the intended UUID is absent; absence alone is not proof that an old ambiguous request never ran.
2. Dispatch exactly once through the qualified execution guard. Begin with middle insertion, then a separate end-insertion case. The request must state the source UUID and relation; a stale numeric index is not sufficient. Record the internal job correlation as supporting evidence only.
3. Treat native page-added/job-completed notifications as observation triggers. The traced save wrapper does not establish a failure-aware durable receipt. Independently verify intended target identity, exactly one permitted insertion, unchanged original page identities/order/content and expected target template/size. Attribute the result to the original request using the mechanism qualified in A/B; target existence by itself is insufficient.
4. Establish a consistent persisted snapshot through qualified native coordination. Check every required constituent file and relevant metadata, original PDF bytes, pre-existing ink and unknown fields. Determine the actual blank-page representation instead of requiring a guessed `.rm` file. The traced `.pagedata` auxiliary file must not be confused with page ink storage.
5. Qualify flush/error propagation and durability of the complete change, including directory-entry persistence where required. Independently syncing a path cannot establish consistency with an uncoordinated dirty native cache. Do not manufacture success by repairing files during verification. Failed or incomplete evidence after dispatch yields Indeterminate, never RejectedWithoutMutation.
6. Persist the verified receipt before binding CAS. Native commit does not authorize rendering: reacquire a fresh current guard and establish the consumer binding first. Verify target writability only under those conditions, on the disposable fixture.

Use bounded observation polling with a fixed monotonic deadline and output cap; record the chosen bounds in the operator's execution sheet before running. Expiry ends observation, not necessarily native execution. No automatic second insertion or compensating deletion follows a timeout. Stop immediately on process identity change, unexpected document/order change or truncated evidence.

## Gate D: interruption and reconciliation

| Interruption / fault | Required observation |
| --- | --- |
| Input/order/visit changes before execution | Proven rejection before mutation, unchanged fixture. |
| Helper/caller loss after dispatch, before callback | Indeterminate persisted locally; reconnect and observe the original request without reissuing it. |
| Save error after native insertion | No committed receipt based solely on callback; preserve uncertainty and original request. Use an isolated fault-capable test environment, not tablet-wide disk exhaustion or unbounded permission changes. |
| Native commit before consumer receipt/binding write | Recovery discovers and attributes exactly the original target; no duplicate page or authority restored from historical evidence. |
| Adapter drop/reopen with unchanged runtime | Fresh instance scope; historical receipt cannot become a mutation guard. |
| Controlled runtime restart after verified persistence | Fresh session; reconcile original target/order/content and fixture preservation without dispatch. A restart here is a qualification test, never the production insertion mechanism. |
| Target later deleted/reordered or original source revisited | Historical outcome stays historical; current state is reported separately, with no recreation or stale rendering authority. |

Runtime restart tests establish restart recovery, not arbitrary power-loss durability. Power-interruption claims require a separately controlled, bounded qualification method and full constituent-file evidence. Do not infer them from one successful reopen.

## Decision needed if Gate A remains unresolved

Production implementation continues only with a named, reviewable entry mechanism and a credible execution-time serialization design. Development stage R may proceed independently under its bounded observation plan. If a shipping route requires changing the existing no-production-injection/lifecycle constraints, bring its concrete tested tradeoffs and explicit design change to the owner before adoption. This plan does not approve such a change or an opaque mutator. Capability remains Unsupported until the required evidence exists.

## Source basis

Candidate roles and save/lock observations derive from host-only static analysis of hash-verified private firmware artifacts documented in [research.md](research.md), plus the explicitly attributed operator inventory. The raw artifacts and locators are private and have no public evidence link. This file contains original conclusions and a proposed experiment, not proprietary implementation text or an executed acceptance report.

[Vendor documentation](https://developer.remarkable.com/documentation/xochitl) recommends stopping xochitl when accessing/changing stored documents; a live coordinated-metadata route therefore needs affirmative cache/lifecycle evidence. [Qt meta-object documentation](https://doc.qt.io/qt-6/qmetaobject.html) describes object-based invocation, and [QLockFile documentation](https://doc.qt.io/qt-6/qlockfile.html) describes cooperating-process locking. These general references do not qualify private runtime behavior; exact target Qt 6.10.3 headers/providers remain the implementation baseline. Gates and acceptance procedures above are design inference from the accepted SDK contract.

The [Linux process_vm_readv manual](https://man7.org/linux/man-pages/man2/process_vm_readv.2.html) documents permission checks, partial reads and non-atomic transfers. Stage R uses these limits explicitly. Exported-symbol and layout findings come from current host ELF/header inspection and compile-only measurement; no live memory read has been performed.
