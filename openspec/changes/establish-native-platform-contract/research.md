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
