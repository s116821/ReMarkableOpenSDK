# Pre-Enter diagnostic source bridge

The source implements the finite borrowed reader/state, an A32 forwarding shim,
and private constructor preparation. Current builds leave the constructor path
unselected. The owned fixture exercises arming and forwarding on an owned ELF.

`Capture::prepare` accepts an admitted Profile, an explicit bounded range and
a budget of at most 120 seconds. It refuses until the route is installed. One
request can arm the route for its process lifetime. An immutable holder retains
the winning state after cancellation, expiry, consumption or handle destruction.
Snapshots report copied historical data, native refusal and original-return or
exception observations. Cancellation leaves the route and its code resident.

The internal native read checks the stock pair against current Scene root/control
before and after copying. It reuses Attachment's join and the same final text
revalidation helper, current UTF-16/budget/structure checks and native/Qt
composition refusal. It neither acquires nor releases the native owner. Public
callers cannot supply native pointers or bind the internal route.

The shim at the ordinary LF callsite performs bounded capture, invokes the exact
original insertion once, then publishes without holding an SDK mutex across
stock execution. Original exceptions publish a refused observation and rethrow;
capture exceptions cannot cause input replay. Publication contention discards
the candidate and produces an explicit refused snapshot. Refused output is zeroed.

## Owned ARM control

Run `composer_pre_enter_owned_arm_test.sh` in the existing matching SDK with
QT_PROBE_SDK_ENV and a fresh COMPOSER_PRE_ENTER_OUTPUT. It creates an owned ELF
with the caller BL at0xb4a9b0 and original at0xb492a0. Its single-threaded test-only
installer checks the original instruction, writes a reserved near veneer and
branch, synchronizes instruction caches and restores executable protection.
It does not map over another allocation or patch any tablet/process artifact.

The actual compiled shared-library shim is reached through that patched route.
The original checks argument identity and stack alignment. An assembly harness
checks r4-r11 and d8-d15 on normal return and has EHABI metadata for original
exceptions crossing the caller, veneer and shim into C++ cleanup. The executable
owns a real QTextDocument and a native-layout synthetic Scene/item with a stock
owner pair from an actual matching-toolchain shared_ptr; the SDK never modifies
that reference count. These objects model native layout; they are not Xochitl.

Controls exercise pre-LF bytes, repeated stock calls after one-shot consumption,
scope-null native nesting, Qt edit blocks, preedit, stock-pair mismatch, drift
before recheck, capture throw, original throw, cancellation, expiry, reentry and
contended publication. The fixture also checks refusal before installation,
dormant forwarding, competing requests, and published-state lifetime after
handle destruction. The script produces disassembly and unwind tables.
The existing text-copy script also compiles macro-free private/ordinary bridge
translation units in its maintained host CI path and keeps its existing six
owned document/delivery groups.

## Remaining integration

The implemented private constructor path still requires runtime qualification
for loader policy, exact stock mappings, owner/thread correspondence, folio and
OSK forwarding and composition refusal. Direct teardown settlement and
consumer-owned range, current-visit, Submit and clear authority remain separate.

The constructor allocates the route before interception. Entry loads the
published route and armed holder. Replacing an idle owned test route never
destroys its retained state. Preparation pins the already loaded shim DSO with
RTLD_NOLOAD|RTLD_NODELETE and retains that loader reference. The owned shim is also
linked with NODELETE; its symbol control rejects lazy guards and Route exit
destruction. These mechanisms retain SDK state/code, not native item ownership.

## Private prepared patch capsule

composer_pre_enter_patch_private.h implements a fixed-profile, one-use transaction
with direct Linux syscall leaves and fixed buffers. It saves/blocks/queries the
kernel mask, checks constructor PID/TID and current task/status observations,
verifies unchanged complete mappings, exact branch bytes and RX site/original/
veneer/shim spans, then changes only the admitted site page. Cache/protection
failure rolls back only while exclusion remains established; unexpected code,
lost exclusion or failed restoration terminates119. Prior-mask restoration occurs
only after coherent code/protection state. The private StartupBuilder is wired
into the existing constructor with selection disabled in current builds.

composer_pre_enter_patch_test.sh runs owned Linux children with real pending
signals, a distinct extra thread, code mismatch, injected cache/protection errors,
rollback and fail-closed exits. It checks exact prior-mask restoration and no C++
allocation during the excluded interval. Host Linux controls pass. ARM compilation
passes, but SDK QEMU8.2.7 makes its private ARM cacheflush syscall a no-op without
setting r0; this fails the capsule's strict native zero-return check in preparation.
The test reports unsupported instead of weakening the native leaf or claiming an
ARM patch-transaction pass. The independent actual ARM forwarding/unwind route
still runs under its existing owned installer. This QEMU refusal remains valid
for that provider; it is separate from the later successful RM2 own-memory run.

The unchanged capsule-test built from source
`9df17f96f4aecffcaf74371c62fdc10a1ea8b0a6` subsequently completed its owned
anonymous-memory cases on the admitted RM2 runtime. The exact success output,
normal exit, original unit identity, empty groups and independent host/transport
settlement were checked. This supplies positive raw ARM cache/task/transaction
evidence under the diagnostic's limited per-process profile. It does not qualify
aggregate resource enforcement, the stock callsite, a production StartupBuilder
or composer Submit/currentness/clear/focus. Explicit fallback recovery was not
exercised. See [bounded native evidence and the remaining source seam](composer-pre-enter-native-evidence.md).

Retained identity reports 5.4.70-v1.6.3-rm11x with source identity unverified. The
company kernel branch at d54fe67bf86e918468b936f97a2ec39f4f87a3d9 is tagged
RM1XX_5.4.70_v1.6.2. Its proc thread count is read under the shared sighand lock;
clone increments the count under that lock before wakeup, and exit decrements it
in exit cleanup. Those source facts do not bind the shipped v1.6.3 binary to this
commit. No production builder can be admitted from that evidence alone. See the
active change's loader-admission design and implementation notes.
