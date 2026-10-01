# Research ledger

## Current evidence, October 1, 2026

No native creation mechanism has been qualified. This ledger records evidence strength and next observations; it is not a compatibility declaration.

### Requirements and repository boundary

Current REM-25 description and all eight comments were read; pagination reports no additional comments and no inline/reply anchors were returned. September 29 excludes XOVI production dependency; October 1 commits the independent SDK and its repository-local OpenSpec. REM-35/29 retain RM2-based 1.0 and later Paper Pro native validation. Current descriptions/comments of REM-37/41 were also read. Public proposal/design mirror these requirements so contributors do not need private tools.

### Private static firmware analysis

The authorized saved RM2 xochitl export from firmware3.28.0.172 has SHA256071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df. The binary remains private and was not executed or published. Static metadata had identified DocumentController page-insertion method names; runtime reachability, defaults, thread ownership, PDF behavior and durability remain unknown.

October 1 ASCII/UTF-16 string and imported-symbol scans found sync and device-policy D-Bus identifiers, including `no.remarkable.sync` and `com.remarkable.devicepolicy`. System/session-bus imports exist. These are discovery leads only: neither bus registration nor a creation API follows from them. No external page-creation endpoint was verified. Absence from this narrow scan is not proof that no endpoint exists.

### Runtime ABI correction

The saved xochitl executable imports versions through GLIBC_2.38; this establishes requirements, not the maximum symbols exported by its libc provider. Main independently reported a bounded read-only October 1 check of the actual authorized RM2: GNU libc2.39, GCC13.4.0, minimum kernel5.4.0; firmware3.28.0.172/CodexLinux5.8.203. Reported libc SHA2568d741a0db9727c97faebfd17e7f8409183c382241ff3d4c9b55a52e98d8dd832 and loader SHA2569e7e72e7d488db4c885eac6a360132690771a0308e8f56e187710d07a6b46a62. This worker did not independently access the tablet. Preserve the coordinator's raw record with release qualification evidence.

Therefore the official Buddy ARMv7 package's reported GLIBC_2.39 import is not, by itself, a demonstrated mismatch with this tablet. Failure under an older emulator sysroot is an emulator/provider mismatch until full artifact/provider evidence says otherwise. This corrects the unsupported inference that the tablet's libc ceiling was2.38.

The official firmware-matched vendor compiler SDK installer was already acquired and freshly hash-verified; no repeated download. SHA256e2daf17a86d375aae6d290af993060f3dfb488568ca83c401eaa47869e975179. Isolated local Docker installation completed successfully in image SHA256416c7a7be0038156797b0892f031f352b841d1921fae83f712d0a272e4724618. Its target libc and loader hashes independently match both coordinator-reported tablet hashes above exactly. The target provider defines GLIBC_2.39; GCC reports13.4.0; QtCore headers and Qt6Core/Gui/Qml pkg-config report6.10.3. This establishes a concrete compiler/sysroot baseline, not qualification of every dependency or private native API. No ARM application/probe has yet been built in it.

## Bounded passive runtime handoff

### Completed inventory and root introspection

The coordinator executed the first two bounded read-only stages on October 1. This worker read the raw records afterward. Firmware/model match the RM2 baseline; xochitl and rm-sync PID/start identities remained unchanged across both stages. Installed busctl is systemd255.21. System-bus inventory shows two unique connections owned by xochitl and one by rm-sync. The acquired-name list has a sync service but no named xochitl service; that does not mean xochitl has no exported objects.

The observed abstract xochitl socket is a connected Unix stream (state03, no listening flag), not evidence of a listening RPC endpoint. It must not be treated as a discovered server address.

With the current sync unique owner verified, a standard Introspect call at `/` returned only the standard Introspectable/Peer interfaces and a child named `Synchronizer`. No property getters or sync/native mutation methods were invoked. The next request is restricted to XML at the discovered `/Synchronizer` and the roots of the two verified xochitl connections, after fresh owner/process guards. Errors or empty roots would narrow these endpoints only, not exhaust every possible native mechanism.

