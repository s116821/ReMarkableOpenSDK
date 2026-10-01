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

The official firmware-matched vendor compiler SDK installer was already acquired and freshly hash-verified; no repeated download. SHA256e2daf17a86d375aae6d290af993060f3dfb488568ca83c401eaa47869e975179. An isolated local Docker build is installing it for actual target sysroot inspection. Installation completion, target provider hashes and compile/import results remain pending; do not infer them from the installer version.

## Bounded passive runtime handoff

Main remains sole tablet operator. First inventory only: current model/firmware and xochitl PID/start identity, presence/version/help of busctl, existing system-bus peers, existing Unix socket names and xochitl socket-FD inode links, then the same process identity afterward. Use the existing host-side 10-second per-command deadline and a64KiB output cap; stop on change/truncation. Do not read document content, environment secrets, network traffic or unrelated account/configuration files.

If busctl is available, inspect help before using supported options and list peers without starting services. Do not yet run tree/introspection, getters, mutator methods, activation, monitor/capture, service restart, file writes or UI actions. A second exact interface-introspection request should name only already-running candidate owners/paths after inventory establishes them, disable auto-start, avoid property values and retain bounded output. Endpoint observation is not permission to invoke its methods.

No script from this repository should run automatically on a tablet. This is a proposed operator handoff, not executed evidence.

## Primary external sources

- [Vendor Xochitl documentation](https://developer.remarkable.com/documentation/xochitl), checked October 1: proprietary runtime, compatibility not guaranteed, rm-sync lifecycle coupled to xochitl. This argues for exact qualification; it is not an API promise.
- [systemd busctl documentation source](https://github.com/systemd/systemd/blob/main/man/busctl.xml), checked October 1: bus peer inventory and interface introspection are distinct operations. Validate installed tool options rather than assuming latest-systemd features.

Source basis: current Linear/GitHub outputs, local static evidence and hashes, official documentation, plus explicitly attributed coordinator tablet report. Proposed API/research ordering is design inference. No copied proprietary implementation is included.
