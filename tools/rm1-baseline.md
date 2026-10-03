# RM1 development baseline collector

This Linux-host tool uses an existing key-authenticated SSH alias. It installs nothing on the tablet and invokes no native page/capture/input operation. Explicit hardware authorization and sole device ownership are still required. The configured host alias must already target the intended USB connection; the tool does not configure routes or grant access.

```sh
python3 -m unittest discover -s tools -p test_rm1_baseline.py -v
python3 tools/collect_rm1_baseline.py --ssh-host remarkable1-usb --output /chosen/path/rm1-baseline.json
```

Python 3.11+ and OpenSSH are sufficient; no Rust toolchain or Python packages are required for this separate tool. Missing SSH access, changed host identity, unknown model, malformed observations or changing runtime fingerprints refuse rather than retry. Output is bounded to 16 KiB with a 30-second outer deadline. Remote diagnostics are suppressed; existing output survives refusal. Validated reports replace the chosen output atomically with mode 0600.

The report contains only observed model/architecture, firmware/Qt version, executable/library hashes, filesystem block accounting and framebuffer allocation facts. It omits credentials, aliases, serial numbers, account identifiers and document data. Runtime hashes/PID checks are two non-atomic samples, not source authority or native lifetime proof. `virtual_allocation_pixels` is not logical screen/page/input geometry. Free reserved blocks and unreserved available blocks are separate.

A successful report always says `native_operations: unsupported`, `source_authority: false`. It does not create an SDK adapter, qualify RM1 from matching RM2 architecture/Qt versions or grant callable page operations. Stock UI responsiveness, physical input calibration, screenshot correctness, page/source binding and native insertion remain separate evidence gates.

## Observed RM1 baseline — October 2, 2026

Authorized SCRAPPY-DOO USB observations: RM1/ARMv7, firmware 3.28.0.172, Qt 6.10.3, framebuffer mxc_epdc_fb/16 bpp/virtual allocation 1408×3840. xochitl SHA-256 `1f4fbb6e14650704b5b036e482da9948e73553178f6116ad464ab072f7b90117`; Qt Core SHA-256 `e774905f474cc6516440b30ff8c1807b7a8707a7ae14c41d414062767070b9b5`. Two collector executions agreed on these identity/framebuffer facts. Root had 12,284,928 free bytes but zero unreserved available bytes; /home had approximately 6.2 GiB unreserved available. Storage changes over time; these are historical observations.

The SDK research contract reviewed was eea96ee94583614537aabe71569648cae943620c; Docs review base 8008389fa676d395401e6f28e049b47baa7b10ef. SDK main contained only bootstrap history at this point. This standalone Python collector can land independently on SDK main; it does not depend on the unfinished research contract being merged. No Buddy/Manager contract or Rust/native observer is changed. This baseline makes no cross-model qualification claim.

Source basis: original collector tests (synthetic), current authorized RM1 read-only SSH observations (hardware), and exact repository revisions (contract). No proprietary firmware bytes are distributed.