Private raw evidence remains with the coordinator, not in Git: `sdk-passive-inventory-20261001-unique.txt` SHA256 `ec43c02242a617c322560ac905b4abd19121f470b84077df643ce50e4f0a4d89`; `sdk-passive-inventory-20261001-sync-root.txt` SHA256 `b5092be8842b0f7e68a6d1fb0a1d6548abbfdacdd783787c09292e3d9c5aadd3`. This worker independently checked those file hashes. Session-specific unique bus names are not a stable SDK API.

### Completed interface inspection and host-only follow-up

The coordinator's third stage verified owners/process identities before each standard Introspect request and reported unchanged identities afterward. `/Synchronizer` exposes the `no.remarkable.sync.Synchronizer` interface, with sync/authentication, protocol, entry-lock, cleanup and availability methods plus sync/progress signals. Its generic `execute` declaration takes a base path and an `ExecuteRequest` structure; the declaration does not establish page creation, atomicity, mutation-time guards or durable operation correlation. No declared method or property getter was invoked. This interface is not an accepted automatic-page mechanism.

The roots of both verified xochitl connections returned `Access denied`. No policy change, identity spoofing, alternate-call bypass or repeated denied request is planned. These results establish a permission boundary for the attempted calls, not absence of exported objects or every possible native mechanism. Live interface discovery pauses here while host research continues.

Private raw stage-three evidence hashes (independently checked by this worker):

| Record suffix | SHA256 |
| --- | --- |
| interface-1-435.txt | 6d7e3170d1d70dd077955b1f586175785265d4e87a2113ee7cde3f83561d3564 |
| interface-1-436.txt | 79aa44eb9a5f0f1f0132443dc8475bd7e5f75e5c80f4ff6716ee5265c60e9cf2 |
| interface-1-437.txt | 2d7237d028e31f0775de41f50ff28b3e4ea808d85db4c58c63b13324b76ee81c |

Filenames share prefix `sdk-passive-inventory-20261001-` in the coordinator's private outputs directory. Raw process/network details are not committed here.

Host-only scans of the preserved xochitl dynamic symbols matched Qt D-Bus client/watcher functions but no Qt registerObject/registerService/QDBusAbstractAdaptor import in that executable's dynamic-symbol table. This does not exclude indirect, static or other-library mechanisms. A bounded vendor SDK filename scan found Qt synchronization facilities, not an identified reMarkable native creation interface; names such as QtLabsSynchronizer must not be confused with the observed rm-sync service.

The saved executable depends on `libQtWebAppHttpServer.so.1` and contains DeviceWebServer, document/upload/download request names and `/documents/`, `/upload`, `/download/`, `/thumbnails/` identifiers. These are a separate document-management discovery lead only. No HTTP listener was enabled, contacted or fuzzed, and no file/document operation was issued. Safe insertion into an open document, cache coherence and exactly attributed durable receipts remain unproved; route names cannot satisfy those requirements.

Next host work should map independently observed interface roles and credible operation/serialization mechanisms, then propose a concrete bounded experiment only if it can test the required safety guarantees. Do not broaden sync calls, modify policy, or select manual fallback merely because these initial endpoints did not qualify.

### Retained operator boundaries

Main remains sole tablet operator. First inventory only: current model/firmware and xochitl PID/start identity, presence/version/help of busctl, existing system-bus peers, existing Unix socket names and xochitl socket-FD inode links, then the same process identity afterward. Use the existing host-side 10-second per-command deadline and a64KiB output cap; stop on change/truncation. Do not read document content, environment secrets, network traffic or unrelated account/configuration files.

