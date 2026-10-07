# Design and evidence boundaries

Delivery boundary: this change ships the fixed read-only metadata probe and its
evidence restrictions. The unfinished gesture/profile/native completion work is
preserved in active `qualify-rm1-logical-navigation`; it is not a prerequisite for
metadata source delivery and is not accepted by this change's review or archive.
The scope split changes no source, firmware, artifact or hardware observation.

Current read-only USB observations: RM1 model, event2 cyttsp5_mt, mxc_epdc_fb,
fbset physical mode 1404x1872 with virtual allocation 1408x3840. Modes list both
portrait and landscape; this does not establish current UI orientation or input
transform. Current Reader ff8ad75 uses RM2/Paper Pro/Unknown routing and defaults
Unknown to RM2 touch mapping; never reuse that fallback as RM1 qualification.

Probe scope: no arguments; verify the fixed model path; open event2 read-only,
nonblocking, close-on-exec and without following a final symlink. Verify Linux
input character device major/minor. Read only EVIOCGNAME, EVIOCGBIT and EVIOCGABS
metadata, twice, on the same descriptor. Require stable identity/capabilities and
static axis bounds. EVIOCGABS also returns volatile current values; immediately
clear them without using, retaining or reporting them. Refuse malformed/changing
snapshots. Emit one bounded JSON record after full verification, no personal
content, current touch coordinates or addresses. Kernel ioctls are synchronous;
a three-second process alarm provides default signal termination, and a host/remote
operator must provide an outer process deadline/owned cleanup.
No hard real-time ioctl bound is claimed by this metadata helper itself.

Host fixtures cover range/capability/identity changes and malformed metadata.
Compile and verify exact source/artifact identity on host and ARMv7 before an
authorized own read-only RM1 run; independent source/profile review is required
before gesture injection. No tablet access follows from compilation alone.
Do not infer a usable physical gesture from kernel bounds alone. Actual logical
Next/Previous will use the shared minimal SDK contract, at most one supported
per-model/orientation gesture, exact adjacent target/order and request continuity.
Completion requires native target identity and attributed fresh settled pixels
before any consumer write. Unknown profiles/source/orientation remain unsupported.

Prior art to inspect, not qualification: libremarkable upstream input handling
and early RM1 cyttsp5 reports use different kernel/firmware generations. Published
input coordinates are not a current-device transform. No code is copied here.
SDK licensing/distribution gates remain unchanged.

Actual authorized read-only run at source 28cfca140b4325999ecea2abce31b612d79fef8a
and ARM ELF SHA256 34af36dd416a0c906e2c018203d63939d8c6e14dea02a6f7bb80b7138159e01a
passed remote hash verification, metadata checks and exit 0. Owned private home
staging was removed and independently confirmed absent; xochitl was active
afterward. Kernel ranges: slot 0..31, X 0..767, Y 0..1023, tracking 0..65535,
pressure/major/minor 0..255, orientation -127..127; all fuzz/flat/resolution 0.
EVIOCGABS incidentally returned current axis values, which were immediately
cleared without use, retention or output. No input event stream or documents
were read. No input injected. This run receipt is author-reported; Main's source
review did not independently inspect the real-RM1 receipt.

Independent Main review of all nine files at 8f51dd278a0a368d012b95b86902d4bb720cf052
found no blocking source correctness issue within the metadata-only scope:
https://github.com/s116821/ReMarkableOpenSDK/pull/4#pullrequestreview-5401421257
Main independently passed strict host compilation, 15 sanitizer-backed fixtures
and quiet invalid-argument refusal, and inspected the successful hosted ARM
emulator checks. The requested current-value documentation correction changes
no probe/test/workflow bytes. Source review does not grant gesture or profile
qualification; canonical sync/archive and final delta review remain pending.

Pinned libremarkable d9125f136ed34926c2528c34722aa3494611d644 dynamically reads
axis sizes and assigns different RM1/RM2 multitouch placement (device/mod.rs).
This is MIT-licensed community prior art, inspected without copying code; it is
not qualification of this device's current UI orientation. Physical mode
1404x1872 and virtual allocation 1408x3840 remain separate from kernel touch
ranges and logical page coordinates. Known equal ranges do not justify borrowing
RM2 inversion. A complete gesture/source test fixture remains pending.
