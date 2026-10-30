# Native adapter build and admission

This source builds an unqualified ARM shared object. It is not part of the Rust
package or current SDK release assets. `build.sh` never installs or loads it.
It requires the existing RM2 SDK (Qt 6.10.3) and the exact repaired Dobby archive
from the REM-57 handoff, SHA-256
`622b17dc65d0dcad717d83264f783ad12044565ed8ea86f9e5088d9abf5328ef`.
An arbitrary upstream Dobby build is not interchangeable. Build/source packaging
for independent reproduction of that archive remains part of native delivery.

Set `QT_PROBE_SDK_ENV`, `RSDK_DOBBY_ARCHIVE`, `RSDK_DOBBY_INCLUDE` and a fresh
`RSDK_NATIVE_OUTPUT`, then invoke `sh native/rm2_3_28_0_172/build.sh` in Linux.
The output retains its profile manifest, ELF report and artifact hashes.
The shared object has ARM hard-float ABI, eager bindings and NODELETE. No source
or build success establishes native constructor ordering, activation or recovery.

Dobby originates from jmpews/Dobby revision
`e9fe7fbecae47a2287e761080f8b1133cc22e8fa` with the retained handoff repairs.
Its Apache-2.0 license is preserved in `Dobby-LICENSE` and copied beside the
build artifact. The SDK's GPL-3.0-only license is copied too. The artifact links
dynamically to the target Qt/runtime libraries; it does not bundle vendor firmware
or those libraries. This build directory is not a complete release distribution.

## Bootstrap contract

The explicit supervisor must verify the approved manifest, executable and adapter
artifact before preparing admission. It remains the launched process's direct
parent and the sole admitted socket peer. Set `RSDK_ACTIVATION_DIRECTORY` to a
new owned mode0700 directory containing a regular mode0600 `admission.bin`.
The file is 168 canonical big-endian bytes:

| Offset | Length | Field |
| --- | --- | --- |
| 0 | 8 | ASCII `RSDKACT1` |
| 8 | 32 | SHA-256 of the exact profile manifest bytes |
| 40 | 32 | Exact executable SHA-256 |
| 72 | 32 | Exact adapter file SHA-256 |
| 104 | 16 | Boot UUID |
| 120 | 16 | Fresh nonnil activation UUID |
| 136 | 4 | Supervisor UID |
| 140 | 4 | Supervisor PID |
| 144 | 8 | Supervisor process start ticks |
| 152 | 8 | Admission expiry in Linux CLOCK_BOOTTIME milliseconds |
| 160 | 4 | Per-operation budget, 1–30000 milliseconds |
| 164 | 4 | Reserved zero |

Rust `ActivationAdmission::encode` creates these bytes without writing a file or
launching anything. The native consumer checks the actual boot, parent PID/start
identity, admitted peer UID against its own effective UID, and an expiry at most
60 seconds away. The endpoint separately verifies peer UID/PID through socket
credentials. The bound gates consumption,
not the entire loader/startup sequence. It creates `admission.consumed` as an
exclusive hard link and retains both names as evidence. Never clear this marker
to retry; any new activation needs fresh admission and recovery authorization.

Before exposing a socket, the bootstrap independently hashes the actual file
backing its executable anchor mapping and invokes the original exact native
profile loader. The latter checks executable/providers, mapped code and eager
binding before installing its resident hooks. `LD_PRELOAD`, `LD_BIND_NOW` and the
activation-directory variable are removed from the inherited environment. The
existing mappings/hooks remain resident. The endpoint is created on a queued GUI
startup turn and retains the admitted directory identity. It derives its own
PID/start ticks, rather than accepting those from admission.

The current endpoint exposes Observe/Reconcile only. Startup still installs
hooks, and any bootstrap refusal terminates the process with code119; "read-only"
describes the request operations, not harmless payload loading. Actual Supervisor
recovery and a fresh bounded native activation must be qualified before use.
The directory owner is trusted; these checks do not isolate hostile same-UID
code or authenticate a signature. No per-request unload or process restart is
part of the contract.
