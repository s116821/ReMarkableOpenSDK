# Worker-local page order accessors

The exact RM2 executable already used by the worker and creation diagnostics
contains two useful passive value accessors. This is an interoperability contract
for one saved build, not a public firmware API or native qualification.

The target is ELF32 little-endian ARMv7 hard-float ET_EXEC, 22,046,100 bytes,
SHA-256 `071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df`.
The pre-call review used that retained image, GNU ARM binutils and the existing
vendor Qt 6.10.3/GCC 13.4 toolchain. No accessor was invoked by that review.

| Question | Static finding | Limit |
| --- | --- | --- |
| ID at index | `0x6d5bf0..0x6d5c98`: typed QString return, complete worker pointer and signed index | Out-of-range returns empty; it does not qualify the frozen index |
| Index for ID | `0x6d5c98..0x6d5d1c`: signed index return, complete worker pointer and QString reference | Missing ID returns minus one; membership can change between calls |
| Locking | Each accessor takes and releases the worker map mutex independently | No six-call transaction or exclusion from renderer writers |
| Value lifetime | ID-at-index copies an implicitly shared QString while locked | The caller must destroy its value with the matching Qt ABI |
| Callback context | Inspected normal command loop releases its separate queue mutex before invoking the callback; no surrounding map lock found | Applies to that exact loop/type-one route, not arbitrary callbacks |
| Failure | Lock-failure edges throw before acquisition/result construction | Catch cannot recover abort, corruption, indefinite wait or a violated noexcept comparator contract |

ARM compile-only checks independently corroborate sizeof(QString)=12,
alignment=4, hidden return storage in R0, worker in R1 and index in R2 for the
first call; the inverse uses worker R0 and QString-reference R1. Use actual typed
vendor-compiler calls and automatic QString destruction. Opaque QObject pointers
carry the receiver address here; the worker callback must not call QObject APIs.

The exact helper bodies inspected for membership and vector lookup read existing
storage and compare borrowed QString views. The imported vendor comparison
declarations are pure/noexcept. That is a bounded ordinary-path finding, not a
proof against arbitrary library interposition or damaged data. Accessor EHABI
supports normal lock-failure unwind; no post-lock cleanup landing pad establishes
recovery from a comparator that violates its noexcept contract.

The [execution-time order investigation](execution-time-page-order-boundary.md)
establishes why these reads are useful but insufficient for mutation authority.
The proposed [fixed tuple witness](../../openspec/changes/establish-native-platform-contract/worker-order-witness.md)
freezes three adjacent indexes and expected IDs, then performs at most six calls
without following newly observed values. All matches would establish only
individual worker-local observations during that callback.

The earlier no-op callback captured plain atomics, so its observer could release
native owners without leaving a later raw-pointer action. A callback that reads
the worker requires a stronger ownership rule: retain native Document/Worker
owners in process-resident quarantine before enqueue, and never release them on
timeout or observer destruction while invocation remains possible. Matching
completion and later GUI quiescent release are distinct from tuple acceptance.
The code remains loaded until process exit.

Private pre-call receipt SHA-256:
`b11ff69efcba44d866bdbf10d643cf1d8bbf44e0b6f047f20a8da757ee3709da`.
Retained callback REA evidence:
`ev_607722522c769a1424258116eb919622bde709ff101df224b91a8f7ed8e5f093`.
Raw firmware, extracted bodies and detailed private build outputs stay outside
the repository. This page reports original contract findings only.

Source basis: retained REA/Ghidra dispatch evidence, exact retained-image ARM
assembly/EHABI, vendor header declarations and a fresh compile-only typed shim.
Quarantine and fixed tuple behavior are proposed engineering design. No new
runtime accessor result, atomic insertion or supported capability is claimed.
