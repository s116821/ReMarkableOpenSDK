# RM2 portrait gesture component result

The October 8, 2026 Main-operated stock RM2 test demonstrated the ordinary SDK
portrait trajectory in both directions through Reader's existing manual probe.
One `next portrait` selected the existing adjacent blank page; one separately
authorized `previous portrait` restored the original paper page and handwriting.
This is a component result, not qualified product navigation.

## Source and evidence provenance

- SDK mapping: `5c0bc303bb9b22e8e01bce9b2bd2a7fe47c8010e`,
  [`navigation::gesture`](../../src/navigation/gesture.rs). Its source review and
  host checks are recorded in the [owning mapping checkpoint](../../openspec/changes/establish-native-platform-contract/gesture-mapping.md).
- Consumer: unchanged, independently hash-reviewed eight-file working changes
  over Buddy `3fb580ca38d14c9f1a525a64a242c6e57e5c30df`, pinned to that SDK revision.
  The consumer remains uncommitted/unfrozen; its base commit alone does not
  reproduce the tested changes. This result does not repair or repackage the
  separately rejected central Docs save/commit operation.
- `hardware_probe`: 9,885,992 bytes, SHA-256
  `5c8242334d9fc8bde3adae9af0799943ab62211e5b0957d743b7273d3320f5ee`.
  Main reports a read-only-source build using the existing GCC 9.4-compatible
  image and Linux Rust 1.98.1, plus fresh six-provider hash/audit checks.
- Stock RM2 firmware `3.28.0.172`; the same boot, xochitl PID 12830 and start
  time 3183152 were retained. No SDK native module was used.
- Canonical operator handoff: [Main's Mem note](https://mem.ai/56ea3184-4cd7-5f7e-93e3-35dd8efa3824),
  version 50 at closeout, with full semantic readback reported by Main.
- Private local evidence locator:
  `remarkable-buddies-mvp/work/rem37-sdk-gesture-a10328ecce494fd2904e7dc9146f59c0/evidence.tar`.
  SHA-256 `31ec4ec04d96ec120ad6c6981e9418b62303ad7ddd80602663a240f70fd03f07`.
  The archive excludes helper binaries; private logs/images and binaries are not
  copied into this repository. This SDK lane independently rehashed the archive.

## Observed result

| Operation | Main's observed input | Observed destination |
| --- | --- | --- |
| Next, once, exit 0 | 36 raw events, 17 SYN frames, one press/release lasting 207,435 microseconds | Existing page `d1261cb9-a8c0-4e15-ab24-7a9172b2027b`, index 1; blank page with ordinary toolbar |
| Previous, once, exit 0 | 34 raw events, 17 SYN frames, one press/release lasting 205,660 microseconds | Original page `a7181850-3f03-4bb0-8b9e-01acfc20032e`, index 0; original paper and ink |

Main reports the raw path matched the plan, with no SYN_DROPPED or input retry.
The Previous contact's initial unchanged X/Y values were omitted by the kernel;
that event-count difference is not evidence of a missing movement or release.
Six complete two-second read-only all-slot/key observer runs reported no
Invalidated result. This is bounded observation, not proof of absolute human
absence or a global input lock. The first raw observer ignored SIGINT; Main
verified its owned stdout/PID and terminated it with TERM (exit 143), without
repeating input.

Root and Main visually inspected the before, blank-next and restored images.
The restored and before PNGs have identical SHA-256
`c1320eee6c47d3df1080a729db09e3490bfdc376e9066abcd3ac28952a996ad4`;
this SDK lane independently verified their hash equality. Immediate and eventual
Next PNGs matched at `5773674c378f206932b196b117af7355f8f1bdc0d6fcf4a1cb81893fb6c0e441`.
Main verified unchanged PDF and all three existing ink files; content changes
were confined to `cPages.lastOpened`, and metadata to page bookkeeping.
Owned remote helpers/output directory were removed; the local cleanup receipt
records `TASK_DIRECTORY_REMOVED` and unchanged active stock services/policy.

## Interpretation and remaining limits

The tested manual path explicitly supplied portrait. It did not exercise product
NativeNavigation source/orientation admission or its completion guard. Metadata,
screenshots and passive input observations here do not create SDK active-page
authority or a native receipt. Successful ordinary release does not establish
release after SIGKILL. No native insertion, shutdown repair, other device or other
orientation was tested. Native capabilities remain Unsupported; task 3.6b and
the broader native qualification, canonical sync and archive gates remain open.

Source basis: Main's final actual closeout, Root/Main visual verification reported
through current project coordination, and this lane's local archive/image hashes
and cleanup receipt readback. Operator findings are attributed; no private raw
logs or reconstructed firmware source are included.