If busctl is available, inspect help before using supported options and list peers without starting services. Do not yet run tree/introspection, getters, mutator methods, activation, monitor/capture, service restart, file writes or UI actions. A second exact interface-introspection request should name only already-running candidate owners/paths after inventory establishes them, disable auto-start, avoid property values and retain bounded output. Endpoint observation is not permission to invoke its methods.

No script from this repository should run automatically on a tablet. The completed stages above are attributed coordinator evidence; subsequent requests remain unexecuted until their raw results are reviewed.

## Primary external sources

- [Vendor Xochitl documentation](https://developer.remarkable.com/documentation/xochitl), checked October 1: proprietary runtime, compatibility not guaranteed, rm-sync lifecycle coupled to xochitl. This argues for exact qualification; it is not an API promise.
- [systemd busctl documentation source](https://github.com/systemd/systemd/blob/main/man/busctl.xml), checked October 1: bus peer inventory and interface introspection are distinct operations. Validate installed tool options rather than assuming latest-systemd features.

Source basis: current Linear/GitHub outputs, local static evidence and hashes, official documentation, plus explicitly attributed coordinator tablet report. Proposed API/research ordering is design inference. No copied proprietary implementation is included.

### Host metadata follow-up: web and document-lock roles

A bounded host-only Qt static-metadata scan of the same private xochitl artifact identified DeviceWebServer enable/disable/settings methods and disabled/failed properties. UploadRequest and DownloadRequest yielded class metadata with no declared methods or properties in this parser output. DocumentLockManager metadata includes document/task-tracker properties and sleep, page-modified and lines-stored callbacks. The original binary was parsed as data in the existing network-disabled local container; it was not executed. Raw metadata and the inspection script remain private.

These observations narrow interface roles only. The web metadata does not establish an external page-insertion route. Document-lock callbacks do not expose a verified external acquire/release protocol, compare-and-act guard or durable receipt. Empty parser results are not proof of absent behavior. No endpoint was contacted or enabled, and no new tablet experiment follows from this scan. The next research question is whether a supported externally callable mechanism can combine source ownership, coordinated mutation and durable attribution; none is yet demonstrated.

### Static insertion/task trace beyond metadata

Further host-only analysis of the same hash-verified private executable mapped DocumentController insertion dispatch into an AddPageTask and a DocumentWorker command queue. The path validates a supplied nonempty UUID, checks duplicate page identity and has explicit document-presence, disk-space, format-version and document-lock/outdated refusal branches. Internal lock/unlock helpers use mutexes and include thread-sensitive behavior. These are concrete internal mechanisms, not a verified externally callable SDK interface or user-input ownership fence.

The worker insertion path reads the current page count and appears to normalize negative/out-of-range positions to append. This static behavior must not be mistaken for an exact source/order compare-and-act guarantee. A caller's old page position is insufficient evidence for safe insertion.

The task/worker path separately exposes page-added and job-completed notifications. The worker emits job-completed after command dispatch and on a skipped-command branch. Therefore a generic job-completed notification is not, by itself, proof of successful creation. Neither the initial boolean nor the page-added callback establishes durable persistence, original request attribution or restart reconciliation. File flush/fsync facilities exist elsewhere in the executable, but their existence does not prove the insertion completion path has crossed a durable commit boundary.

The sync client trace identifies an asynchronous D-Bus request built by a sync workflow with offline/suspended/no-work branches. The three collection fields and server-side effects still require analysis; no generic mutation or page-creation meaning is inferred. The saved private artifact set and vendor sysroot currently lack the rm-sync server executable. A narrowly scoped read-only export was requested through the sole tablet operator to close this static-analysis gap; no sync method invocation or service change was requested.

Raw call traces, private artifact-local addresses, metadata and scripts remain outside the public repository. Source of these conclusions is static control flow corroborated by Qt metadata, RTTI and diagnostic identifiers; no native operation or durability acceptance has been performed. Continue tracing storage scheduling/completion and reachable HTTP/service roles before proposing any native mutation experiment.

### Server-side sync and HTTP upload follow-through

The sole tablet operator completed a bounded read-only export of the running rm-sync executable. The private acquisition record brackets firmware 3.28.0.172 and stable process identity. The final local artifact is 3,409,308 bytes; its independently checked SHA256 is `1caf6493cce2132af89a9377679c19005bef1c5b0e658f704f95b4fdbd07138c`, matching the operator's source/export report. This supersedes the earlier missing-server-artifact limitation. The executable and all raw traces remain private, outside this repository.

Host-only analysis links the D-Bus wrapper's execute dispatch to the internal synchronizer. That path checks for a user ID and aborts when absent; it checks the base path and disk state before continuing. Request dataflow, queued-to-active field copying and matching diagnostic argument labels identify the three collection roles as entries to sync, entries to unarchive and entries to archive, followed by a logout flag. The workflow merges pending requests, so a call cannot be presumed to represent an isolated transaction. These findings establish synchronization roles, not a page insertion command, source ownership guard or durable creation receipt. No account values were read and no sync method was invoked.

Separately, the xochitl upload handler invokes a library mediator with a request ID and filename; the mediator converts the local file to a URL and reaches the library's file-import path. This is concrete evidence for upload/import behavior, not insertion into an existing open document. No HTTP service was enabled or contacted.

Source basis: independently checked private artifact hash; operator-attributed acquisition bracket; static control flow corroborated by Qt metadata, RTTI and diagnostic dataflow. Runtime effects and durability remain unqualified. The next host investigation returns to native insertion's save/commit boundary and externally reachable coordination mechanisms; neither service lead justifies an opaque mutation experiment.

### Insertion callback and content-save boundary

The insertion callback now has a concrete static connection to persistence. It first compares the worker job ID with the task's retained ID, updates document state, calls the entry save/upload-scheduling path, and then emits the document page-added notification. The document content-dirty flag leads through auxiliary saving to JSON content metadata and a temporary-file writer. For the traced default mode, that writer writes the temporary file, renames it into place and calls the concrete filesystem helper that opens and fsyncs the final path. This strengthens the earlier evidence: fsync is connected to content saving, rather than merely found elsewhere in the executable.

This remains insufficient for an SDK durable receipt. The traced save/upload wrapper does not gate the later page-added notification on the auxiliary save's boolean result. The auxiliary save routine also clears its dirty flag after combining the page-data and content-metadata save results. These are static control-flow observations, not a reproduced disk-failure incident. The normal content-save branch does not establish a prior temporary-file fsync or parent-directory fsync; consistency across all constituent page files and crash outcomes still need evidence. Internal job-ID matching is narrower than a durable caller-operation-to-target record.

The upload-related helper schedules later work; it does not prove synchronous cloud success. No sync or page operation was invoked. Remaining research focuses on failure-aware completion, constituent-file persistence, restart reconciliation and an externally reachable mechanism with mutation-time source ownership. Raw addresses and implementation traces remain private.

### Reachability comparison and experiment gates

Further host-only tracing identifies QLockFile-backed entry locking, but has not established a cooperative external protocol covering source ownership, worker serialization and cache reload. The inspected QFileSystemWatcher call sites belong to user-authentication file-watching code rather than demonstrated document reload; no account values were accessed. The inspected command-line option registration supplies a system-service option and standard help/version handling, not a demonstrated page-command transport. These bounded traces are not an exhaustive inventory of every possible interface.

Request-class metadata adds search/list/thumbnail roles to the already traced web upload/import path. Diagnostic provenance identifies the separate generate-and-insert-page lead as internal calendar integration. Neither establishes an acceptable externally callable page insertion route. No HTTP, calendar, sync or account operation was invoked.

The auxiliary save path includes `.pagedata` writing through a QFile-backed wrapper, separately from `.content` JSON saving. The inspected page-data routine does not itself establish temporary-file replacement/fsync; wrapper/destructor behavior and document-type applicability remain unresolved. `.pagedata` must not be confused with per-page ink files.

[Native experiment plan](native-experiment-plan.md) records the candidate matrix and bounded staged acceptance procedure. The first unresolved gate is a concrete operable entry mechanism plus execution-time source serialization. Independent SDK verification may compensate for a weak native completion notification; it cannot retroactively correct an unfenced mutation. The plan separately tests target/order, constituent-file persistence, interruption and fresh-scope restart reconciliation, with no automatic retry or deletion. It authorizes no tablet execution and does not declare all alternatives exhausted.

### Worker affinity and queue serialization refinement

Host static tracing connects document worker acquisition to application-thread QObject affinity, followed by QThread startup. The worker's RTTI/vtable connects its run override to the previously traced command loop. Consequently, the QObject's affinity thread must not be assumed to be the thread executing native commands. Invoking a Qt method on that object does not, by itself, establish worker-execution serialization.

The enqueue path and command loop share a queue mutex. The loop removes a pending command and updates the pending count, then releases that mutex before command dispatch and reacquires it for completion bookkeeping. Separate active-command state is set before dispatch. Therefore an empty queue or acquisition of the queue mutex is not proof of idle execution or a guard covering the whole mutation. Static metadata corroborates a queue-size-change notification; that notification remains narrower than task completion or persistence.

These findings guide a later allowlisted worker/lock observation after the development topology probe locates objects. They do not expand the initial probe's memory-read scope, establish a callable guard or qualify any production capability. Exact fields and trace locators remain private; no worker method or lock was invoked.

### Completed development observation R1

Main, the sole tablet operator, executed the reviewed one-shot external topology observer once on the authorized RM2, firmware 3.28.0.172. Source `e60eecf5adfc8e69c6eed6a279db5a5b20730f13` was independently reviewed and its ARM artifact independently reproduced by the ordinary Sol review lane, as reported by main. Binary SHA256 `14875f7530b75759b5c299d208339172436ae3374073c2774f55ad329dc8c566`. The original tooling is integrated unchanged at `7a55f74b6eaf63733cb5b7da396a3815c66e8089`; the tools tree compares identical to the reviewed source. The research lane did not repeat its test suites.

The operator preflight recorded the previously qualified executable hash and unchanged process/start identity. The actually mapped QtCore provider SHA256 `43b0e210d64e59b534490d78c4c82cc1d2958999b0969aa0f77e11a082704e5d` matches the inspected vendor provider. The executable COPY-relocation root and supplied ARM layout were checked before execution. These provider/layout facts qualify the narrow sampled interpretation, not a portable Qt ABI or native SDK capability.

The returned sample contains seven objects: the application root and six direct children. It reports 752 remote bytes, `incomplete=false`, `atomic=false`, and all classes unknown. Output is 934 bytes, stderr is empty, and the helper exited zero. This research lane independently checked the raw JSON SHA256 `94d5fe2cfa6bd7ef5f22636d81860138aaf969cca378a46ba8e37d2dfd563edd`, read the operator receipt, and compared the sampled vptrs with the private five-class controller/lock-manager/wrapper/worker/task allowlist: zero exact matches. This says nothing about objects outside that child tree or concurrent lifetime/ABA changes. Raw pointers and records remain private.

The operator receipt records no timeout or signal recovery, unchanged xochitl process/start/state, active services, verified helper termination and removal of the exact staged files/directory. Independent SSH responsiveness was checked. Active UI/gesture responsiveness was not exercised and is not inferred from process/service continuity. No native calls, input, service changes, policy changes, document/account reads or automatic retry occurred.

R1 establishes that the bounded external read route worked for this sampled Qt topology. It does not establish current-page authority, fresh capture, callable insertion, an execution-time source fence or persistence. Host tracing identifies a parentless existing-controller QML singleton registration, so [R2's separate registry proposal](native-singleton-observation.md) is a concrete next discovery candidate. No R2 execution follows implicitly from R1 success.
