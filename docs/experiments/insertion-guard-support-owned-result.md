# Insertion-guard support and compile-only composition

October 23, 2026 UTC. Separate owned controls now cover the ordinary caller's
borrowed carrier, a one-shot job snapshot and a pre-main owned loader. The
coordinator/file transport has its [own result](insertion-guard-coordinator-transport-owned-result.md).
The combined exact-build startup library compiles and links, but has never been
loaded or invoked. None of these results is a native rejection, accepted page
creation, supported SDK capability or physical owner-reclamation result.

All ARM runs use GCC 13.4.0, Qt 6.10.3, QEMU 8.2.7 and the local pinned image
`sha256:416c7a7be0038156797b0892f031f352b841d1921fae83f712d0a272e4724618`.
Inputs were read-only, networking disabled and capabilities dropped. The
repaired archive remains SHA-256
`622b17dc65d0dcad717d83264f783ad12044565ed8ea86f9e5088d9abf5328ef`.
Raw private records below are identified by hash; no public download is available.

## Borrowed ordinary-call carrier

The [Core](../../scripts/reader_insertion_guard_hooks.h) now accepts a separate
BorrowedExplicitSize policy described in the [caller contract](../../openspec/changes/establish-native-platform-contract/native-insertion-guard-caller-borrowing.md).
ExactOwned remains the default. Borrowed registration requires no frozen optional
address, explicit finite positive size, size-present 1 and inherit 0. The actual
optional reference must be nonnull/aligned; its bits and the unused integer
source carrier pass through unchanged. Worker/thread/caller PCs/index/UUID/size,
scope, deadline, cancellation and the unconditional negative barrier remain.
This models an independently observed machine contract, not a private C++ type.

The [fixture](../../scripts/reader_insertion_guard_hooks_fixture.cpp) and
[script](../../scripts/reader_insertion_guard_hooks_test.sh) pass 48 normal cases /
505 assertions plus 15 fail-stop controls on ARM/QEMU. The original 17/209 cases
and 15 fail-stop classifications remain equal to the [earlier exact-owned result](insertion-guard-hooks-owned-result.md).
New cases exercise dynamic stack references, arbitrary full-width source bits,
finite fractional/boundary sizes, invalid registrations, wrong carriers and
preservation of ExactOwned pointer/source checks. Source review was independent
of the implementation author; this followup was not independently replayed.

| Frozen item | SHA-256 |
| --- | --- |
| Core | `abd3b600510d28b118088069605152cdf919e12d5a735696a2cd5d7d6122610a` |
| Fixture | `2fed2a885c5ceaf9ddc7fe60cea02975900cf527c36a2df90a2880ed0820bbb3` |
| Script | `7fa31ed714558849b990bc7cc0bfffe28ccaa24f88d41d0212ed15073680dc56` |
| Private receipt | `5e1baa4f16ee41102724dc371edd570f37984831e5d1d2d111f5da63ebd74076` |

## One-shot job snapshot

The [snapshot](../../scripts/reader_insertion_job_snapshot.h) implements its
[separate contract](../../openspec/changes/establish-native-platform-contract/native-insertion-guard-job-snapshot.md):
checked disjoint storage bounds, exactly one try-lock, one eight-byte copy while
owned and one unlock. Busy returns without waiting/retrying/unlocking; zero IDs
refuse. Full unsigned 64-bit IDs stay exact. The consumed context cannot be renewed.
Unexpected try-lock/unlock results or throws terminate with 119 directly, including
uncertain ownership. No logger or supplied failure callback runs in that path.

The [fixture](../../scripts/reader_insertion_job_snapshot_fixture.cpp) and
[script](../../scripts/reader_insertion_job_snapshot_test.sh) pass 4 normal cases /
51 checks and 9 exit-119 controls on ARM/QEMU. A real second owned pthread holds
the busy mutex. Controls cover full-width values, zero/range refusal, throwing
or failing operations before/after acquisition/unlock and poisoned reporters
that must never run. Failure records identify the input scenario before entry;
they do not claim to observe ownership after fail-stop.

