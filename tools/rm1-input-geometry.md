# RM1 read-only input geometry probe

This is a metadata probe, not a navigation adapter. It reads the fixed RM1 model
and event2 kernel identity/capabilities/static ABS ranges. EVIOCGABS incidentally
returns each axis's current value along with its bounds; the probe immediately
clears that value without using, retaining or reporting it. It never reads the
input event stream, grabs a device, sends input, modifies xochitl or inspects
documents. Repeated static observations are not lifetime/source authority.
Unknown orientation/navigation remains unsupported in every report.

Host verification:

```
cc -std=c11 -Wall -Wextra -Werror -pedantic tools/rm1_input_geometry.c -o /tmp/rm1-geometry
cc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined tools/rm1_input_geometry_test.c -o /tmp/rm1-geometry-test
/tmp/rm1-geometry-test
```

ARMv7 candidate compilation uses upstream cross-rs armv7 GNU image at digest
`sha256:4a08bebb60f08d52c1885b643a768f163c7318c05f91cf119f6c02855ea52699`
with source mounted read-only, network disabled and only a separate output mount
writable. Its compatibility compiler is GCC 5.4; this is not a current production
compiler/security qualification. Static glibc linking requires license/source
compliance before public binary distribution; no binary is committed here.
The fixture executable also runs under the image's qemu-arm; that is emulator
proof, not a real RM1 execution.

Before an authorized USB read-only run: verify exact source/artifact hashes,
announce the operation, stage only in a new private owned /home/root directory,
verify remote artifact hash, run once with outer SSH deadline and the helper's
three-second process alarm, and remove only the exact owned staging directory.
No runtime restart, event injection or UI manipulation is needed. The alarm is
not a hard wall-clock bound for uninterruptible kernel waits. A run's stdout must
be a complete bounded record and its exit/cleanup outcome checked; partial
output is not success. Source review/real profile and shared API qualification
remain required before gesture injection or accepting navigation capabilities.

Logical Next/Previous requirements are owned by the shared SDK change, pinned
in this proposal. This probe does not implement a competing navigation API.

Observed RM1 metadata at the exact 28cfca140b4325999ecea2abce31b612d79fef8a
source/above ARM candidate: X 0..767, Y 0..1023, slots 0..31; complete ranges and
operator/cleanup receipt are recorded in the active design. These numbers are
observations, not maintained profile allowlists. Community placement prior art:
https://github.com/canselcik/libremarkable/blob/d9125f136ed34926c2528c34722aa3494611d644/src/device/mod.rs
Native navigation and orientation remain unqualified.
