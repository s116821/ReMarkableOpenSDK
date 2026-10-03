# Design and evidence boundaries

Current read-only USB observations: RM1 model, event2 cyttsp5_mt, mxc_epdc_fb,
fbset physical mode 1404x1872 with virtual allocation 1408x3840. Modes list both
portrait and landscape; this does not establish current UI orientation or input
transform. Current Reader ff8ad75 uses RM2/Paper Pro/Unknown routing and defaults
Unknown to RM2 touch mapping; never reuse that fallback as RM1 qualification.

Probe scope: no arguments; verify the fixed model path; open event2 read-only,
nonblocking, close-on-exec and without following a final symlink. Verify Linux
input character device major/minor. Read only EVIOCGNAME, EVIOCGBIT and EVIOCGABS
metadata, twice, on the same descriptor. Require stable identity/capabilities and
static axis bounds; ignore volatile current values. Refuse malformed/changing
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