R1/R2 records remain historical. Independent review found that invoking an
arbitrary reporter before exit could block under uncertain ownership; R3 exits
directly. Its linked image has BIND_NOW. Successful-path assembly separately
shows the two word loads and unlock call. This is a real owned mutex experiment,
not qualification of a native mutex, native job ID or later Task completion.

| Frozen item | SHA-256 |
| --- | --- |
| Header | `6e3ca83d048c7f87e0464a99bb9d1e28da149e9b43b01953178c22ca921f82ca` |
| Fixture | `d395aac45ff902849f52edfb75d9aa4ed9ed5f8497e4ced87303466ca008a307` |
| Script | `851bcce63c07edf3a2f412a616cb7da82bf362ea3b572bd387af916e8d78fed6` |
| Binary | `018ad84b16497f90a731655eba09a0c7f2b66fe3ac564c9f17e630fefc9a55c5` |
| Private receipt | `7a02ea579bd23d3e7ccbae2b6025ba7a845dd66a93245eb051fbd4966fa2b992` |

Set INSERTION_JOB_OUTPUT to a fresh directory and QT_PROBE_SDK_ENV to the pinned
SDK environment before invoking the script. It builds original owned code and
bounds each subprocess. This helper does not link Dobby or create a native owner.

## Owned pre-main loader

The five original [loader sources](../../scripts/insertion_guard_owned/loader/build.sh)
implement the [owned startup study](../../openspec/changes/establish-native-platform-contract/native-insertion-guard-loader-study.md).
A real LD_PRELOAD DSO installs both hooks before the owned executable's main,
QCoreApplication and owned QThread. One transparent operation on the actual owned
worker then passes 6 checks. Bad original entry and missing host exports each
terminate with 119 before main. No operation is registered and no new predicate or
private-exception path is exercised here.

Actual init_array has frame_dummy, the selected routing-plugin initializer, then
the install constructor; all 3 slots have ARM RELATIVE relocations. The archive's
explicit debug constructor is not selected. Recorded dynamic-linker output puts
QtCore initialization before this payload. Four process tasks were observed;
their identities are unknown, so this is not whole-process thread absence.
Root independently reviewed all five sources, ELF and raw 10 reads/12 mappings,
including actual replacement destinations and complete-pair rechecks. Root did
not replay this loader. QString values/source forwarding were checked; new
runtime reference-address identity was not measured.

For exact reproduction, copy only these five files into /inputs, then export
the Core and predicate headers from commit 454b96a4b066085271fd4a9a5d2eb5ea378714db
into that same private input directory. The required historical Core hash is
`4543c6b43cf61e27c714b9faca678b38bd42385bb3ec092c3e738c890d01be27`;
predicate hash is
`27fa7882a445c650f38d3b1be670661c5600c4e8ea1742654d029f61c274416a`.
The current borrowed-policy Core is a later input and cannot reproduce this
historical receipt by substitution. Supply the matching public Dobby headers at
/dobbyinclude, repaired archive at /library/libdobby.a and fresh /out directory.
The script records compiler/provider/ELF outputs and bounds all owned execution.

| Frozen item | SHA-256 |
| --- | --- |
| owned_api.h | `59df4277a4aa970e7ac597f9cf2eedc5e18c8117bddd2a6d6cd355cbc8327132` |
| core.cpp | `7f4130bf966840dcf0b9c670a930447edd27ca1663851f528d20aa8d2001e057` |
| startup.cpp | `d671263dcea51ddebd1acdd74bd0f32c8b424c21d0890da0a730d2936a40e4d2` |
| host.cpp | `1d108fb3406e80ab444ae78fc7ebf70497f0e8b4b5f5838868c74c5f9395966e` |
| build.sh | `5842dba327f8031d5c80b930e80b3de00fde163854784b79c5bebdda0178f6a5` |
| DSO | `ecda3a9ab016488afb081c1ec04269513b45891ed45f278e8813a82abc992ca6` |
| Private author receipt | `8c41db12c024376c6037e7a3c0b85476197228d82113e13a9f327637d9afd34e` |

## Combined compile/link-only startup

The exact-build [profile](../../scripts/reader_insertion_guard_native_profile.cpp)
and [startup](../../scripts/reader_insertion_guard_startup.cpp) implement the
[profile](../../openspec/changes/establish-native-platform-contract/native-insertion-guard-profile.md)
and [fixed startup precode](../../openspec/changes/establish-native-platform-contract/native-insertion-guard-transport.md).
Profile/source review and a strict ARM -Wall/-Wextra/-Werror compile/link pass.
The separate combined build uses dummy fixed inputs, BIND_NOW, initial-exec TLS,
hidden visibility, no GNU-unique symbols, an all-local linker script and the
selected archive before the startup object. It is never loaded or executed.

Static ELF review verifies 4 init_array entries: frame_dummy, routing plugin,
install, then Qt startup registration, all ARM RELATIVE. No dynamic definitions
are exported and the explicit Dobby debug constructor remains unselected.
The first combined link's generic helper exports are retained in its historical
record; the second link uses the all-local script. Constructor order in this
dummy artifact is not runtime target-use ordering or acceptance of a future
nonce-configured payload. The actual packet must separately pin/review its
imports/providers, startup, original absolute clocks, actor and full restoration.

| Frozen item | SHA-256 |
| --- | --- |
| Profile header | `e7584c19f738b86757668a4a1d22aa6ca2b253e11c56a1e1e4117b0259385f17` |
| Profile source | `6536ddde1b549bb90024c71a7e3c95711eed8d1109db51b495ea4ea6b5060ce7` |
| Startup source | `a0f23e3386f1dd0586a3d917bdd4ce937142991e46a95a0b8bf9ca9218fdeb65` |
| Combined link-only DSO | `24ec91b2d3ced292cec5599abd84d9f8a070b85418262258be8781b25d28a3ed` |
| Combined compile receipt | `fdb4732949f282d82ae3332c58c5870cdacfe2c8dd25f5a16d8289b154b032c3` |
| Combined static ELF review | `d211250f77b1da372b6394b87fc04309629ebbc01acfbf79a21fbaa17a9efc2e` |
| Owned loader Root review | `59b932705b22e92e8a8e90eea73cf3ed85f91d75cbec5ec8402ef4c9d966b162` |
| Root source review/correspondence | `9be20df625f367cb8a284f8b468452136649dfa17f8879295c6402e6418c1af3` |

The combined build compiles profile and startup separately with C++17,
`-marm -fPIC -pthread -fexceptions -frtti -fno-lto -fno-gnu-unique
-fvisibility=hidden -ftls-model=initial-exec`, then links profile.o, the archive,
startup.o, the four generated coordinator/selected-view/worker-callback/joined-
creation moc objects and the creation bridge. Use the target SDK's moc and
pkg-config Qt6Qml/Qt6Gui/Qt6Quick flags. Supply the six fixed definitions required
by startup.cpp with dummy owned UUIDs and no device packet. Link with
`-z now -z defs -Bsymbolic-functions --exclude-libs,ALL` and a version script
containing `{ local: *; };`. This reproduces a compile/link study only: never
load the resulting library as an owned fixture or supply real device paths.
Old unchanged CRLF helper inputs were exported as LF for this combined build;
its private input manifest records those actual normalized bytes separately
from the earlier owned test manifests. No source logic was changed by export.

Source basis: current original sources, precode, retained owned compiler/runtime
records and independent source/static review. Native active-job identity,
borrowed carriers, original unwind, lifetime and critical-section behavior
remain candidate-specific questions. No supported behavior is synchronized or
archived from these intermediate research results.
